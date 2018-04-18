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
  ASSERT_TRUE(job.Run());
  ASSERT_EQ(edges.count(), 0u);
}

TEST(Empty, TrackEdgeTriggerJob) {
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

  ASSERT_TRUE(job.Run());

  ASSERT_EQ(edges.count(), 0u);

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
    ASSERT_TRUE(job.Run());

    ASSERT_EQ(edges.count(), 0u);
  }

  {  // Forward [.1, .1]
    job.from = .1f;
    job.to = .1f;
    FloatTrackTriggeringJob::Edges edges(edges_buffer);
    job.edges = &edges;
    ASSERT_TRUE(job.Run());

    ASSERT_EQ(edges.count(), 0u);
  }

  {  // Forward [.5, .5[
    job.from = .5f;
    job.to = .5f;
    FloatTrackTriggeringJob::Edges edges(edges_buffer);
    job.edges = &edges;
    ASSERT_TRUE(job.Run());

    ASSERT_EQ(edges.count(), 0u);
  }

  {  // Forward [1., 1.]
    job.from = 1.f;
    job.to = 1.f;
    FloatTrackTriggeringJob::Edges edges(edges_buffer);
    job.edges = &edges;
    ASSERT_TRUE(job.Run());

    ASSERT_EQ(edges.count(), 0u);
  }

  ozz::memory::default_allocator()->Delete(track);
}

void TestEdgesExpectationBackward(const FloatTrackTriggeringJob& _fw_job) {
  // Setup backward job from forward one.
  FloatTrackTriggeringJob bw_job = _fw_job;
  std::swap(bw_job.from, bw_job.to);
  FloatTrackTriggeringJob::Edge bw_edges_buffer[128];
  FloatTrackTriggeringJob::Edges bw_edges(bw_edges_buffer);
  bw_job.edges = &bw_edges;

  ASSERT_TRUE(bw_job.Run());

  const FloatTrackTriggeringJob::Edges& fw_edges = *_fw_job.edges;
  ASSERT_EQ(_fw_job.edges->count(), bw_edges.count());

  for (size_t i = 0; i < bw_edges.count(); ++i) {
    EXPECT_FLOAT_EQ(fw_edges[i].time, bw_edges[bw_edges.count() - i - 1].time);
    EXPECT_EQ(fw_edges[i].rising, !bw_edges[bw_edges.count() - i - 1].rising);
  }
}

