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

#include "ozz/animation/offline/track_builder.h"

#include "gtest/gtest.h"
#include "ozz/base/maths/gtest_math_helper.h"

#include "ozz/base/memory/unique_ptr.h"

#include "ozz/animation/offline/raw_track.h"
#include "ozz/animation/runtime/track.h"
#include "ozz/animation/runtime/track_sampling_job.h"

#include <limits>

using ozz::animation::FloatTrack;
using ozz::animation::Float2Track;
using ozz::animation::Float3Track;
using ozz::animation::Float4Track;
using ozz::animation::QuaternionTrack;
using ozz::animation::FloatTrackSamplingJob;
using ozz::animation::offline::RawFloatTrack;
using ozz::animation::offline::RawTrackInterpolation;
using ozz::animation::offline::TrackBuilder;

TEST(Default, TrackBuilder) {
  // Instantiates a builder objects with default parameters.
  TrackBuilder builder;

  {  // Building default RawFloatTrack succeeds.
    RawFloatTrack raw_float_track;
    EXPECT_TRUE(raw_float_track.Validate());

    // Builds track
    ozz::unique_ptr<FloatTrack> track(builder(raw_float_track));
    EXPECT_TRUE(track);
  }
}

TEST(Build, TrackBuilder) {
  // Instantiates a builder objects with default parameters.
  TrackBuilder builder;

  {  // Building a track with unsorted keys fails.
    RawFloatTrack raw_float_track;

    // Adds 2 unordered keys
    const RawFloatTrack::Keyframe first_key = {RawTrackInterpolation::kLinear,
                                               .8f, 0.f};
    raw_float_track.keyframes.push_back(first_key);
    const RawFloatTrack::Keyframe second_key = {RawTrackInterpolation::kLinear,
                                                .2f, 0.f};
    raw_float_track.keyframes.push_back(second_key);

    // Builds track
    EXPECT_FALSE(raw_float_track.Validate());
    EXPECT_TRUE(!builder(raw_float_track));
  }

  {  // Building a track with invalid key frame's ratio fails.
    RawFloatTrack raw_float_track;

    // Adds 2 unordered keys
    const RawFloatTrack::Keyframe first_key = {RawTrackInterpolation::kLinear,
                                               1.8f, 0.f};
    raw_float_track.keyframes.push_back(first_key);

    // Builds track
    EXPECT_FALSE(raw_float_track.Validate());
    EXPECT_TRUE(!builder(raw_float_track));
  }

  {  // Building a track with equal key frame's ratio fails.
    RawFloatTrack raw_float_track;

    // Adds 2 unordered keys
    const RawFloatTrack::Keyframe first_key = {RawTrackInterpolation::kLinear,
                                               .8f, 0.f};
    raw_float_track.keyframes.push_back(first_key);
    const RawFloatTrack::Keyframe second_key = {RawTrackInterpolation::kLinear,
                                                .8f, 1.f};
    raw_float_track.keyframes.push_back(second_key);

    // Builds track
    EXPECT_FALSE(raw_float_track.Validate());
    EXPECT_TRUE(!builder(raw_float_track));
  }

  {  // Building a valid track with 1 key succeeds.
    RawFloatTrack raw_float_track;
    const RawFloatTrack::Keyframe first_key = {RawTrackInterpolation::kLinear,
                                               .8f, 0.f};
    raw_float_track.keyframes.push_back(first_key);
    EXPECT_TRUE(raw_float_track.Validate());

    // Builds track
    ozz::unique_ptr<FloatTrack> track(builder(raw_float_track));
    EXPECT_TRUE(track);
  }
}

TEST(Name, TrackBuilder) {
  // Instantiates a builder objects with default parameters.
  TrackBuilder builder;

  {  // No name
    RawFloatTrack raw_float_track;

    ozz::unique_ptr<FloatTrack> track(builder(raw_float_track));
    EXPECT_TRUE(track);

    EXPECT_STREQ(track->name(), "");
  }

  {  // A name
    RawFloatTrack raw_float_track;
    raw_float_track.name = "test name";

    ozz::unique_ptr<FloatTrack> track(builder(raw_float_track));
    EXPECT_TRUE(track);

    EXPECT_STREQ(track->name(), raw_float_track.name.c_str());
  }
}

TEST(Build0Keys, TrackBuilder) {
  // Instantiates a builder objects with default parameters.
  TrackBuilder builder;

  RawFloatTrack raw_float_track;

  // Builds track
  ozz::unique_ptr<FloatTrack> track(builder(raw_float_track));
  EXPECT_TRUE(track);

  // Samples to verify build output.
  FloatTrackSamplingJob sampling;
  sampling.track = track.get();
  float result;
  sampling.result = &result;
  sampling.ratio = 0.f;
  ASSERT_TRUE(sampling.Run());
  EXPECT_FLOAT_EQ(result, 0.f);
}

