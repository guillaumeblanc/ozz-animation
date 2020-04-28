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

#include "ozz/animation/offline/track_optimizer.h"

#include "gtest/gtest.h"
#include "ozz/base/maths/gtest_math_helper.h"

#include "ozz/animation/offline/raw_track.h"
#include "ozz/animation/offline/track_builder.h"

using ozz::animation::offline::RawFloat2Track;
using ozz::animation::offline::RawFloat3Track;
using ozz::animation::offline::RawFloat4Track;
using ozz::animation::offline::RawFloatTrack;
using ozz::animation::offline::RawQuaternionTrack;
using ozz::animation::offline::RawTrackInterpolation;
using ozz::animation::offline::TrackOptimizer;

TEST(Error, TrackOptimizer) {
  TrackOptimizer optimizer;

  {  // nullptr output.
    RawFloatTrack input;
    EXPECT_TRUE(input.Validate());

    // Builds animation
    EXPECT_FALSE(optimizer(input, nullptr));
  }

  {  // Invalid input animation.
    RawFloatTrack input;
    input.keyframes.resize(1);
    input.keyframes[0].ratio = 99.f;
    EXPECT_FALSE(input.Validate());

    // Builds animation
    RawFloatTrack output;
    output.keyframes.resize(1);
    EXPECT_FALSE(optimizer(input, &output));
    EXPECT_EQ(output.keyframes.size(), 0u);
  }
}

TEST(Name, FloatTrackOptimizer) {
  // Step keys aren't optimized.
  TrackOptimizer optimizer;

  RawFloatTrack raw_float_track;
  raw_float_track.name = "FloatTrackOptimizer test";

  RawFloatTrack output;
  ASSERT_TRUE(optimizer(raw_float_track, &output));

  EXPECT_STREQ(raw_float_track.name.c_str(), output.name.c_str());
}

TEST(OptimizeSteps, TrackOptimizer) {
  // Step keys aren't optimized.
  TrackOptimizer optimizer;

  RawFloatTrack raw_float_track;
  const RawFloatTrack::Keyframe key0 = {RawTrackInterpolation::kStep, .5f,
                                        46.f};
  raw_float_track.keyframes.push_back(key0);
  const RawFloatTrack::Keyframe key1 = {RawTrackInterpolation::kStep, .7f, 0.f};
  raw_float_track.keyframes.push_back(key1);
  const RawFloatTrack::Keyframe key2 = {RawTrackInterpolation::kStep, .8f,
                                        1e-9f};
  raw_float_track.keyframes.push_back(key2);

  RawFloatTrack output;
  ASSERT_TRUE(optimizer(raw_float_track, &output));

  EXPECT_EQ(output.keyframes.size(), 3u);

  EXPECT_EQ(output.keyframes[0].interpolation, key0.interpolation);
  EXPECT_FLOAT_EQ(output.keyframes[0].ratio, key0.ratio);
  EXPECT_FLOAT_EQ(output.keyframes[0].value, key0.value);

  EXPECT_EQ(output.keyframes[1].interpolation, key1.interpolation);
  EXPECT_FLOAT_EQ(output.keyframes[1].ratio, key1.ratio);
  EXPECT_FLOAT_EQ(output.keyframes[1].value, key1.value);

  EXPECT_EQ(output.keyframes[2].interpolation, key2.interpolation);
  EXPECT_FLOAT_EQ(output.keyframes[2].ratio, key2.ratio);
  EXPECT_FLOAT_EQ(output.keyframes[2].value, key2.value);
}

