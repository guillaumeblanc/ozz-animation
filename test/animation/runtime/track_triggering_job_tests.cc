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
#include "ozz/animation/offline/track_builder.h"
#include "ozz/animation/runtime/track.h"
#include "ozz/animation/runtime/track_triggering_job.h"
#include "ozz/base/gtest_helper.h"
#include "ozz/base/maths/gtest_math_helper.h"
#include "ozz/base/memory/unique_ptr.h"

using ozz::animation::FloatTrack;
using ozz::animation::TrackTriggeringJob;
using ozz::animation::offline::RawFloatTrack;
using ozz::animation::offline::RawTrackInterpolation;
using ozz::animation::offline::TrackBuilder;

TEST(JobValidity, TrackTriggeringJob) {
  // Builds track
  ozz::animation::offline::RawFloatTrack raw_track;
  TrackBuilder builder;
  ozz::unique_ptr<FloatTrack> track(builder(raw_track));
  ASSERT_TRUE(track);

  {  // Default is invalid
    TrackTriggeringJob job;
    EXPECT_FALSE(job.Validate());
    EXPECT_FALSE(job.Run());
  }

  {  // No track
    TrackTriggeringJob job;
    TrackTriggeringJob::Iterator iterator;
    job.iterator = &iterator;
    EXPECT_FALSE(job.Validate());
    EXPECT_FALSE(job.Run());
  }

  {  // No output
    TrackTriggeringJob job;
    job.track = track.get();
    EXPECT_FALSE(job.Validate());
    EXPECT_FALSE(job.Run());
  }

  {  // Valid
    TrackTriggeringJob job;
    job.track = track.get();
    TrackTriggeringJob::Iterator iterator;
    job.iterator = &iterator;
    EXPECT_TRUE(job.Validate());
    EXPECT_TRUE(job.Run());
  }

  {  // Valid
    TrackTriggeringJob job;
    job.from = 0.f;
    job.to = 1.f;
    job.track = track.get();
    TrackTriggeringJob::Iterator iterator;
    job.iterator = &iterator;
    EXPECT_TRUE(job.Validate());
    EXPECT_TRUE(job.Run());
  }

  {  // Empty output is valid
    TrackTriggeringJob job;
    job.track = track.get();
    TrackTriggeringJob::Iterator iterator;
    job.iterator = &iterator;
    EXPECT_TRUE(job.Validate());
    EXPECT_TRUE(job.Run());
  }
}

TEST(Default, TrackEdgeTriggerJob) {
  FloatTrack default_track;
  TrackTriggeringJob job;
  job.track = &default_track;
  TrackTriggeringJob::Iterator iterator;
  job.iterator = &iterator;
  ASSERT_TRUE(job.Run());
  EXPECT_EQ(iterator, job.end());
}

TEST(Empty, TrackEdgeTriggerJob) {
  TrackBuilder builder;

  // Builds track
  ozz::animation::offline::RawFloatTrack raw_track;
  ozz::unique_ptr<FloatTrack> track(builder(raw_track));
  ASSERT_TRUE(track);

  TrackTriggeringJob job;
  job.track = track.get();
  job.threshold = 1.f;
  job.from = 0.f;
  job.to = 1.f;
  TrackTriggeringJob::Iterator iterator;
  job.iterator = &iterator;

  ASSERT_TRUE(job.Run());
  EXPECT_EQ(iterator, job.end());
}