TEST(BuildLinear, TrackBuilder) {
  // Instantiates a builder objects with default parameters.
  TrackBuilder builder;
  float result;

  {  // 1 key at the beginning
    RawFloatTrack raw_float_track;

    const RawFloatTrack::Keyframe first_key = {RawTrackInterpolation::kLinear,
                                               0.f, 46.f};
    raw_float_track.keyframes.push_back(first_key);

    // Builds track
    ozz::unique_ptr<FloatTrack> track(builder(raw_float_track));
    EXPECT_TRUE(track);

    // Samples to verify build output.
    FloatTrackSamplingJob sampling;
    sampling.track = track.get();
    sampling.result = &result;

    sampling.ratio = 0.f;
    ASSERT_TRUE(sampling.Run());
    EXPECT_FLOAT_EQ(result, 46.f);

    sampling.ratio = .5f;
    ASSERT_TRUE(sampling.Run());
    EXPECT_FLOAT_EQ(result, 46.f);

    sampling.ratio = 1.f;
    ASSERT_TRUE(sampling.Run());
    EXPECT_FLOAT_EQ(result, 46.f);
  }

  {  // 1 key in the middle
    RawFloatTrack raw_float_track;

    const RawFloatTrack::Keyframe first_key = {RawTrackInterpolation::kLinear,
                                               .5f, 46.f};
    raw_float_track.keyframes.push_back(first_key);

    // Builds track
    ozz::unique_ptr<FloatTrack> track(builder(raw_float_track));
    EXPECT_TRUE(track);

    // Samples to verify build output.
    FloatTrackSamplingJob sampling;
    sampling.track = track.get();
    sampling.result = &result;

    sampling.ratio = 0.f;
    ASSERT_TRUE(sampling.Run());
    EXPECT_FLOAT_EQ(result, 46.f);

    sampling.ratio = .5f;
    ASSERT_TRUE(sampling.Run());
    EXPECT_FLOAT_EQ(result, 46.f);

    sampling.ratio = 1.f;
    ASSERT_TRUE(sampling.Run());
    EXPECT_FLOAT_EQ(result, 46.f);
  }

  {  // 1 key at the end
    RawFloatTrack raw_float_track;

    const RawFloatTrack::Keyframe first_key = {RawTrackInterpolation::kLinear,
                                               1.f, 46.f};
    raw_float_track.keyframes.push_back(first_key);

    // Builds track
    ozz::unique_ptr<FloatTrack> track(builder(raw_float_track));
    EXPECT_TRUE(track);

    // Samples to verify build output.
    FloatTrackSamplingJob sampling;
    sampling.track = track.get();
    sampling.result = &result;

    sampling.ratio = 0.f;
    ASSERT_TRUE(sampling.Run());
    EXPECT_FLOAT_EQ(result, 46.f);

    sampling.ratio = .5f;
    ASSERT_TRUE(sampling.Run());
    EXPECT_FLOAT_EQ(result, 46.f);

    sampling.ratio = 1.f;
    ASSERT_TRUE(sampling.Run());
    EXPECT_FLOAT_EQ(result, 46.f);
  }

  {  // 2 keys
    RawFloatTrack raw_float_track;

    const RawFloatTrack::Keyframe first_key = {RawTrackInterpolation::kLinear,
                                               .5f, 46.f};
    raw_float_track.keyframes.push_back(first_key);
    const RawFloatTrack::Keyframe second_key = {RawTrackInterpolation::kLinear,
                                                .7f, 0.f};
    raw_float_track.keyframes.push_back(second_key);

    // Builds track
    ozz::unique_ptr<FloatTrack> track(builder(raw_float_track));
    EXPECT_TRUE(track);

    // Samples to verify build output.
    FloatTrackSamplingJob sampling;
    sampling.track = track.get();
    sampling.result = &result;

    sampling.ratio = 0.f;
    ASSERT_TRUE(sampling.Run());
    EXPECT_FLOAT_EQ(result, 46.f);

    sampling.ratio = .5f;
    ASSERT_TRUE(sampling.Run());
    EXPECT_FLOAT_EQ(result, 46.f);

    sampling.ratio = .6f;
    ASSERT_TRUE(sampling.Run());
    EXPECT_FLOAT_EQ(result, 23.f);

    sampling.ratio = .7f;
    ASSERT_TRUE(sampling.Run());
    EXPECT_FLOAT_EQ(result, 0.f);

    sampling.ratio = 1.f;
    ASSERT_TRUE(sampling.Run());
    EXPECT_FLOAT_EQ(result, 0.f);
  }

  {  // n keys with same value
    RawFloatTrack raw_float_track;

    const RawFloatTrack::Keyframe key1 = {RawTrackInterpolation::kLinear, .5f,
                                          46.f};
    raw_float_track.keyframes.push_back(key1);
    const RawFloatTrack::Keyframe key2 = {RawTrackInterpolation::kLinear, .7f,
                                          46.f};
    raw_float_track.keyframes.push_back(key2);
    const RawFloatTrack::Keyframe key3 = {RawTrackInterpolation::kLinear, .8f,
                                          46.f};
    raw_float_track.keyframes.push_back(key3);

    // Builds track
    ozz::unique_ptr<FloatTrack> track(builder(raw_float_track));
    EXPECT_TRUE(track);

    // Samples to verify build output.
    FloatTrackSamplingJob sampling;
    sampling.track = track.get();
    sampling.result = &result;

    sampling.ratio = 0.f;
    ASSERT_TRUE(sampling.Run());
    EXPECT_FLOAT_EQ(result, 46.f);

    sampling.ratio = .5f;
    ASSERT_TRUE(sampling.Run());
    EXPECT_FLOAT_EQ(result, 46.f);

    sampling.ratio = .6f;
    ASSERT_TRUE(sampling.Run());
    EXPECT_FLOAT_EQ(result, 46.f);

    sampling.ratio = .7f;
    ASSERT_TRUE(sampling.Run());
    EXPECT_FLOAT_EQ(result, 46.f);

    sampling.ratio = .75f;
    ASSERT_TRUE(sampling.Run());
    EXPECT_FLOAT_EQ(result, 46.f);

    sampling.ratio = 1.f;
    ASSERT_TRUE(sampling.Run());
    EXPECT_FLOAT_EQ(result, 46.f);
  }
}

