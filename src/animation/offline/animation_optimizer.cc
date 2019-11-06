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
#include <chrono>
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
// 3 -> hill
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
    IterateMemFun<HierarchyBuilder, &HierarchyBuilder::ComputeScaleForward> it(
        *this);
    IterateJointsDF(*_skeleton, it);

    // Computes hierarchical length, iterating skeleton backward (leaf to root).
    IterateMemFun<HierarchyBuilder, &HierarchyBuilder::ComputeLengthBackward>
        itr(*this);
    IterateJointsDFReverse(*_skeleton, itr);
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
                  const ozz::Range<ozz::math::Transform>& _models,
                  int from = -1) {
  assert(static_cast<size_t>(_skeleton.num_joints()) <= _locals.count() &&
         static_cast<size_t>(_skeleton.num_joints()) <= _models.count());

  LTMIterator it(_locals, _models);
  ozz::animation::IterateJointsDF(_skeleton, it, from);

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

struct PartialLTMCompareIterator {
  PartialLTMCompareIterator(
      const ozz::Range<const ozz::math::Transform>& _reference_models,
      const ozz::Range<const ozz::math::Transform>& _cached_locals,
      const ozz::Range<const ozz::math::Transform>& _cached_models,
      const ozz::math::Transform& _local, int _track)
      : reference_models_(_reference_models),
        cached_locals_(_cached_locals),
        cached_models_(_cached_models),
        local_(_local),
        track_(_track),
        done(false),
        err_(0.f) {}

  void operator()(int _joint, int _parent) {
    // TODO comment
    // parents[i] >= _from is true as long as "i" is a child of "_from".
    if (!done) {
      done = _joint > track_ && _parent < track_;
    }

    const ozz::math::Transform* model_cmp;
    if (!done && _joint >= track_) {
      model_cmp = &models_[_joint];
      const ozz::math::Transform& local =
          (track_ == _joint) ? local_ : cached_locals_[_joint];

      if (_parent == ozz::animation::Skeleton::kNoParent) {
        models_[_joint] = local;
      } else {
        const ozz::math::Transform& parent =
            (track_ == _joint) ? cached_models_[_parent] : models_[_parent];

        ozz::math::Transform& model = models_[_joint];
        model.translation =
            parent.translation +
            TransformVector(parent.rotation, local.translation * parent.scale),
        model.rotation = parent.rotation * local.rotation;
        model.scale = parent.scale * local.scale;
      }
    } else {
      model_cmp = &cached_models_[_joint];
    }

    const float joint_err = Compare(*model_cmp, reference_models_[_joint]);
    err_ = ozz::math::Max(err_, joint_err);
  }

  // TODO private
  float err_;

 private:
  PartialLTMCompareIterator(const PartialLTMCompareIterator&);
  void operator=(const PartialLTMCompareIterator&);

  const ozz::Range<const ozz::math::Transform>& reference_models_;
  const ozz::Range<const ozz::math::Transform>& cached_locals_;
  const ozz::Range<const ozz::math::Transform>& cached_models_;
  const ozz::math::Transform& local_;
  int track_;
  bool done;

  ozz::math::Transform models_[ozz::animation::Skeleton::kMaxJoints];
};  // namespace

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

bool SampleModelSpace(const RawAnimation& _animation,
                      const RawAnimation::JointTrack& _track, int t,
                      const Skeleton& _skeleton, float _time,
                      const ozz::Range<ozz::math::Transform>& _models) {
  ozz::math::Transform locals[ozz::animation::Skeleton::kMaxJoints];

  bool success = true;
  success &=
      ozz::animation::offline::SampleAnimation(_animation, _time, locals);
  success &= SampleTrack(_track, _time, &locals[t]);
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

}  // namespace

size_t Decimate(const RawAnimation& _animation, int _track, int _type,
                float _tolerance, RawAnimation::JointTrack* _candidate) {
  if (_type == 0) {
    PositionAdapter adap(1.f);
    Decimate(_animation.tracks[_track].translations, adap, _tolerance,
             &_candidate->translations);
    return _animation.tracks[_track].translations.size() -
           _candidate->translations.size();
  } else if (_type == 1) {
    RotationAdapter adap(.1f);
    Decimate(_animation.tracks[_track].rotations, adap, _tolerance,
             &_candidate->rotations);
    return _animation.tracks[_track].rotations.size() -
           _candidate->rotations.size();
  } else {
    ScaleAdapter adap(1.f);
    Decimate(_animation.tracks[_track].scales, adap, _tolerance,
             &_candidate->scales);
    return _animation.tracks[_track].scales.size() - _candidate->scales.size();
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
    DecimateConstant(input.translations, &output.translations, 1e-5f);
    DecimateConstant(input.rotations, &output.rotations, std::cos(1e-5f * .5f));
    DecimateConstant(input.scales, &output.scales, 1e-5f);
  }
}

typedef ozz::Vector<float>::Std KeyTimes;

template <typename _Track>
void CopyKeyTimes(const _Track& _track, KeyTimes* _key_times) {
  for (size_t i = 0; i < _track.size(); ++i) {
    _key_times->push_back(_track[i].time);
  }
}

KeyTimes CopyKeyTimes(const RawAnimation& _animation) {
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

template <typename _Track>
_Track* GetTypeTrack(RawAnimation::JointTrack* _joint_track);

template <>
RawAnimation::JointTrack::Translations* GetTypeTrack(
    RawAnimation::JointTrack* _joint_track) {
  return &_joint_track->translations;
}
template <>
RawAnimation::JointTrack::Rotations* GetTypeTrack(
    RawAnimation::JointTrack* _joint_track) {
  return &_joint_track->rotations;
}
template <>
RawAnimation::JointTrack::Scales* GetTypeTrack(
    RawAnimation::JointTrack* _joint_track) {
  return &_joint_track->scales;
}

class VTrack;

class Comparer {
 public:
  Comparer(const RawAnimation& _reference, const Skeleton& _skeleton)
      : skeleton_(_skeleton),
        reference_(_reference),
        test_(_reference),
        key_times_(CopyKeyTimes(_reference)),
        reference_models_(key_times_.size(),
                          JointTransformKeys(_reference.num_tracks())),
        cached_locals_(reference_models_),
        cached_models_(reference_models_) {
    // Populates reference model space transforms.
    for (size_t i = 0; i < key_times_.size(); ++i) {
      SampleModelSpace(_reference, _skeleton, key_times_[i],
                       ozz::make_range(reference_models_[i]));
    }

    // TODO, copy from reference
    for (size_t i = 0; i < key_times_.size(); ++i) {
      ozz::animation::offline::SampleAnimation(
          test_, key_times_[i], ozz::make_range(cached_locals_[i]));
      LocalToModel(skeleton_, ozz::make_range(cached_locals_[i]),
                   ozz::make_range(cached_models_[i]));
    }
  }

  float operator()(const RawAnimation::JointTrack::Translations& _translations,
                   size_t _track) {
    RawAnimation::JointTrack test = test_.tracks[_track];
    test.translations = _translations;
    return Compare(test, _track);
  }
  float operator()(const RawAnimation::JointTrack::Rotations& _rotations,
                   size_t _track) {
    RawAnimation::JointTrack test = test_.tracks[_track];
    test.rotations = _rotations;
    return Compare(test, _track);
  }
  float operator()(const RawAnimation::JointTrack::Scales& _scales,
                   size_t _track) {
    RawAnimation::JointTrack test = test_.tracks[_track];
    test.scales = _scales;
    return Compare(test, _track);
  }

  void UpdateCurrent(const VTrack& _vtrack);

 private:
  float Compare(const RawAnimation::JointTrack& _test, size_t _track) const {
    float max_err = 0.f;

    ozz::math::Transform local;
    for (size_t i = 0; i < key_times_.size(); ++i) {
      ozz::animation::offline::SampleTrack(_test, key_times_[i], &local);

      PartialLTMCompareIterator cmp(ozz::make_range(reference_models_[i]),
                                    ozz::make_range(cached_locals_[i]),
                                    ozz::make_range(cached_models_[i]), local,
                                    static_cast<int>(_track));

      const float err = ozz::animation::IterateJointsDF(skeleton_, cmp).err_;
      max_err = ozz::math::Max(max_err, err);
    }
    return max_err;
  }

  const Skeleton& skeleton_;
  const RawAnimation& reference_;
  RawAnimation test_;

  KeyTimes key_times_;

  TransformMatrix reference_models_;
  TransformMatrix cached_locals_;
  TransformMatrix cached_models_;
};  // namespace offline

class VTrack {
 public:
  VTrack(float _tolerance, size_t _track)
      : track_(_track),
        tolerance_(_tolerance),
        candidate_err_(0.f),
        dirty_(true) {}
  virtual ~VTrack() {}

  float Transition(Comparer& _comparer) {
    const size_t original = OriginalSize();
    const float solution = static_cast<float>(SolutionSize());
    if (original <= 1 || solution <= 1.f) {
      dirty_ = false;
      return 0.f;
    }

    float candidate = static_cast<float>(CandidateSize());
    if (dirty_ && candidate_err_ < .001f) {
      for (size_t i = 0; candidate == solution && solution > 1.f; ++i) {
        const float f = 1.f; //(.001f - candidate_err_) / .001f;
        tolerance_ *= 1.2f * f;
        Decimate(tolerance_);
        candidate = static_cast<float>(CandidateSize());
      }
      dirty_ = false;
      candidate_err_ = Compare(_comparer);
    }

    const float delta = (candidate - solution) / original;

    return candidate_err_ <= .001f ? delta : 0.f;
  }

  void Validate() {
    ValidateCandidate();
    dirty_ = true;
  }

  virtual void Copy(RawAnimation* _dest) const = 0;

  size_t track() const { return track_; }

  void set_dirty() { dirty_ = true; }

 private:
  virtual void Decimate(float _tolerance) = 0;
  virtual float Compare(Comparer& _comparer) const = 0;
  virtual void ValidateCandidate() = 0;

  virtual size_t CandidateSize() const = 0;
  virtual size_t SolutionSize() const = 0;
  virtual size_t OriginalSize() const = 0;

  const size_t track_;
  float tolerance_;
  float candidate_err_;
  bool dirty_;
};  // namespace offline

template <typename _Track, typename _Adapter>
class TTrack : public VTrack {
 public:
  // TODO find better initial case
  TTrack(const _Track& _original, const _Adapter& _adapter, float _tolerance,
         size_t _track)
      : VTrack(_tolerance, _track),
        adapter_(_adapter),
        candidate_(_original),
        solution_(_original),
        original_(_original) {}

 private:
  virtual void Decimate(float _tolerance) {
    ozz::animation::offline::Decimate(original_, adapter_, _tolerance,
                                      &candidate_);
  }

  virtual float Compare(Comparer& _comparer) const {
    return _comparer(candidate_, track());
  }

  virtual void ValidateCandidate() { solution_ = candidate_; }

  virtual void Copy(RawAnimation* _dest) const {
    RawAnimation::JointTrack& joint_track = _dest->tracks[track()];
    *GetTypeTrack<_Track>(&joint_track) = solution_;
  }

  virtual size_t CandidateSize() const { return candidate_.size(); }
  virtual size_t SolutionSize() const { return solution_.size(); }
  virtual size_t OriginalSize() const { return original_.size(); }

  _Adapter adapter_;

  _Track candidate_;
  _Track solution_;
  const _Track& original_;
};

typedef TTrack<RawAnimation::JointTrack::Translations, PositionAdapter>
    TranslationTrack;
typedef TTrack<RawAnimation::JointTrack::Rotations, RotationAdapter>
    RotationTrack;
typedef TTrack<RawAnimation::JointTrack::Scales, ScaleAdapter> ScaleTrack;

void Comparer::UpdateCurrent(const VTrack& _vtrack) {
  _vtrack.Copy(&test_);

  for (size_t i = 0; i < key_times_.size(); ++i) {
    ozz::animation::offline::SampleTrack(test_.tracks[_vtrack.track()],
                                         key_times_[i],
                                         &cached_locals_[i][_vtrack.track()]);

    // TODO, from current track
    LocalToModel(skeleton_, ozz::make_range(cached_locals_[i]),
                 ozz::make_range(cached_models_[i]));
  }
}

struct DirtyIterator {
  DirtyIterator(const ozz::Range<VTrack*>& _vtracks) : vtracks_(_vtracks) {}

  void operator()(int _joint, int) {
    vtracks_[_joint * 3 + 0]->set_dirty();
    vtracks_[_joint * 3 + 1]->set_dirty();
    vtracks_[_joint * 3 + 2]->set_dirty();
  }
  ozz::Range<VTrack*> vtracks_;

 private:
  DirtyIterator(const DirtyIterator& _it);
  void operator=(const DirtyIterator&);
};

class Stepper {
 public:
  Stepper(const RawAnimation& _original, const Skeleton& _skeleton,
          const HierarchyBuilder& _hierarchy, Comparer& _comparer)
      : original_(_original), skeleton_(_skeleton), comparer_(_comparer) {
    ozz::memory::Allocator* allocator = ozz::memory::default_allocator();
    const int num_tracks = original_.num_tracks();
    for (int i = 0; i < num_tracks; ++i) {
      const float kInitialFactor = .1f;

      // Gets joint specs back.
      const float joint_length = _hierarchy.specs[i].length;
      const int parent = _skeleton.joint_parents()[i];
      const float parent_scale = (parent != Skeleton::kNoParent)
                                     ? _hierarchy.specs[parent].scale
                                     : 1.f;
      const float tolerance =
          _hierarchy.specs[i].tolerance *
          kInitialFactor;  // / (1.f + _hierarchy.specs[i].length);
                           /*
                           (void)_hierarchy;
                       const float joint_length = .1f;
                           const float parent_scale = 1.f;
                       const float tolerance =  _hierarchy.specs[i].tolerance * kInitialFactor;// /
                       ozz::math::Max(1.f, _hierarchy.specs[i].length);
                           */
      const RawAnimation::JointTrack& track = _original.tracks[i];
      const PositionAdapter padap(joint_length);
      vtracks_.push_back(OZZ_NEW(allocator, TranslationTrack)(
          track.translations, padap, tolerance, i));
      const RotationAdapter radap(parent_scale);
      vtracks_.push_back(OZZ_NEW(allocator, RotationTrack)(
          track.rotations, radap, tolerance, i));
      const ScaleAdapter sadap(parent_scale);
      vtracks_.push_back(
          OZZ_NEW(allocator, ScaleTrack)(track.scales, sadap, tolerance, i));
    }
  }
  ~Stepper() {
    ozz::memory::Allocator* allocator = ozz::memory::default_allocator();
    for (size_t i = 0; i < vtracks_.size(); ++i) {
      OZZ_DELETE(allocator, vtracks_[i]);
    }
    vtracks_.clear();
  }

  float Transition() {
    float delta_max = 0.f;
    size_t i_max = 0;

    // Test all candidates independently
    for (size_t i = 0; i < vtracks_.size(); ++i) {
      VTrack* vtrack = vtracks_[i];
      float delta = vtrack->Transition(comparer_);

      // ozz::log::Log() << "-" << vtrack->track() << ", " << i << ": " << delta
      //                << std::endl;

      if (delta < delta_max) {
        delta_max = delta;
        i_max = i;
      }
    }

    // Validates best candidate
    if (delta_max < 0.f) {
      VTrack& selected = *vtracks_[i_max];
      selected.Validate();

      // Flag whole hierarchy of selected track, children and parents
      DirtyIterator it(ozz::make_range(vtracks_));
      ozz::animation::IterateJointsDF(skeleton_, it,
                                      static_cast<int>(selected.track()));
      for (int parent = skeleton_.joint_parents()[selected.track()];
           parent != ozz::animation::Skeleton::kNoParent;
           parent = skeleton_.joint_parents()[parent]) {
        vtracks_[parent * 3 + 0]->set_dirty();
        vtracks_[parent * 3 + 1]->set_dirty();
        vtracks_[parent * 3 + 2]->set_dirty();
      }
      comparer_.UpdateCurrent(selected);

      ozz::log::Log() << selected.track() << ", " << i_max << ": " << delta_max
                      << std::endl;
    }

    return delta_max;
  }

  void Copy(RawAnimation* _output) const {
    _output->duration = original_.duration;
    _output->name = original_.name;
    _output->tracks.resize(original_.tracks.size());
    for (size_t i = 0; i < vtracks_.size(); ++i) {
      VTrack* vtrack = vtracks_[i];
      vtrack->Copy(_output);
    }
  }

 private:
  const RawAnimation& original_;
  const Skeleton& skeleton_;
  Comparer& comparer_;
  ozz::Vector<VTrack*>::Std vtracks_;
};  // namespace offline

void HillClimb(const RawAnimation& _orignal, const Skeleton& _skeleton,
               const HierarchyBuilder& _hierarchy, RawAnimation* _output) {
  Comparer comparer(_orignal, _skeleton);
  Stepper stepper(_orignal, _skeleton, _hierarchy, comparer);
  int iter = 0;
  for (;; ++iter) {
    float delta = stepper.Transition();
    if (delta >= 0.f) {
      stepper.Copy(_output);
      break;
    }
  }
  ozz::log::Log() << "Iterations count: " << iter << std::endl;
}

bool AnimationOptimizer::operator()(const RawAnimation& _input,
                                    const Skeleton& _skeleton,
                                    RawAnimation* _output) const {
  auto start = std::chrono::system_clock::now();

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
  
  if (mode == 5) {
    HillClimb(no_constant, _skeleton, hierarchy, _output);
  }

  const float cmp = Compare(_input, *_output, _skeleton);
  ozz::log::Log() << "Final cmp: " << cmp << std::endl;

  auto end = std::chrono::system_clock::now();
  std::chrono::duration<float> elapsed_seconds = end - start;
  std::cout << "Elapsed time: " << elapsed_seconds.count() << "s\n";

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