void TestEdgesExpectation(
    const ozz::animation::offline::RawFloatTrack& _raw_track, float _threshold,
    const FloatTrackTriggeringJob::Edge* _expected, size_t _size) {
  assert(_size >= 2);

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
    ASSERT_TRUE(job.Run());

    ASSERT_EQ(edges.count(), _size);

    for (size_t i = 0; i < edges.count(); ++i) {
      EXPECT_FLOAT_EQ(edges[i].time, _expected[i].time);
      EXPECT_EQ(edges[i].rising, _expected[i].rising);
    }

    TestEdgesExpectationBackward(job);
  }

  {  // Forward [1, 2]
    job.from = 1.f;
    job.to = 2.f;
    FloatTrackTriggeringJob::Edges edges(edges_buffer);
    job.edges = &edges;
    ASSERT_TRUE(job.Run());

    ASSERT_EQ(edges.count(), _size);

    for (size_t i = 0; i < edges.count(); ++i) {
      EXPECT_FLOAT_EQ(edges[i].time, _expected[i].time + 1.f);
      EXPECT_EQ(edges[i].rising, _expected[i].rising);
    }
    TestEdgesExpectationBackward(job);
  }

  {  // Forward [0, 3]
    job.from = 0.f;
    job.to = 3.f;
    FloatTrackTriggeringJob::Edges edges(edges_buffer);
    job.edges = &edges;
    ASSERT_TRUE(job.Run());

    ASSERT_EQ(edges.count(), _size * 3);

    for (size_t i = 0; i < edges.count(); ++i) {
      const size_t ie = i % _size;
      const float loops = static_cast<float>(i / _size);
      EXPECT_FLOAT_EQ(edges[i].time, _expected[ie].time + loops);
      EXPECT_EQ(edges[i].rising, _expected[ie].rising);
    }
    TestEdgesExpectationBackward(job);
  }

  {  // Forward, first edge to last, last can be included.

    // Last edge is included if it time is 1.f.
    const bool last_included = _expected[_size - 1].time == 1.f;

    job.from = _expected[0].time;
    job.to = _expected[_size - 1].time;
    FloatTrackTriggeringJob::Edges edges(edges_buffer);
    job.edges = &edges;
    ASSERT_TRUE(job.Run());

    ASSERT_EQ(edges.count(), last_included ? _size : _size - 1);

    for (size_t i = 0; i < edges.count(); ++i) {
      EXPECT_FLOAT_EQ(edges[i].time, _expected[i].time);
      EXPECT_EQ(edges[i].rising, _expected[i].rising);
    }
    TestEdgesExpectationBackward(job);
  }

  {  // Forward, after first edge to 1.
    job.from = nexttowardf(_expected[0].time, 1.f);
    job.to = 1.f;
    FloatTrackTriggeringJob::Edges edges(edges_buffer);
    job.edges = &edges;
    ASSERT_TRUE(job.Run());

    ASSERT_EQ(edges.count(), _size - 1);

    for (size_t i = 0; i < edges.count(); ++i) {
      EXPECT_FLOAT_EQ(edges[i].time, _expected[i + 1].time);
      EXPECT_EQ(edges[i].rising, _expected[i + 1].rising);
    }
    TestEdgesExpectationBackward(job);
  }

  {  // Forward, 0 to first edge.
    job.from = 0.f;
    job.to = _expected[0].time;
    FloatTrackTriggeringJob::Edges edges(edges_buffer);
    job.edges = &edges;
    ASSERT_TRUE(job.Run());

    ASSERT_EQ(edges.count(), 0u);
    TestEdgesExpectationBackward(job);
  }

  {  // Forward, 0 to after first edge.
    job.from = 0.f;
    job.to = nexttowardf(_expected[0].time, 1.f);
    FloatTrackTriggeringJob::Edges edges(edges_buffer);
    job.edges = &edges;
    ASSERT_TRUE(job.Run());

    ASSERT_EQ(edges.count(), 1u);

    EXPECT_FLOAT_EQ(edges[0].time, _expected[0].time);
    EXPECT_EQ(edges[0].rising, _expected[0].rising);

    TestEdgesExpectationBackward(job);
  }

  {  // Forward, 0 to before last edge.
    job.from = 0.f;
    job.to = nexttowardf(_expected[_size - 1].time, 0.f);
    FloatTrackTriggeringJob::Edges edges(edges_buffer);
    job.edges = &edges;
    ASSERT_TRUE(job.Run());

    ASSERT_EQ(edges.count(), _size - 1);

    for (size_t i = 0; i < edges.count(); ++i) {
      EXPECT_FLOAT_EQ(edges[i].time, _expected[i].time);
      EXPECT_EQ(edges[i].rising, _expected[i].rising);
    }
    TestEdgesExpectationBackward(job);
  }

  {  // Forward, 0 to after last edge.
    job.from = 0.f;
    job.to = nexttowardf(_expected[_size - 1].time, 1.f);
    FloatTrackTriggeringJob::Edges edges(edges_buffer);
    job.edges = &edges;
    ASSERT_TRUE(job.Run());

    ASSERT_EQ(edges.count(), _size);

    for (size_t i = 0; i < edges.count(); ++i) {
      EXPECT_FLOAT_EQ(edges[i].time, _expected[i].time);
      EXPECT_EQ(edges[i].rising, _expected[i].rising);
    }
    TestEdgesExpectationBackward(job);
  }

  {  // Forward, 1 to after last edge + 1.
    job.from = 1.f;
    job.to = nexttowardf(_expected[_size - 1].time + 1.f, 2.f);
    FloatTrackTriggeringJob::Edges edges(edges_buffer);
    job.edges = &edges;
    ASSERT_TRUE(job.Run());

    ASSERT_EQ(edges.count(), _size);

    for (size_t i = 0; i < edges.count(); ++i) {
      EXPECT_FLOAT_EQ(edges[i].time, _expected[i].time + 1.f);
      EXPECT_EQ(edges[i].rising, _expected[i].rising);
    }
    TestEdgesExpectationBackward(job);
  }

  {  // Forward, 46 to after last edge + 46.
    job.from = 46.f;
    job.to = nexttowardf(_expected[_size - 1].time + 46.f, 100.f);
    FloatTrackTriggeringJob::Edges edges(edges_buffer);
    job.edges = &edges;
    ASSERT_TRUE(job.Run());

    ASSERT_EQ(edges.count(), _size);

    for (size_t i = 0; i < edges.count(); ++i) {
      EXPECT_FLOAT_EQ(edges[i].time, _expected[i].time + 46.f);
      EXPECT_EQ(edges[i].rising, _expected[i].rising);
    }
    TestEdgesExpectationBackward(job);
  }

  {  // Forward, 46 to before last edge + 46.
    job.from = 46.f;
    job.to = nexttowardf(_expected[_size - 1].time + 46.f, -100.f);
    FloatTrackTriggeringJob::Edges edges(edges_buffer);
    job.edges = &edges;
    ASSERT_TRUE(job.Run());

    ASSERT_EQ(edges.count(), _size - 1);

    for (size_t i = 0; i < edges.count(); ++i) {
      EXPECT_FLOAT_EQ(edges[i].time, _expected[i].time + 46.f);
      EXPECT_EQ(edges[i].rising, _expected[i].rising);
    }
    TestEdgesExpectationBackward(job);
  }

  {  // Forward, 46 to last edge + 46.
    const bool last_included = _expected[_size - 1].time == 1.f;

    job.from = 46.f;
    job.to = _expected[_size - 1].time + 46.f;
    FloatTrackTriggeringJob::Edges edges(edges_buffer);
    job.edges = &edges;
    ASSERT_TRUE(job.Run());

    ASSERT_EQ(edges.count(), last_included ? _size : _size - 1);

    for (size_t i = 0; i < edges.count(); ++i) {
      EXPECT_FLOAT_EQ(edges[i].time, _expected[i].time + 46.f);
      EXPECT_EQ(edges[i].rising, _expected[i].rising);
    }
    TestEdgesExpectationBackward(job);
  }

  {  // Forward, 0 to before last edge + 1
    const bool last_included = _expected[_size - 1].time == 1.f;

    job.from = 0.f;
    job.to = _expected[_size - 1].time + 1.f;
    FloatTrackTriggeringJob::Edges edges(edges_buffer);
    job.edges = &edges;
    ASSERT_TRUE(job.Run());

    ASSERT_EQ(edges.count(), last_included ? _size * 2 : _size * 2 - 1);

    for (size_t i = 0; i < _size; ++i) {
      EXPECT_FLOAT_EQ(edges[i].time, _expected[i].time);
      EXPECT_EQ(edges[i].rising, _expected[i].rising);
    }
    for (size_t i = _size; i < (last_included ? _size * 2 : _size * 2 - 1);
         ++i) {
      EXPECT_FLOAT_EQ(edges[i].time, _expected[i - _size].time + 1.f);
      EXPECT_EQ(edges[i].rising, _expected[i - _size].rising);
    }
    TestEdgesExpectationBackward(job);
  }

  // Negative times

  {  // Forward [-1, 0]
    job.from = -1.f;
    job.to = 0.f;
    FloatTrackTriggeringJob::Edges edges(edges_buffer);
    job.edges = &edges;
    ASSERT_TRUE(job.Run());

    ASSERT_EQ(edges.count(), _size);

    for (size_t i = 0; i < edges.count(); ++i) {
      EXPECT_FLOAT_EQ(edges[i].time, _expected[i].time - 1.f);
      EXPECT_EQ(edges[i].rising, _expected[i].rising);
    }
    TestEdgesExpectationBackward(job);
  }

  {  // Forward [-2, -1]
    job.from = -2.f;
    job.to = -1.f;
    FloatTrackTriggeringJob::Edges edges(edges_buffer);
    job.edges = &edges;
    ASSERT_TRUE(job.Run());

    ASSERT_EQ(edges.count(), _size);

    for (size_t i = 0; i < edges.count(); ++i) {
      EXPECT_FLOAT_EQ(edges[i].time, _expected[i].time - 2.f);
      EXPECT_EQ(edges[i].rising, _expected[i].rising);
    }
    TestEdgesExpectationBackward(job);
  }

  {  // Forward [-1, 1]
    job.from = -1.f;
    job.to = 1.f;
    FloatTrackTriggeringJob::Edges edges(edges_buffer);
    job.edges = &edges;
    ASSERT_TRUE(job.Run());

    ASSERT_EQ(edges.count(), _size * 2);

    for (size_t i = 0; i < _size; ++i) {
      EXPECT_FLOAT_EQ(edges[i].time, _expected[i].time - 1.f);
      EXPECT_EQ(edges[i].rising, _expected[i].rising);
    }
    for (size_t i = 0; i < _size; ++i) {
      EXPECT_FLOAT_EQ(edges[i + _size].time, _expected[i].time);
      EXPECT_EQ(edges[i + _size].rising, _expected[i].rising);
    }
    TestEdgesExpectationBackward(job);
  }

  {  // Forward, -1 to first edge.
    job.from = -1.f;
    job.to = _expected[0].time - 1.f;
    FloatTrackTriggeringJob::Edges edges(edges_buffer);
    job.edges = &edges;
    ASSERT_TRUE(job.Run());

    ASSERT_EQ(edges.count(), 0u);
    TestEdgesExpectationBackward(job);
  }

  {  // Forward, -1 to after last edge.
    job.from = -1.f;
    job.to = _expected[_size - 1].time - .999999f;
    FloatTrackTriggeringJob::Edges edges(edges_buffer);
    job.edges = &edges;
    ASSERT_TRUE(job.Run());

    ASSERT_EQ(edges.count(), _size);

    for (size_t i = 0; i < edges.count(); ++i) {
      EXPECT_FLOAT_EQ(edges[i].time, _expected[i].time - 1.f);
      EXPECT_EQ(edges[i].rising, _expected[i].rising);
    }
    TestEdgesExpectationBackward(job);
  }

  {  // Forward [-1, -eps]
    // Last edge is included if it time is not 1.f.
    const bool last_included = _expected[_size - 1].time != 1.f;

    job.from = -1.f;
    job.to = nexttowardf(0.f, -1.f);
    FloatTrackTriggeringJob::Edges edges(edges_buffer);
    job.edges = &edges;
    ASSERT_TRUE(job.Run());

    ASSERT_EQ(edges.count(), last_included ? _size : _size - 1);

    for (size_t i = 0; i < edges.count(); ++i) {
      EXPECT_FLOAT_EQ(edges[i].time, _expected[i].time - 1.f);
      EXPECT_EQ(edges[i].rising, _expected[i].rising);
    }
    TestEdgesExpectationBackward(job);
  }

  {  // Forward [-eps, ..]
    // Last edge is included if it time is 1.f.
    const bool last_included = _expected[_size - 1].time == 1.f;

    job.from = nexttowardf(0.f, -1.f);
    job.to = nexttowardf(_expected[0].time, 1.f);
    FloatTrackTriggeringJob::Edges edges(edges_buffer);
    job.edges = &edges;
    ASSERT_TRUE(job.Run());

    if (last_included) {
      ASSERT_EQ(edges.count(), 2u);

      EXPECT_FLOAT_EQ(edges[0].time, _expected[_size - 1].time - 1.f);
      EXPECT_EQ(edges[0].rising, _expected[_size - 1].rising);

      EXPECT_FLOAT_EQ(edges[1].time, _expected[0].time);
      EXPECT_EQ(edges[1].rising, _expected[0].rising);
    } else {
      ASSERT_EQ(edges.count(), 1u);

      EXPECT_FLOAT_EQ(edges[0].time, _expected[0].time);
      EXPECT_EQ(edges[0].rising, _expected[0].rising);
    }
    TestEdgesExpectationBackward(job);
  }

  {  // Forward, -46 to after last edge + -46.
    job.from = -46.f;
    job.to = nexttowardf(_expected[_size - 1].time + -46.f, 100.f);
    FloatTrackTriggeringJob::Edges edges(edges_buffer);
    job.edges = &edges;
    ASSERT_TRUE(job.Run());

    ASSERT_EQ(edges.count(), _size);

    for (size_t i = 0; i < edges.count(); ++i) {
      EXPECT_FLOAT_EQ(edges[i].time, _expected[i].time + -46.f);
      EXPECT_EQ(edges[i].rising, _expected[i].rising);
    }
    TestEdgesExpectationBackward(job);
  }

  {  // Forward, -46 to before last edge + -46.
    job.from = -46.f;
    job.to = nexttowardf(_expected[_size - 1].time + -46.f, -100.f);
    FloatTrackTriggeringJob::Edges edges(edges_buffer);
    job.edges = &edges;
    ASSERT_TRUE(job.Run());

    ASSERT_EQ(edges.count(), _size - 1);

    for (size_t i = 0; i < edges.count(); ++i) {
      EXPECT_FLOAT_EQ(edges[i].time, _expected[i].time + -46.f);
      EXPECT_EQ(edges[i].rising, _expected[i].rising);
    }
    TestEdgesExpectationBackward(job);
  }

  {  // Forward, -46 to last edge + -46.
    const bool last_included = _expected[_size - 1].time == 1.f;

    job.from = -46.f;
    job.to = _expected[_size - 1].time + -46.f;
    FloatTrackTriggeringJob::Edges edges(edges_buffer);
    job.edges = &edges;
    ASSERT_TRUE(job.Run());

    ASSERT_EQ(edges.count(), last_included ? _size : _size - 1);

    for (size_t i = 0; i < edges.count(); ++i) {
      EXPECT_FLOAT_EQ(edges[i].time, _expected[i].time + -46.f);
      EXPECT_EQ(edges[i].rising, _expected[i].rising);
    }
    TestEdgesExpectationBackward(job);
  }

  {  // Randomized
     //   std::vector<FloatTrackTriggeringJob::Edge> vedges;
     //   std::vector<float> vtimes;
     //    vtimes.push_back(0.f);
    const float kMaxRange = 2.f;
    const size_t kMaxIterations = 100000;
    float time = 0.f;
    bool rising = false;
    bool init = false;
    for (size_t i = 0; i < kMaxIterations; ++i) {
      // Finds new evaluation range
      float new_time =
          time +
          kMaxRange * (1.f - 2.f * static_cast<float>(rand()) / RAND_MAX);

      switch (rand() % 20) {
        case 0: {
          // Set time to a keyframe time.
          new_time = _expected[rand() % _size].time + floorf(new_time);
          break;
        }
        case 1: {
          // Set time to after a keyframe time.
          new_time = nexttowardf(
              _expected[rand() % _size].time + floorf(new_time), 1e15f);
          break;
        }
        case 2: {
          // Set time to before a keyframe time.
          new_time = nexttowardf(
              _expected[rand() % _size].time + floorf(new_time), -1e15f);
          break;
        }
        default:
          break;
      }

      job.from = time;
      time = new_time;
      job.to = time;
      //  vtimes.push_back(time);

      FloatTrackTriggeringJob::Edges edges(edges_buffer);
      job.edges = &edges;

      ASSERT_TRUE(job.Run());

      // Successive edges should always be opposed, whichever direction the
      // time is going.
      for (size_t e = 0; e < edges.count(); ++e) {
        // vedges.push_back(edges[e]);
        if (!init) {
          rising = edges[e].rising;
          init = true;
        } else {
          //   if (rising == edges[e].rising) {
          // assert(false);
          //     break;
          //   }
          ASSERT_TRUE(rising != edges[e].rising);
          rising = edges[e].rising;
        }
      }
    }
  }
  ozz::memory::default_allocator()->Delete(track);
}

