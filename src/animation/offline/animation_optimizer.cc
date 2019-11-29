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

#include "ozz/base/io/stream.h"

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
static int iteration;
static float gerr = 0.f;
static int gsize = 0;

namespace {
class CsvFile : private ozz::io::File {
 public:
  CsvFile(const char* _name) : ozz::io::File(_name, "wt"), first_(true) {
    if (!opened()) {
      ozz::log::Err() << "Failed opening csv file \"" << _name << "\"."
                      << std::endl;
      return;
    }
  }

  bool opened() const { return ozz::io::File::opened(); }

#define F "%f"

  bool Push(int _value) {
    char line[1024];
    sprintf(line, "%s%d", first_ ? "" : ",", _value);
    first_ = false;

    const size_t len = strlen(line);
    return len == Write(line, len);
  }
  bool Push(const char* _value) {
    char line[1024];
    sprintf(line, "%s%s", first_ ? "" : ",", _value);
    first_ = false;

    const size_t len = strlen(line);
    return len == Write(line, len);
  }
  bool Push(float _value) {
    char line[1024];
    sprintf(line, "%s" F, first_ ? "" : ",", _value);
    first_ = false;

    const size_t len = strlen(line);
    return len == Write(line, len);
  }
  bool Push(const ozz::math::Float3& _value) {
    char line[1024];
    sprintf(line, "%s" F "," F "," F, first_ ? "" : ",", _value.x, _value.y,
            _value.z);
    first_ = false;

    const size_t len = strlen(line);
    return len == Write(line, len);
  }
  bool Push(const ozz::math::Quaternion& _value) {
    char line[1024];
    sprintf(line, "%s" F "," F "," F "," F, first_ ? "" : ",", _value.w,
            _value.x, _value.y, _value.z);
    first_ = false;

    const size_t len = strlen(line);
    return len == Write(line, len);
  }

  bool Push(const ozz::math::Transform& _value) {
    bool success = true;
    success &= Push(_value.translation);
    success &= Push(_value.rotation);
    success &= Push(_value.scale);
    return success;
  }

  bool LineEnd() {
    first_ = true;
    return Write("\n", 1) == 1;
  }

 private:
  // First line element.
  bool first_;
};
}  // namespace

CsvFile* csv = NULL;
#define GENERATE_CSV ;

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
    /*

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
    */

    // The idea is to find the rotation axis of the rotation defined by the
    // difference of the two quaternion we're comparing. This will allow to find
    // a normal to this axis. Transforming this normal by the quaternion of the
    // difference will produce the worst case transformation, hence give the
    // error.
    const math::Quaternion diff = _left.value * Conjugate(_right.value);
    const math::Float3 axis = (diff.x == 0.f && diff.y == 0.f && diff.z == 0.f)
                                  ? math::Float3::x_axis()
                                  : math::Float3(diff.x, diff.y, diff.z);
    const math::Float3 abs(std::abs(axis.x), std::abs(axis.y),
                           std::abs(axis.z));
    const size_t smallest =
        abs.x < abs.y ? (abs.x < abs.z ? 0 : 2) : (abs.y < abs.z ? 1 : 2);
    const math::Float3 binormal(smallest == 0, smallest == 1, smallest == 2);
    const math::Float3 normal = Normalize(Cross(binormal, axis));
    const float error = Length(TransformVector(diff, normal) - normal);
    return error * radius_;
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

// < 0 if better than target
// = 0 if on target
// > 0 if worst than target
inline float ErrorToRatio(float _err, float _target) {
  return (_err - _target) / _target;
}

