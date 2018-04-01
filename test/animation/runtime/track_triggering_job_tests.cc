//----------------------------------------------------------------------------//
//                                                                            //
// ozz-animation is hosted at http://github.com/guillaumeblanc/ozz-animation  //
// and distributed under the MIT License (MIT).                               //
//                                                                            //
// Copyright (c) 2017 Guillaume Blanc                                         //
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

#include "ozz/animation/runtime/track_triggering_job.h"

#include "gtest/gtest.h"

#include "ozz/base/maths/gtest_math_helper.h"
#include "ozz/base/memory/allocator.h"

#include "ozz/animation/offline/raw_track.h"
#include "ozz/animation/offline/track_builder.h"

#include "ozz/animation/runtime/track.h"

using ozz::animation::FloatTrack;
using ozz::animation::FloatTrackTriggeringJob;
using ozz::animation::offline::RawFloatTrack;
using ozz::animation::offline::RawTrackInterpolation;
using ozz::animation::offline::TrackBuilder;

TEST(JobValidity, FloatTrackTriggeringJob) {
  FloatTrackTriggeringJob::Edge edges_buffer[8];

  // Builds track
  ozz::animation::offline::RawFloatTrack raw_track;
  TrackBuilder builder;
  ozz::animation::FloatTrack* track = builder(raw_track);
  ASSERT_TRUE(track != NULL);

  {  // Default is invalid
    FloatTrackTriggeringJob job;
    EXPECT_FALSE(job.Validate());
    EXPECT_FALSE(job.Run());
  }

  {  // No track
    FloatTrackTriggeringJob job;
    FloatTrackTriggeringJob::Edges edges(edges_buffer);
    job.edges = &edges;
    EXPECT_FALSE(job.Validate());
    EXPECT_FALSE(job.Run());
  }

  {  // No output
    FloatTrackTriggeringJob job;
    job.track = track;
    EXPECT_FALSE(job.Validate());
    EXPECT_FALSE(job.Run());
  }

  {  // Valid
    FloatTrackTriggeringJob job;
    job.track = track;
    FloatTrackTriggeringJob::Edges edges(edges_buffer);
    job.edges = &edges;
    EXPECT_TRUE(job.Validate());
    EXPECT_TRUE(job.Run());
  }

  {  // Valid
    FloatTrackTriggeringJob job;
    job.from = 0.f;
    job.to = 1.f;
    job.track = track;
    FloatTrackTriggeringJob::Edges edges(edges_buffer);
    job.edges = &edges;
    EXPECT_TRUE(job.Validate());
    EXPECT_TRUE(job.Run());
  }

  {  // Empty output is valid
    FloatTrackTriggeringJob job;
    job.track = track;
    FloatTrackTriggeringJob::Edges edges;
    job.edges = &edges;
    EXPECT_TRUE(job.Validate());
    EXPECT_TRUE(job.Run());
  }
  ozz::memory::default_allocator()->Delete(track);
}

TEST(Default, TrackEdgeTriggerJob) {
  FloatTrack default_track;
  FloatTrackTriggeringJob job;
  job.track = &default_track;
  FloatTrackTriggeringJob::Edge edges_buffer[8];
  FloatTrackTriggeringJob::Edges edges(edges_buffer);
  job.edges = &edges;
  EXPECT_TRUE(job.Validate());
  EXPECT_TRUE(job.Run());
  ASSERT_EQ(edges.Count(), 0u);
}

TEST(Emyty, TrackEdgeTriggerJob) {
  TrackBuilder builder;

  // Builds track
  ozz::animation::offline::RawFloatTrack raw_track;
  ozz::animation::FloatTrack* track = builder(raw_track);
  ASSERT_TRUE(track != NULL);

  FloatTrackTriggeringJob::Edge edges_buffer[3];

  FloatTrackTriggeringJob job;
  job.track = track;
  job.threshold = 1.f;
  job.from = 0.f;
  job.to = 1.f;
  FloatTrackTriggeringJob::Edges edges(edges_buffer);
  job.edges = &edges;

  EXPECT_TRUE(job.Run());

  ASSERT_EQ(edges.Count(), 0u);

  ozz::memory::default_allocator()->Delete(track);
}

TEST(NoRange, TrackEdgeTriggerJob) {
  TrackBuilder builder;
  FloatTrackTriggeringJob::Edge edges_buffer[8];

  ozz::animation::offline::RawFloatTrack raw_track;

  // Keyframe values oscillate in range [0,2].
  const ozz::animation::offline::RawFloatTrack::Keyframe key0 = {
      RawTrackInterpolation::kStep, 0.f, 0.f};
  raw_track.keyframes.push_back(key0);
  const ozz::animation::offline::RawFloatTrack::Keyframe key1 = {
      RawTrackInterpolation::kStep, .5f, 2.f};
  raw_track.keyframes.push_back(key1);
  const ozz::animation::offline::RawFloatTrack::Keyframe key2 = {
      RawTrackInterpolation::kStep, 1.f, 0.f};
  raw_track.keyframes.push_back(key2);

  // Builds track
  ozz::animation::FloatTrack* track = builder(raw_track);
  ASSERT_TRUE(track != NULL);

  FloatTrackTriggeringJob job;
  job.track = track;
  job.threshold = 1.f;

  {  // Forward [0., 0.[
    job.from = 0.f;
    job.to = 0.f;
    FloatTrackTriggeringJob::Edges edges(edges_buffer);
    job.edges = &edges;
    EXPECT_TRUE(job.Run());

    ASSERT_EQ(edges.Count(), 0u);
  }

  {  // Forward [.1, .1]
    job.from = .1f;
    job.to = .1f;
    FloatTrackTriggeringJob::Edges edges(edges_buffer);
    job.edges = &edges;
    EXPECT_TRUE(job.Run());

    ASSERT_EQ(edges.Count(), 0u);
  }

  {  // Forward [.5, .5[
    job.from = .5f;
    job.to = .5f;
    FloatTrackTriggeringJob::Edges edges(edges_buffer);
    job.edges = &edges;
    EXPECT_TRUE(job.Run());

    ASSERT_EQ(edges.Count(), 0u);
  }

  {  // Forward [1., 1.]
    job.from = 1.f;
    job.to = 1.f;
    FloatTrackTriggeringJob::Edges edges(edges_buffer);
    job.edges = &edges;
    EXPECT_TRUE(job.Run());

    ASSERT_EQ(edges.Count(), 0u);
  }

  ozz::memory::default_allocator()->Delete(track);
}

