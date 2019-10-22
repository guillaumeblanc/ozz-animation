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
#include <numeric>
#include <random>

// Internal include file
#define OZZ_INCLUDE_PRIVATE_HEADER  // Allows to include private headers.
#include "animation/offline/decimate.h"

#include "ozz/base/containers/stack.h"
#include "ozz/base/containers/vector.h"

#include "ozz/base/maths/math_constant.h"
#include "ozz/base/maths/math_ex.h"

#include "ozz/base/log.h"

#include "ozz/animation/offline/raw_animation.h"
#include "ozz/animation/offline/raw_animation_utils.h"

#include "ozz/animation/runtime/skeleton.h"
#include "ozz/animation/runtime/skeleton_utils.h"

// -1 -> constant
// 0 -> base
// 1 -> iterate base
// 2 -> random
// 3 -> vis
int mode = -1;

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
        *_skeleton,
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
    // Self translation doesn't matter if joint has no parent.
    if (_parent == Skeleton::kNoParent) {
      return;
    }

    // Compute joint maximum animated length.
    float max_length_sq = 0.f;
    const RawAnimation::JointTrack& track = animation->tracks[_joint];
    for (size_t j = 0; j < track.translations.size(); ++j) {
      max_length_sq =
          math::Max(max_length_sq, LengthSqr(track.translations[j].value));
    }
    const float max_length = std::sqrt(max_length_sq);

    const Spec& joint_spec = specs[_joint];
    Spec& parent_spec = specs[_parent];

    // Set parent hierarchical spec to its most impacting child, aka max
    // length and min tolerance.
    parent_spec.length = math::Max(
        parent_spec.length, joint_spec.length + max_length * parent_spec.scale);
    parent_spec.tolerance =
        math::Min(parent_spec.tolerance, joint_spec.tolerance);
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
    /*
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
    */

    const ozz::math::Quaternion diff = Conjugate(_left.value) * _right.value;
    const ozz::math::Float3 axis(diff.x, diff.y, diff.z);
    ozz::math::Float3 binormal;
    if (std::abs(axis.x) < std::abs(axis.y)) {
      if (std::abs(axis.x) < std::abs(axis.z)) {
        binormal = ozz::math::Float3::x_axis();
      } else {
        binormal = ozz::math::Float3::z_axis();
      }
    } else {
      if (std::abs(axis.y) < std::abs(axis.z)) {
        binormal = ozz::math::Float3::y_axis();
      } else {
        binormal = ozz::math::Float3::z_axis();
      }
    }
    ozz::math::Float3 normal = Cross(binormal, axis);
    normal = NormalizeSafe(normal, ozz::math::Float3::x_axis());

    return Length(TransformVector(diff, normal) - normal) * radius_;
  }

 private:
  float radius_;
};

class ScaleAdapter {
 public:
  ScaleAdapter(float _length) : length_(_length) {}
  bool Decimable(const RawAnimation::ScaleKey&) const { return true; }
  RawAnimation::ScaleKey Lerp(const RawAnimation::ScaleKey& _left,
                              const RawAnimation::ScaleKey& _right,
                              const RawAnimation::ScaleKey& _ref) const {
    const float alpha = (_ref.time - _left.time) / (_right.time - _left.time);
    assert(alpha >= 0.f && alpha <= 1.f);
    const RawAnimation::ScaleKey key = {
        _ref.time, LerpScale(_left.value, _right.value, alpha)};
    return key;
  }
  float Distance(const RawAnimation::ScaleKey& _left,
                 const RawAnimation::ScaleKey& _right) const {
    return Length(_left.value - _right.value) * length_;
  }

 private:
  float length_;
};

struct LTMIterator {
  LTMIterator(const ozz::Range<const ozz::math::Transform>& _locals,
              const ozz::Range<ozz::math::Transform>& _models)
      : locals_(_locals), models_(_models) {}

  LTMIterator(const LTMIterator& _it)
      : locals_(_it.locals_), models_(_it.models_) {}

