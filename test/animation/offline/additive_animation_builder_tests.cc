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

#include "ozz/animation/offline/additive_animation_builder.h"

#include "gtest/gtest.h"

#include "ozz/base/maths/gtest_math_helper.h"
#include "ozz/base/maths/math_constant.h"
#include "ozz/base/maths/transform.h"

#include "ozz/animation/offline/raw_animation.h"

using ozz::animation::offline::AdditiveAnimationBuilder;
using ozz::animation::offline::RawAnimation;

TEST(Error, AdditiveAnimationBuilder) {
  AdditiveAnimationBuilder builder;

  {  // nullptr output.
    RawAnimation input;
    EXPECT_TRUE(input.Validate());

    // Builds animation
    EXPECT_FALSE(builder(input, nullptr));
  }

  {  // Invalid input animation.
    RawAnimation input;
    input.duration = -1.f;
    EXPECT_FALSE(input.Validate());

    // Builds animation
    RawAnimation output;
    output.duration = -1.f;
    output.tracks.resize(1);
    EXPECT_FALSE(builder(input, &output));
    EXPECT_FLOAT_EQ(output.duration, RawAnimation().duration);
    EXPECT_EQ(output.num_tracks(), 0);
  }

  {  // Invalid input animation with custom refpose
    RawAnimation input;
    input.duration = 1.f;
    input.tracks.resize(1);

    // Builds animation
    RawAnimation output;
    output.duration = -1.f;
    output.tracks.resize(1);

    ozz::span<ozz::math::Transform> empty_ref_pose_range;

    EXPECT_FALSE(builder(input, empty_ref_pose_range, &output));
    EXPECT_FLOAT_EQ(output.duration, RawAnimation().duration);
    EXPECT_EQ(output.num_tracks(), 0);
  }
}

