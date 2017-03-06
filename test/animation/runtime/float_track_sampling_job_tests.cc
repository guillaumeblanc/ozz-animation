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

#include "ozz/animation/runtime/float_track_sampling_job.h"

#include "gtest/gtest.h"

#include "ozz/base/maths/gtest_math_helper.h"
#include "ozz/base/memory/allocator.h"

#include "ozz/animation/offline/float_track_builder.h"
#include "ozz/animation/offline/raw_float_track.h"

#include "ozz/animation/runtime/float_track.h"

using ozz::animation::FloatTrack;
using ozz::animation::FloatTrackSamplingJob;
using ozz::animation::offline::RawFloatTrack;
using ozz::animation::offline::FloatTrackBuilder;

// clang-format off

TEST(JobValidity, FloatTrackSamplingJob) {

  // Instantiates a builder objects with default parameters.
  FloatTrackBuilder builder;

  // Building default RawFloatTrack succeeds.
  RawFloatTrack raw_float_track;
  EXPECT_TRUE(raw_float_track.Validate());

  // Builds track
  FloatTrack* track = builder(raw_float_track);
  EXPECT_TRUE(track != NULL);

  {  // Empty/default job
    FloatTrackSamplingJob job;
    EXPECT_FALSE(job.Validate());
    EXPECT_FALSE(job.Run());
  }

  {  // Invalid output
    FloatTrackSamplingJob job;
    job.track = track;
    EXPECT_FALSE(job.Validate());
    EXPECT_FALSE(job.Run());
  }

  {  // Invalid track.
    FloatTrackSamplingJob job;
    float result;
    job.result = &result;
    EXPECT_FALSE(job.Validate());
    EXPECT_FALSE(job.Run());
  }

  {  // Valid
    FloatTrackSamplingJob job;
    job.track = track;
    float result;
    job.result = &result;
    EXPECT_TRUE(job.Validate());
    EXPECT_TRUE(job.Run());
  }
  ozz::memory::default_allocator()->Delete(track);
}

TEST(Bounds, FloatTrackSamplingJob) {
  FloatTrackBuilder builder;
  float result;

  RawFloatTrack raw_float_track;

  RawFloatTrack::Keyframe key0 = {RawFloatTrack::kLinear,
                                        0.f, 0.f};
  raw_float_track.keyframes.push_back(key0);
  RawFloatTrack::Keyframe key1 = {RawFloatTrack::kStep,
                                        .5f, 46.f};
  raw_float_track.keyframes.push_back(key1);
  RawFloatTrack::Keyframe key2 = {RawFloatTrack::kLinear,
                                        .7f, 0.f};
  raw_float_track.keyframes.push_back(key2);

  // Builds track
  FloatTrack* track = builder(raw_float_track);
  ASSERT_TRUE(track != NULL);

  // Samples to verify build output.
  FloatTrackSamplingJob sampling;
  sampling.track = track;
  sampling.result = &result;

  sampling.time = 0.f - 1e-7f;
  ASSERT_TRUE(sampling.Run());
  EXPECT_FLOAT_EQ(result, 0.f);

  sampling.time = 0.f;
  ASSERT_TRUE(sampling.Run());
  EXPECT_FLOAT_EQ(result, 0.f);

  sampling.time = .5f;
  ASSERT_TRUE(sampling.Run());
  EXPECT_FLOAT_EQ(result, 46.f);

  sampling.time = 1.f;
  ASSERT_TRUE(sampling.Run());
  EXPECT_FLOAT_EQ(result, 0.f);

  sampling.time = 1.f + 1e-7f;
  ASSERT_TRUE(sampling.Run());
  EXPECT_FLOAT_EQ(result, 0.f);

  ozz::memory::default_allocator()->Delete(track);
}