TEST(Iterator, TrackEdgeTriggerJob) {
  TrackBuilder builder;

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
  ozz::unique_ptr<FloatTrack> track(builder(raw_track));
  ASSERT_TRUE(track);

  TrackTriggeringJob job;
  job.track = track.get();
  job.threshold = 1.f;

  job.from = 0.f;
  job.to = 2.f;
  TrackTriggeringJob::Iterator iterator;
  job.iterator = &iterator;
  ASSERT_TRUE(job.Run());

  EXPECT_TRUE(iterator == iterator);
  EXPECT_FALSE(iterator != iterator);
  EXPECT_TRUE(job.end() == job.end());
  EXPECT_FALSE(job.end() != job.end());
  EXPECT_TRUE(iterator != job.end());
  EXPECT_FALSE(iterator == job.end());

  TrackTriggeringJob::Iterator iterator_cpy(iterator);
  EXPECT_TRUE(iterator == iterator_cpy);
  EXPECT_FALSE(iterator != iterator_cpy);

  TrackTriggeringJob::Iterator default_iterator;
  EXPECT_TRUE(default_iterator == default_iterator);
  EXPECT_FALSE(default_iterator != default_iterator);
  EXPECT_TRUE(default_iterator != iterator);
  EXPECT_FALSE(default_iterator == iterator);
  EXPECT_TRUE(default_iterator != job.end());
  EXPECT_FALSE(default_iterator == job.end());

  {  // Other jobs
    TrackTriggeringJob job2;
    job2.track = track.get();
    job2.threshold = 1.f;

    job2.from = 0.f;
    job2.to = 1.f;
    TrackTriggeringJob::Iterator iterator2;
    job2.iterator = &iterator2;
    ASSERT_TRUE(job2.Run());

    EXPECT_TRUE(iterator2 != iterator);
    EXPECT_FALSE(iterator2 == iterator);
  }

  {  // *
    EXPECT_TRUE((*iterator).rising);
    TrackTriggeringJob::Edge edge;
    EXPECT_ASSERTION(edge = *job.end(), "Can't dereference end iterator");
    (void)edge;
  }

  {  // ->
    EXPECT_TRUE(iterator->rising);
    bool rising;
    EXPECT_ASSERTION(rising = job.end()->rising,
                     "Can't dereference end iterator");
    (void)rising;
  }

  {  // Pre increment
    TrackTriggeringJob::Iterator iterator_cpy1(iterator);
    TrackTriggeringJob::Iterator iterator_cpy2(iterator);

    ++iterator_cpy1;
    EXPECT_TRUE(iterator_cpy1 != iterator_cpy2);
    EXPECT_FALSE(iterator_cpy1 == iterator_cpy2);

    ++iterator_cpy2;
    EXPECT_TRUE(iterator_cpy1 == iterator_cpy2);
    EXPECT_FALSE(iterator_cpy1 != iterator_cpy2);

    EXPECT_TRUE(++iterator_cpy1 != iterator_cpy2++);

    EXPECT_ASSERTION(++job.end(), "Can't increment end iterator.");
  }

  {  // Post increment
    TrackTriggeringJob::Iterator iterator_cpy1(iterator);
    TrackTriggeringJob::Iterator iterator_cpy2(iterator);

    iterator_cpy1++;
    EXPECT_TRUE(iterator_cpy1 != iterator_cpy2);
    EXPECT_FALSE(iterator_cpy1 == iterator_cpy2);

    iterator_cpy2++;
    EXPECT_TRUE(iterator_cpy1 == iterator_cpy2);
    EXPECT_FALSE(iterator_cpy1 != iterator_cpy2);

    EXPECT_TRUE((++iterator_cpy1)->rising != (iterator_cpy2++)->rising);
    EXPECT_TRUE(iterator_cpy1->rising == iterator_cpy2->rising);

    EXPECT_ASSERTION(job.end()++, "Can't increment end iterator.");
  }

  {  // Assignment
    TrackTriggeringJob::Iterator iterator_cpy1(iterator);
    TrackTriggeringJob::Iterator iterator_cpy2(iterator);

    ++iterator_cpy1;
    EXPECT_TRUE(iterator_cpy1 != iterator_cpy2);

    iterator_cpy2 = iterator_cpy1;
    EXPECT_TRUE(iterator_cpy1 == iterator_cpy2);

    ++iterator_cpy1;
    ++iterator_cpy2;
    EXPECT_TRUE(iterator_cpy1 == iterator_cpy2);
  }

  {  // Loop
    TrackTriggeringJob::Iterator iterator_cpy1(iterator);
    TrackTriggeringJob::Iterator iterator_cpy2(iterator);

    for (; iterator_cpy1 != job.end(); ++iterator_cpy1, iterator_cpy2++) {
    }
    EXPECT_TRUE(iterator_cpy2 == job.end());
  }
}

TEST(NoRange, TrackEdgeTriggerJob) {
  TrackBuilder builder;

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
  ozz::unique_ptr<FloatTrack> track(builder(raw_track));
  ASSERT_TRUE(track);

  TrackTriggeringJob job;
  job.track = track.get();
  job.threshold = 1.f;

  {  // Forward [0., 0.[
    job.from = 0.f;
    job.to = 0.f;
    TrackTriggeringJob::Iterator iterator;
    job.iterator = &iterator;
    ASSERT_TRUE(job.Run());

    EXPECT_TRUE(iterator == job.end());
  }

  {  // Forward [.1, .1]
    job.from = .1f;
    job.to = .1f;
    TrackTriggeringJob::Iterator iterator;
    job.iterator = &iterator;
    ASSERT_TRUE(job.Run());

    EXPECT_TRUE(iterator == job.end());
  }

  {  // Forward [.5, .5[
    job.from = .5f;
    job.to = .5f;
    TrackTriggeringJob::Iterator iterator;
    job.iterator = &iterator;
    ASSERT_TRUE(job.Run());

    EXPECT_TRUE(iterator == job.end());
  }

  {  // Forward [1., 1.]
    job.from = 1.f;
    job.to = 1.f;
    TrackTriggeringJob::Iterator iterator;
    job.iterator = &iterator;
    ASSERT_TRUE(job.Run());

    EXPECT_TRUE(iterator == job.end());
  }

  {  // Forward [-.5, -.5[
    job.from = -.5f;
    job.to = -.5f;
    TrackTriggeringJob::Iterator iterator;
    job.iterator = &iterator;
    ASSERT_TRUE(job.Run());

    EXPECT_TRUE(iterator == job.end());
  }
}

size_t CountEdges(TrackTriggeringJob::Iterator _begin,
                  const TrackTriggeringJob::Iterator& _end) {
  size_t count = 0;
  for (; _begin != _end; ++_begin, ++count) {
  }
  return count;
}

