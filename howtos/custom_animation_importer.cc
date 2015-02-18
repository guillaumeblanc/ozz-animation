//============================================================================//
//                                                                            //
// ozz-animation, 3d skeletal animation libraries and tools.                  //
// https://code.google.com/p/ozz-animation/                                   //
//                                                                            //
//----------------------------------------------------------------------------//
//                                                                            //
// Copyright (c) 2012-2015 Guillaume Blanc                                    //
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

#include "ozz/animation/offline/raw_animation.h"
#include "ozz/animation/offline/animation_builder.h"

#include "ozz/animation/runtime/animation.h"

#include <cstdlib>

// Code for ozz-animation HowTo: "How to write a custon animation importer?"
// "http://code.google.com/p/ozz-animation/wiki/HowTos#How_to_write_a_custom_animation_importer?"

int main(int argc, char const *argv[]) {
  (void)argc;
  (void)argv;

  //////////////////////////////////////////////////////////////////////////////
  // The first section builds a RawAnimation from custom data.
  //////////////////////////////////////////////////////////////////////////////

  // Creates a RawAnimation.
  ozz::animation::offline::RawAnimation raw_animation;

  // Sets animation duration (to 1.4s).
  // All the animation keyframes times must be within range [0, duration].
  raw_animation.duration = 1.4f;

  // Creates 3 animation tracks.
  // There should be as much tracks as there are joints in the skeleton that
  // this animation targets.
  raw_animation.tracks.resize(3);

  // Fills each track with keyframes.
  // Tracks should be ordered in the same order as joints in the
  // ozz::animation::Skeleton. Joint's names can be used to find joint's
  // index in the skeleton.

  // Fills 1st track with 2 translation keyframes.
  {
    // Create a keyframe, at t=0, with a translation value.
    const ozz::animation::offline::RawAnimation::TranslationKey key0 = {
      0.f, ozz::math::Float3(0.f, 4.6f, 0.f)};

    raw_animation.tracks[0].translations.push_back(key0);

    // Create a new keyframe, at t=0.93 (must be less than duration), with a
    // translation value.
    const ozz::animation::offline::RawAnimation::TranslationKey key1 = {
      .93f, ozz::math::Float3(0.f, 9.9f, 0.f)};

    raw_animation.tracks[0].translations.push_back(key1);
  }

  // Fills 1st track with a rotation keyframe. It's not mandatory to have the
  // same number of keyframes for translation, rotations and scales.
  {
    // Create a keyframe, at t=.46, with a quaternion value.
    const ozz::animation::offline::RawAnimation::RotationKey key0 = {
      .46f, ozz::math::Quaternion(0.f, 1.f, 0.f, 0.f)};

    raw_animation.tracks[0].rotations.push_back(key0);
  }

  // For this example, don't fill scale with any key. The default value will be
  // identity, which is ozz::math::Float3(1.f, 1.f, 1.f) for scale.

  //...and so on with all other tracks...

  // Test for animation validity. These are the errors that could invalidate
  // an animation:
  //  1. Animation duration is less than 0.
  //  2. Keyframes' are not sorted in a strict ascending order.
  //  3. Keyframes' are not within [0, duration] range.
  if (!raw_animation.Validate()) {
    return EXIT_FAILURE;
  }

  //////////////////////////////////////////////////////////////////////////////
  // This final section converts the RawAnimation to a runtime Animation.
  //////////////////////////////////////////////////////////////////////////////

  // Creates a AnimationBuilder instance.
  ozz::animation::offline::AnimationBuilder builder;

  // Executes the builder on the previously prepared RawAnimation, which returns
  // a new runtime animation instance.
  // This operation will fail and return NULL if the RawAnimation isn't valid.
  ozz::animation::Animation* animation = builder(raw_animation);

  // ...use the animation as you want...

  // In the end the animation needs to be deleted.
  ozz::memory::default_allocator()->Delete(animation);

  return EXIT_SUCCESS;
}