TEST(BuildStep, TrackBuilder) {
  // Instantiates a builder objects with default parameters.
  TrackBuilder builder;
  float result;

  {  // 1 key at the beginning
    RawFloatTrack raw_float_track;

    const RawFloatTrack::Keyframe first_key = {RawTrackInterpolation::kStep,
                                               0.f, 46.f};
    raw_float_track.keyframes.push_back(first_key);

    // Builds track
    ozz::unique_ptr<FloatTrack> track(builder(raw_float_track));
    EXPECT_TRUE(track);

    // Samples to verify build output.
    FloatTrackSamplingJob sampling;
    sampling.track = track.get();
    sampling.result = &result;

    sampling.ratio = 0.f;
    ASSERT_TRUE(sampling.Run());
    EXPECT_FLOAT_EQ(result, 46.f);

    sampling.ratio = .5f;
    ASSERT_TRUE(sampling.Run());
    EXPECT_FLOAT_EQ(result, 46.f);

    sampling.ratio = 1.f;
    ASSERT_TRUE(sampling.Run());
    EXPECT_FLOAT_EQ(result, 46.f);
  }

  {  // 1 key in the middle
    RawFloatTrack raw_float_track;

    const RawFloatTrack::Keyframe first_key = {RawTrackInterpolation::kStep,
                                               .5f, 46.f};
    raw_float_track.keyframes.push_back(first_key);

    // Builds track
    ozz::unique_ptr<FloatTrack> track(builder(raw_float_track));
    EXPECT_TRUE(track);

    // Samples to verify build output.
    FloatTrackSamplingJob sampling;
    sampling.track = track.get();
    sampling.result = &result;

    sampling.ratio = 0.f;
    ASSERT_TRUE(sampling.Run());
    EXPECT_FLOAT_EQ(result, 46.f);

    sampling.ratio = .5f;
    ASSERT_TRUE(sampling.Run());
    EXPECT_FLOAT_EQ(result, 46.f);

    sampling.ratio = 1.f;
    ASSERT_TRUE(sampling.Run());
    EXPECT_FLOAT_EQ(result, 46.f);
  }

  {  // 1 key at the end
    RawFloatTrack raw_float_track;

    const RawFloatTrack::Keyframe first_key = {RawTrackInterpolation::kStep,
                                               1.f, 46.f};
    raw_float_track.keyframes.push_back(first_key);

    // Builds track
    ozz::unique_ptr<FloatTrack> track(builder(raw_float_track));
    EXPECT_TRUE(track);

    // Samples to verify build output.
    FloatTrackSamplingJob sampling;
    sampling.track = track.get();
    sampling.result = &result;

    sampling.ratio = 0.f;
    ASSERT_TRUE(sampling.Run());
    EXPECT_FLOAT_EQ(result, 46.f);

    sampling.ratio = .5f;
    ASSERT_TRUE(sampling.Run());
    EXPECT_FLOAT_EQ(result, 46.f);

    sampling.ratio = 1.f;
    ASSERT_TRUE(sampling.Run());
    EXPECT_FLOAT_EQ(result, 46.f);
  }

  {  // 1 key at the beginning
    RawFloatTrack raw_float_track;

    const RawFloatTrack::Keyframe first_key = {RawTrackInterpolation::kStep,
                                               0.f, 46.f};
    raw_float_track.keyframes.push_back(first_key);

    // Builds track
    ozz::unique_ptr<FloatTrack> track(builder(raw_float_track));
    EXPECT_TRUE(track);

    // Samples to verify build output.
    FloatTrackSamplingJob sampling;
    sampling.track = track.get();
    sampling.result = &result;

    sampling.ratio = 0.f;
    ASSERT_TRUE(sampling.Run());
    EXPECT_FLOAT_EQ(result, 46.f);

    sampling.ratio = .5f;
    ASSERT_TRUE(sampling.Run());
    EXPECT_FLOAT_EQ(result, 46.f);

    sampling.ratio = 1.f;
    ASSERT_TRUE(sampling.Run());
    EXPECT_FLOAT_EQ(result, 46.f);
  }

  {  // 2 keys
    RawFloatTrack raw_float_track;

    const RawFloatTrack::Keyframe first_key = {RawTrackInterpolation::kStep,
                                               0.f, 46.f};
    raw_float_track.keyframes.push_back(first_key);
    const RawFloatTrack::Keyframe second_key = {RawTrackInterpolation::kStep,
                                                .7f, 0.f};
    raw_float_track.keyframes.push_back(second_key);

    // Builds track
    ozz::unique_ptr<FloatTrack> track(builder(raw_float_track));
    EXPECT_TRUE(track);

    // Samples to verify build output.
    FloatTrackSamplingJob sampling;
    sampling.track = track.get();
    sampling.result = &result;

    sampling.ratio = 0.f;
    ASSERT_TRUE(sampling.Run());
    EXPECT_FLOAT_EQ(result, 46.f);

    sampling.ratio = .5f;
    ASSERT_TRUE(sampling.Run());
    EXPECT_FLOAT_EQ(result, 46.f);

    sampling.ratio = .5f + 1e-7f;
    ASSERT_TRUE(sampling.Run());
    EXPECT_FLOAT_EQ(result, 46.f);

    sampling.ratio = .6f;
    ASSERT_TRUE(sampling.Run());
    EXPECT_FLOAT_EQ(result, 46.f);

    sampling.ratio = .7f - 1e-7f;
    ASSERT_TRUE(sampling.Run());
    EXPECT_FLOAT_EQ(result, 46.f);

    sampling.ratio = .7f;
    ASSERT_TRUE(sampling.Run());
    EXPECT_FLOAT_EQ(result, 0.f);

    sampling.ratio = 1.f;
    ASSERT_TRUE(sampling.Run());
    EXPECT_FLOAT_EQ(result, 0.f);
  }

  {  // 3 keys
    RawFloatTrack raw_float_track;

    const RawFloatTrack::Keyframe first_key = {RawTrackInterpolation::kStep,
                                               0.f, 46.f};
    raw_float_track.keyframes.push_back(first_key);
    const RawFloatTrack::Keyframe second_key = {RawTrackInterpolation::kStep,
                                                .7f, 0.f};
    raw_float_track.keyframes.push_back(second_key);
    const RawFloatTrack::Keyframe third_key = {RawTrackInterpolation::kStep,
                                               1.f, 99.f};
    raw_float_track.keyframes.push_back(third_key);

    // Builds track
    ozz::unique_ptr<FloatTrack> track(builder(raw_float_track));
    EXPECT_TRUE(track);

    // Samples to verify build output.
    FloatTrackSamplingJob sampling;
    sampling.track = track.get();
    sampling.result = &result;

    sampling.ratio = 0.f;
    ASSERT_TRUE(sampling.Run());
    EXPECT_FLOAT_EQ(result, 46.f);

    sampling.ratio = .5f;
    ASSERT_TRUE(sampling.Run());
    EXPECT_FLOAT_EQ(result, 46.f);

    sampling.ratio = .5f + 1e-7f;
    ASSERT_TRUE(sampling.Run());
    EXPECT_FLOAT_EQ(result, 46.f);

    sampling.ratio = .6f;
    ASSERT_TRUE(sampling.Run());
    EXPECT_FLOAT_EQ(result, 46.f);

    sampling.ratio = .7f - 1e-7f;
    ASSERT_TRUE(sampling.Run());
    EXPECT_FLOAT_EQ(result, 46.f);

    sampling.ratio = .7f;
    ASSERT_TRUE(sampling.Run());
    EXPECT_FLOAT_EQ(result, 0.f);

    sampling.ratio = .9f;
    ASSERT_TRUE(sampling.Run());
    EXPECT_FLOAT_EQ(result, 0.f);

    sampling.ratio = 1.f;
    ASSERT_TRUE(sampling.Run());
    EXPECT_FLOAT_EQ(result, 99.f);
  }

  {  // 2 close keys
    RawFloatTrack raw_float_track;

    const RawFloatTrack::Keyframe first_key = {RawTrackInterpolation::kStep,
                                               .5f, 46.f};
    raw_float_track.keyframes.push_back(first_key);
    const RawFloatTrack::Keyframe second_key = {
        RawTrackInterpolation::kStep,
        .5f + 2.f * std::numeric_limits<float>::epsilon(), 0.f};
    raw_float_track.keyframes.push_back(second_key);

    // Builds track
    ozz::unique_ptr<FloatTrack> track(builder(raw_float_track));
    EXPECT_TRUE(track);

    // Samples to verify build output.
    FloatTrackSamplingJob sampling;
    sampling.track = track.get();
    sampling.result = &result;

    sampling.ratio = 0.f;
    ASSERT_TRUE(sampling.Run());
    EXPECT_FLOAT_EQ(result, 46.f);

    sampling.ratio = .5f;
    ASSERT_TRUE(sampling.Run());
    EXPECT_FLOAT_EQ(result, 46.f);

    sampling.ratio = .5f + std::numeric_limits<float>::epsilon();
    ASSERT_TRUE(sampling.Run());
    EXPECT_FLOAT_EQ(result, 46.f);

    sampling.ratio = .5f + 2.f * std::numeric_limits<float>::epsilon();
    ASSERT_TRUE(sampling.Run());
    EXPECT_FLOAT_EQ(result, 0.f);

    sampling.ratio = .7f;
    ASSERT_TRUE(sampling.Run());
    EXPECT_FLOAT_EQ(result, 0.f);
  }

  {  // 3 keys
    RawFloatTrack raw_float_track;

    const RawFloatTrack::Keyframe first_key = {RawTrackInterpolation::kStep,
                                               .5f, 46.f};
    raw_float_track.keyframes.push_back(first_key);
    const RawFloatTrack::Keyframe second_key = {RawTrackInterpolation::kStep,
                                                .7f, 0.f};
    raw_float_track.keyframes.push_back(second_key);
    const RawFloatTrack::Keyframe third_key = {RawTrackInterpolation::kStep,
                                               1.f, 99.f};
    raw_float_track.keyframes.push_back(third_key);

    // Builds track
    ozz::unique_ptr<FloatTrack> track(builder(raw_float_track));
    EXPECT_TRUE(track);

    // Samples to verify build output.
    FloatTrackSamplingJob sampling;
    sampling.track = track.get();
    sampling.result = &result;

    sampling.ratio = 0.f;
    ASSERT_TRUE(sampling.Run());
    EXPECT_FLOAT_EQ(result, 46.f);

    sampling.ratio = .5f;
    ASSERT_TRUE(sampling.Run());
    EXPECT_FLOAT_EQ(result, 46.f);

    sampling.ratio = .5f + 1e-7f;
    ASSERT_TRUE(sampling.Run());
    EXPECT_FLOAT_EQ(result, 46.f);

    sampling.ratio = .6f;
    ASSERT_TRUE(sampling.Run());
    EXPECT_FLOAT_EQ(result, 46.f);

    sampling.ratio = .7f - 1e-7f;
    ASSERT_TRUE(sampling.Run());
    EXPECT_FLOAT_EQ(result, 46.f);

    sampling.ratio = .7f;
    ASSERT_TRUE(sampling.Run());
    EXPECT_FLOAT_EQ(result, 0.f);

    sampling.ratio = .7f + 1e-7f;
    ASSERT_TRUE(sampling.Run());
    EXPECT_FLOAT_EQ(result, 0.f);

    sampling.ratio = 1.f - 1e-7f;
    ASSERT_TRUE(sampling.Run());
    EXPECT_FLOAT_EQ(result, 0.f);

    sampling.ratio = 1.f;
    ASSERT_TRUE(sampling.Run());
    EXPECT_FLOAT_EQ(result, 99.f);
  }
}

