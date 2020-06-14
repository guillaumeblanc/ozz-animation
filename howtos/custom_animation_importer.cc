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

#include "ozz/animation/offline/animation_builder.h"
#include "ozz/animation/offline/raw_animation.h"

#include "ozz/animation/runtime/animation.h"

#include <cstdlib>

// Code for ozz-animation HowTo: "How to write a custon animation importer?"

int main(int argc, char const* argv[]) {
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

  // Fills each track with keyframes, in joint local-space.
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
  // This operation will fail and return an empty unique_ptr if the RawAnimation
  // isn't valid.
  ozz::unique_ptr<ozz::animation::Animation> animation = builder(raw_animation);

  // ...use the animation as you want...

  return EXIT_SUCCESS;
}
