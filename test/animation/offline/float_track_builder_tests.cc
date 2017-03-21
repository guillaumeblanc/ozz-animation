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

#include "ozz/animation/offline/float_track_builder.h"

#include "gtest/gtest.h"
#include "ozz/base/maths/gtest_math_helper.h"

#include "ozz/animation/offline/raw_float_track.h"
#include "ozz/animation/runtime/float_track.h"
#include "ozz/animation/runtime/float_track_sampling_job.h"

#include <limits>

using ozz::animation::FloatTrack;
using ozz::animation::FloatTrackSamplingJob;
using ozz::animation::offline::RawFloatTrack;
using ozz::animation::offline::FloatTrackBuilder;
using ozz::animation::offline::RawTrackInterpolation;

TEST(Default, FloatTrackBuilder) {
  // Instantiates a builder objects with default parameters.
  FloatTrackBuilder builder;

  {  // Building default RawFloatTrack succeeds.
    RawFloatTrack raw_float_track;
    EXPECT_TRUE(raw_float_track.Validate());

    // Builds track
    FloatTrack* track = builder(raw_float_track);
    EXPECT_TRUE(track != NULL);
    ozz::memory::default_allocator()->Delete(track);
  }
}

TEST(Build, FloatTrackBuilder) {
  // Instantiates a builder objects with default parameters.
  FloatTrackBuilder builder;

  {  // Building a track with unsorted keys fails.
    RawFloatTrack raw_float_track;
    raw_float_track.keyframes.resize(2);

    // Adds 2 unordered keys
    RawFloatTrack::Keyframe first_key = {RawTrackInterpolation::kLinear, .8f,
                                         0.f};
    raw_float_track.keyframes.push_back(first_key);
    RawFloatTrack::Keyframe second_key = {RawTrackInterpolation::kLinear, .2f,
                                          0.f};
    raw_float_track.keyframes.push_back(second_key);

    // Builds track
    EXPECT_FALSE(raw_float_track.Validate());
    EXPECT_TRUE(!builder(raw_float_track));
  }

  {  // Building a track with too close keys fails.
    RawFloatTrack raw_float_track;
    raw_float_track.keyframes.resize(2);

    // Adds 2 unordered keys
    RawFloatTrack::Keyframe first_key = {RawTrackInterpolation::kLinear, .1f,
                                         0.f};
    raw_float_track.keyframes.push_back(first_key);
    RawFloatTrack::Keyframe second_key = {
        RawTrackInterpolation::kLinear,
        .1f + std::numeric_limits<float>::epsilon(), 0.f};
    raw_float_track.keyframes.push_back(second_key);

    // Builds track
    EXPECT_FALSE(raw_float_track.Validate());
    EXPECT_TRUE(!builder(raw_float_track));
  }

  {  // Building a track with invalid key frame's time fails.
    RawFloatTrack raw_float_track;
    raw_float_track.keyframes.resize(1);

    // Adds 2 unordered keys
    RawFloatTrack::Keyframe first_key = {RawTrackInterpolation::kLinear, 1.8f,
                                         0.f};
    raw_float_track.keyframes.push_back(first_key);

    // Builds track
    EXPECT_FALSE(raw_float_track.Validate());
    EXPECT_TRUE(!builder(raw_float_track));
  }

  {  // Building a track with equal key frame's time fails.
    RawFloatTrack raw_float_track;
    raw_float_track.keyframes.resize(2);

    // Adds 2 unordered keys
    RawFloatTrack::Keyframe first_key = {RawTrackInterpolation::kLinear, .8f,
                                         0.f};
    raw_float_track.keyframes.push_back(first_key);
    RawFloatTrack::Keyframe second_key = {RawTrackInterpolation::kLinear, .8f,
                                          1.f};
    raw_float_track.keyframes.push_back(second_key);

    // Builds track
    EXPECT_FALSE(raw_float_track.Validate());
    EXPECT_TRUE(!builder(raw_float_track));
  }

  {  // Building a valid track with 1 key succeeds.
    RawFloatTrack raw_float_track;
    RawFloatTrack::Keyframe first_key = {RawTrackInterpolation::kLinear, .8f,
                                         0.f};
    raw_float_track.keyframes.push_back(first_key);
    EXPECT_TRUE(raw_float_track.Validate());

    // Builds track
    FloatTrack* track = builder(raw_float_track);
    EXPECT_TRUE(track != NULL);
    ozz::memory::default_allocator()->Delete(track);
  }
}