float Compare(const ozz::math::Transform& _reference,
              const ozz::math::Transform& _test, float _distance) {
  // return Length(_reference.translation - _test.translation);

  // Translation error in model space takes intrinsically into account
  // translation rotation and scale of its parents. Rotation error is only
  // impacting local space, hence the use of a distance paramete simulating
  // skinning (or any user defined requirement). Its impact on children will be
  // measured as a translation error indeed.

  const math::Quaternion diff = _reference.rotation * Conjugate(_test.rotation);
  const math::Float3 axis = (diff.x == 0.f && diff.y == 0.f && diff.z == 0.f)
                                ? math::Float3::x_axis()
                                : math::Float3(diff.x, diff.y, diff.z);
  const math::Float3 abs(std::abs(axis.x), std::abs(axis.y), std::abs(axis.z));
  const size_t smallest =
      abs.x < abs.y ? (abs.x < abs.z ? 0 : 2) : (abs.y < abs.z ? 1 : 2);
  const math::Float3 binormal(smallest == 0, smallest == 1, smallest == 2);
  const math::Float3 normal = Normalize(Cross(binormal, axis));

  const float rotation_error = Length(TransformVector(diff, normal) - normal);
  const float translation_error =
      Length(_reference.translation - _test.translation);
  //  const float scale_error =
  //      kRadius * Length(_reference.scale - _test.scale);

  const float error = translation_error + rotation_error * _distance;
  return error;
}
/*
float Compare(const ozz::Range<const ozz::math::Transform>& _reference,
              const ozz::Range<const ozz::math::Transform>& _candidate) {
  float mcmp = 0.f;
  for (size_t i = 0; i < _reference.count(); ++i) {
    const float icmp = Compare(_reference[i], _candidate[i]);
    mcmp = ozz::math::Max(mcmp, icmp);
  }
  return mcmp;
}*/

float Compare(const ozz::Range<const ozz::math::Transform>& _reference,
              const ozz::Range<const ozz::math::Transform>& _candidate,
              const ozz::Range<AnimationOptimizer::Setting>& _settings) {
  float mratio = std::numeric_limits<float>::lowest();
  for (size_t i = 0; i < _reference.count(); ++i) {
    const float distance = _settings[i].distance;
    const float err = Compare(_reference[i], _candidate[i], distance);
    const float target = _settings[i].tolerance;
    const float ratio = ErrorToRatio(err, target);
    mratio = ozz::math::Max(mratio, ratio);
  }
  return mratio;
}

struct Res {
  float err, ratio;
};

struct PartialLTMCompareIterator {
  PartialLTMCompareIterator(
      const ozz::Range<const ozz::math::Transform>& _reference_models,
      const ozz::Range<const ozz::math::Transform>& _cached_locals,
      const ozz::Range<const ozz::math::Transform>& _cached_models,
      const ozz::math::Transform& _local,
      const ozz::Range<AnimationOptimizer::Setting>& _settings, int _track)
      : reference_models_(_reference_models),
        cached_locals_(_cached_locals),
        cached_models_(_cached_models),
        local_(_local),
        settings_(_settings),
        track_(_track),
        err_{-1.f, std::numeric_limits<float>::lowest()} {}

  void operator()(int _joint, int _parent) {
    ozz::math::Transform& model = models_[_joint];

    const ozz::math::Transform& local =
        (track_ == _joint) ? local_ : cached_locals_[_joint];

    if (_parent == ozz::animation::Skeleton::kNoParent) {
      model = local;
    } else {
      const ozz::math::Transform& parent =
          (track_ == _joint) ? cached_models_[_parent] : models_[_parent];

      model.translation =
          parent.translation +
          TransformVector(parent.rotation, local.translation * parent.scale),
      model.rotation = parent.rotation * local.rotation;
      model.scale = parent.scale * local.scale;
    }

    const float distance = settings_[_joint].distance;
    const float joint_err = Compare(model, reference_models_[_joint], distance);
    const float target = settings_[_joint].tolerance;
    const float ratio = ErrorToRatio(joint_err, target);

    if (_joint == track_) {
      assert(err_.err == -1.f);  // Should be set once only
      err_.err = joint_err;
    }
    err_.ratio = ozz::math::Max(err_.ratio, ratio);
  }

  // TODO private
  Res err_;

 private:
  PartialLTMCompareIterator(const PartialLTMCompareIterator&);
  void operator=(const PartialLTMCompareIterator&);

  const ozz::Range<const ozz::math::Transform>& reference_models_;
  const ozz::Range<const ozz::math::Transform>& cached_locals_;
  const ozz::Range<const ozz::math::Transform>& cached_models_;
  const ozz::math::Transform& local_;
  ozz::Range<AnimationOptimizer::Setting> settings_;
  int track_;

  ozz::math::Transform models_[ozz::animation::Skeleton::kMaxJoints];
};

struct LTMCompareIterator {
  LTMCompareIterator(
      const ozz::Range<const ozz::math::Transform>& _reference_models,
      const ozz::Range<const ozz::math::Transform>& _locals,
      const ozz::Range<ozz::math::Transform>& _models,
      const ozz::Range<AnimationOptimizer::Setting>& _settings,
      const ozz::Range<Res>& _errors)
      : reference_models_(_reference_models),
        locals_(_locals),
        models_(_models),
        settings_(_settings),
        errors_(_errors),
        err_{-1.f, std::numeric_limits<float>::lowest()} {}

