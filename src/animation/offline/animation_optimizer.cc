//----------------------------------------------------------------------------//
//                                                                            //
// ozz-animation is hosted at http://github.com/guillaumeblanc/ozz-animation  //
// and distributed under the MIT License (MIT).                               //
//                                                                            //
// Copyright (c) Guillaume Blanc                                              //
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

#include <algorithm>
#include <cassert>
#include <functional>
#include <limits>

// Internal include file
#include "ozz/animation/offline/raw_animation.h"
#include "ozz/animation/offline/raw_animation_utils.h"
#include "ozz/animation/runtime/skeleton.h"
#include "ozz/animation/runtime/skeleton_utils.h"
#include "ozz/base/containers/stack.h"
#include "ozz/base/containers/vector.h"
#include "ozz/base/io/stream.h"
#include "ozz/base/log.h"
#include "ozz/base/maths/math_constant.h"
#include "ozz/base/maths/math_ex.h"
#include "ozz/base/memory/unique_ptr.h"

#define OZZ_INCLUDE_PRIVATE_HEADER  // Allows to include private headers.
#include "animation/offline/decimate.h"

namespace ozz {
namespace animation {
namespace offline {

// Setup default values (favoring quality).
AnimationOptimizer::AnimationOptimizer() : fast(false), observer(NULL) {}

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
  HierarchyBuilder(const RawAnimation& _animation, const Skeleton& _skeleton,
                   const AnimationOptimizer& _optimizer)
      : specs(_animation.tracks.size()),
        animation(_animation),
        optimizer(_optimizer) {
    assert(_animation.num_tracks() == _skeleton.num_joints());

    // Computes hierarchical scale, iterating skeleton forward (root to
    // leaf).
    IterateJointsDF(_skeleton,
                    std::bind(&HierarchyBuilder::ComputeScaleForward, this,
                              std::placeholders::_1, std::placeholders::_2));

    // Computes hierarchical length, iterating skeleton backward (leaf to root).
    IterateJointsDFReverse(
        _skeleton, std::bind(&HierarchyBuilder::ComputeLengthBackward, this,
                             std::placeholders::_1, std::placeholders::_2));
  }

  struct Spec {
    float length;  // Length of a joint hierarchy (max of all child).
    float scale;   // Scale of a joint hierarchy (accumulated from all parents).
    float tolerance;  // Tolerance of a joint hierarchy (min of all child).
  };

  // Defines the length of a joint hierarchy (of all child).
  ozz::vector<Spec> specs;