template <size_t _size>
void TestEdgesExpectation(
    const ozz::animation::offline::RawFloatTrack& _raw_track, float _threshold,
    const FloatTrackTriggeringJob::Edge (&_expected)[_size]) {
  OZZ_STATIC_ASSERT(_size >= 2);

  // Builds track
  ozz::animation::FloatTrack* track = TrackBuilder()(_raw_track);
  ASSERT_TRUE(track != NULL);

  FloatTrackTriggeringJob::Edge edges_buffer[128];

  FloatTrackTriggeringJob job;
  job.track = track;
  job.threshold = _threshold;

  {  // Forward [0, 1]
    job.from = 0.f;
    job.to = 1.f;
    FloatTrackTriggeringJob::Edges edges(edges_buffer);
    job.edges = &edges;
    EXPECT_TRUE(job.Run());

    ASSERT_EQ(edges.Count(), _size);

    for (size_t i = 0; i < edges.Count(); ++i) {
      EXPECT_FLOAT_EQ(edges[i].time, _expected[i].time);
      EXPECT_EQ(edges[i].rising, _expected[i].rising);
    }
  }
  {  // Backward [1, 0]
    job.from = 1.f;
    job.to = 0.f;
    FloatTrackTriggeringJob::Edges edges(edges_buffer);
    job.edges = &edges;
    EXPECT_TRUE(job.Run());

    ASSERT_EQ(edges.Count(), _size);

    for (size_t i = 0; i < edges.Count(); ++i) {
      EXPECT_FLOAT_EQ(edges[i].time, _expected[_size - i - 1].time);
      EXPECT_EQ(edges[i].rising, !_expected[_size - i - 1].rising);
    }
  }

  {  // Forward [1, 2]
    job.from = 1.f;
    job.to = 2.f;
    FloatTrackTriggeringJob::Edges edges(edges_buffer);
    job.edges = &edges;
    EXPECT_TRUE(job.Run());

    ASSERT_EQ(edges.Count(), _size);

    for (size_t i = 0; i < edges.Count(); ++i) {
      EXPECT_FLOAT_EQ(edges[i].time, _expected[i].time + 1.f);
      EXPECT_EQ(edges[i].rising, _expected[i].rising);
    }
  }
  {  // Backward [2, 1]
    job.from = 2.f;
    job.to = 1.f;
    FloatTrackTriggeringJob::Edges edges(edges_buffer);
    job.edges = &edges;
    EXPECT_TRUE(job.Run());

    ASSERT_EQ(edges.Count(), _size);

    for (size_t i = 0; i < edges.Count(); ++i) {
      EXPECT_FLOAT_EQ(edges[i].time, _expected[_size - i - 1].time + 1.f);
      EXPECT_EQ(edges[i].rising, !_expected[_size - i - 1].rising);
    }
  }

  {  // Forward [0, 3]
    job.from = 0.f;
    job.to = 3.f;
    FloatTrackTriggeringJob::Edges edges(edges_buffer);
    job.edges = &edges;
    EXPECT_TRUE(job.Run());

    ASSERT_EQ(edges.Count(), _size * 3);

    for (size_t i = 0; i < edges.Count(); ++i) {
      const size_t ie = i % _size;
      const float loops = static_cast<float>(i / _size);
      EXPECT_FLOAT_EQ(edges[i].time, _expected[ie].time + loops);
      EXPECT_EQ(edges[i].rising, _expected[ie].rising);
    }
  }
  {  // Backward [3, 0]
    job.from = 3.f;
    job.to = 0.f;
    FloatTrackTriggeringJob::Edges edges(edges_buffer);
    job.edges = &edges;
    EXPECT_TRUE(job.Run());

    ASSERT_EQ(edges.Count(), _size * 3);

    for (size_t i = 0; i < edges.Count(); ++i) {
      const size_t ie = (edges.Count() - i - 1) % _size;
      const float loops = static_cast<float>((edges.Count() - i - 1) / _size);
      EXPECT_FLOAT_EQ(edges[i].time, _expected[ie].time + loops);
      EXPECT_EQ(edges[i].rising, !_expected[ie].rising);
    }
  }

  {  // Forward, first edge to last, last can be excluded.

    // Last edge is included if it time is 1.f.
    const bool last_included = _expected[_size - 1].time == 1.f;

    job.from = _expected[0].time;
    job.to = _expected[_size - 1].time;
    FloatTrackTriggeringJob::Edges edges(edges_buffer);
    job.edges = &edges;
    EXPECT_TRUE(job.Run());

    ASSERT_EQ(edges.Count(), last_included ? _size : _size - 1);

    for (size_t i = 0; i < edges.Count(); ++i) {
      EXPECT_FLOAT_EQ(edges[i].time, _expected[i].time);
      EXPECT_EQ(edges[i].rising, _expected[i].rising);
    }
  }
  {  // Backward, last edge to first, last can be excluded.
    
    // Last edge is included if it time is 1.f.
    const bool last_included = _expected[_size - 1].time == 1.f;

    job.from = _expected[_size - 1].time;
    job.to = _expected[0].time;
    FloatTrackTriggeringJob::Edges edges(edges_buffer);
    job.edges = &edges;
    EXPECT_TRUE(job.Run());

    ASSERT_EQ(edges.Count(), last_included ? _size : _size - 1);

    for (size_t i = 0; i < edges.Count(); ++i) {
      const size_t ie = last_included ? _size - i - 1 : _size - i - 2;
      EXPECT_FLOAT_EQ(edges[i].time, _expected[ie].time);
      EXPECT_EQ(edges[i].rising, !_expected[ie].rising);
    }
  }

  {  // Forward, after first edge to 1.
    job.from = nexttowardf(_expected[0].time, 1.f);
    job.to = 1.f;
    FloatTrackTriggeringJob::Edges edges(edges_buffer);
    job.edges = &edges;
    EXPECT_TRUE(job.Run());

    ASSERT_EQ(edges.Count(), _size - 1);

    for (size_t i = 0; i < edges.Count(); ++i) {
      EXPECT_FLOAT_EQ(edges[i].time, _expected[i + 1].time);
      EXPECT_EQ(edges[i].rising, _expected[i + 1].rising);
    }
  }
  {  // Backward, 1 to after first edge
    job.from = 1.f;
    job.to = nexttowardf(_expected[0].time, 1.f);
    FloatTrackTriggeringJob::Edges edges(edges_buffer);
    job.edges = &edges;
    EXPECT_TRUE(job.Run());

    ASSERT_EQ(edges.Count(), _size - 1);

    for (size_t i = 0; i < edges.Count(); ++i) {
      EXPECT_FLOAT_EQ(edges[i].time, _expected[_size - i - 1].time);
      EXPECT_EQ(edges[i].rising, !_expected[_size - i - 1].rising);
    }
  }

  {  // Forward, 0 to first edge.
    job.from = 0.f;
    job.to = _expected[0].time;
    FloatTrackTriggeringJob::Edges edges(edges_buffer);
    job.edges = &edges;
    EXPECT_TRUE(job.Run());

    ASSERT_EQ(edges.Count(), 0u);
  }
  {  // Backward, first edge to 0
    job.from = _expected[0].time;
    job.to = 0.f;
    FloatTrackTriggeringJob::Edges edges(edges_buffer);
    job.edges = &edges;
    EXPECT_TRUE(job.Run());

    ASSERT_EQ(edges.Count(), 0u);
  }

  {  // Forward, 0 to after first edge.
    job.from = 0.f;
    job.to = nexttowardf(_expected[0].time, 1.f);
    FloatTrackTriggeringJob::Edges edges(edges_buffer);
    job.edges = &edges;
    EXPECT_TRUE(job.Run());

    ASSERT_EQ(edges.Count(), 1u);

    EXPECT_FLOAT_EQ(edges[0].time, _expected[0].time);
    EXPECT_EQ(edges[0].rising, _expected[0].rising);
  }
  {  // Backward, after first edge to 0
    job.from = nexttowardf(_expected[0].time, 1.f);
    job.to = 0.f;
    FloatTrackTriggeringJob::Edges edges(edges_buffer);
    job.edges = &edges;
    EXPECT_TRUE(job.Run());

    ASSERT_EQ(edges.Count(), 1u);

    EXPECT_FLOAT_EQ(edges[0].time, _expected[0].time);
    EXPECT_EQ(edges[0].rising, !_expected[0].rising);
  }

  {  // Forward, 0 to before last edge.
    job.from = 0.f;
    job.to = nexttowardf(_expected[_size - 1].time, 0.f);
    FloatTrackTriggeringJob::Edges edges(edges_buffer);
    job.edges = &edges;
    EXPECT_TRUE(job.Run());

    ASSERT_EQ(edges.Count(), _size - 1);

    for (size_t i = 0; i < edges.Count(); ++i) {
      EXPECT_FLOAT_EQ(edges[i].time, _expected[i].time);
      EXPECT_EQ(edges[i].rising, _expected[i].rising);
    }
  }
  {  // Backward, before last edge to 0
    job.from = nexttowardf(_expected[_size - 1].time, 0.f);
    job.to = 0.f;
    FloatTrackTriggeringJob::Edges edges(edges_buffer);
    job.edges = &edges;
    EXPECT_TRUE(job.Run());

    ASSERT_EQ(edges.Count(), _size - 1);

    for (size_t i = 0; i < edges.Count(); ++i) {
      EXPECT_FLOAT_EQ(edges[i].time, _expected[_size - i - 2].time);
      EXPECT_EQ(edges[i].rising, !_expected[_size - i - 2].rising);
    }
  }

  {  // Forward, 0 to after last edge.
    job.from = 0.f;
    job.to = nexttowardf(_expected[_size - 1].time, 1.f);
    FloatTrackTriggeringJob::Edges edges(edges_buffer);
    job.edges = &edges;
    EXPECT_TRUE(job.Run());

    ASSERT_EQ(edges.Count(), _size);

    for (size_t i = 0; i < edges.Count(); ++i) {
      EXPECT_FLOAT_EQ(edges[i].time, _expected[i].time);
      EXPECT_EQ(edges[i].rising, _expected[i].rising);
    }
  }
  {  // Backward, after last edge to 0
    job.from = nexttowardf(_expected[_size - 1].time, 1.f);
    job.to = 0.f;
    FloatTrackTriggeringJob::Edges edges(edges_buffer);
    job.edges = &edges;
    EXPECT_TRUE(job.Run());

    ASSERT_EQ(edges.Count(), _size);

    for (size_t i = 0; i < edges.Count(); ++i) {
      EXPECT_FLOAT_EQ(edges[i].time, _expected[_size - i - 1].time);
      EXPECT_EQ(edges[i].rising, !_expected[_size - i - 1].rising);
    }
  }

  // Negative times

  {  // Forward [-1, 0]
    job.from = -1.f;
    job.to = 0.f;
    FloatTrackTriggeringJob::Edges edges(edges_buffer);
    job.edges = &edges;
    EXPECT_TRUE(job.Run());

    ASSERT_EQ(edges.Count(), _size);

    for (size_t i = 0; i < edges.Count(); ++i) {
      EXPECT_FLOAT_EQ(edges[i].time, _expected[i].time - 1.f);
      EXPECT_EQ(edges[i].rising, _expected[i].rising);
    }
  }
  {  // Backward [0, -1]
    job.from = 0.f;
    job.to = -1.f;
    FloatTrackTriggeringJob::Edges edges(edges_buffer);
    job.edges = &edges;
    EXPECT_TRUE(job.Run());

    ASSERT_EQ(edges.Count(), _size);

    for (size_t i = 0; i < edges.Count(); ++i) {
      EXPECT_FLOAT_EQ(edges[i].time, _expected[_size - i - 1].time - 1.f);
      EXPECT_EQ(edges[i].rising, !_expected[_size - i - 1].rising);
    }
  }

  {  // Forward [-2, -1]
    job.from = -2.f;
    job.to = -1.f;
    FloatTrackTriggeringJob::Edges edges(edges_buffer);
    job.edges = &edges;
    EXPECT_TRUE(job.Run());

    ASSERT_EQ(edges.Count(), _size);

    for (size_t i = 0; i < edges.Count(); ++i) {
      EXPECT_FLOAT_EQ(edges[i].time, _expected[i].time - 2.f);
      EXPECT_EQ(edges[i].rising, _expected[i].rising);
    }
  }
  {  // Backward [-1, -2]
    job.from = -1.f;
    job.to = -2.f;
    FloatTrackTriggeringJob::Edges edges(edges_buffer);
    job.edges = &edges;
    EXPECT_TRUE(job.Run());

    ASSERT_EQ(edges.Count(), _size);

    for (size_t i = 0; i < edges.Count(); ++i) {
      EXPECT_FLOAT_EQ(edges[i].time, _expected[_size - i - 1].time - 2.f);
      EXPECT_EQ(edges[i].rising, !_expected[_size - i - 1].rising);
    }
  }

  {  // Forward [-1, 1]
    job.from = -1.f;
    job.to = 1.f;
    FloatTrackTriggeringJob::Edges edges(edges_buffer);
    job.edges = &edges;
    EXPECT_TRUE(job.Run());

    ASSERT_EQ(edges.Count(), _size * 2);

    for (size_t i = 0; i < _size; ++i) {
      EXPECT_FLOAT_EQ(edges[i].time, _expected[i].time - 1.f);
      EXPECT_EQ(edges[i].rising, _expected[i].rising);
    }
    for (size_t i = 0; i < _size; ++i) {
      EXPECT_FLOAT_EQ(edges[i + _size].time, _expected[i].time);
      EXPECT_EQ(edges[i + _size].rising, _expected[i].rising);
    }
  }
  {  // Backward [1, 1]
    job.from = 1.f;
    job.to = -1.f;
    FloatTrackTriggeringJob::Edges edges(edges_buffer);
    job.edges = &edges;
    EXPECT_TRUE(job.Run());

    ASSERT_EQ(edges.Count(), _size * 2);

    for (size_t i = 0; i < _size; ++i) {
      EXPECT_FLOAT_EQ(edges[i].time, _expected[_size - i - 1].time);
      EXPECT_EQ(edges[i].rising, !_expected[_size - i - 1].rising);
    }
    for (size_t i = 0; i < _size; ++i) {
      EXPECT_FLOAT_EQ(edges[i + _size].time,
                      _expected[_size - i - 1].time - 1.f);
      EXPECT_EQ(edges[i + _size].rising, !_expected[_size - i - 1].rising);
    }
  }

  {  // Randomized
    float time = 0.f;
    bool rising = false;
    bool init = false;
    for (size_t i = 0; i < 100000; ++i) {
      // Finds new evaluation range
      const float kMaxLoops = 3.f;
      const float rand_range =
          1.f - 2.f * static_cast<float>(rand()) / RAND_MAX;

      job.from = time;
      time += kMaxLoops * rand_range;
      job.to = time;

      FloatTrackTriggeringJob::Edges edges(edges_buffer);
      job.edges = &edges;

      EXPECT_TRUE(job.Run());

      // Check edges.
      for (size_t e = 0; e < edges.Count(); ++e) {
        if (!init) {
          rising = edges[e].rising;
          init = true;
        } else {
          // Successive edges should always be opposed, whichever direction the
          // time is going.
          EXPECT_TRUE(rising != edges[e].rising);
          rising = edges[e].rising;
        }
      }
    }
  }
  ozz::memory::default_allocator()->Delete(track);
}

