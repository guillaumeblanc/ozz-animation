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

#include "gtest/gtest.h"

#include "ozz/base/maths/math_constant.h"

#include "ozz/base/memory/scoped_ptr.h"

#include "ozz/animation/offline/animation_builder.h"
#include "ozz/animation/offline/raw_animation.h"

using ozz::animation::offline::AnimationConstantOptimizer;
using ozz::animation::offline::RawAnimation;

TEST(Error, AnimationConstantOptimizer) {
  AnimationConstantOptimizer optimizer;

  {  // NULL output.
    RawAnimation input;
    EXPECT_TRUE(input.Validate());

    // Builds animation
    EXPECT_FALSE(optimizer(input, NULL));
  }

  {  // Invalid input animation.
    RawAnimation input;
    input.duration = -1.f;
    EXPECT_FALSE(input.Validate());

    // Builds animation
    RawAnimation output;
    output.duration = -1.f;
    output.name = "invalid";
    output.tracks.resize(1);
    EXPECT_FALSE(optimizer(input, &output));
    EXPECT_FLOAT_EQ(output.duration, 1.f);
    EXPECT_TRUE(output.name.empty());
    EXPECT_EQ(output.num_tracks(), 0);
  }
}

TEST(Name, AnimationConstantOptimizer) {
  AnimationConstantOptimizer optimizer;

  RawAnimation input;
  input.name = "Test_Animation";
  input.duration = 1.f;

  ASSERT_TRUE(input.Validate());

  RawAnimation output;
  ASSERT_TRUE(optimizer(input, &output));
  EXPECT_EQ(output.num_tracks(), 0);
  EXPECT_STRCASEEQ(output.name.c_str(), "Test_Animation");
}