void TestEdgesExpectationBackward(TrackTriggeringJob::Iterator _fw_iterator,
                                  const TrackTriggeringJob& _fw_job) {
  // Compute forward edges;
  ozz::vector<TrackTriggeringJob::Edge> fw_edges;
  for (; _fw_iterator != _fw_job.end(); ++_fw_iterator) {
    fw_edges.push_back(*_fw_iterator);
  }

  // Setup backward job from forward one.
  TrackTriggeringJob bw_job = _fw_job;
  std::swap(bw_job.from, bw_job.to);
  TrackTriggeringJob::Iterator bw_iterator;
  bw_job.iterator = &bw_iterator;
  ASSERT_TRUE(bw_job.Run());

  // Compare forward and backward iterations.
  ASSERT_EQ(fw_edges.size(), CountEdges(bw_iterator, bw_job.end()));
  for (ozz::vector<TrackTriggeringJob::Edge>::const_reverse_iterator fw_rit =
           fw_edges.rbegin();
       fw_rit != fw_edges.rend(); ++fw_rit, ++bw_iterator) {
    EXPECT_FLOAT_EQ(fw_rit->ratio, bw_iterator->ratio);
    EXPECT_EQ(fw_rit->rising, !bw_iterator->rising);
  }
}

void TestEdgesExpectation(
    const ozz::animation::offline::RawFloatTrack& _raw_track, float _threshold,
    const TrackTriggeringJob::Edge* _expected, size_t _size) {
  assert(_size >= 2);

  // Builds track
  ozz::unique_ptr<FloatTrack> track(TrackBuilder()(_raw_track));
  ASSERT_TRUE(track);

  TrackTriggeringJob job;
  job.track = track.get();
  job.threshold = _threshold;

  {  // Forward [0, 1]
    job.from = 0.f;
    job.to = 1.f;
    TrackTriggeringJob::Iterator iterator;
    job.iterator = &iterator;
    ASSERT_TRUE(job.Run());

    ASSERT_EQ(CountEdges(iterator, job.end()), _size);

    TrackTriggeringJob::Iterator it = iterator;
    for (size_t i = 0; it != job.end(); ++it, ++i) {
      EXPECT_FLOAT_EQ(it->ratio, _expected[i].ratio);
      EXPECT_EQ(it->rising, _expected[i].rising);
    }

    TestEdgesExpectationBackward(iterator, job);
  }

  {  // Forward [1, 2]
    job.from = 1.f;
    job.to = 2.f;
    TrackTriggeringJob::Iterator iterator;
    job.iterator = &iterator;
    ASSERT_TRUE(job.Run());

    ASSERT_EQ(CountEdges(iterator, job.end()), _size);

    TrackTriggeringJob::Iterator it = iterator;
    for (size_t i = 0; it != job.end(); ++it, ++i) {
      EXPECT_FLOAT_EQ(it->ratio, _expected[i].ratio + 1.f);
      EXPECT_EQ(it->rising, _expected[i].rising);
    }
    TestEdgesExpectationBackward(iterator, job);
  }

  {  // Forward [0, 3]
    job.from = 0.f;
    job.to = 3.f;
    TrackTriggeringJob::Iterator iterator;
    job.iterator = &iterator;
    ASSERT_TRUE(job.Run());

    ASSERT_EQ(CountEdges(iterator, job.end()), _size * 3);

    TrackTriggeringJob::Iterator it = iterator;
    for (size_t i = 0; it != job.end(); ++it, ++i) {
      const size_t ie = i % _size;
      const float loops = static_cast<float>(i / _size);
      EXPECT_FLOAT_EQ(it->ratio, _expected[ie].ratio + loops);
      EXPECT_EQ(it->rising, _expected[ie].rising);
    }
    TestEdgesExpectationBackward(iterator, job);
  }

  {  // Forward, first edge to last, last can be included.

    // Last edge is included if it ratio is 1.f.
    const bool last_included = _expected[_size - 1].ratio == 1.f;

    job.from = _expected[0].ratio;
    job.to = _expected[_size - 1].ratio;
    TrackTriggeringJob::Iterator iterator;
    job.iterator = &iterator;
    ASSERT_TRUE(job.Run());

    ASSERT_EQ(CountEdges(iterator, job.end()),
              last_included ? _size : _size - 1);

    TrackTriggeringJob::Iterator it = iterator;
    for (size_t i = 0; it != job.end(); ++it, ++i) {
      EXPECT_FLOAT_EQ(it->ratio, _expected[i].ratio);
      EXPECT_EQ(it->rising, _expected[i].rising);
    }
    TestEdgesExpectationBackward(iterator, job);
  }

  {  // Forward, after first edge to 1.
    job.from = nexttowardf(_expected[0].ratio, 1.f);
    job.to = 1.f;
    TrackTriggeringJob::Iterator iterator;
    job.iterator = &iterator;
    ASSERT_TRUE(job.Run());

    ASSERT_EQ(CountEdges(iterator, job.end()), _size - 1);

    TrackTriggeringJob::Iterator it = iterator;
    for (size_t i = 0; it != job.end(); ++it, ++i) {
      EXPECT_FLOAT_EQ(it->ratio, _expected[i + 1].ratio);
      EXPECT_EQ(it->rising, _expected[i + 1].rising);
    }
    TestEdgesExpectationBackward(iterator, job);
  }

  {  // Forward, 0 to first edge.
    job.from = 0.f;
    job.to = _expected[0].ratio;
    TrackTriggeringJob::Iterator iterator;
    job.iterator = &iterator;
    ASSERT_TRUE(job.Run());

    ASSERT_EQ(CountEdges(iterator, job.end()), 0u);
    TestEdgesExpectationBackward(iterator, job);
  }

  {  // Forward, 0 to after first edge.
    job.from = 0.f;
    job.to = nexttowardf(_expected[0].ratio, 1.f);
    TrackTriggeringJob::Iterator iterator;
    job.iterator = &iterator;
    ASSERT_TRUE(job.Run());

    ASSERT_EQ(CountEdges(iterator, job.end()), 1u);

    EXPECT_FLOAT_EQ(iterator->ratio, _expected[0].ratio);
    EXPECT_EQ(iterator->rising, _expected[0].rising);

    TestEdgesExpectationBackward(iterator, job);
  }

  {  // Forward, 0 to before last edge.
    job.from = 0.f;
    job.to = nexttowardf(_expected[_size - 1].ratio, 0.f);
    TrackTriggeringJob::Iterator iterator;
    job.iterator = &iterator;
    ASSERT_TRUE(job.Run());

    ASSERT_EQ(CountEdges(iterator, job.end()), _size - 1);

    TrackTriggeringJob::Iterator it = iterator;
    for (size_t i = 0; it != job.end(); ++it, ++i) {
      EXPECT_FLOAT_EQ(it->ratio, _expected[i].ratio);
      EXPECT_EQ(it->rising, _expected[i].rising);
    }
    TestEdgesExpectationBackward(iterator, job);
  }

  {  // Forward, 0 to after last edge.
    job.from = 0.f;
    job.to = nexttowardf(_expected[_size - 1].ratio, 1.f);
    TrackTriggeringJob::Iterator iterator;
    job.iterator = &iterator;
    ASSERT_TRUE(job.Run());

    ASSERT_EQ(CountEdges(iterator, job.end()), _size);

    TrackTriggeringJob::Iterator it = iterator;
    for (size_t i = 0; it != job.end(); ++it, ++i) {
      EXPECT_FLOAT_EQ(it->ratio, _expected[i].ratio);
      EXPECT_EQ(it->rising, _expected[i].rising);
    }
    TestEdgesExpectationBackward(iterator, job);
  }

  {  // Forward, 1 to after last edge + 1.
    job.from = 1.f;
    job.to = nexttowardf(_expected[_size - 1].ratio + 1.f, 2.f);
    TrackTriggeringJob::Iterator iterator;
    job.iterator = &iterator;
    ASSERT_TRUE(job.Run());

    ASSERT_EQ(CountEdges(iterator, job.end()), _size);

    TrackTriggeringJob::Iterator it = iterator;
    for (size_t i = 0; it != job.end(); ++it, ++i) {
      EXPECT_FLOAT_EQ(it->ratio, _expected[i].ratio + 1.f);
      EXPECT_EQ(it->rising, _expected[i].rising);
    }
    TestEdgesExpectationBackward(iterator, job);
  }

  {  // Forward, 46 to after last edge + 46.
    job.from = 46.f;
    job.to = nexttowardf(_expected[_size - 1].ratio + 46.f, 100.f);
    TrackTriggeringJob::Iterator iterator;
    job.iterator = &iterator;
    ASSERT_TRUE(job.Run());

    ASSERT_EQ(CountEdges(iterator, job.end()), _size);

    TrackTriggeringJob::Iterator it = iterator;
    for (size_t i = 0; it != job.end(); ++it, ++i) {
      EXPECT_FLOAT_EQ(it->ratio, _expected[i].ratio + 46.f);
      EXPECT_EQ(it->rising, _expected[i].rising);
    }
    TestEdgesExpectationBackward(iterator, job);
  }

  {  // Forward, 46 to before last edge + 46.
    job.from = 46.f;
    job.to = nexttowardf(_expected[_size - 1].ratio + 46.f, -100.f);
    TrackTriggeringJob::Iterator iterator;
    job.iterator = &iterator;
    ASSERT_TRUE(job.Run());

    ASSERT_EQ(CountEdges(iterator, job.end()), _size - 1);

    TrackTriggeringJob::Iterator it = iterator;
    for (size_t i = 0; it != job.end(); ++it, ++i) {
      EXPECT_FLOAT_EQ(it->ratio, _expected[i].ratio + 46.f);
      EXPECT_EQ(it->rising, _expected[i].rising);
    }
    TestEdgesExpectationBackward(iterator, job);
  }

  {  // Forward, 46 to last edge + 46.
    const bool last_included = _expected[_size - 1].ratio == 1.f;

    job.from = 46.f;
    job.to = _expected[_size - 1].ratio + 46.f;
    TrackTriggeringJob::Iterator iterator;
    job.iterator = &iterator;
    ASSERT_TRUE(job.Run());

    ASSERT_EQ(CountEdges(iterator, job.end()),
              last_included ? _size : _size - 1);

    TrackTriggeringJob::Iterator it = iterator;
    for (size_t i = 0; it != job.end(); ++it, ++i) {
      EXPECT_FLOAT_EQ(it->ratio, _expected[i].ratio + 46.f);
      EXPECT_EQ(it->rising, _expected[i].rising);
    }
    TestEdgesExpectationBackward(iterator, job);
  }

  {  // Forward, 0 to before last edge + 1
    const bool last_included = _expected[_size - 1].ratio == 1.f;

    job.from = 0.f;
    job.to = _expected[_size - 1].ratio + 1.f;
    TrackTriggeringJob::Iterator iterator;
    job.iterator = &iterator;
    ASSERT_TRUE(job.Run());

    ASSERT_EQ(CountEdges(iterator, job.end()),
              last_included ? _size * 2 : _size * 2 - 1);

    TrackTriggeringJob::Iterator it = iterator;
    for (size_t i = 0; i < _size; ++i, ++it) {
      EXPECT_FLOAT_EQ(it->ratio, _expected[i].ratio);
      EXPECT_EQ(it->rising, _expected[i].rising);
    }
    for (size_t i = _size; i < (last_included ? _size * 2 : _size * 2 - 1);
         ++i, ++it) {
      EXPECT_FLOAT_EQ(it->ratio, _expected[i - _size].ratio + 1.f);
      EXPECT_EQ(it->rising, _expected[i - _size].rising);
    }
    TestEdgesExpectationBackward(iterator, job);
  }

  // Negative times

  {  // Forward [-1, 0]
    job.from = -1.f;
    job.to = 0.f;
    TrackTriggeringJob::Iterator iterator;
    job.iterator = &iterator;
    ASSERT_TRUE(job.Run());

    ASSERT_EQ(CountEdges(iterator, job.end()), _size);

    TrackTriggeringJob::Iterator it = iterator;
    for (size_t i = 0; it != job.end(); ++it, ++i) {
      EXPECT_FLOAT_EQ(it->ratio, _expected[i].ratio - 1.f);
      EXPECT_EQ(it->rising, _expected[i].rising);
    }
    TestEdgesExpectationBackward(iterator, job);
  }

  {  // Forward [-2, -1]
    job.from = -2.f;
    job.to = -1.f;
    TrackTriggeringJob::Iterator iterator;
    job.iterator = &iterator;
    ASSERT_TRUE(job.Run());

    ASSERT_EQ(CountEdges(iterator, job.end()), _size);

    TrackTriggeringJob::Iterator it = iterator;
    for (size_t i = 0; it != job.end(); ++it, ++i) {
      EXPECT_FLOAT_EQ(it->ratio, _expected[i].ratio - 2.f);
      EXPECT_EQ(it->rising, _expected[i].rising);
    }
    TestEdgesExpectationBackward(iterator, job);
  }

  {  // Forward [-1, 1]
    job.from = -1.f;
    job.to = 1.f;
    TrackTriggeringJob::Iterator iterator;
    job.iterator = &iterator;
    ASSERT_TRUE(job.Run());

    ASSERT_EQ(CountEdges(iterator, job.end()), _size * 2);

    TrackTriggeringJob::Iterator it = iterator;
    for (size_t i = 0; i < _size; ++i, ++it) {
      EXPECT_FLOAT_EQ(it->ratio, _expected[i].ratio - 1.f);
      EXPECT_EQ(it->rising, _expected[i].rising);
    }
    for (size_t i = 0; i < _size; ++i, ++it) {
      EXPECT_FLOAT_EQ(it->ratio, _expected[i].ratio);
      EXPECT_EQ(it->rising, _expected[i].rising);
    }
    TestEdgesExpectationBackward(iterator, job);
  }

  {  // Forward, -1 to first edge.
    job.from = -1.f;
    job.to = _expected[0].ratio - 1.f;
    TrackTriggeringJob::Iterator iterator;
    job.iterator = &iterator;
    ASSERT_TRUE(job.Run());

    ASSERT_EQ(CountEdges(iterator, job.end()), 0u);
    TestEdgesExpectationBackward(iterator, job);
  }

  {  // Forward, -1 to after last edge.
    job.from = -1.f;
    job.to = _expected[_size - 1].ratio - .999999f;
    TrackTriggeringJob::Iterator iterator;
    job.iterator = &iterator;
    ASSERT_TRUE(job.Run());

    ASSERT_EQ(CountEdges(iterator, job.end()), _size);

    TrackTriggeringJob::Iterator it = iterator;
    for (size_t i = 0; it != job.end(); ++it, ++i) {
      EXPECT_FLOAT_EQ(it->ratio, _expected[i].ratio - 1.f);
      EXPECT_EQ(it->rising, _expected[i].rising);
    }
    TestEdgesExpectationBackward(iterator, job);
  }

  {  // Forward [-1, -eps]
    // Last edge is included if it ratio is not 1.f.
    const bool last_included = _expected[_size - 1].ratio != 1.f;

    job.from = -1.f;
    job.to = nexttowardf(0.f, -1.f);
    TrackTriggeringJob::Iterator iterator;
    job.iterator = &iterator;
    ASSERT_TRUE(job.Run());

    ASSERT_EQ(CountEdges(iterator, job.end()),
              last_included ? _size : _size - 1);

    TrackTriggeringJob::Iterator it = iterator;
    for (size_t i = 0; it != job.end(); ++it, ++i) {
      EXPECT_FLOAT_EQ(it->ratio, _expected[i].ratio - 1.f);
      EXPECT_EQ(it->rising, _expected[i].rising);
    }
    TestEdgesExpectationBackward(iterator, job);
  }

  {  // Forward [-eps, ..]
    // Last edge is included if it ratio is 1.f.
    const bool last_included = _expected[_size - 1].ratio == 1.f;

    job.from = nexttowardf(0.f, -1.f);
    job.to = nexttowardf(_expected[0].ratio, 1.f);
    TrackTriggeringJob::Iterator iterator;
    job.iterator = &iterator;
    ASSERT_TRUE(job.Run());

    if (last_included) {
      ASSERT_EQ(CountEdges(iterator, job.end()), 2u);

      TrackTriggeringJob::Iterator it = iterator;
      EXPECT_FLOAT_EQ(it->ratio, _expected[_size - 1].ratio - 1.f);
      EXPECT_EQ(it->rising, _expected[_size - 1].rising);
      ++it;

      EXPECT_FLOAT_EQ(it->ratio, _expected[0].ratio);
      EXPECT_EQ(it->rising, _expected[0].rising);
    } else {
      ASSERT_EQ(CountEdges(iterator, job.end()), 1u);

      TrackTriggeringJob::Iterator it = iterator;
      EXPECT_FLOAT_EQ(it->ratio, _expected[0].ratio);
      EXPECT_EQ(it->rising, _expected[0].rising);
    }
    TestEdgesExpectationBackward(iterator, job);
  }

  {  // Forward, -46 to after last edge + -46.
    job.from = -46.f;
    job.to = nexttowardf(_expected[_size - 1].ratio + -46.f, 100.f);
    TrackTriggeringJob::Iterator iterator;
    job.iterator = &iterator;
    ASSERT_TRUE(job.Run());

    ASSERT_EQ(CountEdges(iterator, job.end()), _size);

    TrackTriggeringJob::Iterator it = iterator;
    for (size_t i = 0; it != job.end(); ++it, ++i) {
      EXPECT_FLOAT_EQ(it->ratio, _expected[i].ratio + -46.f);
      EXPECT_EQ(it->rising, _expected[i].rising);
    }
    TestEdgesExpectationBackward(iterator, job);
  }

  {  // Forward, -46 to before last edge + -46.
    job.from = -46.f;
    job.to = nexttowardf(_expected[_size - 1].ratio + -46.f, -100.f);
    TrackTriggeringJob::Iterator iterator;
    job.iterator = &iterator;
    ASSERT_TRUE(job.Run());

    ASSERT_EQ(CountEdges(iterator, job.end()), _size - 1);

    TrackTriggeringJob::Iterator it = iterator;
    for (size_t i = 0; it != job.end(); ++it, ++i) {
      EXPECT_FLOAT_EQ(it->ratio, _expected[i].ratio + -46.f);
      EXPECT_EQ(it->rising, _expected[i].rising);
    }
    TestEdgesExpectationBackward(iterator, job);
  }

  {  // Forward, -46 to last edge + -46.
    const bool last_included = _expected[_size - 1].ratio == 1.f;

    job.from = -46.f;
    job.to = _expected[_size - 1].ratio + -46.f;
    TrackTriggeringJob::Iterator iterator;
    job.iterator = &iterator;
    ASSERT_TRUE(job.Run());

    ASSERT_EQ(CountEdges(iterator, job.end()),
              last_included ? _size : _size - 1);

    TrackTriggeringJob::Iterator it = iterator;
    for (size_t i = 0; it != job.end(); ++it, ++i) {
      EXPECT_FLOAT_EQ(it->ratio, _expected[i].ratio + -46.f);
      EXPECT_EQ(it->rising, _expected[i].rising);
    }
    TestEdgesExpectationBackward(iterator, job);
  }

  {  // Forward [-21.3999996, -21.0000019]
    job.from = -21.3999996f;
    job.to = -21.0000019f;
    TrackTriggeringJob::Iterator iterator;
    job.iterator = &iterator;
    ASSERT_TRUE(job.Run());

    size_t found = 0;
    for (size_t i = 0; i < _size; ++i) {
      if (_expected[i].ratio - 22.f >= job.from &&
          _expected[i].ratio - 22.f < job.to) {
        found++;
      }
    }

    ASSERT_EQ(CountEdges(iterator, job.end()), found);

    TestEdgesExpectationBackward(iterator, job);
  }

  {  // Randomized tests forward/backward coherency
    const float kMaxRange = 10.f;
    const size_t kMaxIterations = 1000;
    for (size_t i = 0; i < kMaxIterations; ++i) {
      job.from =
          kMaxRange * (1.f - 2.f * static_cast<float>(rand()) / RAND_MAX);
      job.to = kMaxRange * (1.f - 2.f * static_cast<float>(rand()) / RAND_MAX);
      TrackTriggeringJob::Iterator iterator;
      job.iterator = &iterator;
      ASSERT_TRUE(job.Run());
      TestEdgesExpectationBackward(iterator, job);
    }
  }
  {  // Randomized tests rising/falling coherency
    const float kMaxRange = 2.f;
    const size_t kMaxIterations = 1000;
    float ratio = 0.f;
    bool rising = false;
    bool init = false;
    for (size_t i = 0; i < kMaxIterations; ++i) {
      // Finds new evaluation range
      float new_time =
          ratio +
          kMaxRange * (1.f - 2.f * static_cast<float>(rand()) / RAND_MAX);

      switch (rand() % 20) {
        case 0: {
          // Set ratio to a keyframe ratio.
          new_time = _expected[rand() % _size].ratio + floorf(new_time);
          break;
        }
        case 1: {
          // Set ratio to after a keyframe ratio.
          new_time = nexttowardf(
              _expected[rand() % _size].ratio + floorf(new_time), 1e15f);
          break;
        }
        case 2: {
          // Set ratio to before a keyframe ratio.
          new_time = nexttowardf(
              _expected[rand() % _size].ratio + floorf(new_time), -1e15f);
          break;
        }
        default:
          break;
      }

      job.from = ratio;
      ratio = new_time;
      job.to = ratio;

      TrackTriggeringJob::Iterator iterator;
      job.iterator = &iterator;

      ASSERT_TRUE(job.Run());

      // Successive edges should always be opposed, whichever direction the
      // ratio is going.
      for (const TrackTriggeringJob::Iterator end = job.end(); iterator != end;
           ++iterator) {
        const TrackTriggeringJob::Edge& edge = *iterator;
        if (!init) {
          rising = edge.rising;
          init = true;
        } else {
          //     if (rising == edges[e].rising) {
          // assert(false);
          //       break;
          //     }
          ASSERT_TRUE(rising != edge.rising);
          rising = edge.rising;
        }
      }
    }
  }
}