  void operator()(int _joint, int _parent) {
    if (_parent == ozz::animation::Skeleton::kNoParent) {
      models_[_joint] = locals_[_joint];
    } else {
      const ozz::math::Transform& local = locals_[_joint];
      const ozz::math::Transform& parent = models_[_parent];
      ozz::math::Transform& model = models_[_joint];

      model.translation =
          parent.translation +
          TransformVector(parent.rotation, local.translation * parent.scale);
      model.rotation = parent.rotation * local.rotation;
      model.scale = parent.scale * local.scale;
    }
  }
  const ozz::Range<const ozz::math::Transform>& locals_;
  const ozz::Range<ozz::math::Transform>& models_;

 private:
  void operator=(const LTMIterator&);
};

// Reimplement local to model-space because ozz runtime version isn't based on
// ozz::math::Transform
bool LocalToModel(const ozz::animation::Skeleton& _skeleton,
                  const ozz::Range<const ozz::math::Transform>& _locals,
                  const ozz::Range<ozz::math::Transform>& _models) {
  assert(static_cast<size_t>(_skeleton.num_joints()) <= _locals.count() &&
         static_cast<size_t>(_skeleton.num_joints()) <= _models.count());

  ozz::animation::IterateJointsDF(_skeleton, LTMIterator(_locals, _models));

  return true;
}

ozz::math::Float3 TransformPoint(const ozz::math::Transform& _transform,
                                 const ozz::math::Float3& _point) {
  return _transform.translation +
         TransformVector(_transform.rotation, _point * _transform.scale);
}

float Compare(const ozz::math::Transform& _reference,
              const ozz::math::Transform& _test) {
  /*
const float kDist = .0f;
ozz::math::Float3 points[] = {ozz::math::Float3::x_axis() * kDist,
                          ozz::math::Float3::y_axis() * kDist,
                          ozz::math::Float3::z_axis() * kDist};
float mlen2 = 0.f;
for (size_t i = 0; i < OZZ_ARRAY_SIZE(points); ++i) {
ozz::math::Float3 ref = TransformPoint(_reference, points[i]);
ozz::math::Float3 test = TransformPoint(_test, points[i]);
const float len2 = LengthSqr(ref - test);
mlen2 = ozz::math::Max(mlen2, len2);
}
return std::sqrt(mlen2);
*/
  return Length(_reference.translation - _test.translation);
}

float Compare(const ozz::Range<const ozz::math::Transform>& _reference,
              const ozz::Range<const ozz::math::Transform>& _candidate) {
  float mcmp = 0.f;
  for (size_t i = 0; i < _reference.count(); ++i) {
    const float icmp = Compare(_reference[i], _candidate[i]);
    mcmp = ozz::math::Max(mcmp, icmp);
  }
  return mcmp;
}

bool SampleModelSpace(const RawAnimation& _animation, const Skeleton& _skeleton,
                      float _time,
                      const ozz::Range<ozz::math::Transform>& _models) {
  ozz::math::Transform locals[ozz::animation::Skeleton::kMaxJoints];

  bool success = true;
  success &=
      ozz::animation::offline::SampleAnimation(_animation, _time, locals);
  success &= LocalToModel(_skeleton, locals, _models);

  return success;
}

float Compare(const RawAnimation& _reference, const RawAnimation& _test,
              float _time, const Skeleton& _skeleton) {
  ozz::math::Transform locals[ozz::animation::Skeleton::kMaxJoints];
  ozz::math::Transform ref_models[ozz::animation::Skeleton::kMaxJoints];
  ozz::math::Transform test_models[ozz::animation::Skeleton::kMaxJoints];

  assert(_reference.duration == _test.duration);

  bool success = true;
  success &=
      ozz::animation::offline::SampleAnimation(_reference, _time, locals);
  success &= LocalToModel(_skeleton, locals, ref_models);
  success &= ozz::animation::offline::SampleAnimation(_test, _time, locals);
  success &= LocalToModel(_skeleton, locals, test_models);
  assert(success);

  float mcmp = 0.f;
  for (int i = 0; i < _skeleton.num_joints(); ++i) {
    const float icmp = Compare(ref_models[i], test_models[i]);
    mcmp = ozz::math::Max(mcmp, icmp);
  }

  return mcmp;
}

