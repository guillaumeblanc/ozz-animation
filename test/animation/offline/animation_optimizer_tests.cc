//----------------------------------------------------------------------------//
//                                                                            //
// ozz-animation is hosted at http://github.com/guillaumeblanc/ozz-animation  //
// and distributed under the MIT License (MIT).                               //
//                                                                            //
// Copyright (c) 2015 Guillaume Blanc                                         //
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

#include "gtest/gtest.h"

#include "ozz/base/maths/math_constant.h"

#include "ozz/animation/offline/raw_animation.h"
#include "ozz/animation/offline/animation_builder.h"

using ozz::animation::offline::RawAnimation;
using ozz::animation::offline::AnimationOptimizer;

TEST(Error, AnimationOptimizer) {
  AnimationOptimizer optimizer;

  { // NULL output.
    RawAnimation input;
    EXPECT_TRUE(input.Validate());

    // Builds animation
    EXPECT_FALSE(optimizer(input, NULL));
  }

  { // Invalid input animation.
    RawAnimation input;
    input.duration = -1.f;
    EXPECT_FALSE(input.Validate());

    // Builds animation
    RawAnimation output;
    output.duration = -1.f;
    output.tracks.resize(1);
    EXPECT_FALSE(optimizer(input, &output));
    EXPECT_FLOAT_EQ(output.duration, RawAnimation().duration);
    EXPECT_EQ(output.num_tracks(), 0);
  }
}

TEST(Optimize, AnimationOptimizer) {
  AnimationOptimizer optimizer;

  RawAnimation input;
  input.duration = 1.f;
  input.tracks.resize(1);
  {
    RawAnimation::TranslationKey key = {
      0.f, ozz::math::Float3(0.f, 0.f, 0.f)};
    input.tracks[0].translations.push_back(key);
  }
  {
    RawAnimation::TranslationKey key = {
      .25f, ozz::math::Float3(.1f, 0.f, 0.f)};  // Not interpolable.
    input.tracks[0].translations.push_back(key);
  }
  {
    RawAnimation::TranslationKey key = {
      .5f, ozz::math::Float3(0.f, 0.f, 0.f)};
    input.tracks[0].translations.push_back(key);
  }
  {
    RawAnimation::TranslationKey key = {
      .625f, ozz::math::Float3(.1f, 0.f, 0.f)};  // Interpolable.
    input.tracks[0].translations.push_back(key);
  }
  {
    RawAnimation::TranslationKey key = {
      .75f, ozz::math::Float3(.21f, 0.f, 0.f)};  // Interpolable.
    input.tracks[0].translations.push_back(key);
  }
  {
    RawAnimation::TranslationKey key = {
      .875f, ozz::math::Float3(.29f, 0.f, 0.f)};  // Interpolable.
    input.tracks[0].translations.push_back(key);
  }
  {
    RawAnimation::TranslationKey key = {
      .9999f, ozz::math::Float3(.4f, 0.f, 0.f)};
    input.tracks[0].translations.push_back(key);
  }
  {
    RawAnimation::TranslationKey key = {
      1.f, ozz::math::Float3(0.f, 0.f, 0.f)};  // Last key.
    input.tracks[0].translations.push_back(key);
  }
  {
    RawAnimation::RotationKey key = {
      0.f, ozz::math::Quaternion::identity()};
    input.tracks[0].rotations.push_back(key);
  }
  {
    RawAnimation::RotationKey key = {
      .5f, ozz::math::Quaternion::FromEuler(
        ozz::math::Float3(1.1f * ozz::math::kPi / 180.f, 0, 0))};
    input.tracks[0].rotations.push_back(key);
  }
  {
    RawAnimation::RotationKey key = {
      1.f, ozz::math::Quaternion::FromEuler(
        ozz::math::Float3(2.f * ozz::math::kPi / 180.f, 0, 0))};
    input.tracks[0].rotations.push_back(key);
  }

  // Builds animation with very little tolerance.
  {
    optimizer.translation_tolerance = 0.f;
    optimizer.rotation_tolerance = 0.f;
    RawAnimation output;
    EXPECT_TRUE(optimizer(input, &output));
    EXPECT_EQ(output.num_tracks(), 1);

    const RawAnimation::JointTrack::Translations& translations =
      output.tracks[0].translations;
    EXPECT_EQ(translations.size(), 8u);
    EXPECT_FLOAT_EQ(translations[0].value.x, 0.f);  // Track 0 begin.
    EXPECT_FLOAT_EQ(translations[1].value.x, .1f);  // Track 0 at .25f.
    EXPECT_FLOAT_EQ(translations[2].value.x, 0.f);  // Track 0 at .5f.
    EXPECT_FLOAT_EQ(translations[3].value.x, .1f);  // Track 0 at .625f.
    EXPECT_FLOAT_EQ(translations[4].value.x, .21f);  // Track 0 at .75f.
    EXPECT_FLOAT_EQ(translations[5].value.x, .29f);  // Track 0 at .875f.
    EXPECT_FLOAT_EQ(translations[6].value.x, .4f);  // Track 0 ~end.
    EXPECT_FLOAT_EQ(translations[7].value.x, 0.f);  // Track 0 end.*/

    const RawAnimation::JointTrack::Rotations& rotations =
      output.tracks[0].rotations;
    EXPECT_EQ(rotations.size(), 3u);
    EXPECT_FLOAT_EQ(rotations[0].value.w, 1.f);  // Track 0 begin.
    EXPECT_FLOAT_EQ(rotations[1].value.w, .9999539f);  // Track 0 at .5f.
    EXPECT_FLOAT_EQ(rotations[2].value.w, .9998477f);  // Track 0 end.
  }
  {
    // Rebuilds with tolerance.
    optimizer.translation_tolerance = .02f;
    optimizer.rotation_tolerance = .2f * 3.14159f / 180.f;  // .2 degree.
    RawAnimation output;
    EXPECT_TRUE(optimizer(input, &output));
    EXPECT_EQ(output.num_tracks(), 1);

    const RawAnimation::JointTrack::Translations& translations =
      output.tracks[0].translations;
    EXPECT_EQ(translations.size(), 5u);
    EXPECT_FLOAT_EQ(translations[0].value.x, 0.f);  // Track 0 begin.
    EXPECT_FLOAT_EQ(translations[1].value.x, .1f);  // Track 0 at .25f.
    EXPECT_FLOAT_EQ(translations[2].value.x, 0.f);  // Track 0 at .5f.
    EXPECT_FLOAT_EQ(translations[3].value.x, 0.4f);  // Track 0 at ~1.f.
    EXPECT_FLOAT_EQ(translations[4].value.x, 0.f);  // Track 0 end.

    const RawAnimation::JointTrack::Rotations& rotations =
      output.tracks[0].rotations;
    EXPECT_EQ(rotations.size(), 2u);
    EXPECT_FLOAT_EQ(rotations[0].value.w, 1.f);  // Track 0 begin.
    EXPECT_FLOAT_EQ(rotations[1].value.w, .9998477f);  // Track 0 end.
  }
}