template <size_t _size>
inline void TestEdgesExpectation(
    const ozz::animation::offline::RawFloatTrack& _raw_track, float _threshold,
    const FloatTrackTriggeringJob::Edge (&_expected)[_size]) {
  OZZ_STATIC_ASSERT(_size >= 2);
  TestEdgesExpectation(_raw_track, _threshold, _expected, _size);
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
    TestEdgesExpectation(raw_track, -1.f, expected);
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

    ASSERT_TRUE(job.Run());

    ASSERT_EQ(edges.count(), 2u);

    EXPECT_FLOAT_EQ(edges[0].time, .5f);
    EXPECT_EQ(edges[0].rising, true);
    EXPECT_FLOAT_EQ(edges[1].time, 1.f);
    EXPECT_EQ(edges[1].rising, false);
  }

  {  // Bottom of range is included
    job.threshold = -1.f;
    FloatTrackTriggeringJob::Edges edges(edges_buffer);
    job.edges = &edges;

    ASSERT_TRUE(job.Run());

    ASSERT_EQ(edges.count(), 2u);

    EXPECT_FLOAT_EQ(edges[0].time, .5f);
    EXPECT_EQ(edges[0].rising, true);
    EXPECT_FLOAT_EQ(edges[1].time, 1.f);
    EXPECT_EQ(edges[1].rising, false);
  }

  {  // Top range is excluded
    job.threshold = 1.f;
    FloatTrackTriggeringJob::Edges edges(edges_buffer);
    job.edges = &edges;

    ASSERT_TRUE(job.Run());

    ASSERT_EQ(edges.count(), 0u);
  }

  {  // In range
    job.threshold = 0.f;
    FloatTrackTriggeringJob::Edges edges(edges_buffer);
    job.edges = &edges;

    ASSERT_TRUE(job.Run());

    ASSERT_EQ(edges.count(), 2u);

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

    ASSERT_TRUE(job.Run());

    ASSERT_EQ(edges.count(), 0u);
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

    ASSERT_TRUE(job.Run());

    ASSERT_EQ(edges.count(), 2u);

    EXPECT_FLOAT_EQ(edges[0].time, .5f);
    EXPECT_EQ(edges[0].rising, true);
    EXPECT_FLOAT_EQ(edges[1].time, 1.f);
    EXPECT_EQ(edges[1].rising, false);
  }

  {  // Top range is excluded
    job.threshold = 1.f;
    FloatTrackTriggeringJob::Edges edges(edges_buffer);
    job.edges = &edges;

    ASSERT_TRUE(job.Run());

    ASSERT_EQ(edges.count(), 0u);
  }

  {  // Bottom range is included
    job.threshold = 0.f;
    FloatTrackTriggeringJob::Edges edges(edges_buffer);
    job.edges = &edges;

    ASSERT_TRUE(job.Run());

    ASSERT_EQ(edges.count(), 2u);

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

    ASSERT_TRUE(job.Run());

    ASSERT_EQ(edges.count(), 2u);

    EXPECT_FLOAT_EQ(edges[0].time, .375f);
    EXPECT_EQ(edges[0].rising, true);
    EXPECT_FLOAT_EQ(edges[1].time, .625f);
    EXPECT_EQ(edges[1].rising, false);
  }

  {  // Top range is excluded
    job.threshold = 1.f;
    FloatTrackTriggeringJob::Edges edges(edges_buffer);
    job.edges = &edges;

    ASSERT_TRUE(job.Run());

    ASSERT_EQ(edges.count(), 0u);
  }

  {  // In range
    job.threshold = 0.f;
    FloatTrackTriggeringJob::Edges edges(edges_buffer);
    job.edges = &edges;

    ASSERT_TRUE(job.Run());

    ASSERT_EQ(edges.count(), 2u);

    EXPECT_FLOAT_EQ(edges[0].time, .25f);
    EXPECT_EQ(edges[0].rising, true);
    EXPECT_FLOAT_EQ(edges[1].time, .75f);
    EXPECT_EQ(edges[1].rising, false);
  }

  {  // Bottom of range is included
    job.threshold = -1.f;
    FloatTrackTriggeringJob::Edges edges(edges_buffer);
    job.edges = &edges;

    ASSERT_TRUE(job.Run());

    ASSERT_EQ(edges.count(), 2u);

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

    ASSERT_TRUE(job.Run());

    ASSERT_EQ(edges.count(), 0u);
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

    ASSERT_TRUE(job.Run());

    ASSERT_EQ(edges.count(), 2u);

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

    ASSERT_TRUE(job.Run());

    ASSERT_EQ(edges.count(), 3u);

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

    ASSERT_EQ(edges.count(), 3u);  // But buffer isn't empty.

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

    ASSERT_EQ(edges.count(), 3u);  // But buffer is full.

    EXPECT_FLOAT_EQ(edges[0].time, .5f);
    EXPECT_EQ(edges[0].rising, true);

    EXPECT_FLOAT_EQ(edges[1].time, 1.f);
    EXPECT_EQ(edges[1].rising, false);

    EXPECT_FLOAT_EQ(edges[2].time, 1.5f);
    EXPECT_EQ(edges[2].rising, true);

    // 2nd pass, starting right after the end of the first one.
    job.from = nexttowardf(edges[2].time, job.to);
    ASSERT_TRUE(job.Run());  // Last pass

    ASSERT_EQ(edges.count(), 1u);  // But buffer isn't empty.

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
