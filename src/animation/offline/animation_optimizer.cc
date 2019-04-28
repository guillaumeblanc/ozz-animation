//----------------------------------------------------------------------------//
//                                                                            //
// ozz-animation is hosted at http://github.com/guillaumeblanc/ozz-animation  //
// and distributed under the MIT License (MIT).                               //
//                                                                            //
// Copyright (c) 2017 Guillaume Blanc                                         //
//                                                                            //
// Permission is hereby granted, free of charge, to any person obtaining a    //
// copy of this software and associated documentation files (the "Software"), //
// to deal in the Software without restriction, including without limitation  //
// the rights to use, copy, modify, merge, publish, distribute, sublicense,   //
// and/or sell copies of the Software, and to permit persons to whom the      //
// Software is furnished to do so, subject to the following conditions:       //
//                                                                            //
// The above copyright notice and this permission notice shall be included in //
// all copies or substantial portions of the Software.                        //
//                                                                            //
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR //
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,   //
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL    //
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER //
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING    //
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER        //
// DEALINGS IN THE SOFTWARE.                                                  //
//                                                                            //
//----------------------------------------------------------------------------//

#include "ozz/animation/offline/animation_optimizer.h"

#include <cassert>
#include <cstddef>

#include "ozz/base/maths/math_constant.h"
#include "ozz/base/maths/math_ex.h"

#include "ozz/animation/offline/raw_animation.h"
#include "ozz/animation/offline/raw_animation_utils.h"

#include "ozz/animation/runtime/skeleton.h"
#include "ozz/animation/runtime/skeleton_utils.h"

namespace ozz {
namespace animation {
namespace offline {

// Setup default values (favoring quality).
AnimationOptimizer::AnimationOptimizer()
    : translation_tolerance(1e-3f),                 // 1 mm.
      rotation_tolerance(.1f * math::kPi / 180.f),  // 0.1 degree.
      scale_tolerance(1e-3f),                       // 0.1%.
      hierarchical_tolerance(1e-3f) {               // 1 mm.
}

namespace {

struct HierarchyBuilder {
  HierarchyBuilder(const RawAnimation* _animation, const Skeleton* _skeleton)
      : animation(_animation),
        lengths(_animation->tracks.size()),
        scales(_animation->tracks.size()) {
    assert(_animation->num_tracks() == _skeleton->num_joints());

    // Computes hierarchycal scale, iterating skeleton forward (root to
    // leaf).
    IterateJointsDF(
        *_skeleton, 0,
        IterateMemFun<HierarchyBuilder, &HierarchyBuilder::ComputeScaleForward>(
            *this));

    // Computes hierarchycal length, iterating skeleton backbard (leaf to root).
    IterateJointsDFReverse(
        *_skeleton,
        IterateMemFun<HierarchyBuilder,
                      &HierarchyBuilder::ComputeLengthBackward>(*this));
  }

  // Defines the length of a joint hierarchy (of all child).
  ozz::Vector<float>::Std lengths;

  // Defines the scale of a joint hierarchy (from all parents).
  ozz::Vector<float>::Std scales;

 private:
  // Extracts maximum translations and scales for each track/joint.
  void ComputeScaleForward(int _joint, int _parent) {
    // Compute joint maximum animated scale.
    float max_scale = 0.f;
    const RawAnimation::JointTrack& track = animation->tracks[_joint];
    if (track.scales.size() != 0) {
      float max_scale_sq = 0.f;
      for (size_t j = 0; j < track.scales.size(); ++j) {
        max_scale_sq = math::Max(max_scale, LengthSqr(track.scales[j].value));
      }
      max_scale = std::sqrt(max_scale_sq);
    } else {
      max_scale = 1.f;  // Default scale.
    }

    // Accumulate with parent scale.
    scales[_joint] = max_scale;
    if (_parent != Skeleton::kNoParent) {
      scales[_joint] *= scales[_parent];
    }
  }

  // Propagate child translations back to the root.
  void ComputeLengthBackward(int _joint, int _parent) {
    // Compute joint maximum animated length.
    float max_length_sq = 0.f;
    const RawAnimation::JointTrack& track = animation->tracks[_joint];
    for (size_t j = 0; j < track.translations.size(); ++j) {
      max_length_sq =
          math::Max(max_length_sq, LengthSqr(track.translations[j].value));
    }
    float max_length = std::sqrt(max_length_sq);

    // Set parent hierarchical spec to its most impacting child.
    if (_parent != Skeleton::kNoParent) {
      const float joint_hierarchy_length =
          lengths[_joint] + max_length * scales[_parent];
      lengths[_parent] = math::Max(lengths[_parent], joint_hierarchy_length);
    }

    // Leaf length is set to 0 during vector initialization.
  }