TEST(BuildMixed, TrackBuilder) {
  // Instantiates a builder objects with default parameters.
  TrackBuilder builder;
  float result;

  RawFloatTrack raw_float_track;

  const RawFloatTrack::Keyframe key0 = {RawTrackInterpolation::kLinear, 0.f,
                                        0.f};
  raw_float_track.keyframes.push_back(key0);
  const RawFloatTrack::Keyframe key1 = {RawTrackInterpolation::kStep, .5f,
                                        46.f};
  raw_float_track.keyframes.push_back(key1);
  const RawFloatTrack::Keyframe key2 = {RawTrackInterpolation::kLinear, .7f,
                                        0.f};
  raw_float_track.keyframes.push_back(key2);

  // Builds track
  ozz::unique_ptr<FloatTrack> track(builder(raw_float_track));
  EXPECT_TRUE(track);

  // Samples to verify build output.
  FloatTrackSamplingJob sampling;
  sampling.track = track.get();
  sampling.result = &result;

  sampling.ratio = 0.f;
  ASSERT_TRUE(sampling.Run());
  EXPECT_FLOAT_EQ(result, 0.f);

  sampling.ratio = .25f;
  ASSERT_TRUE(sampling.Run());
  EXPECT_FLOAT_EQ(result, 23.f);

  sampling.ratio = .5f;
  ASSERT_TRUE(sampling.Run());
  EXPECT_FLOAT_EQ(result, 46.f);

  sampling.ratio = .5f + 1e-7f;
  ASSERT_TRUE(sampling.Run());
  EXPECT_FLOAT_EQ(result, 46.f);

  sampling.ratio = .6f;
  ASSERT_TRUE(sampling.Run());
  EXPECT_FLOAT_EQ(result, 46.f);

  sampling.ratio = .7f - 1e-7f;
  ASSERT_TRUE(sampling.Run());
  EXPECT_FLOAT_EQ(result, 46.f);

  sampling.ratio = .7f;
  ASSERT_TRUE(sampling.Run());
  EXPECT_FLOAT_EQ(result, 0.f);

  sampling.ratio = 1.f;
  ASSERT_TRUE(sampling.Run());
  EXPECT_FLOAT_EQ(result, 0.f);
}