TEST(OptimizeInterpolate, TrackOptimizer) {
  // Step keys aren't optimized.
  TrackOptimizer optimizer;

  RawFloatTrack raw_float_track;
  const RawFloatTrack::Keyframe key0 = {RawTrackInterpolation::kLinear, 0.f,
                                        69.f};
  raw_float_track.keyframes.push_back(key0);
  const RawFloatTrack::Keyframe key1 = {RawTrackInterpolation::kLinear, .25f,
                                        46.f};
  raw_float_track.keyframes.push_back(key1);
  const RawFloatTrack::Keyframe key2 = {RawTrackInterpolation::kLinear, .5f,
                                        23.f};
  raw_float_track.keyframes.push_back(key2);
  const RawFloatTrack::Keyframe key3 = {RawTrackInterpolation::kLinear,
                                        .500001f, 23.000001f};
  raw_float_track.keyframes.push_back(key3);
  const RawFloatTrack::Keyframe key4 = {RawTrackInterpolation::kLinear, .75f,
                                        0.f};
  raw_float_track.keyframes.push_back(key4);
  const RawFloatTrack::Keyframe key5 = {RawTrackInterpolation::kLinear, .8f,
                                        1e-12f};
  raw_float_track.keyframes.push_back(key5);
  const RawFloatTrack::Keyframe key6 = {RawTrackInterpolation::kLinear, 1.f,
                                        -1e-12f};
  raw_float_track.keyframes.push_back(key6);

  {
    RawFloatTrack output;

    optimizer.tolerance = 1e-3f;
    ASSERT_TRUE(optimizer(raw_float_track, &output));

    EXPECT_EQ(output.keyframes.size(), 2u);

    EXPECT_EQ(output.keyframes[0].interpolation, key0.interpolation);
    EXPECT_FLOAT_EQ(output.keyframes[0].ratio, key0.ratio);
    EXPECT_FLOAT_EQ(output.keyframes[0].value, key0.value);

    EXPECT_EQ(output.keyframes[1].interpolation, key4.interpolation);
    EXPECT_FLOAT_EQ(output.keyframes[1].ratio, key4.ratio);
    EXPECT_FLOAT_EQ(output.keyframes[1].value, key4.value);
  }

  {
    RawFloatTrack output;
    optimizer.tolerance = 1e-9f;
    ASSERT_TRUE(optimizer(raw_float_track, &output));

    EXPECT_EQ(output.keyframes.size(), 4u);

    EXPECT_EQ(output.keyframes[0].interpolation, key0.interpolation);
    EXPECT_FLOAT_EQ(output.keyframes[0].ratio, key0.ratio);
    EXPECT_FLOAT_EQ(output.keyframes[0].value, key0.value);

    EXPECT_EQ(output.keyframes[1].interpolation, key2.interpolation);
    EXPECT_FLOAT_EQ(output.keyframes[1].ratio, key2.ratio);
    EXPECT_FLOAT_EQ(output.keyframes[1].value, key2.value);

    EXPECT_EQ(output.keyframes[2].interpolation, key3.interpolation);
    EXPECT_FLOAT_EQ(output.keyframes[2].ratio, key3.ratio);
    EXPECT_FLOAT_EQ(output.keyframes[2].value, key3.value);

    EXPECT_EQ(output.keyframes[3].interpolation, key4.interpolation);
    EXPECT_FLOAT_EQ(output.keyframes[3].ratio, key4.ratio);
    EXPECT_FLOAT_EQ(output.keyframes[3].value, key4.value);
  }
}

