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
#include "ozz/animation/offline/raw_track.h"
#include "ozz/animation/offline/raw_track_utils.h"
#include "ozz/base/maths/gtest_math_helper.h"

using ozz::animation::offline::RawFloat2Track;
using ozz::animation::offline::RawFloat3Track;
using ozz::animation::offline::RawFloat4Track;
using ozz::animation::offline::RawFloatTrack;
using ozz::animation::offline::RawQuaternionTrack;
using ozz::animation::offline::RawTrackInterpolation;

TEST(Invalid, RawTrackUtils) {
  {
    RawFloatTrack raw_track;
    raw_track.keyframes.resize(1);
    raw_track.keyframes[0].ratio = 99.f;
    EXPECT_FALSE(raw_track.Validate());
    float out;
    EXPECT_FALSE(ozz::animation::offline::SampleTrack(raw_track, 0.f, &out));
  }
}

TEST(SampleFloat, RawTrackUtils) {
  {  // Empty
    RawFloatTrack raw_track;
    EXPECT_TRUE(raw_track.Validate());
    float out;
    ASSERT_TRUE(ozz::animation::offline::SampleTrack(raw_track, 0.f, &out));
    EXPECT_FLOAT_EQ(out, 0.f);
  }

  {  // Single key
    RawFloatTrack raw_track;
    raw_track.keyframes.push_back({RawTrackInterpolation::kLinear, .46f, 46.f});
    EXPECT_TRUE(raw_track.Validate());

    float out;
    ASSERT_TRUE(ozz::animation::offline::SampleTrack(raw_track, 0.f, &out));
    EXPECT_FLOAT_EQ(out, 46.f);

    ASSERT_TRUE(ozz::animation::offline::SampleTrack(raw_track, 1.f, &out));
    EXPECT_FLOAT_EQ(out, 46.f);
  }

  {  // Mixed interpolation
    RawFloatTrack raw_track;
    raw_track.keyframes.push_back({RawTrackInterpolation::kLinear, 0.f, 1.f});
    raw_track.keyframes.push_back({RawTrackInterpolation::kStep, .2f, 2.f});
    raw_track.keyframes.push_back({RawTrackInterpolation::kLinear, .5f, 3.f});
    raw_track.keyframes.push_back({RawTrackInterpolation::kLinear, .75f, 3.f});

    EXPECT_TRUE(raw_track.Validate());
    float out;

    ASSERT_TRUE(ozz::animation::offline::SampleTrack(raw_track, 0.f, &out));
    EXPECT_FLOAT_EQ(out, 1.f);

    ASSERT_TRUE(ozz::animation::offline::SampleTrack(raw_track, 1.f, &out));
    EXPECT_FLOAT_EQ(out, 3.f);

    ASSERT_TRUE(ozz::animation::offline::SampleTrack(raw_track, .1f, &out));
    EXPECT_FLOAT_EQ(out, 1.5f);

    ASSERT_TRUE(ozz::animation::offline::SampleTrack(raw_track, .2f, &out));
    EXPECT_FLOAT_EQ(out, 2.f);

    ASSERT_TRUE(ozz::animation::offline::SampleTrack(raw_track, .25f, &out));
    EXPECT_FLOAT_EQ(out, 2.f);  // Step interpolation

    ASSERT_TRUE(ozz::animation::offline::SampleTrack(raw_track, .5f, &out));
    EXPECT_FLOAT_EQ(out, 3.f);
  }
}

TEST(SampleFloat2, RawTrackUtils) {
  {  // Empty
    RawFloat2Track raw_track;
    EXPECT_TRUE(raw_track.Validate());
    ozz::math::Float2 out;
    ASSERT_TRUE(ozz::animation::offline::SampleTrack(raw_track, 0.f, &out));
    EXPECT_FLOAT2_EQ(out, 0.f, 0.f);
  }
}

TEST(SampleFloat3, RawTrackUtils) {
  {  // Empty
    RawFloat3Track raw_track;
    EXPECT_TRUE(raw_track.Validate());
    ozz::math::Float3 out;
    ASSERT_TRUE(ozz::animation::offline::SampleTrack(raw_track, 0.f, &out));
    EXPECT_FLOAT3_EQ(out, 0.f, 0.f, 0.f);
  }
}

TEST(SampleFloat4, RawTrackUtils) {
  {  // Empty
    RawFloat4Track raw_track;
    EXPECT_TRUE(raw_track.Validate());
    ozz::math::Float4 out;
    ASSERT_TRUE(ozz::animation::offline::SampleTrack(raw_track, 0.f, &out));
    EXPECT_FLOAT4_EQ(out, 0.f, 0.f, 0.f, 0.f);
  }
}

TEST(SampleQauternion, RawTrackUtils) {
  {  // Empty
    RawQuaternionTrack raw_track;
    EXPECT_TRUE(raw_track.Validate());

    ozz::math::Quaternion out;
    ASSERT_TRUE(ozz::animation::offline::SampleTrack(raw_track, 0.f, &out));
    EXPECT_QUATERNION_EQ(out, 0.f, 0.f, 0.f, 1.f);
  }

  {  // NLerp
    RawQuaternionTrack raw_track;
    raw_track.keyframes.push_back(
        {RawTrackInterpolation::kLinear, 0.f,
         ozz::math::Quaternion(.70710677f, 0.f, 0.f, .70710677f)});
    raw_track.keyframes.push_back(
        {RawTrackInterpolation::kLinear, 1.f,
         ozz::math::Quaternion(0.f, .70710677f, 0.f, .70710677f)});

    ozz::math::Quaternion out;
    ASSERT_TRUE(ozz::animation::offline::SampleTrack(raw_track, .2f, &out));
    EXPECT_QUATERNION_EQ(out, .6172133f, .1543033f, 0.f, .7715167f);
  }

  {  // NLerp opposite
    RawQuaternionTrack raw_track;
    raw_track.keyframes.push_back(
        {RawTrackInterpolation::kLinear, 0.f,
         ozz::math::Quaternion(.70710677f, 0.f, 0.f, .70710677f)});
    raw_track.keyframes.push_back(
        {RawTrackInterpolation::kLinear, 1.f,
         ozz::math::Quaternion(0.f, -.70710677f, 0.f, -.70710677f)});

    ozz::math::Quaternion out;
    ASSERT_TRUE(ozz::animation::offline::SampleTrack(raw_track, .2f, &out));
    EXPECT_QUATERNION_EQ(out, .6172133f, .1543033f, 0.f, .7715167f);
  }
}