TEST(Float, TrackBuilder) {
  TrackBuilder builder;
  ozz::animation::offline::RawFloatTrack raw_track;

  {
    // Default value for quaternion is identity.
    ozz::unique_ptr<FloatTrack> track(builder(raw_track));
    EXPECT_TRUE(track);

    // Samples to verify build output.
    float result;
    ozz::animation::FloatTrackSamplingJob sampling;
    sampling.track = track.get();
    sampling.result = &result;

    sampling.ratio = .5f;
    ASSERT_TRUE(sampling.Run());
    EXPECT_FLOAT_EQ(result, 0.f);
  }

  {
    const ozz::animation::offline::RawFloatTrack::Keyframe first_key = {
        RawTrackInterpolation::kLinear, .5f, 23.f};
    raw_track.keyframes.push_back(first_key);
    const ozz::animation::offline::RawFloatTrack::Keyframe second_key = {
        RawTrackInterpolation::kLinear, .7f, 46.f};
    raw_track.keyframes.push_back(second_key);

    // Builds track
    ozz::unique_ptr<FloatTrack> track(builder(raw_track));
    ASSERT_TRUE(track);

    // Samples to verify build output.
    float result;
    ozz::animation::FloatTrackSamplingJob sampling;
    sampling.track = track.get();
    sampling.result = &result;

    sampling.ratio = 0.f;
    ASSERT_TRUE(sampling.Run());
    EXPECT_FLOAT_EQ(result, 23.f);

    sampling.ratio = .5f;
    ASSERT_TRUE(sampling.Run());
    EXPECT_FLOAT_EQ(result, 23.f);

    sampling.ratio = .6f;
    ASSERT_TRUE(sampling.Run());
    EXPECT_FLOAT_EQ(result, 34.5f);

    sampling.ratio = .7f;
    ASSERT_TRUE(sampling.Run());
    EXPECT_FLOAT_EQ(result, 46.f);

    sampling.ratio = 1.f;
    ASSERT_TRUE(sampling.Run());
    EXPECT_FLOAT_EQ(result, 46.f);
  }
}