TEST(SquareStep, TrackEdgeTriggerJob) {
  {  // Rising edge at t = 0.5
    ozz::animation::offline::RawFloatTrack raw_track;

    // Keyframe values oscillate in range [0,2].
    const ozz::animation::offline::RawFloatTrack::Keyframe key0 = {
        RawTrackInterpolation::kStep, 0.f, 0.f};
    raw_track.keyframes.push_back(key0);
    const ozz::animation::offline::RawFloatTrack::Keyframe key1 = {
        RawTrackInterpolation::kStep, .5f, 2.f};
    raw_track.keyframes.push_back(key1);
    const ozz::animation::offline::RawFloatTrack::Keyframe key2 = {
        RawTrackInterpolation::kStep, 1.f, 0.f};
    raw_track.keyframes.push_back(key2);

    const FloatTrackTriggeringJob::Edge expected[] = {{.5f, true},
                                                      {1.f, false}};
    TestEdgesExpectation(raw_track, 0.f, expected);
    TestEdgesExpectation(raw_track, 1.f, expected);
  }

  {  // Rising edge at t = 0.6, no falling edge at end
    ozz::animation::offline::RawFloatTrack raw_track;

    // Keyframe values oscillate in range [0,2].
    const ozz::animation::offline::RawFloatTrack::Keyframe key0 = {
        RawTrackInterpolation::kStep, 0.f, 0.f};
    raw_track.keyframes.push_back(key0);
    const ozz::animation::offline::RawFloatTrack::Keyframe key1 = {
        RawTrackInterpolation::kStep, .6f, 2.f};
    raw_track.keyframes.push_back(key1);

    const FloatTrackTriggeringJob::Edge expected[] = {{0.f, false},
                                                      {.6f, true}};
    TestEdgesExpectation(raw_track, 0.f, expected);
    TestEdgesExpectation(raw_track, 1.f, expected);
  }

  {  // Falling edge at t = 0.5
    ozz::animation::offline::RawFloatTrack raw_track;

    // Keyframe values oscillate in range [0,2].
    const ozz::animation::offline::RawFloatTrack::Keyframe key0 = {
        RawTrackInterpolation::kStep, 0.f, 2.f};
    raw_track.keyframes.push_back(key0);
    const ozz::animation::offline::RawFloatTrack::Keyframe key1 = {
        RawTrackInterpolation::kStep, .5f, 0.f};
    raw_track.keyframes.push_back(key1);

    const FloatTrackTriggeringJob::Edge expected[] = {{0.f, true},
                                                      {.5f, false}};
    TestEdgesExpectation(raw_track, 0.f, expected);
    TestEdgesExpectation(raw_track, 1.f, expected);
  }

  {  // Negative values
    ozz::animation::offline::RawFloatTrack raw_track;

    // Keyframe values oscillate in range [-1,1].
    const ozz::animation::offline::RawFloatTrack::Keyframe key0 = {
        RawTrackInterpolation::kStep, 0.f, -1.f};
    raw_track.keyframes.push_back(key0);
    const ozz::animation::offline::RawFloatTrack::Keyframe key1 = {
        RawTrackInterpolation::kStep, .5f, 1.f};
    raw_track.keyframes.push_back(key1);
    const ozz::animation::offline::RawFloatTrack::Keyframe key2 = {
        RawTrackInterpolation::kStep, 1.f, -1.f};
    raw_track.keyframes.push_back(key2);

    const FloatTrackTriggeringJob::Edge expected[] = {{.5f, true},
                                                      {1.f, false}};
    TestEdgesExpectation(raw_track, 0.f, expected);
  }

  {  // More edges
    ozz::animation::offline::RawFloatTrack raw_track;

    const ozz::animation::offline::RawFloatTrack::Keyframe key0 = {
        RawTrackInterpolation::kStep, .0f, 0.f};
    raw_track.keyframes.push_back(key0);
    const ozz::animation::offline::RawFloatTrack::Keyframe key1 = {
        RawTrackInterpolation::kStep, .2f, 2.f};
    raw_track.keyframes.push_back(key1);
    const ozz::animation::offline::RawFloatTrack::Keyframe key2 = {
        RawTrackInterpolation::kStep, .3f, 0.f};
    raw_track.keyframes.push_back(key2);
    const ozz::animation::offline::RawFloatTrack::Keyframe key3 = {
        RawTrackInterpolation::kStep, .4f, 1.f};
    raw_track.keyframes.push_back(key3);
    const ozz::animation::offline::RawFloatTrack::Keyframe key4 = {
        RawTrackInterpolation::kStep, .5f, 0.f};
    raw_track.keyframes.push_back(key4);

    const FloatTrackTriggeringJob::Edge expected0[] = {
        {.2f, true}, {.3f, false}, {.4f, true}, {.5f, false}};
    TestEdgesExpectation(raw_track, 0.f, expected0);

    const FloatTrackTriggeringJob::Edge expected1[] = {{.2f, true},
                                                       {.3f, false}};
    TestEdgesExpectation(raw_track, 1.f, expected1);
  }
}