 private:
  // Extracts maximum translations and scales for each track/joint.
  void ComputeScaleForward(int _joint, int _parent) {
    Spec& joint_spec = specs[_joint];

    // Compute joint maximum animated scale.
    float max_scale = 0.f;
    const RawAnimation::JointTrack& track = animation.tracks[_joint];
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
        GetJointSetting(optimizer, _joint);
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
    const RawAnimation::JointTrack& track = animation.tracks[_joint];
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
  const RawAnimation& animation;

  // Usefull to access settings and compute hierarchy length.
  const AnimationOptimizer& optimizer;
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
    // cos_half_angle is w component of a-1 * b.
    const float cos_half_angle = Dot(_left.value, _right.value);
    const float sine_half_angle =
        std::sqrt(1.f - math::Min(1.f, cos_half_angle * cos_half_angle));
    // Deduces distance between 2 points on a circle with radius and a given
    // angle. Using half angle helps as it allows to have a right-angle
    // triangle.
    const float distance = 2.f * sine_half_angle * radius_;
    return distance;
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
  LTMIterator(const ozz::span<const ozz::math::Transform>& _locals,
              const ozz::span<ozz::math::Transform>& _models)
      : locals_(_locals),
        models_(_models),
        models_out_(_models),
        local_overload_(),
        joint_overload_(-1) {}

  LTMIterator(const ozz::span<const ozz::math::Transform>& _locals,
              const ozz::math::Transform& _local_overload, int _joint_overload,
              const ozz::span<const ozz::math::Transform>& _models,
              const ozz::span<ozz::math::Transform>& _models_out)
      : locals_(_locals),
        models_(_models),
        models_out_(_models_out),
        local_overload_(_local_overload),
        joint_overload_(_joint_overload) {}

  void operator()(int _joint, int _parent) {
    // Test if overloaded local shall be used.
    const ozz::math::Transform& local =
        joint_overload_ == _joint ? local_overload_ : locals_[_joint];

    ozz::math::Transform& model_out = models_out_[_joint];
    if (_parent == ozz::animation::Skeleton::kNoParent) {
      model_out = local;
    } else {
      // Picks parent's transform from precomputed model space transforms, or
      // newly updated ones.
      const ozz::math::Transform& parent =
          joint_overload_ == _joint ? models_[_parent] : models_out_[_parent];

      // Computes model space TRS transform.
      model_out.translation =
          parent.translation +
          TransformVector(parent.rotation, local.translation * parent.scale);
      model_out.rotation = parent.rotation * local.rotation;
      model_out.scale = parent.scale * local.scale;
    }
  }

 private:
  void operator=(const LTMIterator&);

  ozz::span<const ozz::math::Transform> locals_;
  ozz::span<const ozz::math::Transform> models_;
  ozz::span<ozz::math::Transform> models_out_;
  const ozz::math::Transform local_overload_;
  int joint_overload_;
};

template <typename _Track, typename _Times>
inline void CopyKeyTimes(const _Track& _track, _Times* _key_times) {
  for (size_t i = 0; i < _track.size(); ++i) {
    _key_times->push_back(_track[i].time);
  }
}

template <typename _Times>
inline void SetupKeyTimes(const RawAnimation& _animation, _Times* _key_times) {
  // Gets union of all possible keyframe times.
  for (int i = 0; i < _animation.num_tracks(); ++i) {
    const RawAnimation::JointTrack& track = _animation.tracks[i];
    CopyKeyTimes(track.translations, _key_times);
    CopyKeyTimes(track.rotations, _key_times);
    CopyKeyTimes(track.scales, _key_times);
  }
  std::sort(_key_times->begin(), _key_times->end());
  _key_times->erase(std::unique(_key_times->begin(), _key_times->end()),
                    _key_times->end());
}

inline float Compare(const ozz::math::Transform& _reference,
                     const ozz::math::Transform& _test, float _distance) {
  // return Length(_reference.translation - _test.translation);

  // Translation error in model space takes intrinsically into account
  // translation rotation and scale of its parents. Rotation error is only
  // impacting local space, hence the use of a distance paramete simulating
  // skinning (or any user defined requirement). Its impact on children will be
  // measured as a translation error indeed.

  // Finds quaternion difference.
  const math::Quaternion diff = _reference.rotation * Conjugate(_test.rotation);

  // Finds rotation axis and normal to rotation axis.
  const math::Float3 axis = (diff.x == 0.f && diff.y == 0.f && diff.z == 0.f)
                                ? math::Float3::x_axis()
                                : math::Float3(diff.x, diff.y, diff.z);
  const math::Float3 abs(std::abs(axis.x), std::abs(axis.y), std::abs(axis.z));
  const size_t smallest =
      abs.x < abs.y ? (abs.x < abs.z ? 0 : 2) : (abs.y < abs.z ? 1 : 2);
  const math::Float3 binormal(smallest == 0, smallest == 1, smallest == 2);
  const math::Float3 normal = NormalizeSafe(Cross(binormal, axis), binormal);

  // Find scale maximum component.
  const math::Float3 sabs(std::abs(_reference.scale.x),
                          std::abs(_reference.scale.y),
                          std::abs(_reference.scale.z));
  const size_t biggest =
      sabs.x > sabs.y ? (sabs.x > sabs.z ? 0 : 2) : (sabs.y > sabs.z ? 1 : 2);

  // Compute rotation error on normal vector, at scaled distance.
  const float rotation_error = Length(TransformVector(diff, normal) - normal) *
                               _distance * (&sabs.x)[biggest];

  // Model space translation error is measured at joint position.
  const float translation_error =
      Length(_reference.translation - _test.translation);

  // Scale is affecting vertex distance.
  const float scale_error = _distance * Length(_reference.scale - _test.scale);

  // Errors are accumulated
  const float error = translation_error + rotation_error + scale_error;
  return error;
}

class CompareIterator {
 public:
  CompareIterator(
      const ozz::span<const ozz::math::Transform>& _reference_models,
      const ozz::span<ozz::math::Transform>& _models,
      const ozz::span<const AnimationOptimizer::Setting>& _settings,
      const ozz::span<float>& _errors)
      : reference_models_(_reference_models),
        models_(_models),
        settings_(_settings),
        errors_(_errors) {
    assert(reference_models_.size() == models_.size() &&
           reference_models_.size() == settings_.size() &&
           reference_models_.size() == errors_.size());
  }