template <size_t _size>
inline void TestEdgesExpectation(
    const ozz::animation::offline::RawFloatTrack& _raw_track, float _threshold,
    const TrackTriggeringJob::Edge (&_expected)[_size]) {
  static_assert(_size >= 2, "Minimum 2 edges.");
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

    const TrackTriggeringJob::Edge expected[] = {{.5f, true}, {1.f, false}};
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

    const TrackTriggeringJob::Edge expected[] = {{0.f, false}, {.6f, true}};
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

    const TrackTriggeringJob::Edge expected[] = {{0.f, true}, {.5f, false}};
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

    const TrackTriggeringJob::Edge expected[] = {{.5f, true}, {1.f, false}};
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

    const TrackTriggeringJob::Edge expected0[] = {
        {.2f, true}, {.3f, false}, {.4f, true}, {.5f, false}};
    TestEdgesExpectation(raw_track, 0.f, expected0);

    const TrackTriggeringJob::Edge expected1[] = {{.2f, true}, {.3f, false}};
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

    const TrackTriggeringJob::Edge expected0[] = {{.25f, true}, {.75f, false}};
    TestEdgesExpectation(raw_track, 1.f, expected0);

    const TrackTriggeringJob::Edge expected1[] = {{.125f, true},
                                                  {.875f, false}};
    TestEdgesExpectation(raw_track, .5f, expected1);

    const TrackTriggeringJob::Edge expected2[] = {{.375f, true},
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

    const TrackTriggeringJob::Edge expected[] = {{0.f, false}, {.25f, true}};
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

    const TrackTriggeringJob::Edge expected[] = {{0.f, false}, {.25f, true}};
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

    const TrackTriggeringJob::Edge expected0[] = {{.5f, true}, {.75f, false}};
    TestEdgesExpectation(raw_track, 1.f, expected0);

    const TrackTriggeringJob::Edge expected1[] = {{.5f, true}, {.875f, false}};
    TestEdgesExpectation(raw_track, .5f, expected1);

    const TrackTriggeringJob::Edge expected2[] = {{.5f, true}, {.625f, false}};
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

    const TrackTriggeringJob::Edge expected0[] = {{.25f, true}, {1.f, false}};
    TestEdgesExpectation(raw_track, 1.f, expected0);

    const TrackTriggeringJob::Edge expected1[] = {{.125f, true}, {1.f, false}};
    TestEdgesExpectation(raw_track, .5f, expected1);

    const TrackTriggeringJob::Edge expected2[] = {{.375f, true}, {1.f, false}};
    TestEdgesExpectation(raw_track, 1.5f, expected2);
  }
}