TEST(Linear, TrackEdgeTriggerJob) {
  {  // Higher point at t = 0.5
    ozz::animation::offline::RawFloatTrack raw_track;

    // Keyframe values oscillate in range [0,2].
    const ozz::animation::offline::RawFloatTrack::Keyframe key0 = {
        RawTrackInterpolation::kLinear, 0.f, 0.f};
    raw_track.keyframes.push_back(key0);
    const ozz::animation::offline::RawFloatTrack::Keyframe key1 = {
        RawTrackInterpolation::kLinear, .5f, 2.f};
    raw_track.keyframes.push_back(key1);
    const ozz::animation::offline::RawFloatTrack::Keyframe key2 = {
        RawTrackInterpolation::kLinear, 1.f, 0.f};
    raw_track.keyframes.push_back(key2);

    const FloatTrackTriggeringJob::Edge expected0[] = {{.25f, true},
                                                       {.75f, false}};
    TestEdgesExpectation(raw_track, 1.f, expected0);

    const FloatTrackTriggeringJob::Edge expected1[] = {{.125f, true},
                                                       {.875f, false}};
    TestEdgesExpectation(raw_track, .5f, expected1);

    const FloatTrackTriggeringJob::Edge expected2[] = {{.375f, true},
                                                       {.625f, false}};
    TestEdgesExpectation(raw_track, 1.5f, expected2);
  }

  {  // Higher point at t = 0.5
    ozz::animation::offline::RawFloatTrack raw_track;

    // Keyframe values oscillate in range [0,2].
    const ozz::animation::offline::RawFloatTrack::Keyframe key0 = {
        RawTrackInterpolation::kLinear, 0.f, 0.f};
    raw_track.keyframes.push_back(key0);
    const ozz::animation::offline::RawFloatTrack::Keyframe key1 = {
        RawTrackInterpolation::kLinear, .5f, 2.f};
    raw_track.keyframes.push_back(key1);

    const FloatTrackTriggeringJob::Edge expected[] = {{0.f, false},
                                                      {.25f, true}};
    TestEdgesExpectation(raw_track, 1.f, expected);
  }

  {  // Negative values
    ozz::animation::offline::RawFloatTrack raw_track;

    // Keyframe values oscillate in range [-1,1].
    const ozz::animation::offline::RawFloatTrack::Keyframe key0 = {
        RawTrackInterpolation::kLinear, 0.f, -1.f};
    raw_track.keyframes.push_back(key0);
    const ozz::animation::offline::RawFloatTrack::Keyframe key1 = {
        RawTrackInterpolation::kLinear, .5f, 1.f};
    raw_track.keyframes.push_back(key1);

    const FloatTrackTriggeringJob::Edge expected[] = {{0.f, false},
                                                      {.25f, true}};
    TestEdgesExpectation(raw_track, 0.f, expected);
  }
}