  void operator()(int _joint, int /*_parent*/) {
    const float distance = settings_[_joint].distance;
    const float err =
        Compare(reference_models_[_joint], models_[_joint], distance);
    errors_[_joint] = err;
  }

 private:
  void operator=(const CompareIterator&);

  const ozz::span<const ozz::math::Transform> reference_models_;
  const ozz::span<const ozz::math::Transform> models_;
  const ozz::span<const AnimationOptimizer::Setting> settings_;

  const ozz::span<float> errors_;
};

class RatioIterator {
 public:
  RatioIterator(const ozz::span<const AnimationOptimizer::Setting>& _settings,
                const ozz::span<const float>& _errors)
      : settings_(_settings),
        errors_(_errors),
        ratio_(-std::numeric_limits<float>::max()) {
    assert(settings_.size() == errors_.size());
  }

  void operator()(int _joint, int /*_parent*/) {
    // < 0 if better than target
    // = 0 if on target
    // > 0 if worst than target
    const float ratio = (errors_[_joint] - settings_[_joint].tolerance) /
                        settings_[_joint].tolerance;

    // Stores worst ratio of the hierarchy.
    ratio_ = ozz::math::Max(ratio_, ratio);
  }

  float ratio() const { return ratio_; }

 private:
  void operator=(const RatioIterator&);

  const ozz::span<const AnimationOptimizer::Setting> settings_;
  const ozz::span<const float> errors_;

  float ratio_;
};

template <typename _Track>
struct TransformComponent;
template <>
struct TransformComponent<RawAnimation::JointTrack::Translations> {
  static math::Float3* Get(math::Transform* _transform) {
    return &_transform->translation;
  }
};
template <>
struct TransformComponent<RawAnimation::JointTrack::Rotations> {
  static math::Quaternion* Get(math::Transform* _transform) {
    return &_transform->rotation;
  }
};
template <>
struct TransformComponent<RawAnimation::JointTrack::Scales> {
  static math::Float3* Get(math::Transform* _transform) {
    return &_transform->scale;
  }
};

template <typename _Track>
struct TrackComponent;
template <>
struct TrackComponent<RawAnimation::JointTrack::Translations> {
  static const RawAnimation::JointTrack::Translations& Get(
      const RawAnimation::JointTrack& _track) {
    return _track.translations;
  }
};
template <>
struct TrackComponent<RawAnimation::JointTrack::Rotations> {
  static const RawAnimation::JointTrack::Rotations& Get(
      const RawAnimation::JointTrack& _track) {
    return _track.rotations;
  }
};
template <>
struct TrackComponent<RawAnimation::JointTrack::Scales> {
  static const RawAnimation::JointTrack::Scales& Get(
      const RawAnimation::JointTrack& _track) {
    return _track.scales;
  }
};

// Helper class around _included vector. It allows to detect know whever a time
// value is with a span of time that must be updated (aka, whoses keyframes were
// decimated since last update).
template <typename _Track>
class Spanner {
 public:
  Spanner(const _Track& _track, const ozz::vector<bool>& _included)
      : track_(_track), included_(_included), span_end_(0), inside_(false) {
    assert(track_.size() == included_.size());
    ++*this;
  }

  // Returns true if time is inside a span of updated (aka not-included)
  // keyframes.
  bool Update(float _time) {
    const bool inside = _time > begin_ && _time < end_;
    if (inside_ && _time >= end_) {
      ++*this;
    }
    inside_ = inside;
    return inside_;
  }

 private:
  Spanner(const Spanner&);
  void operator=(const Spanner&);

  Spanner& operator++() {
    const size_t size = included_.size();

    // Finds first removed keyframe.
    size_t span_begin = span_end_;
    for (; span_begin < size && included_[span_begin]; ++span_begin) {
    }

    // Span starts at previous keyframe
    begin_ = span_begin == 0 ? 0.f : track_[span_begin - 1].time;

    // Find next maintained keyframe.
    for (span_end_ = span_begin + 1; span_end_ < size && !included_[span_end_];
         ++span_end_) {
    }

    // Span ends at next keyframe.
    end_ = span_end_ >= size ? std::numeric_limits<float>::max()
                             : track_[span_end_].time;
    return *this;
  }

  const _Track& track_;
  const ozz::vector<bool>& included_;