TEST(Float2, TrackBuilder) {
  TrackBuilder builder;
  ozz::animation::offline::RawFloat2Track raw_track;

  {
    // Default value for quaternion is identity.
    ozz::unique_ptr<Float2Track> track(builder(raw_track));
    EXPECT_TRUE(track);

    // Samples to verify build output.
    ozz::math::Float2 result;
    ozz::animation::Float2TrackSamplingJob sampling;
    sampling.track = track.get();
    sampling.result = &result;

    sampling.ratio = .5f;
    ASSERT_TRUE(sampling.Run());
    EXPECT_FLOAT2_EQ(result, 0.f, 0.f);
  }

  {
    const ozz::animation::offline::RawFloat2Track::Keyframe first_key = {
        RawTrackInterpolation::kLinear, .5f, ozz::math::Float2(0.f, 23.f)};
    raw_track.keyframes.push_back(first_key);
    const ozz::animation::offline::RawFloat2Track::Keyframe second_key = {
        RawTrackInterpolation::kLinear, .7f, ozz::math::Float2(23.f, 46.f)};
    raw_track.keyframes.push_back(second_key);

    // Builds track
    ozz::unique_ptr<Float2Track> track(builder(raw_track));
    ASSERT_TRUE(track);

    // Samples to verify build output.
    ozz::math::Float2 result;
    ozz::animation::Float2TrackSamplingJob sampling;
    sampling.track = track.get();
    sampling.result = &result;

    sampling.ratio = 0.f;
    ASSERT_TRUE(sampling.Run());
    EXPECT_FLOAT2_EQ(result, 0.f, 23.f);

    sampling.ratio = .5f;
    ASSERT_TRUE(sampling.Run());
    EXPECT_FLOAT2_EQ(result, 0.f, 23.f);

    sampling.ratio = .6f;
    ASSERT_TRUE(sampling.Run());
    EXPECT_FLOAT2_EQ(result, 11.5f, 34.5f);

    sampling.ratio = .7f;
    ASSERT_TRUE(sampling.Run());
    EXPECT_FLOAT2_EQ(result, 23.f, 46.f);

    sampling.ratio = 1.f;
    ASSERT_TRUE(sampling.Run());
    EXPECT_FLOAT2_EQ(result, 23.f, 46.f);
  }
}