TEST(Name, FloatTrackBuilder) { /*
   // Instantiates a builder objects with default parameters.
   AnimationBuilder builder;

   {  // Building an unnamed animation.
     RawAnimation raw_animation;
     raw_animation.duration = 1.f;
     raw_animation.tracks.resize(46);

     // Builds animation
     Animation* anim = builder(raw_animation);
     EXPECT_TRUE(anim != NULL);

     // Should
     EXPECT_STREQ(anim->name(), "");
     ozz::memory::default_allocator()->Delete(anim);
   }

   {  // Building an unnamed animation.
     RawAnimation raw_animation;
     raw_animation.duration = 1.f;
     raw_animation.tracks.resize(46);
     raw_animation.name = "46";

     // Builds animation
     Animation* anim = builder(raw_animation);
     EXPECT_TRUE(anim != NULL);

     // Should
     EXPECT_STREQ(anim->name(), "46");
     ozz::memory::default_allocator()->Delete(anim);
   }*/
}

TEST(Build0Keys, FloatTrackBuilder) {
  // Instantiates a builder objects with default parameters.
  FloatTrackBuilder builder;

  RawFloatTrack raw_float_track;

  // Builds track
  FloatTrack* track = builder(raw_float_track);
  ASSERT_TRUE(track != NULL);

  // Samples to verify build output.
  FloatTrackSamplingJob sampling;
  sampling.track = track;
  float result;
  sampling.result = &result;
  sampling.time = 0.f;
  ASSERT_TRUE(sampling.Run());
  EXPECT_FLOAT_EQ(result, 0.f);

  ozz::memory::default_allocator()->Delete(track);
}

TEST(BuildLinear, FloatTrackBuilder) {
  // Instantiates a builder objects with default parameters.
  FloatTrackBuilder builder;
  float result;

  {  // 1 key at the beginning
    RawFloatTrack raw_float_track;

    RawFloatTrack::Keyframe first_key = {RawTrackInterpolation::kLinear, 0.f,
                                         46.f};
    raw_float_track.keyframes.push_back(first_key);

    // Builds track
    FloatTrack* track = builder(raw_float_track);
    ASSERT_TRUE(track != NULL);

    // Samples to verify build output.
    FloatTrackSamplingJob sampling;
    sampling.track = track;
    sampling.result = &result;

    sampling.time = 0.f;
    ASSERT_TRUE(sampling.Run());
    EXPECT_FLOAT_EQ(result, 46.f);

    sampling.time = .5f;
    ASSERT_TRUE(sampling.Run());
    EXPECT_FLOAT_EQ(result, 46.f);

    sampling.time = 1.f;
    ASSERT_TRUE(sampling.Run());
    EXPECT_FLOAT_EQ(result, 46.f);

    ozz::memory::default_allocator()->Delete(track);
  }

  {  // 1 key in the middle
    RawFloatTrack raw_float_track;

    RawFloatTrack::Keyframe first_key = {RawTrackInterpolation::kLinear, .5f,
                                         46.f};
    raw_float_track.keyframes.push_back(first_key);

    // Builds track
    FloatTrack* track = builder(raw_float_track);
    ASSERT_TRUE(track != NULL);

    // Samples to verify build output.
    FloatTrackSamplingJob sampling;
    sampling.track = track;
    sampling.result = &result;

    sampling.time = 0.f;
    ASSERT_TRUE(sampling.Run());
    EXPECT_FLOAT_EQ(result, 46.f);

    sampling.time = .5f;
    ASSERT_TRUE(sampling.Run());
    EXPECT_FLOAT_EQ(result, 46.f);

    sampling.time = 1.f;
    ASSERT_TRUE(sampling.Run());
    EXPECT_FLOAT_EQ(result, 46.f);

    ozz::memory::default_allocator()->Delete(track);
  }

  {  // 1 key at the end
    RawFloatTrack raw_float_track;

    RawFloatTrack::Keyframe first_key = {RawTrackInterpolation::kLinear, 1.f,
                                         46.f};
    raw_float_track.keyframes.push_back(first_key);

    // Builds track
    FloatTrack* track = builder(raw_float_track);
    ASSERT_TRUE(track != NULL);

    // Samples to verify build output.
    FloatTrackSamplingJob sampling;
    sampling.track = track;
    sampling.result = &result;

    sampling.time = 0.f;
    ASSERT_TRUE(sampling.Run());
    EXPECT_FLOAT_EQ(result, 46.f);

    sampling.time = .5f;
    ASSERT_TRUE(sampling.Run());
    EXPECT_FLOAT_EQ(result, 46.f);

    sampling.time = 1.f;
    ASSERT_TRUE(sampling.Run());
    EXPECT_FLOAT_EQ(result, 46.f);

    ozz::memory::default_allocator()->Delete(track);
  }

  {  // 2 keys at the end
    RawFloatTrack raw_float_track;

    RawFloatTrack::Keyframe first_key = {RawTrackInterpolation::kLinear, .5f,
                                         46.f};
    raw_float_track.keyframes.push_back(first_key);
    RawFloatTrack::Keyframe second_key = {RawTrackInterpolation::kLinear, .7f,
                                          0.f};
    raw_float_track.keyframes.push_back(second_key);

    // Builds track
    FloatTrack* track = builder(raw_float_track);
    ASSERT_TRUE(track != NULL);

    // Samples to verify build output.
    FloatTrackSamplingJob sampling;
    sampling.track = track;
    sampling.result = &result;

    sampling.time = 0.f;
    ASSERT_TRUE(sampling.Run());
    EXPECT_FLOAT_EQ(result, 46.f);

    sampling.time = .5f;
    ASSERT_TRUE(sampling.Run());
    EXPECT_FLOAT_EQ(result, 46.f);

    sampling.time = .6f;
    ASSERT_TRUE(sampling.Run());
    EXPECT_FLOAT_EQ(result, 23.f);

    sampling.time = .7f;
    ASSERT_TRUE(sampling.Run());
    EXPECT_FLOAT_EQ(result, 0.f);

    sampling.time = 1.f;
    ASSERT_TRUE(sampling.Run());
    EXPECT_FLOAT_EQ(result, 0.f);

    ozz::memory::default_allocator()->Delete(track);
  }
}