  size_t span_end_;
  bool inside_;
  float begin_;
  float end_;
};

class Comparer {
 public:
  Comparer(const RawAnimation& _original, const RawAnimation& _solution,
           const Skeleton& _skeleton,
           const ozz::span<const AnimationOptimizer::Setting>& _settings)
      : solution_(_solution), skeleton_(_skeleton), settings_(_settings) {
    // Comparison only needs to happen at each of the keyframe times.
    // So we get the union of all possible keyframe times
    SetupKeyTimes(_original, &key_times_);

    const size_t key_times_count = key_times_.size();
    const int joints_count = _skeleton.num_joints();
    solution_locals_.resize(key_times_count, SkeletonTransforms(joints_count));
    solution_models_ = solution_locals_;

    // Populates test local & model space transforms.
    for (size_t i = 0; i < key_times_.size(); ++i) {
      ozz::animation::offline::SampleAnimation(
          _original, key_times_[i], ozz::make_span(solution_locals_[i]), false);

      ozz::animation::IterateJointsDF(
          _skeleton, LTMIterator(ozz::make_span(solution_locals_[i]),
                                 ozz::make_span(solution_models_[i])));
    }

    // All tracks have 0 error.
    track_own_errors_.resize(joints_count, 0.f);

    // Solution animation equals reference at initialization time.
    reference_models_ = solution_models_;
    cached_errors_.resize(key_times_count, TrackErrors(joints_count, 0.f));
  }

  // Updates cached error matrix following an update of _joint
  // Solution animation track shall have already been update.
  template <typename _Track>
  void UpdateError(const _Track& _track, int _joint,
                   const ozz::vector<bool>& _included) {
    Spanner<_Track> spanner(
        TrackComponent<_Track>::Get(solution_.tracks[_joint]), _included);

    // Store maximum of this track error.
    float track_own_error = 0.f;

    // Iterates all keyframe times and update relevant ones.
    for (size_t i = 0; i < key_times_.size(); ++i) {
      const float key_time = key_times_[i];
      if (spanner.Update(key_time)) {
        // Update joint local and model space
        SampleTrackComponent(
            _track, key_time,
            TransformComponent<_Track>::Get(&solution_locals_[i][_joint]),
            false);

        // Updates model space
        ozz::animation::IterateJointsDF(
            skeleton_,
            LTMIterator(ozz::make_span(solution_locals_[i]),
                        ozz::make_span(solution_models_[i])),
            _joint);

        // Compare
        IterateJointsDF(
            skeleton_,
            CompareIterator(make_span(reference_models_[i]),
                            make_span(solution_models_[i]), settings_,
                            make_span(cached_errors_[i])),
            _joint);
      }

      track_own_error = math::Max(track_own_error, cached_errors_[i][_joint]);
    }
    track_own_errors_[_joint] = track_own_error;
  }

  template <typename _Track>
  float EstimateError(const _Track& _track, int _joint,
                      const ozz::vector<bool>& _included) const {
    // Temporary arrays used to store estimated values without modyfing current
    // ones (ensured for const function).
    // Only a subrange of those array might be actually used, depending on
    // _joint hierarchy. A 10x optimization factor was measured compared to
    // copying the full array each test.
    SkeletonTransforms models(solution_.tracks.size());
    TrackErrors errors(solution_.tracks.size());

    Spanner<_Track> spanner(
        TrackComponent<_Track>::Get(solution_.tracks[_joint]), _included);

    float worst_ratio = -std::numeric_limits<float>::max();
    // Checking worst_ratio < 0.f is an early out optimization. It prevents from
    // knowing the real error ratio though.
    // Note: Loops though all time range, but exits as soon as worst_ratio is
    // past the limit. This means the real worst ratio is not computed.
    for (size_t i = 0; i < key_times_.size() && worst_ratio < 0.f; ++i) {
      const float key_time = key_times_[i];
      if (!spanner.Update(key_time)) {
        // Reuses precomputed errors
        const float ratio =
            IterateJointsDF(
                skeleton_,
                RatioIterator(settings_, make_span(cached_errors_[i])), _joint)
                .ratio();

        worst_ratio = ozz::math::Max(worst_ratio, ratio);
      } else {
        // Error value needs to be recomputed, as some keyframes were modified
        // for this key_time.

        // Samples new local.
        ozz::math::Transform local = solution_locals_[i][_joint];
        SampleTrackComponent(_track, key_time,
                             TransformComponent<_Track>::Get(&local), false);

        // Updates LTM, only the relevant ones for _joint hierarchy are written.
        ozz::animation::IterateJointsDF(
            skeleton_,
            LTMIterator(ozz::make_span(solution_locals_[i]), local, _joint,
                        ozz::make_span(solution_models_[i]),
                        ozz::make_span(models)),
            _joint);

        // Compare, only the relevant ones for _joint hierarchy are written.
        IterateJointsDF(
            skeleton_,
            CompareIterator(make_span(reference_models_[i]), make_span(models),
                            settings_, make_span(errors)),
            _joint);

        // Gets joint error ratio, which is the worst ratio of _joint's
        // hierarchy.
        const float ratio =
            IterateJointsDF(skeleton_,
                            RatioIterator(settings_, make_span(errors)), _joint)
                .ratio();

        // Stores worst ratio of whole time range.
        worst_ratio = ozz::math::Max(worst_ratio, ratio);
      }
    }

    return worst_ratio;
  }