TEST(Float3, TrackBuilder) {
  TrackBuilder builder;
  ozz::animation::offline::RawFloat3Track raw_track;

  {
    // Default value for quaternion is identity.
    ozz::unique_ptr<Float3Track> track(builder(raw_track));
    EXPECT_TRUE(track);

    // Samples to verify build output.
    ozz::math::Float3 result;
    ozz::animation::Float3TrackSamplingJob sampling;
    sampling.track = track.get();
    sampling.result = &result;

    sampling.ratio = .5f;
    ASSERT_TRUE(sampling.Run());
    EXPECT_FLOAT3_EQ(result, 0.f, 0.f, 0.f);
  }

  {
    const ozz::animation::offline::RawFloat3Track::Keyframe first_key = {
        RawTrackInterpolation::kLinear, .5f,
        ozz::math::Float3(0.f, 23.f, 46.f)};
    raw_track.keyframes.push_back(first_key);
    const ozz::animation::offline::RawFloat3Track::Keyframe second_key = {
        RawTrackInterpolation::kLinear, .7f,
        ozz::math::Float3(23.f, 46.f, 92.f)};
    raw_track.keyframes.push_back(second_key);

    // Builds track
    ozz::unique_ptr<Float3Track> track(builder(raw_track));
    EXPECT_TRUE(track);

    // Samples to verify build output.
    ozz::math::Float3 result;
    ozz::animation::Float3TrackSamplingJob sampling;
    sampling.track = track.get();
    sampling.result = &result;

    sampling.ratio = 0.f;
    ASSERT_TRUE(sampling.Run());
    EXPECT_FLOAT3_EQ(result, 0.f, 23.f, 46.f);

    sampling.ratio = .5f;
    ASSERT_TRUE(sampling.Run());
    EXPECT_FLOAT3_EQ(result, 0.f, 23.f, 46.f);

    sampling.ratio = .6f;
    ASSERT_TRUE(sampling.Run());
    EXPECT_FLOAT3_EQ(result, 11.5f, 34.5f, 69.f);

    sampling.ratio = .7f;
    ASSERT_TRUE(sampling.Run());
    EXPECT_FLOAT3_EQ(result, 23.f, 46.f, 92.f);

    sampling.ratio = 1.f;
    ASSERT_TRUE(sampling.Run());
    EXPECT_FLOAT3_EQ(result, 23.f, 46.f, 92.f);
  }
}