float Compare(const RawAnimation& _reference, const RawAnimation& _test,
              float _from, float _to, const Skeleton& _skeleton) {
  _from = ozz::math::Max(_from, 0.f);
  _to = ozz::math::Max(_to, _reference.duration);

  float mcmp = 0.f;
  const float step = 1.f / 30.f;
  for (float t = _from; t <= _to; t += step, ozz::math::Min(t, _to)) {
    const float icmp = Compare(_reference, _test, t, _skeleton);
    mcmp = ozz::math::Max(mcmp, icmp);
  }

  return mcmp;
}

float Compare(const RawAnimation& _reference, const RawAnimation& _test,
              const Skeleton& _skeleton) {
  float mcmp = 0.f;
  const float step = 1.f / 30.f;
  for (float t = 0.f; t <= _reference.duration;
       t += step, ozz::math::Min(t, _reference.duration)) {
    const float icmp = Compare(_reference, _test, t, _skeleton);
    mcmp = ozz::math::Max(mcmp, icmp);
  }

  return mcmp;
}

struct GlobalRdpKey {
  bool decimable;
  int type;
  float time;
};
/*
class GlobalRdpAdapter {
 public:
  GlobalRdpAdapter() {}
  bool Decimable(const GlobalRdpKey&) const { return true; }

  GlobalRdpKey Lerp(const GlobalRdpKey& _left, const GlobalRdpKey& _right,
                    const GlobalRdpKey& _ref) const {}

  float Distance(const GlobalRdpKey& _left, const GlobalRdpKey& _right) const {
    return 0.f;
  }

 private:
};
*/
}  // namespace

bool TrackKeysEnd(const RawAnimation::JointTrack& _track, size_t t, size_t k) {
  if (t == 0) {
    return k >= _track.translations.size();
  } else if (t == 1) {
    return k >= _track.rotations.size();
  } else {
    return k >= _track.scales.size();
  }
}

float TrackKeyTime(const RawAnimation::JointTrack& _track, size_t t, size_t k) {
  if (t == 0) {
    return _track.translations[k].time;
  } else if (t == 1) {
    return _track.rotations[k].time;
  } else {
    return _track.scales[k].time;
  }
}

void RemoveKey(RawAnimation::JointTrack& _track, size_t t, size_t k) {
  if (t == 0) {
    _track.translations.erase(_track.translations.begin() + k);
  } else if (t == 1) {
    _track.rotations.erase(_track.rotations.begin() + k);
  } else {
    _track.scales.erase(_track.scales.begin() + k);
  }
}

template <typename _Track>
typename _Track::iterator FindInsertion(_Track& _dest, float _time) {
  return std::lower_bound(_dest.begin(), _dest.end(), _time,
                          [](_Track::const_reference _key, float _time) {
                            return _key.time < _time;
                          });
}

void PushKey(const RawAnimation::JointTrack& _src, size_t t, size_t k,
             RawAnimation::JointTrack* _dest) {
  if (t == 0) {
    const RawAnimation::TranslationKey& key = _src.translations[k];
    RawAnimation::JointTrack::Translations::iterator it =
        FindInsertion(_dest->translations, key.time);
    _dest->translations.insert(it, key);
  } else if (t == 1) {
    const RawAnimation::RotationKey& key = _src.rotations[k];
    RawAnimation::JointTrack::Rotations::iterator it =
        FindInsertion(_dest->rotations, key.time);
    _dest->rotations.insert(it, key);
  } else {
    const RawAnimation::ScaleKey& key = _src.scales[k];
    RawAnimation::JointTrack::Scales::iterator it =
        FindInsertion(_dest->scales, key.time);
    _dest->scales.insert(it, key);
  }
}

