//============================================================================//
// Copyright (c) <2012> <Guillaume Blanc>                                     //
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
//============================================================================//

#ifndef OZZ_OZZ_ANIMATION_OFFLINE_ANIMATION_BUILDER_H_
#define OZZ_OZZ_ANIMATION_OFFLINE_ANIMATION_BUILDER_H_

#include "ozz/base/containers/vector.h"
#include "ozz/base/maths/vec_float.h"
#include "ozz/base/maths/quaternion.h"

namespace ozz {
namespace animation {

// Forward declares the run time animation type.
class Animation;

namespace offline {

// Offline animation type.
// This animation type is not intended to be used in run time. Its format is
// dedicated to ease initialization and analysis, rather than reduce memory
// footprint and optimize sampling performance.
struct RawAnimation {
  // Constructs an invalid RawAnimation (duration must be set).
  RawAnimation();

  // Deallocates raw animation.
  ~RawAnimation();

  // Tests for *this validity.
  // Returns true on success or false on failure:
  // - if duration is <= 0.
  // - if key frames time are not sorted in a strict
  // ascending order.
  // - if a key frame time is not within [0,duration].
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

// Defines the class responsible of building runtime animation instances from
// offline raw animations.
// No optimization at all is performed on the raw animation.
class AnimationBuilder {
 public:
  // Creates an Animation based on _raw_animation and *this builder parameters.
  // Returns a valid Animation on success
  // The returned wanimation will then need to be deleted using the default 
  // allocator Delete() function.
  // See RawAnimation::Validate() for more details about failure reasons.
  Animation* operator()(const RawAnimation& _raw_animation) const;
};
}  // offline
}  // animation
}  // ozz
#endif  // OZZ_OZZ_ANIMATION_OFFLINE_ANIMATION_BUILDER_H_