TEST(Mixed, TrackEdgeTriggerJob) {
  {  // Higher point at t = 0.5
    ozz::animation::offline::RawFloatTrack raw_track;

    // Keyframe values oscillate in range [0,2].
    const ozz::animation::offline::RawFloatTrack::Keyframe key0 = {
        RawTrackInterpolation::kStep, 0.f, 0.f};
    raw_track.keyframes.push_back(key0);
    const ozz::animation::offline::RawFloatTrack::Keyframe key1 = {
        RawTrackInterpolation::kLinear, .5f, 2.f};
    raw_track.keyframes.push_back(key1);
    const ozz::animation::offline::RawFloatTrack::Keyframe key2 = {
        RawTrackInterpolation::kLinear, 1.f, 0.f};
    raw_track.keyframes.push_back(key2);

    const FloatTrackTriggeringJob::Edge expected0[] = {{.5f, true},
                                                       {.75f, false}};
    TestEdgesExpectation(raw_track, 1.f, expected0);

    const FloatTrackTriggeringJob::Edge expected1[] = {{.5f, true},
                                                       {.875f, false}};
    TestEdgesExpectation(raw_track, .5f, expected1);

    const FloatTrackTriggeringJob::Edge expected2[] = {{.5f, true},
                                                       {.625f, false}};
    TestEdgesExpectation(raw_track, 1.5f, expected2);
  }

  {  // Higher point at t = 0.5
    ozz::animation::offline::RawFloatTrack raw_track;

    // Keyframe values oscillate in range [0,2].
    const ozz::animation::offline::RawFloatTrack::Keyframe key0 = {
        RawTrackInterpolation::kLinear, 0.f, 0.f};
    raw_track.keyframes.push_back(key0);
    const ozz::animation::offline::RawFloatTrack::Keyframe key1 = {
        RawTrackInterpolation::kStep, .5f, 2.f};
    raw_track.keyframes.push_back(key1);
    const ozz::animation::offline::RawFloatTrack::Keyframe key2 = {
        RawTrackInterpolation::kLinear, 1.f, 0.f};
    raw_track.keyframes.push_back(key2);

    const FloatTrackTriggeringJob::Edge expected0[] = {{.25f, true},
                                                       {1.f, false}};
    TestEdgesExpectation(raw_track, 1.f, expected0);

    const FloatTrackTriggeringJob::Edge expected1[] = {{.125f, true},
                                                       {1.f, false}};
    TestEdgesExpectation(raw_track, .5f, expected1);

    const FloatTrackTriggeringJob::Edge expected2[] = {{.375f, true},
                                                       {1.f, false}};
    TestEdgesExpectation(raw_track, 1.5f, expected2);
  }
}
TEST(StepThreshold, TrackEdgeTriggerJob) {
  TrackBuilder builder;
  FloatTrackTriggeringJob::Edge edges_buffer[8];

  // Rising edge at t = 0.5
  ozz::animation::offline::RawFloatTrack raw_track;

  // Keyframe values oscillate in range [-1,1].
  const ozz::animation::offline::RawFloatTrack::Keyframe key0 = {
      RawTrackInterpolation::kStep, 0.f, -1.f};
  raw_track.keyframes.push_back(key0);
  const ozz::animation::offline::RawFloatTrack::Keyframe key1 = {
      RawTrackInterpolation::kStep, .5f, 1.f};
  raw_track.keyframes.push_back(key1);
  const ozz::animation::offline::RawFloatTrack::Keyframe key2 = {
      RawTrackInterpolation::kStep, 1.f, -1.f};
  raw_track.keyframes.push_back(key2);

  // Builds track
  ozz::animation::FloatTrack* track = builder(raw_track);
  ASSERT_TRUE(track != NULL);

  FloatTrackTriggeringJob job;
  job.track = track;
  job.from = 0.f;
  job.to = 1.f;

  {  // In range
    job.threshold = .5f;
    FloatTrackTriggeringJob::Edges edges(edges_buffer);
    job.edges = &edges;

    EXPECT_TRUE(job.Run());

    ASSERT_EQ(edges.Count(), 2u);

    EXPECT_FLOAT_EQ(edges[0].time, .5f);
    EXPECT_EQ(edges[0].rising, true);
    EXPECT_FLOAT_EQ(edges[1].time, 1.f);
    EXPECT_EQ(edges[1].rising, false);
  }

  {  // Bottom of range is included
    job.threshold = -1.f;
    FloatTrackTriggeringJob::Edges edges(edges_buffer);
    job.edges = &edges;

    EXPECT_TRUE(job.Run());

    ASSERT_EQ(edges.Count(), 2u);

    EXPECT_FLOAT_EQ(edges[0].time, .5f);
    EXPECT_EQ(edges[0].rising, true);
    EXPECT_FLOAT_EQ(edges[1].time, 1.f);
    EXPECT_EQ(edges[1].rising, false);
  }

  {  // Top range is excluded
    job.threshold = 1.f;
    FloatTrackTriggeringJob::Edges edges(edges_buffer);
    job.edges = &edges;

    EXPECT_TRUE(job.Run());

    ASSERT_EQ(edges.Count(), 0u);
  }

  {  // In range
    job.threshold = 0.f;
    FloatTrackTriggeringJob::Edges edges(edges_buffer);
    job.edges = &edges;

    EXPECT_TRUE(job.Run());

    ASSERT_EQ(edges.Count(), 2u);

    EXPECT_FLOAT_EQ(edges[0].time, .5f);
    EXPECT_EQ(edges[0].rising, true);
    EXPECT_FLOAT_EQ(edges[1].time, 1.f);
    EXPECT_EQ(edges[1].rising, false);
  }

  {  // Out of range
    job.from = 0.f;
    job.to = 1.f;
    job.threshold = 2.f;
    FloatTrackTriggeringJob::Edges edges(edges_buffer);
    job.edges = &edges;

    EXPECT_TRUE(job.Run());

    ASSERT_EQ(edges.Count(), 0u);
  }

  ozz::memory::default_allocator()->Delete(track);
}