TEST(StepThreshold, TrackEdgeTriggerJob) {
  TrackBuilder builder;

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
  ozz::unique_ptr<FloatTrack> track(builder(raw_track));
  ASSERT_TRUE(track);

  TrackTriggeringJob job;
  job.track = track.get();
  job.from = 0.f;
  job.to = 1.f;

  {  // In range
    job.threshold = .5f;
    TrackTriggeringJob::Iterator iterator;
    job.iterator = &iterator;

    ASSERT_TRUE(job.Run());

    ASSERT_EQ(CountEdges(iterator, job.end()), 2u);

    EXPECT_FLOAT_EQ(iterator->ratio, .5f);
    EXPECT_EQ(iterator->rising, true);
    ++iterator;
    EXPECT_FLOAT_EQ(iterator->ratio, 1.f);
    EXPECT_EQ(iterator->rising, false);
  }

  {  // Bottom of range is included
    job.threshold = -1.f;
    TrackTriggeringJob::Iterator iterator;
    job.iterator = &iterator;

    ASSERT_TRUE(job.Run());

    ASSERT_EQ(CountEdges(iterator, job.end()), 2u);

    EXPECT_FLOAT_EQ(iterator->ratio, .5f);
    EXPECT_EQ(iterator->rising, true);
    ++iterator;
    EXPECT_FLOAT_EQ(iterator->ratio, 1.f);
    EXPECT_EQ(iterator->rising, false);
  }

  {  // Top range is excluded
    job.threshold = 1.f;
    TrackTriggeringJob::Iterator iterator;
    job.iterator = &iterator;

    ASSERT_TRUE(job.Run());

    ASSERT_EQ(CountEdges(iterator, job.end()), 0u);
  }

  {  // In range
    job.threshold = 0.f;
    TrackTriggeringJob::Iterator iterator;
    job.iterator = &iterator;

    ASSERT_TRUE(job.Run());

    ASSERT_EQ(CountEdges(iterator, job.end()), 2u);

    EXPECT_FLOAT_EQ(iterator->ratio, .5f);
    EXPECT_EQ(iterator->rising, true);
    ++iterator;
    EXPECT_FLOAT_EQ(iterator->ratio, 1.f);
    EXPECT_EQ(iterator->rising, false);
  }

  {  // Out of range
    job.from = 0.f;
    job.to = 1.f;
    job.threshold = 2.f;
    TrackTriggeringJob::Iterator iterator;
    job.iterator = &iterator;

    ASSERT_TRUE(job.Run());

    ASSERT_EQ(CountEdges(iterator, job.end()), 0u);
  }
}

