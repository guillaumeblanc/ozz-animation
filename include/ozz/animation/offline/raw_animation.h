//============================================================================//
//                                                                            //
// ozz-animation, 3d skeletal animation libraries and tools.                  //
// https://code.google.com/p/ozz-animation/                                   //
//                                                                            //
//----------------------------------------------------------------------------//
//                                                                            //
// Copyright (c) 2012-2014 Guillaume Blanc                                    //
//                                                                            //
// This software is provided 'as-is', without any express or implied          //
// warranty. In no event will the authors be held liable for any damages      //
// arising from the use of this software.                                     //
//                                                                            //
// Permission is granted to anyone to use this software for any purpose,      //
// including commercial applications, and to alter it and redistribute it     //
// freely, subject to the following restrictions:                             //
//                                                                            //
// 1. The origin of this software must not be misrepresented; you must not    //
// claim that you wrote the original software. If you use this software       //
// in a product, an acknowledgment in the product documentation would be      //
// appreciated but is not required.                                           //
//                                                                            //
// 2. Altered source versions must be plainly marked as such, and must not be //
// misrepresented as being the original software.                             //
//                                                                            //
// 3. This notice may not be removed or altered from any source               //
// distribution.                                                              //
//                                                                            //
//============================================================================//

#ifndef OZZ_OZZ_ANIMATION_OFFLINE_RAW_ANIMATION_H_
#define OZZ_OZZ_ANIMATION_OFFLINE_RAW_ANIMATION_H_

#include "ozz/base/containers/vector.h"
#include "ozz/base/maths/vec_float.h"
#include "ozz/base/maths/quaternion.h"

namespace ozz {
namespace animation {
namespace offline {

// Offline animation type.
// This animation type is not intended to be used in run time. It is used to
// define the offline animation object that can be converted to the runtime
// animation using the AnimationBuilder.
// This animation structure exposes tracks of keyframes. Keyframes are defined
// with a time and a value which can either be a translation (3 float x, y, z),
// a rotation (a quaternion) or scale coefficient (3 floats x, y, z). Tracks are
// defined as a set of three different std::vectors (translation, rotation and
// scales). Animation structure is then a vector of tracks, along with a
// duration value.
// Finally the RawAnimation structure exposes Validate() function to check that
// it is valid, meaning that all the following rules are respected:
//  1. Animation duration is greater than 0.
//  2. Keyframes' time are sorted in a strict ascending order.
//  3. Keyframes' time are all within [0,animation duration] range.
// Animations that would fail this validation will fail to be converted by the
// AnimationBuilder.
struct RawAnimation {
  // Constructs an valid RawAnimation with a 1s default duration.
  RawAnimation();

  // Deallocates raw animation.
  ~RawAnimation();

  // Tests for *this validity.
  // Returns true if animation data (duration, tracks) is valid:
  //  1. Animation duration is greater than 0.
  //  2. Keyframes' time are sorted in a strict ascending order.
  //  3. Keyframes' time are all within [0,animation duration] range.
  bool Validate() const;

  // Defines a raw translation key frame.
  struct TranslationKey {
    float time;
    math::Float3 value;

    // Provides identity transformation for a translation key.
    static math::Float3 identity() {
      return math::Float3::zero();
    }
  };

  // Defines a raw rotation key frame.
  struct RotationKey {
    float time;
    math::Quaternion value;

    // Provides identity transformation for a rotation key.
    static math::Quaternion identity() {
      return math::Quaternion::identity();
    }
  };

  // Defines a raw scaling key frame.
  struct ScaleKey {
    float time;
    math::Float3 value;

    // Provides identity transformation for a scale key.
    static math::Float3 identity() {
      return math::Float3::one();
    }
  };

  // Defines a track of key frames for a bone, including translation, rotation
  // and scale.
  struct JointTrack {
    typedef ozz::Vector<TranslationKey>::Std Translations;
    Translations translations;
    typedef ozz::Vector<RotationKey>::Std Rotations;
    Rotations rotations;
    typedef ozz::Vector<ScaleKey>::Std Scales;
    Scales scales;
  };

  // Returns the number of tracks of this animation.
  int num_tracks() const {
    return static_cast<int>(tracks.size());
  }

  // Stores per joint JointTrack, ie: per joint animation key-frames.
  // tracks_.size() gives the number of animated joints.
  ozz::Vector<JointTrack>::Std tracks;

  // The duration of the animation. All the keys of a valid RawAnimation are in
  // the range [0,duration].
  float duration;
};
}  // offline
}  // animation
}  // ozz
#endif  // OZZ_OZZ_ANIMATION_OFFLINE_RAW_ANIMATION_H_
