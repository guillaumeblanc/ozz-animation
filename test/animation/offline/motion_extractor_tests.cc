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

#include "gtest/gtest.h"
#include "ozz/animation/offline/motion_extractor.h"
#include "ozz/animation/offline/raw_animation.h"
#include "ozz/animation/offline/raw_animation_utils.h"
#include "ozz/animation/offline/raw_skeleton.h"
#include "ozz/animation/offline/raw_track.h"
#include "ozz/animation/offline/skeleton_builder.h"
#include "ozz/animation/runtime/skeleton.h"
#include "ozz/base/maths/gtest_math_helper.h"
#include "ozz/base/maths/math_constant.h"
#include "ozz/base/maths/math_ex.h"
#include "ozz/base/memory/unique_ptr.h"

using ozz::animation::Skeleton;
using ozz::animation::offline::MotionExtractor;
using ozz::animation::offline::RawAnimation;
using ozz::animation::offline::RawFloat3Track;
using ozz::animation::offline::RawQuaternionTrack;
using ozz::animation::offline::RawSkeleton;
using ozz::animation::offline::SkeletonBuilder;

TEST(Error, MotionExtractor) {
  RawAnimation input;
  input.tracks.resize(1);
  ASSERT_TRUE(input.Validate());

  RawSkeleton raw_skeleton;
  raw_skeleton.roots.resize(1);
  raw_skeleton.roots[0].transform = ozz::math::Transform::identity();
  ASSERT_TRUE(raw_skeleton.Validate());
  const auto skeleton = SkeletonBuilder()(raw_skeleton);
  ASSERT_TRUE(skeleton);

  RawFloat3Track motion_position;
  RawQuaternionTrack motion_rotation;
  RawAnimation output;

  {  // Same input and output.
    MotionExtractor extractor;
    EXPECT_FALSE(extractor(input, *skeleton, &motion_position, &motion_rotation,
                           &input));
  }

  {
    // Invalid animation
    RawAnimation input_invalid;
    input_invalid.tracks.resize(1);
    input_invalid.duration = -1.f;
    EXPECT_FALSE(input_invalid.Validate());

    MotionExtractor extractor;
    EXPECT_FALSE(extractor(input_invalid, *skeleton, &motion_position,
                           &motion_rotation, &output));
  }

  {
    // Non matching animation & skeleton
    RawSkeleton raw_skeleton_2;
    raw_skeleton_2.roots.resize(2);
    const auto skeleton_2 = SkeletonBuilder()(raw_skeleton_2);
    ASSERT_TRUE(skeleton_2);

    MotionExtractor extractor;
    EXPECT_FALSE(extractor(input, *skeleton_2, &motion_position,
                           &motion_rotation, &output));
  }

  {  // Null output
    MotionExtractor extractor;
    EXPECT_FALSE(
        extractor(input, *skeleton, nullptr, &motion_rotation, &output));
    EXPECT_FALSE(
        extractor(input, *skeleton, &motion_position, nullptr, &output));
    EXPECT_FALSE(extractor(input, *skeleton, &motion_position, &motion_rotation,
                           nullptr));
  }

  {  // Invalid root
    MotionExtractor extractor;
    extractor.joint = 93;
    EXPECT_FALSE(extractor(input, *skeleton, &motion_position, &motion_rotation,
                           &output));
  }

  {  // Valid
    MotionExtractor extractor;
    EXPECT_TRUE(extractor(input, *skeleton, &motion_position, &motion_rotation,
                          &output));
  }
}