TEST(float, TrackOptimizer) {
  TrackOptimizer optimizer;

  RawFloatTrack raw_track;
  const RawFloatTrack::Keyframe key0 = {RawTrackInterpolation::kLinear, 0.f,
                                        6.9f};
  raw_track.keyframes.push_back(key0);
  const RawFloatTrack::Keyframe key1 = {RawTrackInterpolation::kLinear, .25f,
                                        4.6f};
  raw_track.keyframes.push_back(key1);
  const RawFloatTrack::Keyframe key2 = {RawTrackInterpolation::kLinear, .5f,
                                        2.3f};
  raw_track.keyframes.push_back(key2);
  const RawFloatTrack::Keyframe key3 = {RawTrackInterpolation::kLinear,
                                        .500001f, 2.300001f};
  raw_track.keyframes.push_back(key3);
  const RawFloatTrack::Keyframe key4 = {RawTrackInterpolation::kLinear, .75f,
                                        0.f};
  raw_track.keyframes.push_back(key4);
  const RawFloatTrack::Keyframe key5 = {RawTrackInterpolation::kLinear, .8f,
                                        1e-12f};
  raw_track.keyframes.push_back(key5);
  const RawFloatTrack::Keyframe key6 = {RawTrackInterpolation::kLinear, 1.f,
                                        -1e-12f};
  raw_track.keyframes.push_back(key6);

  RawFloatTrack output;
  ASSERT_TRUE(optimizer(raw_track, &output));

  EXPECT_EQ(output.keyframes.size(), 2u);

  EXPECT_EQ(output.keyframes[0].interpolation, key0.interpolation);
  EXPECT_FLOAT_EQ(output.keyframes[0].ratio, key0.ratio);
  EXPECT_FLOAT_EQ(output.keyframes[0].value, key0.value);

  EXPECT_EQ(output.keyframes[1].interpolation, key4.interpolation);
  EXPECT_FLOAT_EQ(output.keyframes[1].ratio, key4.ratio);
  EXPECT_FLOAT_EQ(output.keyframes[1].value, key4.value);
}

TEST(Float2, TrackOptimizer) {
  TrackOptimizer optimizer;

  RawFloat2Track raw_track;
  const RawFloat2Track::Keyframe key0 = {RawTrackInterpolation::kLinear, 0.f,
                                         ozz::math::Float2(6.9f, 0.f)};
  raw_track.keyframes.push_back(key0);
  const RawFloat2Track::Keyframe key1 = {RawTrackInterpolation::kLinear, .25f,
                                         ozz::math::Float2(4.6f, 0.f)};
  raw_track.keyframes.push_back(key1);
  const RawFloat2Track::Keyframe key2 = {RawTrackInterpolation::kLinear, .5f,
                                         ozz::math::Float2(2.3f, 0.f)};
  raw_track.keyframes.push_back(key2);
  const RawFloat2Track::Keyframe key3 = {RawTrackInterpolation::kLinear,
                                         .500001f,
                                         ozz::math::Float2(2.3000001f, 0.f)};
  raw_track.keyframes.push_back(key3);
  const RawFloat2Track::Keyframe key4 = {RawTrackInterpolation::kLinear, .75f,
                                         ozz::math::Float2(0.f, 0.f)};
  raw_track.keyframes.push_back(key4);
  const RawFloat2Track::Keyframe key5 = {RawTrackInterpolation::kLinear, .8f,
                                         ozz::math::Float2(0.f, 1e-12f)};
  raw_track.keyframes.push_back(key5);
  const RawFloat2Track::Keyframe key6 = {RawTrackInterpolation::kLinear, 1.f,
                                         ozz::math::Float2(-1e-12f, 0.f)};
  raw_track.keyframes.push_back(key6);

  RawFloat2Track output;
  ASSERT_TRUE(optimizer(raw_track, &output));

  EXPECT_EQ(output.keyframes.size(), 2u);

  EXPECT_EQ(output.keyframes[0].interpolation, key0.interpolation);
  EXPECT_FLOAT_EQ(output.keyframes[0].ratio, key0.ratio);
  EXPECT_FLOAT2_EQ(output.keyframes[0].value, key0.value.x, key0.value.y);

  EXPECT_EQ(output.keyframes[1].interpolation, key4.interpolation);
  EXPECT_FLOAT_EQ(output.keyframes[1].ratio, key4.ratio);
  EXPECT_FLOAT2_EQ(output.keyframes[1].value, key4.value.x, key4.value.y);
}

