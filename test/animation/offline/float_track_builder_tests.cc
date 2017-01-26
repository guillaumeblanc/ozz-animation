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

// TEMP !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
#include <algorithm>
#include "ozz/base/maths/math_ex.h"

namespace ozz {
namespace animation {
struct FloatSamplingJob {
  FloatSamplingJob() : time(0.f), track(NULL), result(NULL) {}

  bool Validate() const {
    bool success = true;
    success &= result != NULL;
    success &= time >= 0.f;
    success &= track != NULL && time <= track->duration();
    return success;
  }

  bool Run() const {
    if (!Validate()) {
      return false;
    }

    // Search keyframes to interpolate.
    const Range<const float> times = track->times();
    const Range<const float> values = track->values();
    assert(times.Size() == values.Size());

    // Search for the first key frame with a time value greater than input time.
    const float* ptk1 = std::upper_bound(times.begin, times.end, time);

    // upper_bound returns "end" if time == duration. Then patch the selected
    // keyframe to enter the lerp algorithm.
    if (ptk1 == times.end) {
      ptk1 = (times.end - 1);
    }

    // Lerp relevant keys.
    const float tk1 = *ptk1;
    const float tk0 = *(ptk1 - 1);
    assert(time >= tk0);
    const float* pvk1 = values.begin + (ptk1 - times.begin);
    const float* pvk0 = pvk1 - 1;
    const float alpha = (time - tk0) / (tk1 - tk0);
    *result = math::Lerp(*pvk0, *pvk1, alpha);

    return true;
  }

  // Time used to sample animation, clamped in range [0,duration] before
  // job execution. This resolves approximations issues on range bounds.
  float time;

  const FloatTrack* track;

  // Job output.
  float* result;
};
}
}

using ozz::animation::FloatTrack;
using ozz::animation::FloatSamplingJob;
using ozz::animation::offline::RawFloatTrack;
using ozz::animation::offline::FloatTrackBuilder;

