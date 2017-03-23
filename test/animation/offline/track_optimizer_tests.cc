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

#include "ozz/animation/offline/track_optimizer.h"

#include "gtest/gtest.h"
#include "ozz/base/maths/gtest_math_helper.h"

#include "ozz/animation/offline/track_builder.h"
#include "ozz/animation/offline/raw_track.h"

using ozz::animation::offline::RawFloatTrack;
using ozz::animation::offline::RawFloat3Track;
using ozz::animation::offline::TrackOptimizer;
using ozz::animation::offline::RawTrackInterpolation;

TEST(Error, TrackOptimizer) {
  TrackOptimizer optimizer;

  {  // NULL output.
    RawFloatTrack input;
    EXPECT_TRUE(input.Validate());

    // Builds animation
    EXPECT_FALSE(optimizer(input, NULL));
  }

  {  // Invalid input animation.
    RawFloatTrack input;
    input.keyframes.resize(1);
    input.keyframes[0].time = 99.f;
    EXPECT_FALSE(input.Validate());

    // Builds animation
    RawFloatTrack output;
    output.keyframes.resize(1);
    EXPECT_FALSE(optimizer(input, &output));
    EXPECT_EQ(output.keyframes.size(), 0u);
  }
}
/*
TEST(Name, FloatTrackOptimizer) {
  // Prepares a skeleton.
  RawSkeleton raw_skeleton;
  SkeletonBuilder skeleton_builder;
  Skeleton* skeleton = skeleton_builder(raw_skeleton);
  ASSERT_TRUE(skeleton != NULL);

  FloatTrackOptimizer optimizer;

  RawFloatTrack input;
  input.name = "Test_Animation";
  input.duration = 1.f;

  ASSERT_TRUE(input.Validate());

  RawFloatTrack output;
  ASSERT_TRUE(optimizer(input, *&output));
  EXPECT_EQ(output.num_tracks(), 0);
  EXPECT_STRCASEEQ(output.name.c_str(), "Test_Animation");

  ozz::memory::default_allocator()->Delete(skeleton);
}
*/

TEST(OptimizeSteps, TrackOptimizer) {
  // Step keys aren't optimized.
  TrackOptimizer optimizer;

  RawFloatTrack raw_float_track;
  RawFloatTrack::Keyframe key0 = {RawTrackInterpolation::kStep, .5f, 46.f};
  raw_float_track.keyframes.push_back(key0);
  RawFloatTrack::Keyframe key1 = {RawTrackInterpolation::kStep, .7f, 0.f};
  raw_float_track.keyframes.push_back(key1);
  RawFloatTrack::Keyframe key2 = {RawTrackInterpolation::kStep, .8f, 1e-9f};
  raw_float_track.keyframes.push_back(key2);

  RawFloatTrack output;
  ASSERT_TRUE(optimizer(raw_float_track, &output));

  EXPECT_EQ(output.keyframes.size(), 3u);

  EXPECT_EQ(output.keyframes[0].interpolation, key0.interpolation);
  EXPECT_FLOAT_EQ(output.keyframes[0].time, key0.time);
  EXPECT_FLOAT_EQ(output.keyframes[0].value, key0.value);

  EXPECT_EQ(output.keyframes[1].interpolation, key1.interpolation);
  EXPECT_FLOAT_EQ(output.keyframes[1].time, key1.time);
  EXPECT_FLOAT_EQ(output.keyframes[1].value, key1.value);

  EXPECT_EQ(output.keyframes[2].interpolation, key2.interpolation);
  EXPECT_FLOAT_EQ(output.keyframes[2].time, key2.time);
  EXPECT_FLOAT_EQ(output.keyframes[2].value, key2.value);
}

TEST(OptimizeInterpolate, TrackOptimizer) {
  // Step keys aren't optimized.
  TrackOptimizer optimizer;

  RawFloatTrack raw_float_track;
  RawFloatTrack::Keyframe key0 = {RawTrackInterpolation::kLinear, 0.f, 69.f};
  raw_float_track.keyframes.push_back(key0);
  RawFloatTrack::Keyframe key1 = {RawTrackInterpolation::kLinear, .25f, 46.f};
  raw_float_track.keyframes.push_back(key1);
  RawFloatTrack::Keyframe key2 = {RawTrackInterpolation::kLinear, .5f, 23.f};
  raw_float_track.keyframes.push_back(key2);
  RawFloatTrack::Keyframe key3 = {RawTrackInterpolation::kLinear, .500001f,
                                  23.000001f};
  raw_float_track.keyframes.push_back(key3);
  RawFloatTrack::Keyframe key4 = {RawTrackInterpolation::kLinear, .75f, 0.f};
  raw_float_track.keyframes.push_back(key4);
  RawFloatTrack::Keyframe key5 = {RawTrackInterpolation::kLinear, .8f, 1e-12f};
  raw_float_track.keyframes.push_back(key5);
  RawFloatTrack::Keyframe key6 = {RawTrackInterpolation::kLinear, 1.f, -1e-12f};
  raw_float_track.keyframes.push_back(key6);

  {
    RawFloatTrack output;

    optimizer.tolerance = 1e-3f;
    ASSERT_TRUE(optimizer(raw_float_track, &output));

    EXPECT_EQ(output.keyframes.size(), 2u);

    EXPECT_EQ(output.keyframes[0].interpolation, key0.interpolation);
    EXPECT_FLOAT_EQ(output.keyframes[0].time, key0.time);
    EXPECT_FLOAT_EQ(output.keyframes[0].value, key0.value);

    EXPECT_EQ(output.keyframes[1].interpolation, key4.interpolation);
    EXPECT_FLOAT_EQ(output.keyframes[1].time, key4.time);
    EXPECT_FLOAT_EQ(output.keyframes[1].value, key4.value);
  }

  {
    RawFloatTrack output;
    optimizer.tolerance = 1e-9f;
    ASSERT_TRUE(optimizer(raw_float_track, &output));

    EXPECT_EQ(output.keyframes.size(), 4u);

    EXPECT_EQ(output.keyframes[0].interpolation, key0.interpolation);
    EXPECT_FLOAT_EQ(output.keyframes[0].time, key0.time);
    EXPECT_FLOAT_EQ(output.keyframes[0].value, key0.value);

    EXPECT_EQ(output.keyframes[1].interpolation, key2.interpolation);
    EXPECT_FLOAT_EQ(output.keyframes[1].time, key2.time);
    EXPECT_FLOAT_EQ(output.keyframes[1].value, key2.value);

    EXPECT_EQ(output.keyframes[2].interpolation, key3.interpolation);
    EXPECT_FLOAT_EQ(output.keyframes[2].time, key3.time);
    EXPECT_FLOAT_EQ(output.keyframes[2].value, key3.value);

    EXPECT_EQ(output.keyframes[3].interpolation, key4.interpolation);
    EXPECT_FLOAT_EQ(output.keyframes[3].time, key4.time);
    EXPECT_FLOAT_EQ(output.keyframes[3].value, key4.value);
  }
}