  void operator()(int _joint, int _parent) {
    ozz::math::Transform& model = models_[_joint];
    const ozz::math::Transform& local = locals_[_joint];

    if (_parent == ozz::animation::Skeleton::kNoParent) {
      model = local;
    } else {
      const ozz::math::Transform& parent = models_[_parent];
      model.translation =
          parent.translation +
          TransformVector(parent.rotation, local.translation * parent.scale),
      model.rotation = parent.rotation * local.rotation;
      model.scale = parent.scale * local.scale;
    }

    const float distance = settings_[_joint].distance;
    const float joint_err = Compare(model, reference_models_[_joint], distance);
    const float target = settings_[_joint].tolerance;
    const float ratio = ErrorToRatio(joint_err, target);

    errors_[_joint].err = joint_err;
    errors_[_joint].ratio = ratio;
  }

  // TODO private
  Res err_;

 private:
  LTMCompareIterator(const LTMCompareIterator&);
  void operator=(const LTMCompareIterator&);

  const ozz::Range<const ozz::math::Transform>& reference_models_;
  const ozz::Range<const ozz::math::Transform>& locals_;
  const ozz::Range<ozz::math::Transform>& models_;
  const ozz::Range<AnimationOptimizer::Setting>& settings_;
  const ozz::Range<Res>& errors_;
};

struct RatioBack {
  RatioBack(const ozz::Range<AnimationOptimizer::Setting>& _settings,
            const ozz::Range<Res>& _errors)
      : settings_(_settings), errors_(_errors) {}

  void operator()(int _joint, int _parent) {
    errors_[_joint].ratio =
        ErrorToRatio(errors_[_joint].err, settings_[_joint].tolerance);
    if (_parent != ozz::animation::Skeleton::kNoParent) {
      errors_[_parent].ratio =
          ozz::math::Max(errors_[_parent].ratio, errors_[_joint].ratio);
    }
  }

 private:
  RatioBack(const RatioBack&);
  void operator=(const RatioBack&);

  const ozz::Range<AnimationOptimizer::Setting>& settings_;
  const ozz::Range<Res>& errors_;
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
              float _time, const Skeleton& _skeleton,
              const ozz::Range<AnimationOptimizer::Setting>& _settings) {
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
    const float icmp =
        Compare(ref_models[i], test_models[i], _settings[i].distance);
    mcmp = ozz::math::Max(mcmp, icmp);
  }

  return mcmp;
}

/*
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
*/