  // Gets track current error
  float track_own_error(size_t _joint) const {
    return track_own_errors_[_joint];
  }

 private:
  Comparer(const Comparer&);
  void operator=(const Comparer&);

  const RawAnimation& solution_;
  const Skeleton& skeleton_;
  const ozz::span<const AnimationOptimizer::Setting> settings_;

  typedef ozz::vector<ozz::math::Transform> SkeletonTransforms;
  typedef ozz::vector<SkeletonTransforms> SkeletonTransformKeys;

  // All model-space transforms for the reference animation.
  SkeletonTransformKeys reference_models_;

  // Locals and models-space transforms for the solution animation.
  SkeletonTransformKeys solution_locals_;
  SkeletonTransformKeys solution_models_;

  // Error metric of each track, independently. Index is track number.
  typedef ozz::vector<float> TrackErrors;
  TrackErrors track_own_errors_;

  // Index is keyframe time.
  typedef ozz::vector<TrackErrors> TrackErrorKeys;
  TrackErrorKeys cached_errors_;

  // Union of all keyframe times.
  typedef ozz::vector<float> KeyTimes;
  ozz::vector<float> key_times_;
};

// Abstract class used for hill climbing algorithm
class VTrack {
 public:
  VTrack(float _initial_tolerance, int _joint)
      : joint_(_joint),
        tolerance_(_initial_tolerance),
        hierarchy_error_ratio_(-1.f),
        optimization_delta_(-1.f) {}

  virtual ~VTrack() {}

  // Computes transition to next solution.
  void Transition() {
    size_t loops = 0;
    const size_t kMaxLoops = 1000;
    for (size_t candidate_size = CandidateSize(),
                validated_size = ValidatedSize();
         loops < kMaxLoops && candidate_size > 1 &&
         candidate_size == validated_size;
         ++loops, candidate_size = CandidateSize()) {
      // Decimates validated track in order to find next candidate track.
      Decimate(tolerance_);

      // Computes next tolerance to use for decimation.
      assert(hierarchy_error_ratio_ < 0.f);

      // TODO: this equation shall be exposed to advanced settings.
      const float mul = 1.2f + -hierarchy_error_ratio_ * .3f;
      tolerance_ *= mul;
    }

    if (loops == kMaxLoops) {
      log::LogV() << "Track could not be decimated with the maximum number of "
                     "tries allowed. Tolerance setting is probably too small."
                  << std::endl;
    }
  }

  // Updates error ratio of the candidate track compared to original.
  void UpdateDelta(const Comparer& _comparer) {
    // (original_size > 1) is an optimization that prevents rebuilding error for
    // constant tracks. They haven't been decimated, so they don't own any
    // error.
    // (hierarchy_error_ratio__ < 0.f) is an optimization. If error is already
    // too big, don't mind updating it.
    const size_t original_size = OriginalSize();
    if (original_size > 1 && hierarchy_error_ratio_ < 0.f) {
      // Computes error ratio of this track and its hierarchy.
      hierarchy_error_ratio_ = EstimateCandidateError(_comparer);

      // Computes optimization delta from error.
      const size_t validated_size = ValidatedSize();
      const size_t candidate_size = CandidateSize();
      const float size_ratio =
          static_cast<float>(validated_size - candidate_size) /
          static_cast<float>(original_size);
      optimization_delta_ = hierarchy_error_ratio_ * size_ratio;
    } else {
      hierarchy_error_ratio_ = 0.f;
      optimization_delta_ = 0.f;
    }
  }

  // Proposed candidate was validated, must be retained as a solution.
  virtual void Validate(Comparer& _comparer) = 0;

