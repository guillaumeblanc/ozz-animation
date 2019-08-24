//----------------------------------------------------------------------------//
//                                                                            //
// ozz-animation is hosted at http://github.com/guillaumeblanc/ozz-animation  //
// and distributed under the MIT License (MIT).                               //
//                                                                            //
// Copyright (c) 2019 Guillaume Blanc                                         //
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

// Internal include file
#define OZZ_INCLUDE_PRIVATE_HEADER  // Allows to include private headers.
#include "animation/offline/decimate.h"

#include "ozz/base/containers/stack.h"
#include "ozz/base/containers/vector.h"

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
AnimationOptimizer::AnimationOptimizer() {}

namespace {

AnimationOptimizer::Setting GetJointSetting(
    const AnimationOptimizer& _optimizer, int _joint) {
  AnimationOptimizer::Setting setting = _optimizer.setting;
  AnimationOptimizer::JointsSetting::const_iterator it =
      _optimizer.joints_setting_override.find(_joint);
  if (it != _optimizer.joints_setting_override.end()) {
    setting = it->second;
  }
  return setting;
}

struct HierarchyBuilder {
  HierarchyBuilder(const RawAnimation* _animation, const Skeleton* _skeleton,
                   const AnimationOptimizer* _optimizer)
      : specs(_animation->tracks.size()),
        animation(_animation),
        optimizer(_optimizer) {
    assert(_animation->num_tracks() == _skeleton->num_joints());

    // Computes hierarchical scale, iterating skeleton forward (root to
    // leaf).
    IterateJointsDF(
        *_skeleton, Skeleton::kNoParent,
        IterateMemFun<HierarchyBuilder, &HierarchyBuilder::ComputeScaleForward>(
            *this));

    // Computes hierarchical length, iterating skeleton backward (leaf to root).
    IterateJointsDFReverse(
        *_skeleton,
        IterateMemFun<HierarchyBuilder,
                      &HierarchyBuilder::ComputeLengthBackward>(*this));
  }

  struct Spec {
    float length;  // Length of a joint hierarchy (max of all child).
    float scale;   // Scale of a joint hierarchy (accumulated from all parents).
    float tolerance;  // Tolerance of a joint hierarchy (min of all child).
  };

  // Defines the length of a joint hierarchy (of all child).
  ozz::Vector<Spec>::Std specs;

 private:
  // Extracts maximum translations and scales for each track/joint.
  void ComputeScaleForward(int _joint, int _parent) {
    Spec& joint_spec = specs[_joint];

    // Compute joint maximum animated scale.
    float max_scale = 0.f;
    const RawAnimation::JointTrack& track = animation->tracks[_joint];
    if (track.scales.size() != 0) {
      for (size_t j = 0; j < track.scales.size(); ++j) {
        const math::Float3& scale = track.scales[j].value;
        const float max_element = math::Max(
            math::Max(std::abs(scale.x), std::abs(scale.y)), std::abs(scale.z));
        max_scale = math::Max(max_scale, max_element);
      }
    } else {
      max_scale = 1.f;  // Default scale.
    }

    // Accumulate with parent scale.
    joint_spec.scale = max_scale;
    if (_parent != Skeleton::kNoParent) {
      const Spec& parent_spec = specs[_parent];
      joint_spec.scale *= parent_spec.scale;
    }

    // Computes self setting distance and tolerance.
    // Distance is now scaled with accumulated parent scale.
    const AnimationOptimizer::Setting setting =
        GetJointSetting(*optimizer, _joint);
    joint_spec.length = setting.distance * specs[_joint].scale;
    joint_spec.tolerance = setting.tolerance;
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
    const float max_length = std::sqrt(max_length_sq);

    if (_parent != Skeleton::kNoParent) {
      const Spec& joint_spec = specs[_joint];
      Spec& parent_spec = specs[_parent];

      // Set parent hierarchical spec to its most impacting child, aka max
      // length and min tolerance.
      const float joint_hierarchy_length =
          joint_spec.length + max_length * parent_spec.scale;

      parent_spec.length =
          math::Max(parent_spec.length, joint_hierarchy_length);

      parent_spec.tolerance =
          math::Min(parent_spec.tolerance, joint_spec.tolerance);
    }
  }

  // Disables copy and assignment.
  HierarchyBuilder(const HierarchyBuilder&);
  void operator=(const HierarchyBuilder&);

  // Targeted animation.
  const RawAnimation* animation;