float Compare(const RawAnimation& _reference, const RawAnimation& _test,
              const Skeleton& _skeleton,
              const ozz::Range<AnimationOptimizer::Setting>& _settings) {
  float mcmp = 0.f;
  const float step = 1.f / 30.f;
  for (float t = 0.f; t <= _reference.duration;
       t += step, ozz::math::Min(t, _reference.duration)) {
    const float icmp = Compare(_reference, _test, t, _skeleton, _settings);
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

typedef ozz::Vector<Res>::Std ErrorKeys;
typedef ozz::Vector<ErrorKeys>::Std ErrorMatrix;

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
        cached_locals_(key_times_.size(),
                       JointTransformKeys(_reference.num_tracks())),
        cached_models_(cached_locals_),
        cached_errors_(key_times_.size(), ErrorKeys(_reference.num_tracks())) {
    // Populates test local & model space transforms.
    for (size_t i = 0; i < key_times_.size(); ++i) {
      ozz::animation::offline::SampleAnimation(
          test_, key_times_[i], ozz::make_range(cached_locals_[i]));
      LocalToModel(skeleton_, ozz::make_range(cached_locals_[i]),
                   ozz::make_range(cached_models_[i]));
    }

    // Copy to reference
    reference_models_ = cached_models_;
  }

  struct Span {
    float begin, end;
  };
  typedef ozz::Vector<Span>::Std Spans;
  Res operator()(const RawAnimation::JointTrack::Translations& _translations,
                 size_t _track,
                 const ozz::Range<AnimationOptimizer::Setting>& _settings,
                 const Spans& _spans) {
    RawAnimation::JointTrack test = test_.tracks[_track];
    test.translations = _translations;
    return Compare(test, _track, _settings, _spans);
  }
  Res operator()(const RawAnimation::JointTrack::Rotations& _rotations,
                 size_t _track,
                 const ozz::Range<AnimationOptimizer::Setting>& _settings,
                 const Spans& _spans) {
    RawAnimation::JointTrack test = test_.tracks[_track];
    test.rotations = _rotations;
    return Compare(test, _track, _settings, _spans);
  }
  Res operator()(const RawAnimation::JointTrack::Scales& _scales, size_t _track,
                 const ozz::Range<AnimationOptimizer::Setting>& _settings,
                 const Spans& _spans) {
    RawAnimation::JointTrack test = test_.tracks[_track];
    test.scales = _scales;
    return Compare(test, _track, _settings, _spans);
  }

  float UpdateCurrent(const VTrack& _vtrack,
                      const ozz::Range<AnimationOptimizer::Setting>& _settings);

 private:
  Res Compare(const RawAnimation::JointTrack& _test, size_t _track,
              const ozz::Range<AnimationOptimizer::Setting>& _settings,
              const Spans& _spans) const {
    (void)_spans;
    Res res = {0.f, std::numeric_limits<float>::lowest()};

    ozz::math::Transform local;
    size_t s = 0;
    for (size_t i = 0; i < key_times_.size(); ++i) {
      Res ret;
      if (s < _spans.size() && (key_times_[i] <= _spans[s].begin ||
                                key_times_[i] == _spans[s].end)) {
        ret = cached_errors_[i][_track];

        if (key_times_[i] == _spans[s].end) {
          ++s;
        }
        continue;
      } else {
        ozz::animation::offline::SampleTrack(_test, key_times_[i], &local);

        PartialLTMCompareIterator cmp(ozz::make_range(reference_models_[i]),
                                      ozz::make_range(cached_locals_[i]),
                                      ozz::make_range(cached_models_[i]), local,
                                      _settings, static_cast<int>(_track));

        // TODO, iterating children of track only (and doing a mx with current
        // err) is a possible optimisation. It considers error can only increase
        // from a step to the next. It's generally true, but not always due to
        // track compensating error from others.
        ret = ozz::animation::IterateJointsDF(skeleton_, cmp,
                                              static_cast<int>(_track))
                  .err_;
      }
      res.err = ozz::math::Max(res.err, ret.err);
      res.ratio = ozz::math::Max(res.ratio, ret.ratio);
    }
    return res;
  }

  const Skeleton& skeleton_;
  const RawAnimation& reference_;
  RawAnimation test_;

  KeyTimes key_times_;

  TransformMatrix reference_models_;
  TransformMatrix cached_locals_;
  TransformMatrix cached_models_;
  ErrorMatrix cached_errors_;
};  // namespace offline

class VTrack {
 public:
  VTrack(float _tolerance, size_t _track)
      : track_(_track),
        tolerance_(_tolerance),
        candidate_err_{0.f, -1.f},
        dirty_(true) {}
  virtual ~VTrack() {}

  float Transition(Comparer& _comparer,
                   const ozz::Range<AnimationOptimizer::Setting>& _settings) {
    const size_t original = OriginalSize();
    const float solution = static_cast<float>(SolutionSize());
    float candidate = static_cast<float>(CandidateSize());

    if (original <= 1 || solution <= 1.f) {
      candidate_err_.ratio = 0.f;
      dirty_ = false;
    } else if (dirty_ && candidate_err_.ratio < 0) {
      for (size_t i = 0; candidate >= solution && solution > 1.f; ++i) {
        const float mul = 1.1f + -candidate_err_.ratio * .3f;  // * f * 1.f;
        tolerance_ *= mul;
        Decimate(tolerance_);
        candidate = static_cast<float>(CandidateSize());
      }
      dirty_ = false;
      candidate_err_ =
          /*ozz::math::Max(candidate_err_,*/ Compare(_comparer,
                                                     _settings) /*)*/;
    }

    // Delta must be computed whever track is dirty or not
    float err_ratio = candidate_err_.ratio;
    // float delta = solution > 1.f ? err_ratio : 0;  //(candidate - solution) /
    // (original);
    float delta = err_ratio * (solution - candidate) / (original);
    // float delta = (candidate - solution) / (original);

    if (csv) {
      csv->Push(static_cast<int>(original));
      csv->Push(solution);
      csv->Push(static_cast<float>(CandidateSize()));
      csv->Push(tolerance_);
      csv->Push(delta);
      const float target = _settings[track()].tolerance;
      csv->Push(target);
      csv->Push(candidate_err_.err);
    }

    // TODO Use ratio instead
    return delta;
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
  virtual Res Compare(
      Comparer& _comparer,
      const ozz::Range<AnimationOptimizer::Setting>& _settings) const = 0;
  virtual void ValidateCandidate() = 0;

  virtual size_t CandidateSize() const = 0;
  virtual size_t SolutionSize() const = 0;
  virtual size_t OriginalSize() const = 0;

  const size_t track_;
  float tolerance_;
  Res candidate_err_;
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
        original_size_(_original.size()) {}

 private:
  virtual void Decimate(float _tolerance) {
    // TODO proove decimate doesn't need original track
    ozz::animation::offline::Decimate(solution_, adapter_, _tolerance,
                                      &candidate_);
  }

  virtual Res Compare(
      Comparer& _comparer,
      const ozz::Range<AnimationOptimizer::Setting>& _settings) const {
    Comparer::Spans spans;

    bool in = false;
    Comparer::Span next;
    for (size_t o = 0, c = 0; o < solution_.size() && c < candidate_.size();
         ++o) {
      if (solution_[o].time == candidate_[c].time) {
        if (in) {
          next.end = solution_[o].time;
          spans.push_back(next);
          in = false;
        }
        next.begin = solution_[o].time;
        ++c;
      } else {
        in = true;
      }
    }
    if (in) {
      next.end = solution_.back().time;
      spans.push_back(next);
    }
    // Comparer::Span s = {solution_.front().time, solution_.back().time};
    // spans.push_back(s);

    return _comparer(candidate_, track(), _settings, spans);
  }

  virtual void ValidateCandidate() { solution_ = candidate_; }

  virtual void Copy(RawAnimation* _dest) const {
    RawAnimation::JointTrack& joint_track = _dest->tracks[track()];
    *GetTypeTrack<_Track>(&joint_track) = solution_;
  }

  virtual size_t CandidateSize() const { return candidate_.size(); }
  virtual size_t SolutionSize() const { return solution_.size(); }
  virtual size_t OriginalSize() const { return original_size_; }

  _Adapter adapter_;

  _Track candidate_;
  _Track solution_;
  size_t original_size_;
};  // namespace offline

typedef TTrack<RawAnimation::JointTrack::Translations, PositionAdapter>
    TranslationTrack;
typedef TTrack<RawAnimation::JointTrack::Rotations, RotationAdapter>
    RotationTrack;
typedef TTrack<RawAnimation::JointTrack::Scales, ScaleAdapter> ScaleTrack;

float Comparer::UpdateCurrent(
    const VTrack& _vtrack,
    const ozz::Range<AnimationOptimizer::Setting>& _settings) {
  _vtrack.Copy(&test_);

  gsize = static_cast<int>(test_.size());

  // Res res = { 0.f, 0.f};
  float max_err = std::numeric_limits<float>::lowest();

  for (size_t i = 0; i < key_times_.size(); ++i) {
    const size_t track = _vtrack.track();

    ozz::animation::offline::SampleTrack(test_.tracks[track], key_times_[i],
                                         &cached_locals_[i][track]);
    /*
    LocalToModel(skeleton_, ozz::make_range(cached_locals_[i]),
                 ozz::make_range(cached_models_[i]), static_cast<int>(track));

    */

    LTMCompareIterator cmp(ozz::make_range(reference_models_[i]),
                           ozz::make_range(cached_locals_[i]),
                           ozz::make_range(cached_models_[i]), _settings,
                           ozz::make_range(cached_errors_[i]));

    ozz::animation::IterateJointsDF(skeleton_, cmp, static_cast<int>(track));

    RatioBack bck(_settings, ozz::make_range(cached_errors_[i]));
    ozz::animation::IterateJointsDFReverse(skeleton_, bck);
  }

  return max_err;
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
  Stepper(const AnimationOptimizer& _optimizer, RawAnimation& _original,
          const Skeleton& _skeleton, const HierarchyBuilder& _hierarchy,
          Comparer& _comparer)
      : original_(_original), skeleton_(_skeleton), comparer_(_comparer) {
    ozz::memory::Allocator* allocator = ozz::memory::default_allocator();

    const int num_tracks = original_.num_tracks();
    for (int i = 0; i < num_tracks; ++i) {
      const float kInitialFactor = .2f;

      // Using hierarchy spec for joint_length (aka comparing rotations) is
      // important as it prevents from optimizing too quickly joints at the
      // root. This length helps RotationAdapter at computing/simulating the
      // tolerance at hierarchy end. Hill climbing will have the last word,
      // figuring out if this decimation is ok or not in the end.
      const float joint_length = _hierarchy.specs[i].length;
      // const float joint_length = .1f;

      /*
      const int parent = _skeleton.joint_parents()[i];
      const float parent_scale = (parent != Skeleton::kNoParent)
                                 ? _hierarchy.specs[parent].scale
                                 : 1.f;*/
      const float parent_scale = 1.f;

      const AnimationOptimizer::Setting& setting =
          GetJointSetting(_optimizer, i);
      vsettings_.push_back(setting);

      const float initial = _hierarchy.specs[i].tolerance * kInitialFactor;
      const RawAnimation::JointTrack& track = _original.tracks[i];
      const PositionAdapter padap(parent_scale);
      vtracks_.push_back(OZZ_NEW(allocator, TranslationTrack)(
          track.translations, padap, initial, i));
      const RotationAdapter radap(joint_length);
      vtracks_.push_back(OZZ_NEW(allocator, RotationTrack)(track.rotations,
                                                           radap, initial, i));
      const ScaleAdapter sadap(joint_length);
      vtracks_.push_back(
          OZZ_NEW(allocator, ScaleTrack)(track.scales, sadap, initial, i));
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

      if (csv) {
        csv->Push(iteration);
        int ii = static_cast<int>(i);
        csv->Push(ii / 3);  // joint
        csv->Push(ii % 3);  // track type
      }

      float delta = vtrack->Transition(comparer_, ozz::make_range(vsettings_));

      if (csv) {
        csv->Push(gerr);
        csv->Push(gsize);
        csv->LineEnd();
      }

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
      FlagDirty(static_cast<int>(selected.track()));

      gerr = comparer_.UpdateCurrent(selected, ozz::make_range(vsettings_));

      ozz::log::Log() << selected.track() << ", " << i_max << ": " << delta_max
                      << std::endl;
    }

    return delta_max;
  }

  void FlagDirty(int _track) {
    // Flag whole children hierarchy of selected track
    DirtyIterator it(ozz::make_range(vtracks_));
    ozz::animation::IterateJointsDF(skeleton_, it, _track);

    // Now parents, as their error can change due to a children track change.
    for (int parent = skeleton_.joint_parents()[_track];
         parent != ozz::animation::Skeleton::kNoParent;
         parent = skeleton_.joint_parents()[parent]) {
      vtracks_[parent * 3 + 0]->set_dirty();
      vtracks_[parent * 3 + 1]->set_dirty();
      vtracks_[parent * 3 + 2]->set_dirty();
    }
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
  ozz::Vector<AnimationOptimizer::Setting>::Std vsettings_;
};  // namespace offline

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

#ifdef GENERATE_CSV
  CsvFile out("hill_climbing.cvs");
  csv = &out;

  csv->Push(
      "iteration,joint,type,original,solution,candidate,tolerance,delta,target,"
      "terr,"
      "gerr,gsize");
  csv->LineEnd();

#endif  // GENERATE_CSV

  // First computes bone lengths, that will be used when filtering.
  const HierarchyBuilder hierarchy(&_input, &_skeleton, this);

  // TEMP
  ozz::Vector<AnimationOptimizer::Setting>::Std vsettings;
  for (int i = 0; i < _skeleton.num_joints(); ++i) {
    vsettings.push_back(GetJointSetting(*this, i));
  }

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

      const float cmp =
          Compare(no_constant, *_output, _skeleton, ozz::make_range(vsettings));
      ozz::log::Log() << "factor / error : " << factor << '/' << cmp
                      << std::endl;
      if (cmp < setting.tolerance) {
        break;
      }
    }
  }

  if (mode == 5) {
    Comparer comparer(no_constant, _skeleton);
    Stepper stepper(*this, no_constant, _skeleton, hierarchy, comparer);
    iteration = 0;
    for (;; ++iteration) {
      float delta = stepper.Transition();

      if (delta >= 0.f) {
        stepper.Copy(_output);
        break;
      }
    }
    ozz::log::Log() << "Iterations count: " << iteration << std::endl;
  }

  const float cmp =
      Compare(_input, *_output, _skeleton, ozz::make_range(vsettings));
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