TEST(Optimize, AnimationConstantOptimizer) {
  RawAnimation input;
  input.duration = 1.f;
  input.tracks.resize(4);

  // Track 0, variations
  // Translations
  {
    RawAnimation::TranslationKey key = {.1f, ozz::math::Float3(7.f, 0.f, 0.f)};
    input.tracks[0].translations.push_back(key);
  }
  {
    RawAnimation::TranslationKey key = {
        .2f, ozz::math::Float3(7.f + 5e-6f, 0.f, 0.f)};
    input.tracks[0].translations.push_back(key);
  }
  {
    RawAnimation::TranslationKey key = {.3f, ozz::math::Float3(7.f, 0.f, 0.f)};
    input.tracks[0].translations.push_back(key);
  }
  // Rotations
  {
    RawAnimation::RotationKey key = {
        0.f, ozz::math::Quaternion::FromEuler(1.f, 0.f, 0.f)};
    input.tracks[0].rotations.push_back(key);
  }
  {
    RawAnimation::RotationKey key = {
        .1f, ozz::math::Quaternion::FromEuler(1.f + 5e-4f, 0.f, 0.f)};
    input.tracks[0].rotations.push_back(key);
  }
  {  // Opposite quaternion
    RawAnimation::RotationKey key = {
        .2f, -ozz::math::Quaternion::FromEuler(1.f, 0.f, 0.f)};
    input.tracks[0].rotations.push_back(key);
  }
  // Scales
  {
    RawAnimation::ScaleKey key = {.1f,
                                  ozz::math::Float3(1.f + 5e-6f, 0.f, 0.f)};
    input.tracks[0].scales.push_back(key);
  }
  {
    RawAnimation::ScaleKey key = {.2f, ozz::math::Float3(1.f, 0.f, 0.f)};
    input.tracks[0].scales.push_back(key);
  }

  // Track 1, single constant value.
  // Single translations on track 1.
  {
    RawAnimation::TranslationKey key = {0.f, ozz::math::Float3(16.f, 0.f, 0.f)};
    input.tracks[1].translations.push_back(key);
  }
  {
    RawAnimation::RotationKey key = {
        .1f, ozz::math::Quaternion::FromEuler(0.f, -1.f, 0.f)};
    input.tracks[1].rotations.push_back(key);
  }
  {
    RawAnimation::ScaleKey key = {.5f, ozz::math::Float3(1.f, 0.f, 0.f)};
    input.tracks[1].scales.push_back(key);
  }

  // Track 2, non constant.
  {
    RawAnimation::TranslationKey key = {.1f, ozz::math::Float3(7.f, 0.f, 0.f)};
    input.tracks[2].translations.push_back(key);
  }
  {
    RawAnimation::TranslationKey key = {.3f, ozz::math::Float3(8.f, 0.f, 0.f)};
    input.tracks[2].translations.push_back(key);
  }
  // Rotations
  {
    RawAnimation::RotationKey key = {
        0.f, ozz::math::Quaternion::FromEuler(0.f, 0.f, 0.f)};
    input.tracks[2].rotations.push_back(key);
  }
  {
    RawAnimation::RotationKey key = {
        .1f, ozz::math::Quaternion::FromEuler(ozz::math::kPi_4, 0.f, 0.f)};
    input.tracks[2].rotations.push_back(key);
  }
  // Scales
  {
    RawAnimation::ScaleKey key = {.1f, ozz::math::Float3(1.f, 0.f, 0.f)};
    input.tracks[2].scales.push_back(key);
  }
  {
    RawAnimation::ScaleKey key = {.2f, ozz::math::Float3(2.f, 0.f, 0.f)};
    input.tracks[2].scales.push_back(key);
  }

  // Track 3, no value.

  ASSERT_TRUE(input.Validate());

  // Negative tolerance -> all key maintained
  {
    RawAnimation output;

    AnimationConstantOptimizer optimizer;
    optimizer.translation_tolerance = 0.f;
    optimizer.rotation_tolerance = 1.f;
    optimizer.scale_tolerance = 0.f;
    ASSERT_TRUE(optimizer(input, &output));
    EXPECT_EQ(output.num_tracks(), 4);

    EXPECT_TRUE(input.tracks[0].translations.size() ==
                output.tracks[0].translations.size());
    EXPECT_TRUE(input.tracks[0].rotations.size() ==
                output.tracks[0].rotations.size());
    EXPECT_TRUE(input.tracks[0].scales.size() ==
                output.tracks[0].scales.size());
    EXPECT_TRUE(input.tracks[1].translations.size() ==
                output.tracks[1].translations.size());
    EXPECT_TRUE(input.tracks[1].rotations.size() ==
                output.tracks[1].rotations.size());
    EXPECT_TRUE(input.tracks[1].scales.size() ==
                output.tracks[1].scales.size());
    EXPECT_TRUE(input.tracks[2].translations.size() ==
                output.tracks[2].translations.size());
    EXPECT_TRUE(input.tracks[2].rotations.size() ==
                output.tracks[2].rotations.size());
    EXPECT_TRUE(input.tracks[2].scales.size() ==
                output.tracks[2].scales.size());
    EXPECT_TRUE(input.tracks[3].translations.size() ==
                output.tracks[3].translations.size());
    EXPECT_TRUE(input.tracks[3].rotations.size() ==
                output.tracks[3].rotations.size());
    EXPECT_TRUE(input.tracks[3].scales.size() ==
                output.tracks[3].scales.size());
  }

  // Small tolerance -> some keys are stripped
  {
    RawAnimation output;

    AnimationConstantOptimizer optimizer;
    optimizer.translation_tolerance = 1e-5f;
    optimizer.rotation_tolerance = 1.f - 5e-6f;
    optimizer.scale_tolerance = 1e-5f;
    ASSERT_TRUE(optimizer(input, &output));
    ASSERT_EQ(output.num_tracks(), 4);

    EXPECT_TRUE(output.tracks[0].translations.size() == 1);
    EXPECT_FLOAT_EQ(output.tracks[0].translations[0].time, 0.f);
    EXPECT_TRUE(output.tracks[0].rotations.size() == 1);
    EXPECT_FLOAT_EQ(output.tracks[0].rotations[0].time, 0.f);
    EXPECT_TRUE(output.tracks[0].scales.size() == 1);
    EXPECT_FLOAT_EQ(output.tracks[0].scales[0].time, 0.f);
    EXPECT_TRUE(output.tracks[1].translations.size() == 1);
    EXPECT_TRUE(output.tracks[1].rotations.size() == 1);
    EXPECT_TRUE(output.tracks[1].scales.size() == 1);
    EXPECT_TRUE(output.tracks[2].translations.size() == 2);
    EXPECT_TRUE(output.tracks[2].rotations.size() == 2);
    EXPECT_TRUE(output.tracks[2].scales.size() == 2);
    EXPECT_TRUE(output.tracks[3].translations.size() == 0);
    EXPECT_TRUE(output.tracks[3].rotations.size() == 0);
    EXPECT_TRUE(output.tracks[3].scales.size() == 0);
  }

  // High tolerance -> all keys are stripped
  {
    RawAnimation output;

    AnimationConstantOptimizer optimizer;
    optimizer.translation_tolerance = 1e3f;
    optimizer.rotation_tolerance = 1.f - .5f;
    optimizer.scale_tolerance = 1e3f;
    ASSERT_TRUE(optimizer(input, &output));
    EXPECT_EQ(output.num_tracks(), 4);

    EXPECT_TRUE(output.tracks[0].translations.size() == 1);
    EXPECT_TRUE(output.tracks[0].rotations.size() == 1);
    EXPECT_TRUE(output.tracks[0].scales.size() == 1);
    EXPECT_TRUE(output.tracks[1].translations.size() == 1);
    EXPECT_TRUE(output.tracks[1].rotations.size() == 1);
    EXPECT_TRUE(output.tracks[1].scales.size() == 1);
    EXPECT_TRUE(output.tracks[2].translations.size() == 1);
    EXPECT_TRUE(output.tracks[2].rotations.size() == 1);
    EXPECT_TRUE(output.tracks[2].scales.size() == 1);
    EXPECT_TRUE(output.tracks[3].translations.size() == 0);
    EXPECT_TRUE(output.tracks[3].rotations.size() == 0);
    EXPECT_TRUE(output.tracks[3].scales.size() == 0);
  }
}
