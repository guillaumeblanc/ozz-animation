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

#include "ozz/animation/offline/motion_extractor.h"

#include <cassert>

#include "ozz/animation/offline/raw_animation.h"
#include "ozz/animation/offline/raw_animation_utils.h"
#include "ozz/animation/offline/raw_track.h"
#include "ozz/animation/offline/raw_track_utils.h"
#include "ozz/animation/runtime/skeleton.h"
#include "ozz/animation/runtime/skeleton_utils.h"
#include "ozz/base/maths/transform.h"

namespace ozz {
namespace animation {
namespace offline {

namespace {
std::pair<bool, ozz::math::Transform> BuildReference(
    MotionExtractor::Reference _position_reference,
    MotionExtractor::Reference _rotation_reference,
    const ozz::animation::Skeleton& _skeleton, int _joint,
    const RawAnimation::JointTrack& _track) {
  assert(_joint >= 0 && _joint < _skeleton.num_joints());

  // Builds skeleton reference transform.
  const auto rest_pose_ms = GetRestPoseModelSpace(_skeleton)[_joint];
  math::Transform skeleton_ref;
  if (!ToAffine(rest_pose_ms, &skeleton_ref)) {
    return {false, {}};
  }

  auto ref = ozz::math::Transform::identity();

  // Position reference
  switch (_position_reference) {
    case MotionExtractor::Reference::kSkeleton: {
      ref.translation = skeleton_ref.translation;
    } break;
    case MotionExtractor::Reference::kAnimation: {
      if (!_track.translations.empty()) {
        ref.translation = _track.translations[0].value;
      }
    } break;
    default:
      break;
  }

  // Rotation reference
  switch (_rotation_reference) {
    case MotionExtractor::Reference::kSkeleton: {
      ref.rotation = skeleton_ref.rotation;
    } break;
    case MotionExtractor::Reference::kAnimation: {
      if (!_track.rotations.empty()) {
        ref.rotation = _track.rotations[0].value;
      }
    } break;
    default:
      break;
  }
  return {true, ref};
}

std::pair<bool, RawAnimation::JointTrack> BuildTrackModelSpace(
    const RawAnimation& _input, const Skeleton& _skeleton, int joint) {
  const auto [sampled, input_ms] =
      SampleTrackModelSpace(_input, _skeleton, joint);
  if (!sampled) {
    return {false, {}};
  }

  // Builds a temporary track for model-space joint.
  bool success = true;
  RawAnimation::JointTrack track;
  std::for_each(
      input_ms.begin(), input_ms.end(), [&track, &success](const auto& _key) {
        math::Transform transform;
        if (!ToAffine(_key.second, &transform)) {
          success = false;
          return;  // Stop if not decompasable.
        }
        track.translations.push_back({_key.first, transform.translation});
        track.rotations.push_back({_key.first, transform.rotation});
        track.scales.push_back({_key.first, transform.scale});
      });

  return {success, track};
}
}  // namespace

bool MotionExtractor::operator()(const RawAnimation& _input,
                                 const Skeleton& _skeleton,
                                 RawFloat3Track* _motion_position,
                                 RawQuaternionTrack* _motion_rotation,
                                 RawAnimation* _output,
                                 math::Transform* _reference) const {
  // Cannot read/write from/to the same animation.
  if (&_input == _output) {
    return false;
  }

  // All outputs are expected to be valid.
  if (!_output || !_motion_position || !_motion_rotation) {
    return false;
  }

  // Animation must match skeleton.
  if (_input.num_tracks() != _skeleton.num_joints()) {
    return false;
  }

  // Root index must be within skeleton range.
  if (joint < 0 || joint >= _skeleton.num_joints()) {
    return false;
  }

  // Validate animation.
  if (!_input.Validate()) {
    return false;
  }

  // Builds a temporary track for model-space joint.
  const auto [valid_track, input_track_ms] =
      BuildTrackModelSpace(_input, _skeleton, joint);
  if (!valid_track) {
    return false;
  }

  // Compute extraction reference
  const auto [valid_ref, ref] =
      BuildReference(position_settings.reference, rotation_settings.reference,
                     _skeleton, joint, input_track_ms);
  if (!valid_ref) {
    return false;
  }

  if (_reference) *_reference = ref;

  // Extract root motion
  // -----------------------------------------------------------------------------

  // Copy function, used to copy animation keyframes to motion keyframes.
  auto extract = [duration = _input.duration](const auto& _keframes,
                                              auto _extract, auto& output) {
    output.clear();
    for (const auto& joint_key : _keframes) {
      const auto& motion = _extract(joint_key.value);
      output.push_back({ozz::animation::offline::RawTrackInterpolation::kLinear,
                        joint_key.time / duration, motion});
    }
  };

  // Copies root position, selecting only expecting components.
  const math::Float3 position_mask{1.f * position_settings.x,
                                   1.f * position_settings.y,
                                   1.f * position_settings.z};
  extract(
      input_track_ms.translations,
      [&mask = position_mask, &ref = ref.translation](const auto& _joint) {
        return (_joint - ref) * mask;
      },
      _motion_position->keyframes);

  // Copies root rotation, selecting only expecting decomposed rotation
  // components.
  const math::Float3 rotation_mask{1.f * rotation_settings.y,   // Yaw
                                   1.f * rotation_settings.x,   // Pitch
                                   1.f * rotation_settings.z};  // Roll
  extract(
      input_track_ms.rotations,
      [&mask = rotation_mask, &ref = ref.rotation](const auto& _joint) {
        const auto euler = ToEuler(_joint * Conjugate(ref));
        return math::Quaternion::FromEuler(euler * mask);
      },
      _motion_rotation->keyframes);

  // Bake
  // -----------------------------------------------------------------------------

  // Find root joint to bake motion into
  int root_joint = joint;
  for (; _skeleton.joint_parents()[root_joint] !=
         ozz::animation::Skeleton::kNoParent;
       root_joint = _skeleton.joint_parents()[root_joint]);

  // Creates a root track sampled with motion track time points.
  const RawAnimation::JointTrack& root_track = _input.tracks[root_joint];
  RawAnimation::JointTrack output_track;
  std::for_each(
      _motion_rotation->keyframes.begin(), _motion_rotation->keyframes.end(),
      [&root_track, &output_track,
       duration = _input.duration](const auto& _key) {
        float time = _key.ratio * duration;
        math::Transform transform;
        [[maybe_unused]] bool valid = SampleTrack(root_track, time, &transform);
        assert(valid);
        output_track.translations.push_back({time, transform.translation});
        output_track.rotations.push_back({time, transform.rotation});
        output_track.scales.push_back({time, transform.scale});
      });

  // Extract root motion position from the animation, aka bake it.
  if (position_settings.bake) {
    assert(output_track.translations.size() ==
           _motion_position->keyframes.size());
    for (size_t i = 0; i < output_track.translations.size(); i++) {
      const auto& motion_p = _motion_position->keyframes[i].value;
      auto& joint_p = output_track.translations[i].value;
      joint_p = joint_p - motion_p - ref.translation;
    }
  }

  // Extract root motion rotation from the animation, aka bake it.
  if (rotation_settings.bake) {
    assert(output_track.rotations.size() == _motion_rotation->keyframes.size());
    for (size_t i = 0; i < output_track.rotations.size(); i++) {
      const auto& motion_q = _motion_rotation->keyframes[i].value;
      auto& joint_q = output_track.rotations[i].value;
      joint_q = Conjugate(ref.rotation) * Conjugate(motion_q) * joint_q;
    }
  }

  // Loopify
  // -----------------------------------------------------------------------------
  // Distributes the difference between the first and last keyframes all along
  // animation duration, so tha animation can loop.
  auto loopify = [](auto& _keyframes, auto _diff, auto _lerp) {
    if (_keyframes.size() < 2) {
      return;
    }
    const auto delta = _diff(_keyframes.front().value, _keyframes.back().value);
    for (size_t i = 0; i < _keyframes.size(); i++) {
      const float alpha = i / (_keyframes.size() - 1.f);
      auto& value = _keyframes[i].value;
      value = _lerp(value, delta, alpha);
    }
  };

  // Loopify translations
  if (rotation_settings.loop) {
    loopify(
        _motion_rotation->keyframes,
        [](auto _front, auto _back) { return _front * Conjugate(_back); },
        [](auto _value, auto _delta, float _alpha) {
          return NLerp(math::Quaternion::identity(), _delta, _alpha) * _value;
        });
  }

  // Loopify rotations
  if (position_settings.loop) {
    loopify(
        _motion_position->keyframes,
        [](auto _front, auto _back) { return _front - _back; },
        [](auto _value, auto _delta, float _alpha) {
          return _delta * _alpha + _value;
        });
  }

  // Fixup animation translations.
  // -----------------------------------------------------------------------------
  // When root motion is applied at runtime, then root rotation is applied
  // before joint translation. Hence joint's translation should be corrected to
  // support this new composition order.
  if (rotation_settings.bake) {  // Considers that if rotation is baked, then
    // motion rotation will be applied
    assert(output_track.translations.size() ==
           _motion_position->keyframes.size());
    for (size_t i = 0; i < output_track.translations.size(); i++) {
      auto& joint_p = output_track.translations[i].value;
      joint_p = TransformVector(
          Conjugate(ref.rotation * _motion_rotation->keyframes[i].value),
          joint_p);
    }
  }

  // Build output animation
  // -----------------------------------------------------------------------------
  *_output = _input;
  _output->tracks[root_joint] = output_track;

  // Validate outputs
  // -----------------------------------------------------------------------------
  bool success = true;
  success &= _motion_position->Validate();
  success &= _motion_rotation->Validate();
  success &= _output->Validate();

  return success;
}

}  // namespace offline
}  // namespace animation
}  // namespace ozz