TEST(StepThresholdBool, TrackEdgeTriggerJob) {
  TrackBuilder builder;

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
  ozz::unique_ptr<FloatTrack> track(builder(raw_track));
  ASSERT_TRUE(track);

  TrackTriggeringJob job;
  job.track = track.get();
  job.from = 0.f;
  job.to = 1.f;

  {  // In range
    job.threshold = .5f;
    TrackTriggeringJob::Iterator iterator;
    job.iterator = &iterator;

    ASSERT_TRUE(job.Run());

    ASSERT_EQ(CountEdges(iterator, job.end()), 2u);

    EXPECT_FLOAT_EQ(iterator->ratio, .5f);
    EXPECT_EQ(iterator->rising, true);
    ++iterator;
    EXPECT_FLOAT_EQ(iterator->ratio, 1.f);
    EXPECT_EQ(iterator->rising, false);
  }

  {  // Top range is excluded
    job.threshold = 1.f;
    TrackTriggeringJob::Iterator iterator;
    job.iterator = &iterator;

    ASSERT_TRUE(job.Run());

    ASSERT_EQ(CountEdges(iterator, job.end()), 0u);
  }

  {  // Bottom range is included
    job.threshold = 0.f;
    TrackTriggeringJob::Iterator iterator;
    job.iterator = &iterator;

    ASSERT_TRUE(job.Run());

    ASSERT_EQ(CountEdges(iterator, job.end()), 2u);

    EXPECT_FLOAT_EQ(iterator->ratio, .5f);
    EXPECT_EQ(iterator->rising, true);
    ++iterator;
    EXPECT_FLOAT_EQ(iterator->ratio, 1.f);
    EXPECT_EQ(iterator->rising, false);
  }
}