TEST(StepThresholdBool, TrackEdgeTriggerJob) {
  TrackBuilder builder;
  FloatTrackTriggeringJob::Edge edges_buffer[8];

  // Rising edge at t = 0.5
  ozz::animation::offline::RawFloatTrack raw_track;

  // Keyframe values oscillate in range [0,1].
  const ozz::animation::offline::RawFloatTrack::Keyframe key0 = {
      RawTrackInterpolation::kStep, 0.f, 0.f};
  raw_track.keyframes.push_back(key0);
  const ozz::animation::offline::RawFloatTrack::Keyframe key1 = {
      RawTrackInterpolation::kStep, .5f, 1.f};
  raw_track.keyframes.push_back(key1);
  const ozz::animation::offline::RawFloatTrack::Keyframe key2 = {
      RawTrackInterpolation::kStep, 1.f, 0.f};
  raw_track.keyframes.push_back(key2);

  // Builds track
  ozz::animation::FloatTrack* track = builder(raw_track);
  ASSERT_TRUE(track != NULL);

  FloatTrackTriggeringJob job;
  job.track = track;
  job.from = 0.f;
  job.to = 1.f;

  {  // In range
    job.threshold = .5f;
    FloatTrackTriggeringJob::Edges edges(edges_buffer);
    job.edges = &edges;

    EXPECT_TRUE(job.Run());

    ASSERT_EQ(edges.Count(), 2u);

    EXPECT_FLOAT_EQ(edges[0].time, .5f);
    EXPECT_EQ(edges[0].rising, true);
    EXPECT_FLOAT_EQ(edges[1].time, 1.f);
    EXPECT_EQ(edges[1].rising, false);
  }

  {  // Top range is excluded
    job.threshold = 1.f;
    FloatTrackTriggeringJob::Edges edges(edges_buffer);
    job.edges = &edges;

    EXPECT_TRUE(job.Run());

    ASSERT_EQ(edges.Count(), 0u);
  }

  {  // Bottom range is included
    job.threshold = 0.f;
    FloatTrackTriggeringJob::Edges edges(edges_buffer);
    job.edges = &edges;

    EXPECT_TRUE(job.Run());

    ASSERT_EQ(edges.Count(), 2u);

    EXPECT_FLOAT_EQ(edges[0].time, .5f);
    EXPECT_EQ(edges[0].rising, true);
    EXPECT_FLOAT_EQ(edges[1].time, 1.f);
    EXPECT_EQ(edges[1].rising, false);
  }

  ozz::memory::default_allocator()->Delete(track);
}