  virtual size_t OriginalSize() const = 0;
  virtual size_t ValidatedSize() const = 0;
  virtual size_t CandidateSize() const = 0;

  virtual int Type() const = 0;

  // -1 < x < 0 if transition pass is within tolerance range. The minimum
  // the better.
  // x >= 0 if transition pass is exceeding tolerance range.
  float optimization_delta() const { return optimization_delta_; }

  int joint() const { return joint_; }

  float tolerance() const { return tolerance_; }
  float hierarchy_error_ratio() const { return hierarchy_error_ratio_; }

 private:
  virtual float EstimateCandidateError(const Comparer& _comparer) const = 0;

  // Decimates track at given _tolerance;
  virtual void Decimate(float _tolerance) = 0;

  // Joint that this track applies to.
  int joint_;

  // Error tolerance of candidate track.
  float tolerance_;

  // Error value for candidate track.
  // Takes into account its whole hierarchy.
  float hierarchy_error_ratio_;

  // Optimization delta for candidate track.
  float optimization_delta_;
};

template <typename _Track, typename _Adapter, int _Type>
class TTrack : public VTrack {
 public:
  TTrack(const _Track& _original, _Track* _solution, const _Adapter& _adapter,
         float _initial_tolerance, int _joint)
      : VTrack(_initial_tolerance, _joint),
        adapter_(_adapter),
        original_(&_original),
        validated_(_solution),
        candidate_(_original) {
    // Initialize validated track with a copy of original.
    *validated_ = _original;
  }

 private:
  TTrack(const TTrack&);
  TTrack& operator=(const TTrack&);

  virtual float EstimateCandidateError(const Comparer& _comparer) const {
    return _comparer.EstimateError(candidate_, joint(), included_);
  }

  virtual void Validate(Comparer& _comparer) {
    _comparer.UpdateError(candidate_, joint(), included_);

    *validated_ = candidate_;

    // included_ keyframes vector is now outdated.
    // Outdated is reset to true (all keyframe maintained), as decimate might
    // not be call (if keyframe number is too small for instance).
    included_.clear();
    included_.resize(validated_->size(), true);
  }

  virtual void Decimate(float _tolerance) {
    ozz::animation::offline::Decimate(*validated_, adapter_, _tolerance,
                                      &candidate_, &included_);
  }

  virtual size_t OriginalSize() const { return original_->size(); }
  virtual size_t ValidatedSize() const { return validated_->size(); }
  virtual size_t CandidateSize() const { return candidate_.size(); }

  virtual int Type() const { return _Type; }

  const _Adapter adapter_;
  const _Track* original_;
  _Track* validated_;
  _Track candidate_;

  // vector used to store keyframes from candidate_ that are included from
  // validated_.
  ozz::vector<bool> included_;
};

typedef TTrack<RawAnimation::JointTrack::Translations, PositionAdapter, 0>
    TranslationTrack;
typedef TTrack<RawAnimation::JointTrack::Rotations, RotationAdapter, 1>
    RotationTrack;
typedef TTrack<RawAnimation::JointTrack::Scales, ScaleAdapter, 2> ScaleTrack;

class HillClimber {
 public:
  HillClimber(const AnimationOptimizer& _optimizer,
              const RawAnimation& _original, const Skeleton& _skeleton,
              RawAnimation* _output)
      : original_(_original),
        skeleton_(_skeleton),
        observer_(_optimizer.observer) {
    // Checks output
    assert(_output->tracks.size() == original_.tracks.size());

    // Computes skeleton hierarchy specs, used to find initial settings.
    const HierarchyBuilder hierarchy(original_, _skeleton, _optimizer);

    // Setup virtual tracks and parameters.
    const int num_tracks = original_.num_tracks();
    translations_.reserve(num_tracks);
    rotations_.reserve(num_tracks);
    scales_.reserve(num_tracks);
    settings_.reserve(num_tracks);

    for (int i = 0; i < num_tracks; ++i) {
      const float kInitialFactor = .25f;

      // Using hierarchy spec for joint_length (aka comparing rotations)
      // is important as it prevents from optimizing too quickly joints at
      // the root. This length helps RotationAdapter at
      // computing/simulating the tolerance at hierarchy end. Hill
      // climbing will have the last word, figuring out if this decimation
      // is OK or not in the end.
      const float joint_length = hierarchy.specs[i].length;

      const int parent = _skeleton.joint_parents()[i];
      const float parent_scale =
          (parent != Skeleton::kNoParent) ? hierarchy.specs[parent].scale : 1.f;

      const AnimationOptimizer::Setting& setting =
          GetJointSetting(_optimizer, i);
      settings_.push_back(setting);

      const float initial = hierarchy.specs[i].tolerance * kInitialFactor;
      const RawAnimation::JointTrack& itrack = _original.tracks[i];
      RawAnimation::JointTrack& otrack = _output->tracks[i];

      {  // Translation track, translation is affected by parent scale.
        const PositionAdapter adap(parent_scale);
        translations_.push_back(ozz::make_unique<TranslationTrack>(
            itrack.translations, &otrack.translations, adap, initial, i));
      }
      {  // Rotation track, rotation affects children translations/length.
        const RotationAdapter adap(joint_length);
        rotations_.push_back(ozz::make_unique<RotationTrack>(
            itrack.rotations, &otrack.rotations, adap, initial, i));
      }
      {  // Scale track, scale affects children translations/length.
        const ScaleAdapter adap(joint_length);
        scales_.push_back(ozz::make_unique<ScaleTrack>(
            itrack.scales, &otrack.scales, adap, initial, i));
      }
    }

    // Collects all remaining tracks to process. Doing this at this point
    // ensures tracks won't be reallocated/moved.
    remainings_.reserve(num_tracks * 3);
    for (int i = 0; i < num_tracks; ++i) {
      remainings_.push_back(translations_[i].get());
      remainings_.push_back(rotations_[i].get());
      remainings_.push_back(scales_[i].get());
    }

    // Setup comparer
    comparer_ = ozz::make_unique<Comparer>(_original, *_output, _skeleton,
                                           ozz::make_span(settings_));
  }

