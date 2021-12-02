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

#ifndef OZZ_OZZ_ANIMATION_OFFLINE_RAW_ANIMATION_UTILS_H_
#define OZZ_OZZ_ANIMATION_OFFLINE_RAW_ANIMATION_UTILS_H_

#include "ozz/animation/offline/export.h"
#include "ozz/animation/offline/raw_animation.h"

#include "ozz/base/maths/transform.h"
#include "ozz/base/span.h"

namespace ozz {
namespace animation {
namespace offline {

// Validates that a RawAnimation::JointTrack is valid.
// Returns true if animation data (duration, tracks) is valid:
//  1. Animation duration is greater than 0.
//  2. Keyframes' time are sorted in a strict ascending order.
//  3. Keyframes' time are all within [0,animation duration] range.
bool ValidateTrack(const RawAnimation::JointTrack& _track, float _duration);

// Validates that a RawAnimation::JointTrack component (aka , translation or
// rotation or scale) is valid.
// See ValidateTrack for more info.
bool ValidateTrackComponent(
    const RawAnimation::JointTrack::Translations& _translations,
    float _duration);
bool ValidateTrackComponent(
    const RawAnimation::JointTrack::Rotations& _rotations, float _duration);
bool ValidateTrackComponent(const RawAnimation::JointTrack::Scales& _scales,
                            float _duration);

// Translation interpolation method.
OZZ_ANIMOFFLINE_DLL math::Float3 LerpTranslation(const math::Float3& _a,
                                                 const math::Float3& _b,
                             float _alpha);

// Rotation interpolation method.
OZZ_ANIMOFFLINE_DLL math::Quaternion LerpRotation(const math::Quaternion& _a,
                              const math::Quaternion& _b, float _alpha);

// Scale interpolation method.
OZZ_ANIMOFFLINE_DLL math::Float3 LerpScale(const math::Float3& _a,
                                           const math::Float3& _b,
                       float _alpha);

// Samples a RawAnimation track component. This function shall be used for
// offline purpose. Use ozz::animation::Animation and
// ozz::animation::SamplingJob for runtime purpose.
// Behavior is undetermined if track is invalid but _validate is set to false.
// Returns false if track is invalid.
bool SampleTrackComponent(
    const RawAnimation::JointTrack::Translations& _translations, float _time,
    ozz::math::Float3* _translation, bool _validate = true);
bool SampleTrackComponent(const RawAnimation::JointTrack::Rotations& _rotations,
                          float _time, ozz::math::Quaternion* _rotation,
                          bool _validate = true);
bool SampleTrackComponent(const RawAnimation::JointTrack::Scales& _scales,
                          float _time, ozz::math::Float3* _scale,
                          bool _validate = true);

// Samples a RawAnimation track. This function shall be used for offline
// purpose. Use ozz::animation::Animation and ozz::animation::SamplingJob for
// runtime purpose.
// Behavior is undetermined if track is invalid but _validate is set to false.
// Returns false if track is invalid.
OZZ_ANIMOFFLINE_DLL bool SampleTrack(const RawAnimation::JointTrack& _track,
                                     float _time,
                 ozz::math::Transform* _transform, bool _validate = true);

// Samples a RawAnimation. This function shall be used for offline
// purpose. Use ozz::animation::Animation and ozz::animation::SamplingJob for
// runtime purpose.
// _animation must be valid.
// Behavior is undetermined if track is invalid but _validate is set to false.
// Returns false output range is too small or animation is invalid.
OZZ_ANIMOFFLINE_DLL bool SampleAnimation(
    const RawAnimation& _animation, float _time,
                     const span<ozz::math::Transform>& _transforms,
                     bool _validate = true);


// Implement fixed rate keyframe time iteration. This utility purpose is to
// ensure that sampling goes strictly from 0 to duration, and that period
// between consecutive time samples have a fixed period.
// This sounds trivial, but floating point error could occur if keyframe time
// was accumulated for a long duration.
class OZZ_ANIMOFFLINE_DLL FixedRateSamplingTime {
 public:
  FixedRateSamplingTime(float _duration, float _frequency);

  float time(size_t _key) const {
    assert(_key < num_keys_);
    return ozz::math::Min(_key * period_, duration_);
  }

  size_t num_keys() const { return num_keys_; }

 private:
  float duration_;
  float period_;
  size_t num_keys_;
};
}  // namespace offline
}  // namespace animation
}  // namespace ozz
#endif  // OZZ_OZZ_ANIMATION_OFFLINE_RAW_ANIMATION_UTILS_H_