TEST(Float3, TrackOptimizer) {
  TrackOptimizer optimizer;

  RawFloat3Track raw_track;
  const RawFloat3Track::Keyframe key0 = {RawTrackInterpolation::kLinear, 0.f,
                                         ozz::math::Float3(6.9f, 0.f, 0.f)};
  raw_track.keyframes.push_back(key0);
  const RawFloat3Track::Keyframe key1 = {RawTrackInterpolation::kLinear, .25f,
                                         ozz::math::Float3(4.6f, 0.f, 0.f)};
  raw_track.keyframes.push_back(key1);
  const RawFloat3Track::Keyframe key2 = {RawTrackInterpolation::kLinear, .5f,
                                         ozz::math::Float3(2.3f, 0.f, 0.f)};
  raw_track.keyframes.push_back(key2);
  const RawFloat3Track::Keyframe key3 = {
      RawTrackInterpolation::kLinear, .500001f,
      ozz::math::Float3(2.3000001f, 0.f, 0.f)};
  raw_track.keyframes.push_back(key3);
  const RawFloat3Track::Keyframe key4 = {RawTrackInterpolation::kLinear, .75f,
                                         ozz::math::Float3(0.f, 0.f, 0.f)};
  raw_track.keyframes.push_back(key4);
  const RawFloat3Track::Keyframe key5 = {RawTrackInterpolation::kLinear, .8f,
                                         ozz::math::Float3(0.f, 0.f, 1e-12f)};
  raw_track.keyframes.push_back(key5);
  const RawFloat3Track::Keyframe key6 = {RawTrackInterpolation::kLinear, 1.f,
                                         ozz::math::Float3(0.f, -1e-12f, 0.f)};
  raw_track.keyframes.push_back(key6);

  RawFloat3Track output;
  ASSERT_TRUE(optimizer(raw_track, &output));

  EXPECT_EQ(output.keyframes.size(), 2u);

  EXPECT_EQ(output.keyframes[0].interpolation, key0.interpolation);
  EXPECT_FLOAT_EQ(output.keyframes[0].ratio, key0.ratio);
  EXPECT_FLOAT3_EQ(output.keyframes[0].value, key0.value.x, key0.value.y,
                   key0.value.z);

  EXPECT_EQ(output.keyframes[1].interpolation, key4.interpolation);
  EXPECT_FLOAT_EQ(output.keyframes[1].ratio, key4.ratio);
  EXPECT_FLOAT3_EQ(output.keyframes[1].value, key4.value.x, key4.value.y,
                   key4.value.z);
}

TEST(Float4, TrackOptimizer) {
  TrackOptimizer optimizer;

  RawFloat4Track raw_track;
  const RawFloat4Track::Keyframe key0 = {
      RawTrackInterpolation::kLinear, 0.f,
      ozz::math::Float4(6.9f, 0.f, 0.f, 0.f)};
  raw_track.keyframes.push_back(key0);
  const RawFloat4Track::Keyframe key1 = {
      RawTrackInterpolation::kLinear, .25f,
      ozz::math::Float4(4.6f, 0.f, 0.f, 0.f)};
  raw_track.keyframes.push_back(key1);
  const RawFloat4Track::Keyframe key2 = {
      RawTrackInterpolation::kLinear, .5f,
      ozz::math::Float4(2.3f, 0.f, 0.f, 0.f)};
  raw_track.keyframes.push_back(key2);
  const RawFloat4Track::Keyframe key3 = {
      RawTrackInterpolation::kLinear, .500001f,
      ozz::math::Float4(2.3000001f, 0.f, 0.f, 0.f)};
  raw_track.keyframes.push_back(key3);
  const RawFloat4Track::Keyframe key4 = {RawTrackInterpolation::kLinear, .75f,
                                         ozz::math::Float4(0.f, 0.f, 0.f, 0.f)};
  raw_track.keyframes.push_back(key4);
  const RawFloat4Track::Keyframe key5 = {
      RawTrackInterpolation::kLinear, .8f,
      ozz::math::Float4(0.f, 0.f, 0.f, 1e-12f)};
  raw_track.keyframes.push_back(key5);
  const RawFloat4Track::Keyframe key6 = {
      RawTrackInterpolation::kLinear, 1.f,
      ozz::math::Float4(0.f, 0.f, 0.f, -1e-12f)};
  raw_track.keyframes.push_back(key6);

  RawFloat4Track output;
  ASSERT_TRUE(optimizer(raw_track, &output));

  EXPECT_EQ(output.keyframes.size(), 2u);

  EXPECT_EQ(output.keyframes[0].interpolation, key0.interpolation);
  EXPECT_FLOAT_EQ(output.keyframes[0].ratio, key0.ratio);
  EXPECT_FLOAT4_EQ(output.keyframes[0].value, key0.value.x, key0.value.y,
                   key0.value.z, key0.value.w);

  EXPECT_EQ(output.keyframes[1].interpolation, key4.interpolation);
  EXPECT_FLOAT_EQ(output.keyframes[1].ratio, key4.ratio);
  EXPECT_FLOAT4_EQ(output.keyframes[1].value, key4.value.x, key4.value.y,
                   key4.value.z, key0.value.w);
}