  void operator()() {
    // Initializes all tracks with a decimated candidate.
    for (VTrack* track : remainings_) {
      track->Transition();
      track->UpdateDelta(*comparer_);
    }

    // Loops as long as there's still optimizable tracks to process.
    for (int iteration = 0;; ++iteration) {
      // Within a single loop over all remaining tracks, finds the one
      // that has the best bang for the buck.
      VTrack* best_track = NULL;
      float best_delta = 0.f;
      for (ozz::vector<VTrack*>::iterator it = remainings_.begin();
           it != remainings_.end();) {
        VTrack* track = *it;

        // Gets this track optimization delta.
        const float delta = track->optimization_delta();

        if (observer_) {
          // Fills up tracking data.
          AnimationOptimizer::Observer::Data data;
          data.iteration = iteration;
          data.joint = track->joint();
          data.type = track->Type();
          data.target_error = settings_[data.joint].tolerance;
          data.distance = settings_[data.joint].distance;
          data.original_size = static_cast<int>(track->OriginalSize());
          data.validated_size = static_cast<int>(track->ValidatedSize());
          data.candidate_size = static_cast<int>(track->CandidateSize());
          data.own_tolerance = track->tolerance();
          data.own_error = comparer_->track_own_error(data.joint);
          data.hierarchy_error_ratio = track->hierarchy_error_ratio();
          data.optimization_delta = delta;

          // Push tracking data.
          observer_->Push(data);
        }

        // Removes track from those to process if it can't be optimized anymore.
        if (delta >= 0) {
          it = remainings_.erase(it);
          continue;
        }

        // Remembers best option.
        if (delta < best_delta) {
          best_delta = delta;
          best_track = track;
        }

        ++it;  // Proceed with next track.
      }

      // Check if we reached end of possible options.
      assert(best_track != NULL || remainings_.empty());
      if (best_track == NULL) {
        break;
      }

      // Retains best track option
      best_track->Validate(*comparer_);

      // Computes next candidate for track.
      best_track->Transition();

      // Updates all dependent tracks of the hierarchy.
      IterateJointsDF(skeleton_,
                      std::bind(&HillClimber::UpdateDelta, this,
                                std::placeholders::_1, std::placeholders::_2),
                      best_track->joint());
    }
  }

  void UpdateDelta(int _joint, int) {
    translations_[_joint]->UpdateDelta(*comparer_);
    rotations_[_joint]->UpdateDelta(*comparer_);
    scales_[_joint]->UpdateDelta(*comparer_);
  }

 private:
  HillClimber(const HillClimber&);
  void operator=(const HillClimber&);