TEST(Build, AdditiveAnimationBuilder) {
  AdditiveAnimationBuilder builder;

  RawAnimation input;
  input.duration = 1.f;
  input.tracks.resize(3);

  // First track is empty
  {
      // input.tracks[0]
  }

  // 2nd track
  // 1 key at the beginning
  {
    const RawAnimation::TranslationKey key = {0.f,
                                              ozz::math::Float3(2.f, 3.f, 4.f)};
    input.tracks[1].translations.push_back(key);
  }
  {
    const RawAnimation::RotationKey key = {
        0.f, ozz::math::Quaternion(.70710677f, 0.f, 0.f, .70710677f)};
    input.tracks[1].rotations.push_back(key);
  }
  {
    const RawAnimation::ScaleKey key = {0.f, ozz::math::Float3(5.f, 6.f, 7.f)};
    input.tracks[1].scales.push_back(key);
  }

  // 3rd track
  // 2 keys after the beginning
  {
    const RawAnimation::TranslationKey key0 = {
        .5f, ozz::math::Float3(2.f, 3.f, 4.f)};
    input.tracks[2].translations.push_back(key0);
    const RawAnimation::TranslationKey key1 = {
        .7f, ozz::math::Float3(20.f, 30.f, 40.f)};
    input.tracks[2].translations.push_back(key1);
  }
  {
    const RawAnimation::RotationKey key0 = {
        .5f, ozz::math::Quaternion(.70710677f, 0.f, 0.f, .70710677f)};
    input.tracks[2].rotations.push_back(key0);
    const RawAnimation::RotationKey key1 = {
        .7f, ozz::math::Quaternion(-.70710677f, 0.f, 0.f, .70710677f)};
    input.tracks[2].rotations.push_back(key1);
  }
  {
    const RawAnimation::ScaleKey key0 = {.5f, ozz::math::Float3(5.f, 6.f, 7.f)};
    input.tracks[2].scales.push_back(key0);
    const RawAnimation::ScaleKey key1 = {.7f,
                                         ozz::math::Float3(50.f, 60.f, 70.f)};
    input.tracks[2].scales.push_back(key1);
  }

  // Builds animation with very little tolerance.
  {
    RawAnimation output;
    ASSERT_TRUE(builder(input, &output));
    EXPECT_EQ(output.num_tracks(), 3);

    // 1st track.
    {
      EXPECT_EQ(output.tracks[0].translations.size(), 0u);
      EXPECT_EQ(output.tracks[0].rotations.size(), 0u);
      EXPECT_EQ(output.tracks[0].scales.size(), 0u);
    }

    // 2nd track.
    {
      const RawAnimation::JointTrack::Translations& translations =
          output.tracks[1].translations;
      EXPECT_EQ(translations.size(), 1u);
      EXPECT_FLOAT_EQ(translations[0].time, 0.f);
      EXPECT_FLOAT3_EQ(translations[0].value, 0.f, 0.f, 0.f);
      const RawAnimation::JointTrack::Rotations& rotations =
          output.tracks[1].rotations;
      EXPECT_EQ(rotations.size(), 1u);
      EXPECT_FLOAT_EQ(rotations[0].time, 0.f);
      EXPECT_QUATERNION_EQ(rotations[0].value, 0.f, 0.f, 0.f, 1.f);
      const RawAnimation::JointTrack::Scales& scales = output.tracks[1].scales;
      EXPECT_EQ(scales.size(), 1u);
      EXPECT_FLOAT_EQ(scales[0].time, 0.f);
      EXPECT_FLOAT3_EQ(scales[0].value, 1.f, 1.f, 1.f);
    }

    // 3rd track.
    {
      const RawAnimation::JointTrack::Translations& translations =
          output.tracks[2].translations;
      EXPECT_EQ(translations.size(), 2u);
      EXPECT_FLOAT_EQ(translations[0].time, .5f);
      EXPECT_FLOAT3_EQ(translations[0].value, 0.f, 0.f, 0.f);
      EXPECT_FLOAT_EQ(translations[1].time, .7f);
      EXPECT_FLOAT3_EQ(translations[1].value, 18.f, 27.f, 36.f);
      const RawAnimation::JointTrack::Rotations& rotations =
          output.tracks[2].rotations;
      EXPECT_EQ(rotations.size(), 2u);
      EXPECT_FLOAT_EQ(rotations[0].time, .5f);
      EXPECT_QUATERNION_EQ(rotations[0].value, 0.f, 0.f, 0.f, 1.f);
      EXPECT_FLOAT_EQ(rotations[1].time, .7f);
      EXPECT_QUATERNION_EQ(rotations[1].value, -1.f, 0.f, 0.f, 0.f);
      const RawAnimation::JointTrack::Scales& scales = output.tracks[2].scales;
      EXPECT_EQ(scales.size(), 2u);
      EXPECT_FLOAT_EQ(scales[0].time, .5f);
      EXPECT_FLOAT3_EQ(scales[0].value, 1.f, 1.f, 1.f);
      EXPECT_FLOAT_EQ(scales[1].time, .7f);
      EXPECT_FLOAT3_EQ(scales[1].value, 10.f, 10.f, 10.f);
    }
  }
}