TEST(LinearThreshold, TrackEdgeTriggerJob) {
  TrackBuilder builder;
  FloatTrackTriggeringJob::Edge edges_buffer[8];

  // Rising edge at t = 0.5
  ozz::animation::offline::RawFloatTrack raw_track;

  // Keyframe values oscillate in range [-1,1].
  const ozz::animation::offline::RawFloatTrack::Keyframe key0 = {
      RawTrackInterpolation::kLinear, 0.f, -1.f};
  raw_track.keyframes.push_back(key0);
  const ozz::animation::offline::RawFloatTrack::Keyframe key1 = {
      RawTrackInterpolation::kLinear, .5f, 1.f};
  raw_track.keyframes.push_back(key1);
  const ozz::animation::offline::RawFloatTrack::Keyframe key2 = {
      RawTrackInterpolation::kLinear, 1.f, -1.f};
  raw_track.keyframes.push_back(key2);

  // Builds track
  ozz::animation::FloatTrack* track = builder(raw_track);
  ASSERT_TRUE(track != NULL);

  FloatTrackTriggeringJob job;
  job.track = track;
  job.from = 0.f;
  job.to = 1.f;

  {  // In range
    job.threshold = .5f;
    FloatTrackTriggeringJob::Edges edges(edges_buffer);
    job.edges = &edges;

    EXPECT_TRUE(job.Run());

    ASSERT_EQ(edges.Count(), 2u);

    EXPECT_FLOAT_EQ(edges[0].time, .375f);
    EXPECT_EQ(edges[0].rising, true);
    EXPECT_FLOAT_EQ(edges[1].time, .625f);
    EXPECT_EQ(edges[1].rising, false);
  }

  {  // Top range is excluded
    job.threshold = 1.f;
    FloatTrackTriggeringJob::Edges edges(edges_buffer);
    job.edges = &edges;

    EXPECT_TRUE(job.Run());

    ASSERT_EQ(edges.Count(), 0u);
  }

  {  // In range
    job.threshold = 0.f;
    FloatTrackTriggeringJob::Edges edges(edges_buffer);
    job.edges = &edges;

    EXPECT_TRUE(job.Run());

    ASSERT_EQ(edges.Count(), 2u);

    EXPECT_FLOAT_EQ(edges[0].time, .25f);
    EXPECT_EQ(edges[0].rising, true);
    EXPECT_FLOAT_EQ(edges[1].time, .75f);
    EXPECT_EQ(edges[1].rising, false);
  }

  {  // Bottom of range is included
    job.threshold = -1.f;
    FloatTrackTriggeringJob::Edges edges(edges_buffer);
    job.edges = &edges;

    EXPECT_TRUE(job.Run());

    ASSERT_EQ(edges.Count(), 2u);

    EXPECT_FLOAT_EQ(edges[0].time, 0.f);
    EXPECT_EQ(edges[0].rising, true);
    EXPECT_FLOAT_EQ(edges[1].time, 1.f);
    EXPECT_EQ(edges[1].rising, false);
  }

  {  // Out of range
    job.from = 0.f;
    job.to = 1.f;
    job.threshold = 2.f;
    FloatTrackTriggeringJob::Edges edges(edges_buffer);
    job.edges = &edges;

    EXPECT_TRUE(job.Run());

    ASSERT_EQ(edges.Count(), 0u);
  }

  ozz::memory::default_allocator()->Delete(track);
}