TEST(LinearThreshold, TrackEdgeTriggerJob) {
  TrackBuilder builder;

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
  ozz::unique_ptr<FloatTrack> track(builder(raw_track));
  ASSERT_TRUE(track);

  TrackTriggeringJob job;
  job.track = track.get();
  job.from = 0.f;
  job.to = 1.f;

  {  // In range
    job.threshold = .5f;
    TrackTriggeringJob::Iterator iterator;
    job.iterator = &iterator;

    ASSERT_TRUE(job.Run());

    ASSERT_EQ(CountEdges(iterator, job.end()), 2u);

    EXPECT_FLOAT_EQ(iterator->ratio, .375f);
    EXPECT_EQ(iterator->rising, true);
    ++iterator;
    EXPECT_FLOAT_EQ(iterator->ratio, .625f);
    EXPECT_EQ(iterator->rising, false);
  }

  {  // Top range is excluded
    job.threshold = 1.f;
    TrackTriggeringJob::Iterator iterator;
    job.iterator = &iterator;

    ASSERT_TRUE(job.Run());

    ASSERT_EQ(CountEdges(iterator, job.end()), 0u);
  }

  {  // In range
    job.threshold = 0.f;
    TrackTriggeringJob::Iterator iterator;
    job.iterator = &iterator;

    ASSERT_TRUE(job.Run());

    ASSERT_EQ(CountEdges(iterator, job.end()), 2u);

    EXPECT_FLOAT_EQ(iterator->ratio, .25f);
    EXPECT_EQ(iterator->rising, true);
    ++iterator;
    EXPECT_FLOAT_EQ(iterator->ratio, .75f);
    EXPECT_EQ(iterator->rising, false);
  }

  {  // Bottom of range is included
    job.threshold = -1.f;
    TrackTriggeringJob::Iterator iterator;
    job.iterator = &iterator;

    ASSERT_TRUE(job.Run());

    ASSERT_EQ(CountEdges(iterator, job.end()), 2u);

    EXPECT_FLOAT_EQ(iterator->ratio, 0.f);
    EXPECT_EQ(iterator->rising, true);
    ++iterator;
    EXPECT_FLOAT_EQ(iterator->ratio, 1.f);
    EXPECT_EQ(iterator->rising, false);
  }

  {  // Out of range
    job.from = 0.f;
    job.to = 1.f;
    job.threshold = 2.f;
    TrackTriggeringJob::Iterator iterator;
    job.iterator = &iterator;

    ASSERT_TRUE(job.Run());

    ASSERT_EQ(CountEdges(iterator, job.end()), 0u);
  }
}