TEST(BuildStep, FloatTrackBuilder) {
  // Instantiates a builder objects with default parameters.
  FloatTrackBuilder builder;
  float result;

  {  // 1 key at the beginning
    RawFloatTrack raw_float_track;

    RawFloatTrack::Keyframe first_key = {RawTrackInterpolation::kStep, 0.f,
                                         46.f};
    raw_float_track.keyframes.push_back(first_key);

    // Builds track
    FloatTrack* track = builder(raw_float_track);
    ASSERT_TRUE(track != NULL);

    // Samples to verify build output.
    FloatTrackSamplingJob sampling;
    sampling.track = track;
    sampling.result = &result;

    sampling.time = 0.f;
    ASSERT_TRUE(sampling.Run());
    EXPECT_FLOAT_EQ(result, 46.f);

    sampling.time = .5f;
    ASSERT_TRUE(sampling.Run());
    EXPECT_FLOAT_EQ(result, 46.f);

    sampling.time = 1.f;
    ASSERT_TRUE(sampling.Run());
    EXPECT_FLOAT_EQ(result, 46.f);

    ozz::memory::default_allocator()->Delete(track);
  }

  {  // 1 key in the middle
    RawFloatTrack raw_float_track;

    RawFloatTrack::Keyframe first_key = {RawTrackInterpolation::kStep, .5f,
                                         46.f};
    raw_float_track.keyframes.push_back(first_key);

    // Builds track
    FloatTrack* track = builder(raw_float_track);
    ASSERT_TRUE(track != NULL);

    // Samples to verify build output.
    FloatTrackSamplingJob sampling;
    sampling.track = track;
    sampling.result = &result;

    sampling.time = 0.f;
    ASSERT_TRUE(sampling.Run());
    EXPECT_FLOAT_EQ(result, 46.f);

    sampling.time = .5f;
    ASSERT_TRUE(sampling.Run());
    EXPECT_FLOAT_EQ(result, 46.f);

    sampling.time = 1.f;
    ASSERT_TRUE(sampling.Run());
    EXPECT_FLOAT_EQ(result, 46.f);

    ozz::memory::default_allocator()->Delete(track);
  }

  {  // 1 key at the end
    RawFloatTrack raw_float_track;

    RawFloatTrack::Keyframe first_key = {RawTrackInterpolation::kStep, 1.f,
                                         46.f};
    raw_float_track.keyframes.push_back(first_key);

    // Builds track
    FloatTrack* track = builder(raw_float_track);
    ASSERT_TRUE(track != NULL);

    // Samples to verify build output.
    FloatTrackSamplingJob sampling;
    sampling.track = track;
    sampling.result = &result;

    sampling.time = 0.f;
    ASSERT_TRUE(sampling.Run());
    EXPECT_FLOAT_EQ(result, 46.f);

    sampling.time = .5f;
    ASSERT_TRUE(sampling.Run());
    EXPECT_FLOAT_EQ(result, 46.f);

    sampling.time = 1.f;
    ASSERT_TRUE(sampling.Run());
    EXPECT_FLOAT_EQ(result, 46.f);

    ozz::memory::default_allocator()->Delete(track);
  }

  {  // 1 key at the beginning
    RawFloatTrack raw_float_track;

    RawFloatTrack::Keyframe first_key = {RawTrackInterpolation::kStep, 0.f,
                                         46.f};
    raw_float_track.keyframes.push_back(first_key);

    // Builds track
    FloatTrack* track = builder(raw_float_track);
    ASSERT_TRUE(track != NULL);

    // Samples to verify build output.
    FloatTrackSamplingJob sampling;
    sampling.track = track;
    sampling.result = &result;

    sampling.time = 0.f;
    ASSERT_TRUE(sampling.Run());
    EXPECT_FLOAT_EQ(result, 46.f);

    sampling.time = .5f;
    ASSERT_TRUE(sampling.Run());
    EXPECT_FLOAT_EQ(result, 46.f);

    sampling.time = 1.f;
    ASSERT_TRUE(sampling.Run());
    EXPECT_FLOAT_EQ(result, 46.f);

    ozz::memory::default_allocator()->Delete(track);
  }

  {  // 2 keys
    RawFloatTrack raw_float_track;

    RawFloatTrack::Keyframe first_key = {RawTrackInterpolation::kStep, 0.f,
                                         46.f};
    raw_float_track.keyframes.push_back(first_key);
    RawFloatTrack::Keyframe second_key = {RawTrackInterpolation::kStep, .7f,
                                          0.f};
    raw_float_track.keyframes.push_back(second_key);

    // Builds track
    FloatTrack* track = builder(raw_float_track);
    ASSERT_TRUE(track != NULL);

    // Samples to verify build output.
    FloatTrackSamplingJob sampling;
    sampling.track = track;
    sampling.result = &result;

    sampling.time = 0.f;
    ASSERT_TRUE(sampling.Run());
    EXPECT_FLOAT_EQ(result, 46.f);

    sampling.time = .5f;
    ASSERT_TRUE(sampling.Run());
    EXPECT_FLOAT_EQ(result, 46.f);

    sampling.time = .5f + 1e-7f;
    ASSERT_TRUE(sampling.Run());
    EXPECT_FLOAT_EQ(result, 46.f);

    sampling.time = .6f;
    ASSERT_TRUE(sampling.Run());
    EXPECT_FLOAT_EQ(result, 46.f);

    sampling.time = .7f - 1e-7f;
    ASSERT_TRUE(sampling.Run());
    EXPECT_FLOAT_EQ(result, 46.f);

    sampling.time = .7f;
    ASSERT_TRUE(sampling.Run());
    EXPECT_FLOAT_EQ(result, 0.f);

    sampling.time = 1.f;
    ASSERT_TRUE(sampling.Run());
    EXPECT_FLOAT_EQ(result, 0.f);

    ozz::memory::default_allocator()->Delete(track);
  }

  {  // 3 keys
    RawFloatTrack raw_float_track;

    RawFloatTrack::Keyframe first_key = {RawTrackInterpolation::kStep, 0.f,
                                         46.f};
    raw_float_track.keyframes.push_back(first_key);
    RawFloatTrack::Keyframe second_key = {RawTrackInterpolation::kStep, .7f,
                                          0.f};
    raw_float_track.keyframes.push_back(second_key);
    RawFloatTrack::Keyframe third_key = {RawTrackInterpolation::kStep, 1.f,
                                         99.f};
    raw_float_track.keyframes.push_back(third_key);

    // Builds track
    FloatTrack* track = builder(raw_float_track);
    ASSERT_TRUE(track != NULL);

    // Samples to verify build output.
    FloatTrackSamplingJob sampling;
    sampling.track = track;
    sampling.result = &result;

    sampling.time = 0.f;
    ASSERT_TRUE(sampling.Run());
    EXPECT_FLOAT_EQ(result, 46.f);

    sampling.time = .5f;
    ASSERT_TRUE(sampling.Run());
    EXPECT_FLOAT_EQ(result, 46.f);

    sampling.time = .5f + 1e-7f;
    ASSERT_TRUE(sampling.Run());
    EXPECT_FLOAT_EQ(result, 46.f);

    sampling.time = .6f;
    ASSERT_TRUE(sampling.Run());
    EXPECT_FLOAT_EQ(result, 46.f);

    sampling.time = .7f - 1e-7f;
    ASSERT_TRUE(sampling.Run());
    EXPECT_FLOAT_EQ(result, 46.f);

    sampling.time = .7f;
    ASSERT_TRUE(sampling.Run());
    EXPECT_FLOAT_EQ(result, 0.f);

    sampling.time = .9f;
    ASSERT_TRUE(sampling.Run());
    EXPECT_FLOAT_EQ(result, 0.f);

    sampling.time = 1.f;
    ASSERT_TRUE(sampling.Run());
    EXPECT_FLOAT_EQ(result, 99.f);

    ozz::memory::default_allocator()->Delete(track);
  }

  {  // 2 close keys
    RawFloatTrack raw_float_track;

    RawFloatTrack::Keyframe first_key = {RawTrackInterpolation::kStep, .5f,
                                         46.f};
    raw_float_track.keyframes.push_back(first_key);
    RawFloatTrack::Keyframe second_key = {
        RawTrackInterpolation::kStep,
        .5f + 2.f * std::numeric_limits<float>::epsilon(), 0.f};
    raw_float_track.keyframes.push_back(second_key);

    // Builds track
    FloatTrack* track = builder(raw_float_track);
    ASSERT_TRUE(track != NULL);

    // Samples to verify build output.
    FloatTrackSamplingJob sampling;
    sampling.track = track;
    sampling.result = &result;

    sampling.time = 0.f;
    ASSERT_TRUE(sampling.Run());
    EXPECT_FLOAT_EQ(result, 46.f);

    sampling.time = .5f;
    ASSERT_TRUE(sampling.Run());
    EXPECT_FLOAT_EQ(result, 46.f);

    sampling.time = .5f + std::numeric_limits<float>::epsilon();
    ASSERT_TRUE(sampling.Run());
    EXPECT_FLOAT_EQ(result, 46.f);

    sampling.time = .5f + 2.f * std::numeric_limits<float>::epsilon();
    ASSERT_TRUE(sampling.Run());
    EXPECT_FLOAT_EQ(result, 0.f);

    sampling.time = .7f;
    ASSERT_TRUE(sampling.Run());
    EXPECT_FLOAT_EQ(result, 0.f);

    ozz::memory::default_allocator()->Delete(track);
  }

  {  // 3 keys
    RawFloatTrack raw_float_track;

    RawFloatTrack::Keyframe first_key = {RawTrackInterpolation::kStep, .5f,
                                         46.f};
    raw_float_track.keyframes.push_back(first_key);
    RawFloatTrack::Keyframe second_key = {RawTrackInterpolation::kStep, .7f,
                                          0.f};
    raw_float_track.keyframes.push_back(second_key);
    RawFloatTrack::Keyframe third_key = {RawTrackInterpolation::kStep, 1.f,
                                         99.f};
    raw_float_track.keyframes.push_back(third_key);

    // Builds track
    FloatTrack* track = builder(raw_float_track);
    ASSERT_TRUE(track != NULL);

    // Samples to verify build output.
    FloatTrackSamplingJob sampling;
    sampling.track = track;
    sampling.result = &result;

    sampling.time = 0.f;
    ASSERT_TRUE(sampling.Run());
    EXPECT_FLOAT_EQ(result, 46.f);

    sampling.time = .5f;
    ASSERT_TRUE(sampling.Run());
    EXPECT_FLOAT_EQ(result, 46.f);

    sampling.time = .5f + 1e-7f;
    ASSERT_TRUE(sampling.Run());
    EXPECT_FLOAT_EQ(result, 46.f);

    sampling.time = .6f;
    ASSERT_TRUE(sampling.Run());
    EXPECT_FLOAT_EQ(result, 46.f);

    sampling.time = .7f - 1e-7f;
    ASSERT_TRUE(sampling.Run());
    EXPECT_FLOAT_EQ(result, 46.f);

    sampling.time = .7f;
    ASSERT_TRUE(sampling.Run());
    EXPECT_FLOAT_EQ(result, 0.f);

    sampling.time = .7f + 1e-7f;
    ASSERT_TRUE(sampling.Run());
    EXPECT_FLOAT_EQ(result, 0.f);

    sampling.time = 1.f - 1e-7f;
    ASSERT_TRUE(sampling.Run());
    EXPECT_FLOAT_EQ(result, 0.f);

    sampling.time = 1.f;
    ASSERT_TRUE(sampling.Run());
    EXPECT_FLOAT_EQ(result, 99.f);

    ozz::memory::default_allocator()->Delete(track);
  }
}