TEST(Overflow, TrackEdgeTriggerJob) {
  TrackBuilder builder;

  // Rising edge at t = 0.5
  ozz::animation::offline::RawFloatTrack raw_track;

  // Keyframe values oscillate in range [0,2].
  const ozz::animation::offline::RawFloatTrack::Keyframe key0 = {
      RawTrackInterpolation::kStep, 0.f, 0.f};
  raw_track.keyframes.push_back(key0);
  const ozz::animation::offline::RawFloatTrack::Keyframe key1 = {
      RawTrackInterpolation::kStep, .5f, 2.f};
  raw_track.keyframes.push_back(key1);
  const ozz::animation::offline::RawFloatTrack::Keyframe key2 = {
      RawTrackInterpolation::kStep, 1.f, 0.f};
  raw_track.keyframes.push_back(key2);

  // Builds track
  ozz::animation::FloatTrack* track = builder(raw_track);
  ASSERT_TRUE(track != NULL);

  FloatTrackTriggeringJob::Edge edges_buffer[3];

  FloatTrackTriggeringJob job;
  job.track = track;
  job.threshold = 1.f;

  {  // No overflow
    job.from = 0.f;
    job.to = 1.f;
    FloatTrackTriggeringJob::Edges edges(edges_buffer);
    job.edges = &edges;

    EXPECT_TRUE(job.Run());

    ASSERT_EQ(edges.Count(), 2u);

    EXPECT_FLOAT_EQ(edges[0].time, .5f);
    EXPECT_EQ(edges[0].rising, true);

    EXPECT_FLOAT_EQ(edges[1].time, 1.f);
    EXPECT_EQ(edges[1].rising, false);
  }

  {  // Full but no overflow
    job.from = 0.f;
    job.to = 1.6f;
    FloatTrackTriggeringJob::Edges edges(edges_buffer);
    job.edges = &edges;

    EXPECT_TRUE(job.Run());

    ASSERT_EQ(edges.Count(), 3u);

    EXPECT_FLOAT_EQ(edges[0].time, .5f);
    EXPECT_EQ(edges[0].rising, true);

    EXPECT_FLOAT_EQ(edges[1].time, 1.f);
    EXPECT_EQ(edges[1].rising, false);

    EXPECT_FLOAT_EQ(edges[2].time, 1.5f);
    EXPECT_EQ(edges[2].rising, true);
  }

  {  // Overflow
    job.from = 0.f;
    job.to = 2.f;
    FloatTrackTriggeringJob::Edges edges(edges_buffer);
    job.edges = &edges;

    EXPECT_FALSE(job.Run());  // Return false

    ASSERT_EQ(edges.Count(), 3u);  // But buffer isn't empty.

    EXPECT_FLOAT_EQ(edges[0].time, .5f);
    EXPECT_EQ(edges[0].rising, true);

    EXPECT_FLOAT_EQ(edges[1].time, 1.f);
    EXPECT_EQ(edges[1].rising, false);

    EXPECT_FLOAT_EQ(edges[2].time, 1.5f);
    EXPECT_EQ(edges[2].rising, true);
  }

  {  // Overflow 2 passes
    job.from = 0.f;
    job.to = 2.f;
    FloatTrackTriggeringJob::Edges edges(edges_buffer);
    job.edges = &edges;

    // 1st pass
    EXPECT_FALSE(job.Run());  // Return false

    ASSERT_EQ(edges.Count(), 3u);  // But buffer is full.

    EXPECT_FLOAT_EQ(edges[0].time, .5f);
    EXPECT_EQ(edges[0].rising, true);

    EXPECT_FLOAT_EQ(edges[1].time, 1.f);
    EXPECT_EQ(edges[1].rising, false);

    EXPECT_FLOAT_EQ(edges[2].time, 1.5f);
    EXPECT_EQ(edges[2].rising, true);

    // 2nd pass, starting right after the end of the first one.
    job.from = nexttowardf(edges[2].time, job.to);
    EXPECT_TRUE(job.Run());  // Last pass

    ASSERT_EQ(edges.Count(), 1u);  // But buffer isn't empty.

    EXPECT_FLOAT_EQ(edges[0].time, 2.f);
    EXPECT_EQ(edges[0].rising, false);
  }

  {  // Empty output
    job.from = 0.f;
    job.to = 2.f;
    FloatTrackTriggeringJob::Edges edges;
    job.edges = &edges;

    EXPECT_FALSE(job.Run());
  }

  ozz::memory::default_allocator()->Delete(track);
}