  ozz::unique_ptr<Comparer> comparer_;
  const RawAnimation& original_;
  const Skeleton& skeleton_;
  ozz::vector<unique_ptr<TranslationTrack>> translations_;
  ozz::vector<unique_ptr<RotationTrack>> rotations_;
  ozz::vector<unique_ptr<ScaleTrack>> scales_;
  ozz::vector<AnimationOptimizer::Setting> settings_;
  ozz::vector<VTrack*> remainings_;

  AnimationOptimizer::Observer* observer_;
};

bool ValidateSettings(const AnimationOptimizer& _optimizer) {
  const auto& InvalideSetting =
      [](const AnimationOptimizer::Setting& _setting) {
        return _setting.tolerance <= 0.f;
      };

  if (InvalideSetting(_optimizer.setting)) {
    return false;
  }

  const auto& overrides = _optimizer.joints_setting_override;
  if (std::find_if(
          overrides.cbegin(), overrides.cend(),
          [&](const AnimationOptimizer::JointsSetting::value_type& _entry) {
            return InvalideSetting(_entry.second);
          }) != overrides.cend()) {
    return false;
  }

  return true;
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

  // Validates setting.
  if (!ValidateSettings(*this)) {
    return false;
  }

  // Validates the skeleton matches the animation.
  const int num_tracks = _input.num_tracks();
  if (num_tracks != _skeleton.num_joints()) {
    return false;
  }

  // _input validation is handled by AnimationConstantOptimizer.
  // Removing constants from the starts allows to efficiently reduces number of
  // combinatorial search iterations.
  RawAnimation no_constant;
  AnimationConstantOptimizer constant_optimizer;
  if (!constant_optimizer(_input, &no_constant)) {
    return false;
  }

  // Setups output animation.
  _output->name = no_constant.name;
  _output->duration = no_constant.duration;
  _output->tracks.resize(num_tracks);

  if (fast) {
    // Temporary vector used to store included keyframes during decimation.
    ozz::vector<bool> included;

    // First computes bone lengths, that will be used when filtering.
    const HierarchyBuilder hierarchy(no_constant, _skeleton, *this);

    for (int i = 0; i < num_tracks; ++i) {
      const RawAnimation::JointTrack& input = no_constant.tracks[i];
      RawAnimation::JointTrack& output = _output->tracks[i];

      // Gets joint specs back.
      const float joint_length = hierarchy.specs[i].length;
      const int parent = _skeleton.joint_parents()[i];
      const float parent_scale =
          (parent != Skeleton::kNoParent) ? hierarchy.specs[parent].scale : 1.f;
      const float tolerance = hierarchy.specs[i].tolerance;

      // Filters independently T, R and S tracks.
      // This joint translation is affected by parent scale.
      const PositionAdapter tadap(parent_scale);
      Decimate(input.translations, tadap, tolerance, &output.translations,
               &included);
      // This joint rotation affects children translations/length.
      const RotationAdapter radap(joint_length);
      Decimate(input.rotations, radap, tolerance, &output.rotations, &included);
      // This joint scale affects children translations/length.
      const ScaleAdapter sadap(joint_length);
      Decimate(input.scales, sadap, tolerance, &output.scales, &included);
    }
  } else {
    // CsvTracking tracking("hill_climbing.cvs");
    HillClimber(*this, no_constant, _skeleton, _output)();
  }

  // Output animation is always valid though.
  return _output->Validate();
}

namespace {
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
  assert(_output->empty());
  const bool constant = IsConstant(_input, _tolerance);
  if (constant) {
    if (_input.size() > 0) {
      _output->push_back(_input[0]);
      _output->at(0).time = 0.f;
    }
  } else {
    *_output = _input;
  }
  return constant;
}
}  // namespace

AnimationConstantOptimizer::AnimationConstantOptimizer()
    : translation_tolerance(1e-5f),
      rotation_tolerance(1.f - 5e-6f),  // cosinse of half angle.
      scale_tolerance(1e-5f) {}

bool AnimationConstantOptimizer::operator()(const RawAnimation& _input,
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
  _output->name = _input.name;
  _output->duration = _input.duration;
  _output->tracks.resize(num_tracks);

  for (int i = 0; i < num_tracks; ++i) {
    const RawAnimation::JointTrack& input = _input.tracks[i];
    RawAnimation::JointTrack& output = _output->tracks[i];
    DecimateConstant(input.translations, &output.translations,
                     translation_tolerance);
    DecimateConstant(input.rotations, &output.rotations, rotation_tolerance);
    DecimateConstant(input.scales, &output.scales, scale_tolerance);
  }
  return true;
};
}  // namespace offline
}  // namespace animation
}  // namespace ozz