TEST(BuildMixed, FloatTrackBuilder) {
  // Instantiates a builder objects with default parameters.
  FloatTrackBuilder builder;
  float result;

  RawFloatTrack raw_float_track;

  RawFloatTrack::Keyframe key0 = {RawTrackInterpolation::kLinear, 0.f, 0.f};
  raw_float_track.keyframes.push_back(key0);
  RawFloatTrack::Keyframe key1 = {RawTrackInterpolation::kStep, .5f, 46.f};
  raw_float_track.keyframes.push_back(key1);
  RawFloatTrack::Keyframe key2 = {RawTrackInterpolation::kLinear, .7f, 0.f};
  raw_float_track.keyframes.push_back(key2);

  // Builds track
  FloatTrack* track = builder(raw_float_track);
  ASSERT_TRUE(track != NULL);

  // Samples to verify build output.
  FloatTrackSamplingJob sampling;
  sampling.track = track;
  sampling.result = &result;

  sampling.time = 0.f;
  ASSERT_TRUE(sampling.Run());
  EXPECT_FLOAT_EQ(result, 0.f);

  sampling.time = .25f;
  ASSERT_TRUE(sampling.Run());
  EXPECT_FLOAT_EQ(result, 23.f);

  sampling.time = .5f;
  ASSERT_TRUE(sampling.Run());
  EXPECT_FLOAT_EQ(result, 46.f);

  sampling.time = .5f + 1e-7f;
  ASSERT_TRUE(sampling.Run());
  EXPECT_FLOAT_EQ(result, 46.f);

  sampling.time = .6f;
  ASSERT_TRUE(sampling.Run());
  EXPECT_FLOAT_EQ(result, 46.f);

  sampling.time = .7f - 1e-7f;
  ASSERT_TRUE(sampling.Run());
  EXPECT_FLOAT_EQ(result, 46.f);

  sampling.time = .7f;
  ASSERT_TRUE(sampling.Run());
  EXPECT_FLOAT_EQ(result, 0.f);

  sampling.time = 1.f;
  ASSERT_TRUE(sampling.Run());
  EXPECT_FLOAT_EQ(result, 0.f);

  ozz::memory::default_allocator()->Delete(track);
}