struct J {
  float cmp;
  size_t j, t, k;
};

typedef ozz::Vector<J>::Std JS;

void RemoveKeys(RawAnimation& _animation, const JS& _js) {
  for (size_t i = 0; i < _js.size(); ++i) {
    const J& j = _js[i];
    RawAnimation::JointTrack& _track = _animation.tracks[j.j];
    if (j.t == 0) {
      _track.translations.erase(_track.translations.begin() + j.k);
    } else if (j.t == 1) {
      _track.rotations.erase(_track.rotations.begin() + j.k);
    } else {
      _track.scales.erase(_track.scales.begin() + j.k);
    }
  }
}

template <typename _Track>
bool IsConstant(const _Track& _track, float _tolerance) {
  if (_track.size() <= 1) {
    return true;
  }
  bool constant = true;
  for (size_t i = 1; constant && i < _track.size(); ++i) {
    constant &= Compare(_track[i - 1].value, _track[i].value, _tolerance);
  }
  return constant;
}
template <typename _Track>
bool DecimateConstant(const _Track& _input, _Track* _output, float _tolerance) {
  const bool constant = IsConstant(_input, _tolerance);
  if (constant) {
    _output->clear();
    if (_input.size() > 0) {
      _output->push_back(_input[0]);
      _output->at(0).time = 0.f;
    }
  } else {
    *_output = _input;
  }
  return constant;
}

void DecimateConstants(const RawAnimation& _input, RawAnimation* _output) {
  const int num_tracks = _input.num_tracks();
  _output->name = _input.name;
  _output->duration = _input.duration;
  _output->tracks.resize(num_tracks);

  for (int i = 0; i < num_tracks; ++i) {
    const RawAnimation::JointTrack& input = _input.tracks[i];
    RawAnimation::JointTrack& output = _output->tracks[i];
    DecimateConstant(input.translations, &output.translations, 1e-6f);
    DecimateConstant(input.rotations, &output.rotations, std::cos(1e-6f * .5f));
    DecimateConstant(input.scales, &output.scales, 1e-6f);
  }
}

bool VisGlobal(const Skeleton& _skeleton, const RawAnimation& _input,
               RawAnimation* _output, float _maxerr) {
  JS js;
  //      bool mfound = false;
  J mj = {0};
  //      float mincmp = 1e9f;
  const size_t num_joints = static_cast<size_t>(_skeleton.num_joints());
  for (J j = {0};;) {
    *_output = _input;

    // Finds next key to process
    for (++j.k; TrackKeysEnd(_output->tracks[j.j], j.t, j.k); ++j.k) {
      j.k = 0;
      ++j.t;

      if (j.t >= 3) {
        j.t = 0;
        ++j.j;
        if (j.j >= num_joints) {
          break;
        }
      }
    }

    if (j.j >= num_joints) {
      break;
    }

    const float keytime = TrackKeyTime(_output->tracks[j.j], j.t, j.k);
    RemoveKey(_output->tracks[j.j], j.t, j.k);

    j.cmp = Compare(_input, *_output, _skeleton);
    if (j.cmp < _maxerr) {
      //  mincmp = cmp;
      js.push_back(j);
      //  mfound = true;
    }

    // if (mincmp < 1e-5f) {
    //   break;
    // }
  }

  if (js.size() == 0) {
    return false;
  }

  std::sort(js.begin(), js.end(),
            [](const J& a, const J& b) { return (a.k > b.k); });

  *_output = _input;
  RemoveKeys(*_output, js);

  return true;
}

typedef ozz::Vector<float>::Std KeyTimes;

template <typename _Track>
void CopyKeyTimes(const _Track& _track, KeyTimes* _key_times) {
  for (size_t i = 0; i < _track.size(); ++i) {
    _key_times->push_back(_track[i].time);
  }
}

