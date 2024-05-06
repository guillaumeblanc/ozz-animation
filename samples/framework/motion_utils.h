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

#ifndef OZZ_SAMPLES_FRAMEWORK_MOTION_UTILS_H_
#define OZZ_SAMPLES_FRAMEWORK_MOTION_UTILS_H_

#include "ozz/animation/runtime/track.h"
#include "ozz/base/maths/transform.h"
#include "ozz/base/span.h"

namespace ozz {
namespace math {
struct Float4x4;
}
namespace sample {
class Renderer;

// A motion track object, composed of a position and a rotation track.
struct MotionTrack {
  ozz::animation::Float3Track position;
  ozz::animation::QuaternionTrack rotation;
};

// Loads motion tracks (position and rotation) from an ozz archive file named
// _filename. This function will fail and return false if the file cannot be
// opened or if it is not a valid ozz track archive. A valid track archive can
// be produced with ozz tools (*2ozz) or using ozz serialization API. _filename
// and _track must be non-nullptr.
bool LoadMotionTrack(const char* _filename, MotionTrack* _track);

// Helper object that manages motion accumulation of delta motions/moves to
// compute character's transform.
struct MotionDeltaAccumulator {
  //  Accumulates motion delta to updates current transform.
  void Update(const ozz::math::Transform& _delta);

  //  Accumulates motion delta to updates current transform.
  // _delta_rotation is the rotation to apply to deform the path since last
  // update. Hence, user is responsible for taking care of applying delta time
  // if he wants to achieve a specific angular speed.
  void Update(const ozz::math::Transform& _delta,
              const ozz::math::Quaternion& _rotation);

  // Teleports accumulator to a new origin.
  void Teleport(const ozz::math::Transform& _origin);

  // Character's current transform.
  ozz::math::Transform current = ozz::math::Transform::identity();

  // Delta transformation between last and current frame.
  ozz::math::Transform delta = ozz::math::Transform::identity();

  // Accumulated rotation (since last teleport).
  ozz::math::Quaternion rotation_accum = ozz::math::Quaternion::identity();
};

// Helper object that manages motion accumulation to compute character's
// transform. Delta motion is automatically computed from the difference between
// the last and the new transform.
struct MotionAccumulator : public MotionDeltaAccumulator {
  //  Computes motion delta (new - last) and updates current transform.
  void Update(const ozz::math::Transform& _new);

  //  Accumulates motion delta (new - last) and updates current transform.
  // _delta_rotation is the rotation to apply to deform the path since last
  // update. Hence, user is responsible for taking care of applying delta time
  // if he wants to achieve a specific angular speed.
  void Update(const ozz::math::Transform& _new,
              const ozz::math::Quaternion& _rotation);

  // Tells the accumulator that the _new transform is the new origin.
  // This is useful when animation loops, so next delta is computed from the new
  // origin.
  void ResetOrigin(const ozz::math::Transform& _origin);

  // Teleports accumulator to a new transform. This also resets the origin, so
  // next delta is computed from the new origin.
  void Teleport(const ozz::math::Transform& _origin);

  // Last value sample from the motion track, used to compute delta.
  ozz::math::Transform last = ozz::math::Transform::identity();
};

// Helper object samples a motion track to update a MotionAccumulator.
struct MotionSampler : public MotionAccumulator {
  // Updates the accumulator with a new motion sample.
  bool Update(const MotionTrack& _motion, float _ratio, int _loops);

  // Updates the accumulator with a new motion sample.
  // _delta_rotation is the rotation to apply to deform the path since last
  // update. Hence, user is responsible for taking care of applying delta time
  // if he wants to achieve a specific angular speed.
  bool Update(const MotionTrack& _motion, float _ratio, int _loops,
              const ozz::math::Quaternion& _delta_rotation);
};

// Samples a motion track at a given ratio.
bool SampleMotion(const MotionTrack& _tracks, float _ratio,
                  ozz::math::Transform* _transform);

// Draws a motion track around ratio _at, in range [_from, _to].
// _step is the delta ratio between each sample / point.
bool DrawMotion(ozz::sample::Renderer* _renderer,
                const MotionTrack& _motion_track, float _from, float _at,
                float _to, float _step, const ozz::math::Float4x4& _transform,
                float _alpha = 1.f);

// See DrawMotion above. This version allows to apply a delta rotation to the
// path, where _delta_rotation is the rotation to apply each step.
bool DrawMotion(ozz::sample::Renderer* _renderer,
                const MotionTrack& _motion_track, float _from, float _at,
                float _to, float _step, const ozz::math::Float4x4& _transform,
                const ozz::math::Quaternion& _delta_rotation,
                float _alpha = 1.f);

}  // namespace sample
}  // namespace ozz
#endif  // OZZ_SAMPLES_FRAMEWORK_MOTION_UTILS_H_