TEST(Float4, TrackBuilder) {
  TrackBuilder builder;
  ozz::animation::offline::RawFloat4Track raw_track;

  {
    // Default value for quaternion is identity.
    ozz::unique_ptr<Float4Track> track(builder(raw_track));
    EXPECT_TRUE(track);

    // Samples to verify build output.
    ozz::math::Float4 result;
    ozz::animation::Float4TrackSamplingJob sampling;
    sampling.track = track.get();
    sampling.result = &result;

    sampling.ratio = .5f;
    ASSERT_TRUE(sampling.Run());
    EXPECT_FLOAT4_EQ(result, 0.f, 0.f, 0.f, 0.f);
  }

  {
    const ozz::animation::offline::RawFloat4Track::Keyframe first_key = {
        RawTrackInterpolation::kLinear, .5f,
        ozz::math::Float4(0.f, 23.f, 46.f, 5.f)};
    raw_track.keyframes.push_back(first_key);
    const ozz::animation::offline::RawFloat4Track::Keyframe second_key = {
        RawTrackInterpolation::kLinear, .7f,
        ozz::math::Float4(23.f, 46.f, 92.f, 25.f)};
    raw_track.keyframes.push_back(second_key);

    // Builds track
    ozz::unique_ptr<Float4Track> track(builder(raw_track));
    EXPECT_TRUE(track);

    // Samples to verify build output.
    ozz::math::Float4 result;
    ozz::animation::Float4TrackSamplingJob sampling;
    sampling.track = track.get();
    sampling.result = &result;

    sampling.ratio = 0.f;
    ASSERT_TRUE(sampling.Run());
    EXPECT_FLOAT4_EQ(result, 0.f, 23.f, 46.f, 5.f);

    sampling.ratio = .5f;
    ASSERT_TRUE(sampling.Run());
    EXPECT_FLOAT4_EQ(result, 0.f, 23.f, 46.f, 5.f);

    sampling.ratio = .6f;
    ASSERT_TRUE(sampling.Run());
    EXPECT_FLOAT4_EQ(result, 11.5f, 34.5f, 69.f, 15.f);

    sampling.ratio = .7f;
    ASSERT_TRUE(sampling.Run());
    EXPECT_FLOAT4_EQ(result, 23.f, 46.f, 92.f, 25.f);

    sampling.ratio = 1.f;
    ASSERT_TRUE(sampling.Run());
    EXPECT_FLOAT4_EQ(result, 23.f, 46.f, 92.f, 25.f);
  }
}

TEST(Quaternion, TrackBuilder) {
  TrackBuilder builder;
  ozz::animation::offline::RawQuaternionTrack raw_track;

  {
    // Default value for quaternion is identity.
    ozz::unique_ptr<QuaternionTrack> track(builder(raw_track));
    EXPECT_TRUE(track);

    // Samples to verify build output.
    ozz::math::Quaternion result;
    ozz::animation::QuaternionTrackSamplingJob sampling;
    sampling.track = track.get();
    sampling.result = &result;

    sampling.ratio = .5f;
    ASSERT_TRUE(sampling.Run());
    EXPECT_QUATERNION_EQ(result, 0.f, 0.f, 0.f, 1.f);
  }

  {
    const ozz::animation::offline::RawQuaternionTrack::Keyframe key0 = {
        RawTrackInterpolation::kLinear, .5f,
        ozz::math::Quaternion(
            -.70710677f, -0.f, -0.f,
            -.70710677f)};  // Will be opposed to be on the other hemisphere
    raw_track.keyframes.push_back(key0);
    const ozz::animation::offline::RawQuaternionTrack::Keyframe key1 = {
        RawTrackInterpolation::kLinear, .7f,
        ozz::math::Quaternion(0.f, .70710677f, 0.f, .70710677f)};
    raw_track.keyframes.push_back(key1);
    const ozz::animation::offline::RawQuaternionTrack::Keyframe key2 = {
        RawTrackInterpolation::kLinear, .8f,
        ozz::math::Quaternion(-0.f, -.70710677f, -0.f, -.70710677f)};
    raw_track.keyframes.push_back(key2);

    // Builds track
    ozz::unique_ptr<QuaternionTrack> track(builder(raw_track));
    EXPECT_TRUE(track);

    // Samples to verify build output.
    ozz::math::Quaternion result;
    ozz::animation::QuaternionTrackSamplingJob sampling;
    sampling.track = track.get();
    sampling.result = &result;

    sampling.ratio = 0.f;
    ASSERT_TRUE(sampling.Run());
    EXPECT_QUATERNION_EQ(result, .70710677f, 0.f, 0.f, .70710677f);

    sampling.ratio = .5f;
    ASSERT_TRUE(sampling.Run());
    EXPECT_QUATERNION_EQ(result, .70710677f, 0.f, 0.f, .70710677f);

    sampling.ratio = .54f;
    ASSERT_TRUE(sampling.Run());
    EXPECT_QUATERNION_EQ(result, .61721331f, .15430345f, 0.f, .77151674f);

    sampling.ratio = .7f;
    ASSERT_TRUE(sampling.Run());
    EXPECT_QUATERNION_EQ(result, 0.f, .70710677f, 0.f, .70710677f);

    sampling.ratio = .75f;
    ASSERT_TRUE(sampling.Run());
    EXPECT_QUATERNION_EQ(result, 0.f, .70710677f, 0.f, .70710677f);

    sampling.ratio = .8f;
    ASSERT_TRUE(sampling.Run());
    EXPECT_QUATERNION_EQ(result, 0.f, .70710677f, 0.f, .70710677f);

    sampling.ratio = 1.f;
    ASSERT_TRUE(sampling.Run());
    EXPECT_QUATERNION_EQ(result, 0.f, .70710677f, 0.f, .70710677f);
  }
}