TEST(Error, FloatTrackBuilder) {
  // Instantiates a builder objects with default parameters.
  FloatTrackBuilder builder;

  {  // Building an empty track fails because duration
    // must be >= 0.
    RawFloatTrack raw_float_track;
    raw_float_track.duration = -1.f;  // Negative duration.
    EXPECT_FALSE(raw_float_track.Validate());

    // Builds track
    EXPECT_TRUE(!builder(raw_float_track));
  }

  {  // Building an empty RawFloatTrack fails because track duration
    // must be >= 0.
    RawFloatTrack raw_float_track;
    raw_float_track.duration = 0.f;
    EXPECT_FALSE(raw_float_track.Validate());

    // Builds track
    EXPECT_TRUE(!builder(raw_float_track));
  }

  {  // Building default RawFloatTrack succeeds.
    RawFloatTrack raw_float_track;
    EXPECT_EQ(raw_float_track.duration, 1.f);
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
    raw_float_track.duration = 1.f;
    raw_float_track.keyframes.resize(2);

    // Adds 2 unordered keys
    RawFloatTrack::Keyframe first_key = {RawFloatTrack::kLinear,
                                         .8f, 0.f};
    raw_float_track.keyframes.push_back(first_key);
    RawFloatTrack::Keyframe second_key = {RawFloatTrack::kLinear,
                                          .2f, 0.f};
    raw_float_track.keyframes.push_back(second_key);

    // Builds track
    EXPECT_FALSE(raw_float_track.Validate());
    EXPECT_TRUE(!builder(raw_float_track));
  }

  {  // Building a track with invalid key frame's time fails.
    RawFloatTrack raw_float_track;
    raw_float_track.duration = 1.f;
    raw_float_track.keyframes.resize(1);

    // Adds 2 unordered keys
    RawFloatTrack::Keyframe first_key = {RawFloatTrack::kLinear,
                                         1.8f, 0.f};
    raw_float_track.keyframes.push_back(first_key);

    // Builds track
    EXPECT_FALSE(raw_float_track.Validate());
    EXPECT_TRUE(!builder(raw_float_track));
  }

  {  // Building a track with equal key frame's time fails.
    RawFloatTrack raw_float_track;
    raw_float_track.duration = 1.f;
    raw_float_track.keyframes.resize(2);

    // Adds 2 unordered keys
    RawFloatTrack::Keyframe first_key = {RawFloatTrack::kLinear,
                                         .8f, 0.f};
    raw_float_track.keyframes.push_back(first_key);
    RawFloatTrack::Keyframe second_key = {RawFloatTrack::kLinear,
                                          .8f, 1.f};
    raw_float_track.keyframes.push_back(second_key);

    // Builds track
    EXPECT_FALSE(raw_float_track.Validate());
    EXPECT_TRUE(!builder(raw_float_track));
  }

  {  // Building a valid track with 1 key succeeds.
    RawFloatTrack raw_float_track;
    EXPECT_EQ(raw_float_track.duration, 1.f);
    RawFloatTrack::Keyframe first_key = {RawFloatTrack::kLinear,
                                         .8f, 0.f};
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
  raw_float_track.duration = 1.f;

  // Builds track
  FloatTrack* track = builder(raw_float_track);
  ASSERT_TRUE(track != NULL);

  // Samples to verify build output.
  FloatSamplingJob sampling;
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
    raw_float_track.duration = 1.f;

    RawFloatTrack::Keyframe first_key = {RawFloatTrack::kLinear,
                                         0.f, 46.f};
    raw_float_track.keyframes.push_back(first_key);

    // Builds track
    FloatTrack* track = builder(raw_float_track);
    ASSERT_TRUE(track != NULL);

    // Samples to verify build output.
    FloatSamplingJob sampling;
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
    raw_float_track.duration = 1.f;

    RawFloatTrack::Keyframe first_key = {RawFloatTrack::kLinear,
                                         .5f, 46.f};
    raw_float_track.keyframes.push_back(first_key);

    // Builds track
    FloatTrack* track = builder(raw_float_track);
    ASSERT_TRUE(track != NULL);

    // Samples to verify build output.
    FloatSamplingJob sampling;
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
    raw_float_track.duration = 1.f;

    RawFloatTrack::Keyframe first_key = {RawFloatTrack::kLinear,
                                         1.f, 46.f};
    raw_float_track.keyframes.push_back(first_key);

    // Builds track
    FloatTrack* track = builder(raw_float_track);
    ASSERT_TRUE(track != NULL);

    // Samples to verify build output.
    FloatSamplingJob sampling;
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
    raw_float_track.duration = 1.f;

    RawFloatTrack::Keyframe first_key = {RawFloatTrack::kLinear,
                                         .5f, 46.f};
    raw_float_track.keyframes.push_back(first_key);
    RawFloatTrack::Keyframe second_key = {RawFloatTrack::kLinear,
                                         .7f, 0.f};
    raw_float_track.keyframes.push_back(second_key);

    // Builds track
    FloatTrack* track = builder(raw_float_track);
    ASSERT_TRUE(track != NULL);

    // Samples to verify build output.
    FloatSamplingJob sampling;
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
    raw_float_track.duration = 1.f;

    RawFloatTrack::Keyframe first_key = {RawFloatTrack::kStep,
                                         0.f, 46.f};
    raw_float_track.keyframes.push_back(first_key);

    // Builds track
    FloatTrack* track = builder(raw_float_track);
    ASSERT_TRUE(track != NULL);

    // Samples to verify build output.
    FloatSamplingJob sampling;
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
    raw_float_track.duration = 1.f;

    RawFloatTrack::Keyframe first_key = {RawFloatTrack::kStep,
                                         .5f, 46.f};
    raw_float_track.keyframes.push_back(first_key);

    // Builds track
    FloatTrack* track = builder(raw_float_track);
    ASSERT_TRUE(track != NULL);

    // Samples to verify build output.
    FloatSamplingJob sampling;
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
    raw_float_track.duration = 1.f;

    RawFloatTrack::Keyframe first_key = {RawFloatTrack::kStep,
                                         1.f, 46.f};
    raw_float_track.keyframes.push_back(first_key);

    // Builds track
    FloatTrack* track = builder(raw_float_track);
    ASSERT_TRUE(track != NULL);

    // Samples to verify build output.
    FloatSamplingJob sampling;
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
    raw_float_track.duration = 1.f;

    RawFloatTrack::Keyframe first_key = {RawFloatTrack::kStep,
                                         .5f, 46.f};
    raw_float_track.keyframes.push_back(first_key);
    RawFloatTrack::Keyframe second_key = {RawFloatTrack::kStep,
                                         .7f, 0.f};
    raw_float_track.keyframes.push_back(second_key);

    // Builds track
    FloatTrack* track = builder(raw_float_track);
    ASSERT_TRUE(track != NULL);

    // Samples to verify build output.
    FloatSamplingJob sampling;
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
}

TEST(BuildMixed, FloatTrackBuilder) {
  // Instantiates a builder objects with default parameters.
  FloatTrackBuilder builder;
  float result;

  {  // 2 keys
    RawFloatTrack raw_float_track;
    raw_float_track.duration = 1.f;

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
    FloatSamplingJob sampling;
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
}