TEST(BuildRefPose, AdditiveAnimationBuilder) {
  AdditiveAnimationBuilder builder;

  RawAnimation input;
  input.duration = 1.f;
  input.tracks.resize(3);

  // First track is empty
  {
      // input.tracks[0]
  }

  // 2nd track
  // 1 key at the beginning
  {
    const RawAnimation::TranslationKey key = {0.f,
                                              ozz::math::Float3(2.f, 3.f, 4.f)};
    input.tracks[1].translations.push_back(key);
  }
  {
    const RawAnimation::RotationKey key = {
        0.f, ozz::math::Quaternion(.70710677f, 0.f, 0.f, .70710677f)};
    input.tracks[1].rotations.push_back(key);
  }
  {
    const RawAnimation::ScaleKey key = {0.f, ozz::math::Float3(5.f, 6.f, 7.f)};
    input.tracks[1].scales.push_back(key);
  }

  // 3rd track
  // 2 keys after the beginning
  {
    const RawAnimation::TranslationKey key0 = {
        .5f, ozz::math::Float3(2.f, 3.f, 4.f)};
    input.tracks[2].translations.push_back(key0);
    const RawAnimation::TranslationKey key1 = {
        .7f, ozz::math::Float3(20.f, 30.f, 40.f)};
    input.tracks[2].translations.push_back(key1);
  }
  {
    const RawAnimation::RotationKey key0 = {
        .5f, ozz::math::Quaternion(.70710677f, 0.f, 0.f, .70710677f)};
    input.tracks[2].rotations.push_back(key0);
    const RawAnimation::RotationKey key1 = {
        .7f, ozz::math::Quaternion(-.70710677f, 0.f, 0.f, .70710677f)};
    input.tracks[2].rotations.push_back(key1);
  }
  {
    const RawAnimation::ScaleKey key0 = {.5f, ozz::math::Float3(5.f, 6.f, 7.f)};
    input.tracks[2].scales.push_back(key0);
    const RawAnimation::ScaleKey key1 = {.7f,
                                         ozz::math::Float3(50.f, 60.f, 70.f)};
    input.tracks[2].scales.push_back(key1);
  }

  // Builds animation with a custom refpose & very little tolerance
  {
    ozz::math::Transform ref_pose[3];
    ref_pose[0] = ozz::math::Transform::identity();
    ref_pose[1].translation = ozz::math::Float3(1.f, 1.f, 1.f);
    ref_pose[1].rotation =
        ozz::math::Quaternion(0.f, 0.f, .70710677f, .70710677f);
    ref_pose[1].scale = ozz::math::Float3(1.f, -1.f, 2.f);
    ref_pose[2].translation = input.tracks[2].translations[0].value;
    ref_pose[2].rotation = input.tracks[2].rotations[0].value;
    ref_pose[2].scale = input.tracks[2].scales[0].value;

    RawAnimation output;
    ASSERT_TRUE(
        builder(input, ozz::span<ozz::math::Transform>(ref_pose), &output));
    EXPECT_EQ(output.num_tracks(), 3);

    // 1st track.
    {
      EXPECT_EQ(output.tracks[0].translations.size(), 0u);
      EXPECT_EQ(output.tracks[0].rotations.size(), 0u);
      EXPECT_EQ(output.tracks[0].scales.size(), 0u);
    }

    // 2nd track.
    {
      const RawAnimation::JointTrack::Translations& translations =
          output.tracks[1].translations;
      EXPECT_EQ(translations.size(), 1u);
      EXPECT_FLOAT_EQ(translations[0].time, 0.f);
      EXPECT_FLOAT3_EQ(translations[0].value, 1.f, 2.f, 3.f);
      const RawAnimation::JointTrack::Rotations& rotations =
          output.tracks[1].rotations;
      EXPECT_EQ(rotations.size(), 1u);
      EXPECT_FLOAT_EQ(rotations[0].time, 0.f);
      EXPECT_QUATERNION_EQ(rotations[0].value, .5f, .5f, -.5f, .5f);
      const RawAnimation::JointTrack::Scales& scales = output.tracks[1].scales;
      EXPECT_EQ(scales.size(), 1u);
      EXPECT_FLOAT_EQ(scales[0].time, 0.f);
      EXPECT_FLOAT3_EQ(scales[0].value, 5.f, -6.f, 3.5f);
    }

    // 3rd track.
    {
      const RawAnimation::JointTrack::Translations& translations =
          output.tracks[2].translations;
      EXPECT_EQ(translations.size(), 2u);
      EXPECT_FLOAT_EQ(translations[0].time, .5f);
      EXPECT_FLOAT3_EQ(translations[0].value, 0.f, 0.f, 0.f);
      EXPECT_FLOAT_EQ(translations[1].time, .7f);
      EXPECT_FLOAT3_EQ(translations[1].value, 18.f, 27.f, 36.f);
      const RawAnimation::JointTrack::Rotations& rotations =
          output.tracks[2].rotations;
      EXPECT_EQ(rotations.size(), 2u);
      EXPECT_FLOAT_EQ(rotations[0].time, .5f);
      EXPECT_QUATERNION_EQ(rotations[0].value, 0.f, 0.f, 0.f, 1.f);
      EXPECT_FLOAT_EQ(rotations[1].time, .7f);
      EXPECT_QUATERNION_EQ(rotations[1].value, -1.f, 0.f, 0.f, 0.f);
      const RawAnimation::JointTrack::Scales& scales = output.tracks[2].scales;
      EXPECT_EQ(scales.size(), 2u);
      EXPECT_FLOAT_EQ(scales[0].time, .5f);
      EXPECT_FLOAT3_EQ(scales[0].value, 1.f, 1.f, 1.f);
      EXPECT_FLOAT_EQ(scales[1].time, .7f);
      EXPECT_FLOAT3_EQ(scales[1].value, 10.f, 10.f, 10.f);
    }
  }
}