TEST(float, TrackOptimizer) {
  TrackOptimizer optimizer;

  RawFloatTrack raw_track;
  RawFloatTrack::Keyframe key0 = {RawTrackInterpolation::kLinear, 0.f, 6.9f};
  raw_track.keyframes.push_back(key0);
  RawFloatTrack::Keyframe key1 = {RawTrackInterpolation::kLinear, .25f, 4.6f};
  raw_track.keyframes.push_back(key1);
  RawFloatTrack::Keyframe key2 = {RawTrackInterpolation::kLinear, .5f, 2.3f};
  raw_track.keyframes.push_back(key2);
  RawFloatTrack::Keyframe key3 = {RawTrackInterpolation::kLinear, .500001f,
                                  2.300001f};
  raw_track.keyframes.push_back(key3);
  RawFloatTrack::Keyframe key4 = {RawTrackInterpolation::kLinear, .75f, 0.f};
  raw_track.keyframes.push_back(key4);
  RawFloatTrack::Keyframe key5 = {RawTrackInterpolation::kLinear, .8f, 1e-12f};
  raw_track.keyframes.push_back(key5);
  RawFloatTrack::Keyframe key6 = {RawTrackInterpolation::kLinear, 1.f, -1e-12f};
  raw_track.keyframes.push_back(key6);

  RawFloatTrack output;
  ASSERT_TRUE(optimizer(raw_track, &output));

  EXPECT_EQ(output.keyframes.size(), 2u);

  EXPECT_EQ(output.keyframes[0].interpolation, key0.interpolation);
  EXPECT_FLOAT_EQ(output.keyframes[0].time, key0.time);
  EXPECT_FLOAT_EQ(output.keyframes[0].value, key0.value);

  EXPECT_EQ(output.keyframes[1].interpolation, key4.interpolation);
  EXPECT_FLOAT_EQ(output.keyframes[1].time, key4.time);
  EXPECT_FLOAT_EQ(output.keyframes[1].value, key4.value);
}

TEST(Float3, TrackOptimizer) {
  TrackOptimizer optimizer;

  RawFloat3Track raw_track;
  RawFloat3Track::Keyframe key0 = {RawTrackInterpolation::kLinear, 0.f,
                                   ozz::math::Float3(6.9f, 0.f, 0.f)};
  raw_track.keyframes.push_back(key0);
  RawFloat3Track::Keyframe key1 = {RawTrackInterpolation::kLinear, .25f,
                                   ozz::math::Float3(4.6f, 0.f, 0.f)};
  raw_track.keyframes.push_back(key1);
  RawFloat3Track::Keyframe key2 = {RawTrackInterpolation::kLinear, .5f,
                                   ozz::math::Float3(2.3f, 0.f, 0.f)};
  raw_track.keyframes.push_back(key2);
  RawFloat3Track::Keyframe key3 = {RawTrackInterpolation::kLinear, .500001f,
                                   ozz::math::Float3(2.3000001f, 0.f, 0.f)};
  raw_track.keyframes.push_back(key3);
  RawFloat3Track::Keyframe key4 = {RawTrackInterpolation::kLinear, .75f,
                                   ozz::math::Float3(0.f, 0.f, 0.f)};
  raw_track.keyframes.push_back(key4);
  RawFloat3Track::Keyframe key5 = {RawTrackInterpolation::kLinear, .8f,
                                   ozz::math::Float3(0.f, 0.f, 1e-12f)};
  raw_track.keyframes.push_back(key5);
  RawFloat3Track::Keyframe key6 = {RawTrackInterpolation::kLinear, 1.f,
                                   ozz::math::Float3(0.f, -1e-12f, 0.f)};
  raw_track.keyframes.push_back(key6);

  RawFloat3Track output;
  ASSERT_TRUE(optimizer(raw_track, &output));

  EXPECT_EQ(output.keyframes.size(), 2u);

  EXPECT_EQ(output.keyframes[0].interpolation, key0.interpolation);
  EXPECT_FLOAT_EQ(output.keyframes[0].time, key0.time);
  EXPECT_FLOAT3_EQ(output.keyframes[0].value, key0.value.x, key0.value.y,
                   key0.value.z);

  EXPECT_EQ(output.keyframes[1].interpolation, key4.interpolation);
  EXPECT_FLOAT_EQ(output.keyframes[1].time, key4.time);
  EXPECT_FLOAT3_EQ(output.keyframes[1].value, key4.value.x, key4.value.y,
                   key4.value.z);
}