TEST(Extract, MotionExtractor) {
  RawAnimation input;
  input.duration = 2.f;
  input.name = "test";
  input.tracks.resize(2);

  input.tracks[0].translations.push_back({.2f, {1.f, 3.f, 0.f}});
  input.tracks[0].translations.push_back({.4f, {2.f, 0.f, 0.f}});
  input.tracks[0].rotations.push_back(
      {.4f, ozz::math::Quaternion::FromAxisAngle(ozz::math::Float3::y_axis(),
                                                 ozz::math::kPi_2)});
  input.tracks[0].rotations.push_back(
      {2.f, ozz::math::Quaternion::FromAxisAngle(ozz::math::Float3::y_axis(),
                                                 -ozz::math::kPi_2)});

  input.tracks[1].translations.push_back({.3f, {0.f, 0.f, 1.f}});
  input.tracks[1].translations.push_back({2.f, {0.f, 0.f, 18.f}});

  ASSERT_TRUE(input.Validate());

  RawSkeleton raw_skeleton;
  raw_skeleton.roots.resize(1);
  raw_skeleton.roots[0].transform = ozz::math::Transform::identity();
  raw_skeleton.roots[0].children.resize(1);
  raw_skeleton.roots[0].children[0].transform =
      ozz::math::Transform::identity();
  ASSERT_TRUE(raw_skeleton.Validate());
  const auto skeleton = SkeletonBuilder()(raw_skeleton);

  RawFloat3Track motion_position;
  RawQuaternionTrack motion_rotation;
  RawAnimation baked;

  const auto anim_track_eq = [](const auto& _a, const auto& _b) {
    const auto anim_cmpnt_eq = [](const auto& _a, const auto& _b) {
      if (_a.size() != _b.size()) {
        return false;
      }
      for (size_t i = 0; i < _a.size(); ++i) {
        if (_a[i].time != _b[i].time) {
          return false;
        }
        if (_a[i].value != _b[i].value) {
          return false;
        }
      }
      return true;
    };
    return anim_cmpnt_eq(_a.translations, _b.translations) &&
           anim_cmpnt_eq(_a.rotations, _b.rotations) &&
           anim_cmpnt_eq(_a.scales, _b.scales);
  };

  {  // No extraction
    const MotionExtractor extractor{
        0,  // Joint
        {false, false, false, MotionExtractor::Reference::kAbsolute, true},
        {false, false, false, MotionExtractor::Reference::kAbsolute, true}};
    EXPECT_TRUE(extractor(input, *skeleton, &motion_position, &motion_rotation,
                          &baked));

    // Animation
    EXPECT_EQ(input.name, baked.name);
    EXPECT_FLOAT_EQ(input.duration, baked.duration);
    EXPECT_EQ(input.num_tracks(), baked.num_tracks());

    const auto& track0 = baked.tracks[0];
    ASSERT_EQ(track0.translations.size(), 3u);
    EXPECT_FLOAT_EQ(track0.translations[0].time, .2f);
    EXPECT_FLOAT3_EQ(track0.translations[0].value, 1.f, 3.f, 0.f);
    EXPECT_FLOAT_EQ(track0.translations[1].time, .4f);
    EXPECT_FLOAT3_EQ(track0.translations[1].value, 2.f, 0.f, 0.f);
    EXPECT_FLOAT_EQ(track0.translations[2].time, 2.f);
    EXPECT_FLOAT3_EQ(track0.translations[2].value, 2.f, 0.f, 0.f);

    ASSERT_EQ(track0.rotations.size(), 3u);
    EXPECT_FLOAT_EQ(track0.rotations[0].time, .2f);
    EXPECT_QUATERNION_EQ(track0.rotations[0].value, 0.f, .7071067f, 0.f,
                         .7071067f);
    EXPECT_FLOAT_EQ(track0.rotations[1].time, .4f);
    EXPECT_QUATERNION_EQ(track0.rotations[1].value, 0.f, .7071067f, 0.f,
                         .7071067f);
    EXPECT_FLOAT_EQ(track0.rotations[2].time, 2.f);
    EXPECT_QUATERNION_EQ(track0.rotations[2].value, 0.f, -.7071067f, 0,
                         .7071067f);

    EXPECT_TRUE(anim_track_eq(input.tracks[1], baked.tracks[1]));

    // Motion
    for (const auto& key : motion_position.keyframes) {
      EXPECT_FLOAT3_EQ(key.value, 0.f, 0.f, 0.f);
    }
    for (const auto& key : motion_rotation.keyframes) {
      EXPECT_QUATERNION_EQ(key.value, 0.f, 0.f, 0.f, 1.f);
    }
  }

  {  // Extract all, no baking
    const MotionExtractor extractor{
        0,  // Joint
        {true, true, true, MotionExtractor::Reference::kAbsolute, false},
        {true, true, true, MotionExtractor::Reference::kAbsolute, false}};
    EXPECT_TRUE(extractor(input, *skeleton, &motion_position, &motion_rotation,
                          &baked));

    // Animation
    EXPECT_EQ(input.name, baked.name);
    EXPECT_FLOAT_EQ(input.duration, baked.duration);
    EXPECT_EQ(baked.num_tracks(), input.num_tracks());

    const auto& track0 = baked.tracks[0];
    ASSERT_EQ(track0.translations.size(), 3u);
    EXPECT_FLOAT_EQ(track0.translations[0].time, .2f);
    EXPECT_FLOAT3_EQ(track0.translations[0].value, 1.f, 3.f, 0.f);
    EXPECT_FLOAT_EQ(track0.translations[1].time, .4f);
    EXPECT_FLOAT3_EQ(track0.translations[1].value, 2.f, 0.f, 0.f);
    EXPECT_FLOAT_EQ(track0.translations[2].time, 2.f);
    EXPECT_FLOAT3_EQ(track0.translations[2].value, 2.f, 0.f, 0.f);

    ASSERT_EQ(track0.rotations.size(), 3u);
    EXPECT_FLOAT_EQ(track0.rotations[0].time, .2f);
    EXPECT_QUATERNION_EQ(track0.rotations[0].value, 0.f, .7071067f, 0.f,
                         .7071067f);
    EXPECT_FLOAT_EQ(track0.rotations[1].time, .4f);
    EXPECT_QUATERNION_EQ(track0.rotations[1].value, 0.f, .7071067f, 0.f,
                         .7071067f);
    EXPECT_FLOAT_EQ(track0.rotations[2].time, 2.f);
    EXPECT_QUATERNION_EQ(track0.rotations[2].value, 0.f, -.7071067f, 0.f,
                         .7071067f);

    EXPECT_TRUE(anim_track_eq(input.tracks[1], baked.tracks[1]));

    // Track
    const auto& positions = motion_position.keyframes;
    ASSERT_EQ(positions.size(), 3u);
    EXPECT_FLOAT_EQ(positions[0].ratio, .1f);
    EXPECT_FLOAT3_EQ(positions[0].value, 1.f, 3.f, 0.f);
    EXPECT_FLOAT_EQ(positions[1].ratio, .2f);
    EXPECT_FLOAT3_EQ(positions[1].value, 2.f, 0.f, 0.f);
    EXPECT_FLOAT_EQ(positions[2].ratio, 1.f);
    EXPECT_FLOAT3_EQ(positions[2].value, 2.f, 0.f, 0.f);

    const auto& rotations = motion_rotation.keyframes;
    ASSERT_EQ(rotations.size(), 3u);
    EXPECT_FLOAT_EQ(rotations[0].ratio, .1f);
    EXPECT_QUATERNION_EQ(rotations[0].value, 0.f, .7071067f, 0.f, .7071067f);
    EXPECT_FLOAT_EQ(rotations[1].ratio, .2f);
    EXPECT_QUATERNION_EQ(rotations[1].value, 0.f, .7071067f, 0.f, .7071067f);
    EXPECT_FLOAT_EQ(rotations[2].ratio, 1.f);
    EXPECT_QUATERNION_EQ(rotations[2].value, 0.f, -.7071067f, 0.f, .7071067f);
  }

  {  // Extract all, bake
    const MotionExtractor extractor{
        0,  // Joint
        {true, true, true, MotionExtractor::Reference::kAbsolute, true},
        {true, true, true, MotionExtractor::Reference::kAbsolute, true}};
    EXPECT_TRUE(extractor(input, *skeleton, &motion_position, &motion_rotation,
                          &baked));

    // Animation
    EXPECT_EQ(input.name, baked.name);
    EXPECT_FLOAT_EQ(input.duration, baked.duration);
    EXPECT_EQ(input.num_tracks(), baked.num_tracks());

    const auto& track0 = baked.tracks[0];
    ASSERT_EQ(track0.translations.size(), 3u);
    for (const auto& key : track0.translations) {
      EXPECT_FLOAT3_EQ(key.value, 0.f, 0.f, 0.f);
    }
    ASSERT_EQ(track0.rotations.size(), 3u);
    for (const auto& key : track0.rotations) {
      EXPECT_QUATERNION_EQ(key.value, 0.f, 0.f, 0.f, 1.f);
    }

    EXPECT_TRUE(anim_track_eq(input.tracks[1], baked.tracks[1]));

    // Track
    const auto& positions = motion_position.keyframes;
    ASSERT_EQ(positions.size(), 3u);
    EXPECT_FLOAT_EQ(positions[0].ratio, .1f);
    EXPECT_FLOAT3_EQ(positions[0].value, 1.f, 3.f, 0.f);
    EXPECT_FLOAT_EQ(positions[1].ratio, .2f);
    EXPECT_FLOAT3_EQ(positions[1].value, 2.f, 0.f, 0.f);
    EXPECT_FLOAT_EQ(positions[2].ratio, 1.f);
    EXPECT_FLOAT3_EQ(positions[2].value, 2.f, 0.f, 0.f);

    const auto& rotations = motion_rotation.keyframes;
    ASSERT_EQ(rotations.size(), 3u);
    EXPECT_FLOAT_EQ(rotations[0].ratio, .1f);
    EXPECT_QUATERNION_EQ(rotations[0].value, 0.f, .7071067f, 0.f, .7071067f);
    EXPECT_FLOAT_EQ(rotations[1].ratio, .2f);
    EXPECT_QUATERNION_EQ(rotations[1].value, 0.f, .7071067f, 0.f, .7071067f);
    EXPECT_FLOAT_EQ(rotations[2].ratio, 1.f);
    EXPECT_QUATERNION_EQ(rotations[2].value, 0.f, -.7071067f, 0.f, .7071067f);
  }

  {  // Extract only y position, bake
    const MotionExtractor extractor{
        0,  // Joint
        {false, true, false, MotionExtractor::Reference::kAbsolute, true},
        {false, false, false, MotionExtractor::Reference::kAbsolute, true}};
    EXPECT_TRUE(extractor(input, *skeleton, &motion_position, &motion_rotation,
                          &baked));

    // Animation
    EXPECT_EQ(input.name, baked.name);
    EXPECT_FLOAT_EQ(input.duration, baked.duration);
    EXPECT_EQ(input.num_tracks(), baked.num_tracks());

    const auto& track0 = baked.tracks[0];
    ASSERT_EQ(track0.translations.size(), 3u);
    EXPECT_FLOAT_EQ(track0.translations[0].time, .2f);
    EXPECT_FLOAT3_EQ(track0.translations[0].value, 1.f, 0.f, 0.f);
    EXPECT_FLOAT_EQ(track0.translations[1].time, .4f);
    EXPECT_FLOAT3_EQ(track0.translations[1].value, 2.f, 0.f, 0.f);
    EXPECT_FLOAT_EQ(track0.translations[2].time, 2.f);
    EXPECT_FLOAT3_EQ(track0.translations[2].value, 2.f, 0.f, 0.f);

    ASSERT_EQ(track0.rotations.size(), 3u);
    EXPECT_FLOAT_EQ(track0.rotations[0].time, .2f);
    EXPECT_QUATERNION_EQ(track0.rotations[0].value, 0.f, .7071067f, 0.f,
                         .7071067f);
    EXPECT_FLOAT_EQ(track0.rotations[1].time, .4f);
    EXPECT_QUATERNION_EQ(track0.rotations[1].value, 0.f, .7071067f, 0.f,
                         .7071067f);
    EXPECT_FLOAT_EQ(track0.rotations[2].time, 2.f);
    EXPECT_QUATERNION_EQ(track0.rotations[2].value, 0.f, -.7071067f, 0.f,
                         .7071067f);

    EXPECT_TRUE(anim_track_eq(input.tracks[1], baked.tracks[1]));

    // Track
    const auto& positions = motion_position.keyframes;
    ASSERT_EQ(positions.size(), 3u);
    EXPECT_FLOAT_EQ(positions[0].ratio, .1f);
    EXPECT_FLOAT3_EQ(positions[0].value, 0.f, 3.f, 0.f);
    EXPECT_FLOAT_EQ(positions[1].ratio, .2f);
    EXPECT_FLOAT3_EQ(positions[1].value, 0.f, 0.f, 0.f);
    EXPECT_FLOAT_EQ(positions[2].ratio, 1.f);
    EXPECT_FLOAT3_EQ(positions[2].value, 0.f, 0.f, 0.f);

    const auto& rotations = motion_rotation.keyframes;
    ASSERT_EQ(rotations.size(), 3u);
    EXPECT_FLOAT_EQ(rotations[0].ratio, .1f);
    EXPECT_QUATERNION_EQ(rotations[0].value, 0.f, 0.f, 0.f, 1.f);
    EXPECT_FLOAT_EQ(rotations[1].ratio, .2f);
    EXPECT_QUATERNION_EQ(rotations[1].value, 0.f, 0.f, 0.f, 1.f);
    EXPECT_FLOAT_EQ(rotations[2].ratio, 1.f);
    EXPECT_QUATERNION_EQ(rotations[2].value, 0.f, 0.f, 0.f, 1.f);
  }

  {  // Extract only x position and y/yaw rotation
    const MotionExtractor extractor{
        0,  // Joint
        {true, false, false, MotionExtractor::Reference::kAbsolute, true},
        {false, true, false, MotionExtractor::Reference::kAbsolute, true}};
    EXPECT_TRUE(extractor(input, *skeleton, &motion_position, &motion_rotation,
                          &baked));

    EXPECT_EQ(input.name, baked.name);
    EXPECT_FLOAT_EQ(input.duration, baked.duration);
    EXPECT_EQ(input.num_tracks(), baked.num_tracks());

    // Animation
    EXPECT_EQ(input.name, baked.name);
    EXPECT_FLOAT_EQ(input.duration, baked.duration);
    EXPECT_EQ(input.num_tracks(), baked.num_tracks());

    const auto& track0 = baked.tracks[0];
    ASSERT_EQ(track0.translations.size(), 3u);
    EXPECT_FLOAT_EQ(track0.translations[0].time, .2f);
    EXPECT_FLOAT3_EQ(track0.translations[0].value, 0.f, 3.f, 0.f);
    EXPECT_FLOAT_EQ(track0.translations[1].time, .4f);
    EXPECT_FLOAT3_EQ(track0.translations[1].value, 0.f, 0.f, 0.f);
    EXPECT_FLOAT_EQ(track0.translations[2].time, 2.f);
    EXPECT_FLOAT3_EQ(track0.translations[2].value, 0.f, 0.f, 0.f);

    ASSERT_EQ(track0.rotations.size(), 3u);
    EXPECT_FLOAT_EQ(track0.rotations[0].time, .2f);
    EXPECT_QUATERNION_EQ(track0.rotations[0].value, 0.f, 0.f, 0.f, 1.f);
    EXPECT_FLOAT_EQ(track0.rotations[1].time, .4f);
    EXPECT_QUATERNION_EQ(track0.rotations[1].value, 0.f, 0.f, 0.f, 1.f);
    EXPECT_FLOAT_EQ(track0.rotations[2].time, 2.f);
    EXPECT_QUATERNION_EQ(track0.rotations[2].value, 0.f, 0.f, 0.f, 1.f);

    EXPECT_TRUE(anim_track_eq(input.tracks[1], baked.tracks[1]));

    // Track
    const auto& positions = motion_position.keyframes;
    ASSERT_EQ(positions.size(), 3u);
    EXPECT_FLOAT_EQ(positions[0].ratio, .1f);
    EXPECT_FLOAT3_EQ(positions[0].value, 1.f, 0.f, 0.f);
    EXPECT_FLOAT_EQ(positions[1].ratio, .2f);
    EXPECT_FLOAT3_EQ(positions[1].value, 2.f, 0.f, 0.f);
    EXPECT_FLOAT_EQ(positions[2].ratio, 1.f);
    EXPECT_FLOAT3_EQ(positions[2].value, 2.f, 0.f, 0.f);

    const auto& rotations = motion_rotation.keyframes;
    ASSERT_EQ(rotations.size(), 3u);
    EXPECT_FLOAT_EQ(rotations[0].ratio, .1f);
    EXPECT_QUATERNION_EQ(rotations[0].value, 0.f, .7071067f, 0.f, .7071067f);
    EXPECT_FLOAT_EQ(rotations[1].ratio, .2f);
    EXPECT_QUATERNION_EQ(rotations[1].value, 0.f, .7071067f, 0.f, .7071067f);
    EXPECT_FLOAT_EQ(rotations[2].ratio, 1.f);
    EXPECT_QUATERNION_EQ(rotations[2].value, 0.f, -.7071067f, 0.f, .7071067f);
  }

  {  // Extract joint 1, hence model-space
    const MotionExtractor extractor{
        1,  // Joint
        {true, true, true, MotionExtractor::Reference::kAbsolute, true},
        {true, true, true, MotionExtractor::Reference::kAbsolute, true}};
    EXPECT_TRUE(extractor(input, *skeleton, &motion_position, &motion_rotation,
                          &baked));

    // Animation
    EXPECT_EQ(input.name, baked.name);
    EXPECT_FLOAT_EQ(input.duration, baked.duration);
    EXPECT_EQ(input.num_tracks(), baked.num_tracks());

    // Baking is done on the root
    const auto& track0 = baked.tracks[0];
    ASSERT_EQ(track0.translations.size(), 4u);
    EXPECT_FLOAT_EQ(track0.translations[0].time, .2f);
    EXPECT_FLOAT3_EQ(track0.translations[0].value, 0.f, 0.f, -1.f);
    EXPECT_FLOAT_EQ(track0.translations[1].time, .3f);
    EXPECT_FLOAT3_EQ(track0.translations[1].value, 0.f, 0.f, -1.f);
    EXPECT_FLOAT_EQ(track0.translations[2].time, .4f);
    EXPECT_FLOAT3_EQ(track0.translations[2].value, 0.f, 0.f, -2.f);
    EXPECT_FLOAT_EQ(track0.translations[3].time, 2.f);
    EXPECT_FLOAT3_EQ(track0.translations[3].value, 0.f, 0.f, -18.f);

    ASSERT_EQ(track0.rotations.size(), 4u);
    EXPECT_FLOAT_EQ(track0.rotations[0].time, .2f);
    EXPECT_QUATERNION_EQ(track0.rotations[0].value, 0.f, 0.f, 0.f, 1.f);
    EXPECT_FLOAT_EQ(track0.rotations[1].time, .3f);
    EXPECT_QUATERNION_EQ(track0.rotations[1].value, 0.f, 0.f, 0.f, 1.f);
    EXPECT_FLOAT_EQ(track0.rotations[2].time, .4f);
    EXPECT_QUATERNION_EQ(track0.rotations[2].value, 0.f, 0.f, 0.f, 1.f);
    EXPECT_FLOAT_EQ(track0.rotations[3].time, 2.f);
    EXPECT_QUATERNION_EQ(track0.rotations[3].value, 0.f, 0.f, 0.f, 1.f);

    EXPECT_TRUE(anim_track_eq(input.tracks[1], baked.tracks[1]));

    // Track
    const auto& positions = motion_position.keyframes;
    ASSERT_EQ(positions.size(), 4u);
    EXPECT_FLOAT_EQ(positions[0].ratio, .1f);
    EXPECT_FLOAT3_EQ(positions[0].value, 2.f, 3.f, 0.f);
    EXPECT_FLOAT_EQ(positions[1].ratio, .15f);
    EXPECT_FLOAT3_EQ(positions[1].value, 2.5f, 1.5f, 0.f);
    EXPECT_FLOAT_EQ(positions[2].ratio, .2f);
    EXPECT_FLOAT3_EQ(positions[2].value, 4.f, 0.f, 0.f);
    EXPECT_FLOAT_EQ(positions[3].ratio, 1.f);
    EXPECT_FLOAT3_EQ(positions[3].value, -16.f, 0.f, 0.f);

    const auto& rotations = motion_rotation.keyframes;
    ASSERT_EQ(rotations.size(), 4u);
    EXPECT_FLOAT_EQ(rotations[0].ratio, .1f);
    EXPECT_QUATERNION_EQ(rotations[0].value, 0.f, .7071067f, 0.f, .7071067f);
    EXPECT_FLOAT_EQ(rotations[1].ratio, .15f);
    EXPECT_QUATERNION_EQ(rotations[2].value, 0.f, .7071067f, 0.f, .7071067f);
    EXPECT_FLOAT_EQ(rotations[2].ratio, .2f);
    EXPECT_QUATERNION_EQ(rotations[2].value, 0.f, .7071067f, 0.f, .7071067f);
    EXPECT_FLOAT_EQ(rotations[3].ratio, 1.f);
    EXPECT_QUATERNION_EQ(rotations[3].value, 0.f, -.7071067f, 0.f, .7071067f);
  }
}