TEST(Quaternion, TrackOptimizer) {
  TrackOptimizer optimizer;

  RawQuaternionTrack raw_track;
  const RawQuaternionTrack::Keyframe key0 = {
      RawTrackInterpolation::kLinear, 0.f,
      ozz::math::Quaternion(.70710677f, 0.f, 0.f, .70710677f)};
  raw_track.keyframes.push_back(key0);
  const RawQuaternionTrack::Keyframe key1 = {
      RawTrackInterpolation::kLinear, .1f,
      ozz::math::Quaternion(.6172133f, .1543033f, 0.f, .7715167f)};  // NLerp
  raw_track.keyframes.push_back(key1);
  const RawQuaternionTrack::Keyframe key2 = {
      RawTrackInterpolation::kLinear, .5f,
      ozz::math::Quaternion(0.f, .70710677f, 0.f, .70710677f)};
  raw_track.keyframes.push_back(key2);
  const RawQuaternionTrack::Keyframe key3 = {
      RawTrackInterpolation::kLinear, .500001f,
      ozz::math::Quaternion(0.f, .70710676f, 0.f, .70710678f)};
  raw_track.keyframes.push_back(key3);
  const RawQuaternionTrack::Keyframe key4 = {
      RawTrackInterpolation::kLinear, .75f,
      ozz::math::Quaternion(0.f, .70710677f, 0.f, .70710677f)};
  raw_track.keyframes.push_back(key4);
  const RawQuaternionTrack::Keyframe key5 = {
      RawTrackInterpolation::kLinear, .8f,
      ozz::math::Quaternion(-0.f, -0.70710677f, -0.f, -.70710677f)};
  raw_track.keyframes.push_back(key5);
  const RawQuaternionTrack::Keyframe key6 = {
      RawTrackInterpolation::kLinear, 1.f,
      ozz::math::Quaternion(0.f, .70710677f, 0.f, .70710677f)};
  raw_track.keyframes.push_back(key6);

  RawQuaternionTrack output;
  ASSERT_TRUE(optimizer(raw_track, &output));

  EXPECT_EQ(output.keyframes.size(), 2u);

  EXPECT_EQ(output.keyframes[0].interpolation, key0.interpolation);
  EXPECT_FLOAT_EQ(output.keyframes[0].ratio, key0.ratio);
  EXPECT_QUATERNION_EQ(output.keyframes[0].value, key0.value.x, key0.value.y,
                       key0.value.z, key0.value.w);

  EXPECT_EQ(output.keyframes[1].interpolation, key2.interpolation);
  EXPECT_FLOAT_EQ(output.keyframes[1].ratio, key2.ratio);
  EXPECT_QUATERNION_EQ(output.keyframes[1].value, key2.value.x, key2.value.y,
                       key2.value.z, key2.value.w);
}