KeyTimes FindAllKeyTimes(const RawAnimation& _animation) {
  KeyTimes key_times;
  for (int i = 0; i < _animation.num_tracks(); ++i) {
    const RawAnimation::JointTrack& track = _animation.tracks[i];
    CopyKeyTimes(track.translations, &key_times);
    CopyKeyTimes(track.rotations, &key_times);
    CopyKeyTimes(track.scales, &key_times);
  }

  std::sort(key_times.begin(), key_times.end());
  key_times.erase(std::unique(key_times.begin(), key_times.end()),
                  key_times.end());

  return key_times;
}

typedef ozz::Vector<ozz::math::Transform>::Std JointTransformKeys;
typedef ozz::Vector<JointTransformKeys>::Std TransformMatrix;

struct ErrorKey {
  float types[3];  // T R S
};
typedef ozz::Vector<ErrorKey>::Std JointErrorKeys;
typedef ozz::Vector<JointErrorKeys>::Std ErrorMatrix;

struct RemainingTrackKeys {
  ozz::Vector<int>::Std types[3];  // T R S
};
typedef ozz::Vector<RemainingTrackKeys>::Std RemainingKeys;

struct RemainingKeysIt {
  RemainingKeysIt(const RemainingKeys& _keys)
      : track(0), type(0), key(0), index(-1), keys_(_keys) {
    ++(*this);  // Setup initial state
  }

  RemainingKeysIt& operator++() {
    for (;;) {
      ++index;
      if (index == static_cast<int>(keys_[track].types[type].size())) {
        index = -1;
        ++type;
        if (type == 3) {
          type = 0;
          ++track;
          if (track == static_cast<int>(keys_.size())) {
            track = -1;
            break;
          }
        }
      } else {
        break;
      }
    }

    if (track != -1) {
      key = keys_[track].types[type][index];
    }
    return *this;
  }
  operator bool() const { return track != -1; }

  int track, type, key;
  int index;
  const RemainingKeys& keys_;
};

template <typename _ITrack, typename _OTrack>
void SetupRemainingTrackKeys(const _ITrack& _track, _OTrack* _remaining) {
  _remaining->resize(_track.size());
  std::iota(_remaining->begin(), _remaining->end(), 0);
}