TEST(Lerp, Float3TrackBuilder) {
  FloatTrackBuilder builder;
  ozz::animation::offline::RawFloat3Track raw_track;

  ozz::animation::offline::RawFloat3Track::Keyframe first_key = {
      RawTrackInterpolation::kLinear, .5f, ozz::math::Float3(0.f, 23.f, 46.f)};
  raw_track.keyframes.push_back(first_key);
  ozz::animation::offline::RawFloat3Track::Keyframe second_key = {
      RawTrackInterpolation::kLinear, .7f, ozz::math::Float3(23.f, 46.f, 92.f)};
  raw_track.keyframes.push_back(second_key);

  // Builds track
  ozz::animation::Float3Track* track = builder(raw_track);
  ASSERT_TRUE(track != NULL);

  // Samples to verify build output.
  ozz::math::Float3 result;
  ozz::animation::Float3TrackSamplingJob sampling;
  sampling.track = track;
  sampling.result = &result;

  sampling.time = 0.f;
  ASSERT_TRUE(sampling.Run());
  EXPECT_FLOAT3_EQ(result, 0.f, 23.f, 46.f);

  sampling.time = .5f;
  ASSERT_TRUE(sampling.Run());
  EXPECT_FLOAT3_EQ(result, 0.f, 23.f, 46.f);

  sampling.time = .6f;
  ASSERT_TRUE(sampling.Run());
  EXPECT_FLOAT3_EQ(result, 11.5f, 34.5f, 69.f);

  sampling.time = .7f;
  ASSERT_TRUE(sampling.Run());
  EXPECT_FLOAT3_EQ(result, 23.f, 46.f, 92.f);

  sampling.time = 1.f;
  ASSERT_TRUE(sampling.Run());
  EXPECT_FLOAT3_EQ(result, 23.f, 46.f, 92.f);

  ozz::memory::default_allocator()->Delete(track);
}