  const RawAnimation* animation;
};

// Copy _src keys to _dest but except the ones that can be interpolated.
template <typename _RawTrack, typename _Comparator, typename _Lerp>
void Filter(const _RawTrack& _src, const _Comparator& _comparator,
            const _Lerp& _lerp, float _tolerance, float _hierarchical_tolerance,
            float _hierarchy_length, _RawTrack* _dest) {
  _dest->reserve(_src.size());

  // Only copies the key that cannot be interpolated from the others.
  size_t last_src_pushed = 0;  // Index (in src) of the last pushed key.
  for (size_t i = 0; i < _src.size(); ++i) {
    // First and last keys are always pushed.
    if (i == 0) {
      _dest->push_back(_src[i]);
      last_src_pushed = i;
    } else if (i == _src.size() - 1) {
      // Don't push the last value if it's the same as last_src_pushed.
      typename _RawTrack::const_reference left = _src[last_src_pushed];
      typename _RawTrack::const_reference right = _src[i];
      if (!_comparator(left.value, right.value, _tolerance,
                       _hierarchical_tolerance, _hierarchy_length)) {
        _dest->push_back(right);
        last_src_pushed = i;
      }
    } else {
      // Only inserts i key if keys in range ]last_src_pushed,i] cannot be
      // interpolated from keys last_src_pushed and i + 1.
      typename _RawTrack::const_reference left = _src[last_src_pushed];
      typename _RawTrack::const_reference right = _src[i + 1];
      for (size_t j = last_src_pushed + 1; j <= i; ++j) {
        typename _RawTrack::const_reference test = _src[j];
        const float alpha = (test.time - left.time) / (right.time - left.time);
        assert(alpha >= 0.f && alpha <= 1.f);
        if (!_comparator(_lerp(left.value, right.value, alpha), test.value,
                         _tolerance, _hierarchical_tolerance,
                         _hierarchy_length)) {
          _dest->push_back(_src[i]);
          last_src_pushed = i;
          break;
        }
      }
    }
  }
  assert(_dest->size() <= _src.size());
}

// Translation filtering comparator.
bool CompareTranslation(const math::Float3& _a, const math::Float3& _b,
                        float _tolerance, float _hierarchical_tolerance,
                        float _hierarchy_scale) {
  if (!Compare(_a, _b, _tolerance)) {
    return false;
  }

  // Compute the position of the end of the hierarchy.
  const math::Float3 s(_hierarchy_scale);
  return Compare(_a * s, _b * s, _hierarchical_tolerance);
}

// Rotation filtering comparator.
bool CompareRotation(const math::Quaternion& _a, const math::Quaternion& _b,
                     float _tolerance, float _hierarchical_tolerance,
                     float _hierarchy_length) {
  // Compute the shortest unsigned angle between the 2 quaternions.
  // diff_w is w component of a-1 * b.
  const float diff_w = _a.x * _b.x + _a.y * _b.y + _a.z * _b.z + _a.w * _b.w;
  const float angle = 2.f * std::acos(math::Min(std::abs(diff_w), 1.f));
  if (std::abs(angle) > _tolerance) {
    return false;
  }

  // Deduces the length of the opposite segment at a distance _hierarchy_length.
  const float arc_length = std::sin(angle) * _hierarchy_length;
  return std::abs(arc_length) < _hierarchical_tolerance;
}

// Scale filtering comparator.
bool CompareScale(const math::Float3& _a, const math::Float3& _b,
                  float _tolerance, float _hierarchical_tolerance,
                  float _hierarchy_length) {
  if (!Compare(_a, _b, _tolerance)) {
    return false;
  }
  // Compute the position of the end of the hierarchy, in both cases _a and _b.
  const math::Float3 l(_hierarchy_length);
  return Compare(_a * l, _b * l, _hierarchical_tolerance);
}
}  // namespace

bool AnimationOptimizer::operator()(const RawAnimation& _input,
                                    const Skeleton& _skeleton,
                                    RawAnimation* _output) const {
  if (!_output) {
    return false;
  }
  // Reset output animation to default.
  *_output = RawAnimation();

  // Validate animation.
  if (!_input.Validate()) {
    return false;
  }

  // Validates the skeleton matches the animation.
  if (_input.num_tracks() != _skeleton.num_joints()) {
    return false;
  }

  // First computes bone lengths, that will be used when filtering.
  HierarchyBuilder specs(&_input, &_skeleton);

  // Rebuilds output animation.
  _output->name = _input.name;
  _output->duration = _input.duration;
  _output->tracks.resize(_input.tracks.size());

  for (size_t i = 0; i < _input.tracks.size(); ++i) {
    const RawAnimation::JointTrack& input_track = _input.tracks[i];
    RawAnimation::JointTrack& output_track = _output->tracks[i];

    const float hierarchical_length = specs.lengths[i];
    const int parent = _skeleton.joint_parents()[i];
    const float hierarchical_scale =
        (parent != Skeleton::kNoParent) ? specs.scales[parent] : 1.f;

    Filter(input_track.translations, CompareTranslation, LerpTranslation,
           translation_tolerance, hierarchical_tolerance, hierarchical_scale,
           &output_track.translations);
    Filter(input_track.rotations, CompareRotation, LerpRotation,
           rotation_tolerance, hierarchical_tolerance, hierarchical_length,
           &output_track.rotations);
    Filter(input_track.scales, CompareScale, LerpScale, scale_tolerance,
           hierarchical_tolerance, hierarchical_length, &output_track.scales);
  }

  // Output animation is always valid though.
  return _output->Validate();
}
}  // namespace offline
}  // namespace animation
}  // namespace ozz