RemainingKeys SetupRemainingKeys(const RawAnimation& _animation) {
  RemainingKeys remaining_keys(_animation.tracks.size());

  for (int i = 0; i < _animation.num_tracks(); ++i) {
    const RawAnimation::JointTrack& track = _animation.tracks[i];
    RemainingTrackKeys& remaining = remaining_keys[i];
    SetupRemainingTrackKeys(track.translations, &remaining.types[0]);
    SetupRemainingTrackKeys(track.rotations, &remaining.types[1]);
    SetupRemainingTrackKeys(track.scales, &remaining.types[2]);
  }

  return remaining_keys;
}

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

  const int num_tracks = _input.num_tracks();

  // Validates the skeleton matches the animation.
  if (num_tracks != _skeleton.num_joints()) {
    return false;
  }

  // First computes bone lengths, that will be used when filtering.
  const HierarchyBuilder hierarchy(&_input, &_skeleton, this);

  // Rebuilds output animation.
  _output->name = _input.name;
  _output->duration = _input.duration;
  _output->tracks.resize(num_tracks);

  RawAnimation no_constant;
  DecimateConstants(_input, &no_constant);

  if (mode == -1) {
    *_output = no_constant;
  } else if (mode <= 1) {
    const float kDescent = .8f;
    for (float factor = 1.f;; factor *= kDescent) {
      for (int i = 0; i < num_tracks; ++i) {
        const RawAnimation::JointTrack& input = no_constant.tracks[i];
        RawAnimation::JointTrack& output = _output->tracks[i];
        output.translations.clear();
        output.rotations.clear();
        output.scales.clear();

        // Gets joint specs back.
        const float joint_length = hierarchy.specs[i].length;
        const int parent = _skeleton.joint_parents()[i];
        const float parent_scale = (parent != Skeleton::kNoParent)
                                       ? hierarchy.specs[parent].scale
                                       : 1.f;
        const float tolerance = hierarchy.specs[i].tolerance * factor;

        // Filters independently T, R and S tracks.
        // This joint translation is affected by parent scale.
        const PositionAdapter tadap(parent_scale);
        Decimate(input.translations, tadap, tolerance, &output.translations);
        // This joint rotation affects children translations/length.
        const RotationAdapter radap(joint_length);
        Decimate(input.rotations, radap, tolerance, &output.rotations);
        // This joint scale affects children translations/length.
        const ScaleAdapter sadap(joint_length);
        Decimate(input.scales, sadap, tolerance, &output.scales);
      }

      if (mode == 0) {
        break;
      }

      const float cmp = Compare(no_constant, *_output, _skeleton);
      ozz::log::Log() << "factor / error : " << factor << '/' << cmp
                      << std::endl;
      if (cmp < setting.tolerance) {
        break;
      }
    }
  }

  if (mode == 2) {
    int tries = 0;
    RawAnimation current = no_constant;

    std::random_device rd;
    std::mt19937 gen(rd());

    for (size_t i = 0;; ++i) {
      RawAnimation candidate = current;

      const int rj = std::uniform_int_distribution<int>(
          0, _skeleton.num_joints() - 1)(gen);
      const int rt = std::uniform_int_distribution<>(0, 3 - 1)(gen);
      float keytime;
      if (rt == 0) {
        RawAnimation::JointTrack::Translations& keys =
            candidate.tracks[rj].translations;
        if (keys.size() <= 1) {
          continue;
        }
        const size_t rk =
            std::uniform_int_distribution<size_t>(0, keys.size() - 1)(gen);
        keytime = (keys.begin() + rk)->time;
        keys.erase(keys.begin() + rk);
      } else if (rt == 1) {
        RawAnimation::JointTrack::Rotations& keys =
            candidate.tracks[rj].rotations;
        if (keys.size() <= 1) {
          continue;
        }
        const size_t rk =
            std::uniform_int_distribution<size_t>(0, keys.size() - 1)(gen);
        keytime = (keys.begin() + rk)->time;
        keys.erase(keys.begin() + rk);
      } else {
        RawAnimation::JointTrack::Scales& keys = candidate.tracks[rj].scales;
        if (keys.size() <= 1) {
          continue;
        }
        const size_t rk =
            std::uniform_int_distribution<size_t>(0, keys.size() - 1)(gen);
        keytime = (keys.begin() + rk)->time;
        keys.erase(keys.begin() + rk);
      }

      const float cmp = Compare(no_constant, candidate, keytime - .04f,
                                keytime + .04f, _skeleton);

      ozz::log::Log() << i << "\t" << tries << "\t" << cmp << std::endl;
      if (cmp > setting.tolerance) {
        if (tries >= 1000) {
          *_output = current;
          break;
        }
        ++tries;
      } else {
        current = candidate;
        tries = 0;
      }
    }
  }

  if (mode == 3) {
    const KeyTimes& key_times = FindAllKeyTimes(no_constant);
    ozz::log::Log() << "Unique keyframe times: " << key_times.size()
                    << std::endl;

    // Allocates matrix cache.
    ErrorMatrix error_matrix(key_times.size(),
                             JointErrorKeys(_input.num_tracks()));
    TransformMatrix candidate_matrix(key_times.size(),
                                     JointTransformKeys(_input.num_tracks()));
    TransformMatrix reference_matrix(key_times.size(),
                                     JointTransformKeys(_input.num_tracks()));

    // Populates reference model space transforms.
    for (size_t i = 0; i < key_times.size(); ++i) {
      SampleModelSpace(no_constant, _skeleton, key_times[i],
                       ozz::make_range(reference_matrix[i]));
    }

    // Copy first key frames as an initial state.
    RawAnimation current;
    current.duration = no_constant.duration;
    current.name = no_constant.name;
    current.tracks.resize(no_constant.num_tracks());
    RemainingKeys remaining_keys = SetupRemainingKeys(no_constant);

    int iter = 0;
    // float from = 0.f, to = no_constant.duration;

    ozz::Vector<ozz::math::Transform>::Std current_models(
        no_constant.num_tracks());

    for (;; ++iter) {
      {
        // Populates reference model space transforms.
        float maxcmp = 0.f;
        for (size_t i = 0; i < key_times.size(); ++i) {
          SampleModelSpace(current, _skeleton, key_times[i], ozz::make_range(current_models));
          const float cmp = Compare(ozz::make_range(current_models),
                                    ozz::make_range(reference_matrix[i]));
          if (cmp > maxcmp) {
            maxcmp = cmp;
          }
        }
        if (maxcmp < .001f) {
          *_output = current;
          break;
        }
      }
      size_t track = 0, type = 0, key = 0;
      int index = 0;
      float maxcmp = 0.f;
      bool found = false;
      for (RemainingKeysIt it(remaining_keys); it; ++it) {
        // Next keyframe time
        float time =
            TrackKeyTime(no_constant.tracks[it.track], it.type, it.key);

        // Time index in matrix
        const size_t time_id =
            std::find(key_times.begin(), key_times.end(), time) -
            key_times.begin();

        RawAnimation candidate = current;
        PushKey(no_constant.tracks[it.track], it.type, it.key,
                &candidate.tracks[it.track]);

        // Updates error generated by this key (it)
        SampleModelSpace(current, _skeleton, time,
                         ozz::make_range(current_models));
        SampleModelSpace(candidate, _skeleton, time,
                         ozz::make_range(candidate_matrix[time_id]));
        const float cmp = Compare(ozz::make_range(current_models),
                                  ozz::make_range(candidate_matrix[time_id]));
        // error_matrix[time_id][it.track].types[it.type] = cmp;

        if (cmp > maxcmp) {
          maxcmp = cmp;
          // TODO Use iterator instead
          track = it.track;
          type = it.type;
          key = it.key;
          index = it.index;
          found = true;
        }
      }

      ozz::log::Log() << "Iter, err: " << iter << ", " << maxcmp << std::endl;

      if (found) {
        ozz::Vector<int>::Std& remaining = remaining_keys[track].types[type];
        remaining.erase(remaining.begin() + index);
        PushKey(no_constant.tracks[track], type, key, &current.tracks[track]);
      }

      /*
  float maxcmp = 0.f;
  size_t track, type, key;
  bool found = false;
  for (size_t i = 0; i < error_matrix.size(); ++i) {
    const JointErrorKeys& joint_errors = error_matrix[i];
    for (size_t j = 0; j < joint_errors.size(); ++j) {
      const ErrorKey& error_key = joint_errors[j];
      for (size_t t = 0; t < 3; ++t) {
        if (error_key.types[t] > maxcmp) {
          track = j;
          type = t;
          key = i;
          maxcmp = error_key.types[t];
          found = true;
        }
      }
    }
  }
      */
    }

    ozz::log::Log() << "Number of iterations: " << iter << std::endl;
  }

  /*
  // contribution of each track to global error
   for (float e = 0.f; e < 1e-3f; e += 1e-4f) {
RawAnimation truc;
VisGlobal(_skeleton, no_constant, &truc, e);
float truccmp = Compare(_input, truc, _skeleton);
ozz::log::Log() << e << "\t" << truccmp << std::endl;
}
  */

  const float cmp = Compare(_input, *_output, _skeleton);
  ozz::log::Log() << "Final cmp: " << cmp << std::endl;

  // Output animation is always valid though.
  bool valid = _output->Validate();
  if (!valid) {
    ozz::log::Err() << "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! Failed to "
                       "validate output animation"
                    << std::endl;
  }
  return valid;
}
}  // namespace offline
}  // namespace animation
}  // namespace ozz