  // Usefull to access settings and compute hierarchy length.
  const AnimationOptimizer* optimizer;
};

class PositionAdapter {
 public:
  PositionAdapter(float _scale) : scale_(_scale) {}
  bool Decimable(const RawAnimation::TranslationKey&) const { return true; }
  RawAnimation::TranslationKey Lerp(
      const RawAnimation::TranslationKey& _left,
      const RawAnimation::TranslationKey& _right,
      const RawAnimation::TranslationKey& _ref) const {
    const float alpha = (_ref.time - _left.time) / (_right.time - _left.time);
    assert(alpha >= 0.f && alpha <= 1.f);
    const RawAnimation::TranslationKey key = {
        _ref.time, LerpTranslation(_left.value, _right.value, alpha)};
    return key;
  }
  float Distance(const RawAnimation::TranslationKey& _a,
                 const RawAnimation::TranslationKey& _b) const {
    return Length(_a.value - _b.value) * scale_;
  }

 private:
  float scale_;
};

class RotationAdapter {
 public:
  RotationAdapter(float _radius) : radius_(_radius) {}
  bool Decimable(const RawAnimation::RotationKey&) const { return true; }
  RawAnimation::RotationKey Lerp(const RawAnimation::RotationKey& _left,
                                 const RawAnimation::RotationKey& _right,
                                 const RawAnimation::RotationKey& _ref) const {
    const float alpha = (_ref.time - _left.time) / (_right.time - _left.time);
    assert(alpha >= 0.f && alpha <= 1.f);
    const RawAnimation::RotationKey key = {
        _ref.time, LerpRotation(_left.value, _right.value, alpha)};
    return key;
  }
  float Distance(const RawAnimation::RotationKey& _left,
                 const RawAnimation::RotationKey& _right) const {
    // Compute the shortest unsigned angle between the 2 quaternions.
    // diff_w is w component of a-1 * b.
    const float cos_half_angle = Dot(_left.value, _right.value);
    const float sine_half_angle =
        std::sqrt(1.f - math::Min(1.f, cos_half_angle * cos_half_angle));
    // Deduces distance between 2 points on a circle with radius and a given
    // angle. Using half angle helps as it allows to have a right-angle
    // triangle.
    const float distance = 2.f * sine_half_angle * radius_;
    return distance;
  }

 private:
  float radius_;
};

class ScaleAdapter {
 public:
  ScaleAdapter(float _scale) : scale_(_scale) {}
  bool Decimable(const RawAnimation::ScaleKey&) const { return true; }
  typename RawAnimation::ScaleKey Lerp(
      const RawAnimation::ScaleKey& _left, const RawAnimation::ScaleKey& _right,
      const RawAnimation::ScaleKey& _ref) const {
    const float alpha = (_ref.time - _left.time) / (_right.time - _left.time);
    assert(alpha >= 0.f && alpha <= 1.f);
    const RawAnimation::ScaleKey key = {
        _ref.time, LerpTranslation(_left.value, _right.value, alpha)};
    return key;
  }
  float Distance(const RawAnimation::ScaleKey& _left,
                 const RawAnimation::ScaleKey& _right) const {
    return Length(_left.value - _right.value) * scale_;
  }

 private:
  float scale_;
};
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
  const HierarchyBuilder specs(&_input, &_skeleton, this);

  // Rebuilds output animation.
  _output->name = _input.name;
  _output->duration = _input.duration;
  _output->tracks.resize(_input.tracks.size());

  for (size_t i = 0; i < _input.tracks.size(); ++i) {
    const RawAnimation::JointTrack& input_track = _input.tracks[i];
    RawAnimation::JointTrack& output_track = _output->tracks[i];

    // Gets joint specs back.
    const float hierarchical_length = specs.specs[i].length;
    const int parent = _skeleton.joint_parents()[i];
    const float hierarchical_scale =
        (parent != Skeleton::kNoParent) ? specs.specs[parent].scale : 1.f;

    // Gets joint settings back. Use default ones if no specific setting is
    // available.
    Setting joint_setting = setting;
    JointsSetting::const_iterator it = joints_setting_override.find(i);
    if (it != joints_setting_override.end()) {
      joint_setting = it->second;
    }

    // Filters independently T, R and S tracks.
    const PositionAdapter tadap(hierarchical_scale);
    Decimate(input_track.translations, tadap, specs.specs[i].tolerance,
             &output_track.translations);
    const RotationAdapter radap(hierarchical_length);
    Decimate(input_track.rotations, radap, specs.specs[i].tolerance,
             &output_track.rotations);
    const ScaleAdapter sadap(hierarchical_length);
    Decimate(input_track.scales, sadap, specs.specs[i].tolerance,
             &output_track.scales);
  }

  // Output animation is always valid though.
  return _output->Validate();
}
}  // namespace offline
}  // namespace animation
}  // namespace ozz
