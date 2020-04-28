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

#include "ozz/animation/runtime/track_triggering_job_trait.h"

#include "gtest/gtest.h"
#include "ozz/base/gtest_helper.h"
#include "ozz/base/maths/gtest_math_helper.h"
#include "ozz/base/memory/unique_ptr.h"

#include "ozz/animation/offline/raw_track.h"
#include "ozz/animation/offline/track_builder.h"

#include "ozz/animation/runtime/track.h"

#include <algorithm>

using ozz::animation::FloatTrack;
using ozz::animation::TrackTriggeringJob;
using ozz::animation::offline::RawFloatTrack;
using ozz::animation::offline::RawTrackInterpolation;
using ozz::animation::offline::TrackBuilder;

namespace {
bool IsRising(const TrackTriggeringJob::Edge& _edge) { return _edge.rising; }
}  // namespace

TEST(Algorithm, TrackEdgeTriggerJob) {
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

  {  // copy
    ozz::vector<TrackTriggeringJob::Edge> edges;
    std::copy(iterator, job.end(), std::back_inserter(edges));
    EXPECT_EQ(edges.size(), 4u);
  }

  {  // count
    ozz::vector<TrackTriggeringJob::Edge> edges;
    std::iterator_traits<TrackTriggeringJob::Iterator>::difference_type count =
        std::count_if(iterator, job.end(), IsRising);
    EXPECT_EQ(count, 2);

    std::iterator_traits<TrackTriggeringJob::Iterator>::difference_type
        count_end = std::count_if(job.end(), job.end(), IsRising);
    EXPECT_EQ(count_end, 0);
  }

  {  // for_each
    ozz::vector<TrackTriggeringJob::Edge> edges;
    std::for_each(iterator, job.end(), IsRising);
  }

  {  // find_if
    ozz::vector<TrackTriggeringJob::Edge> edges;
    TrackTriggeringJob::Iterator it_if =
        std::find_if(iterator, job.end(), IsRising);
    EXPECT_TRUE(it_if->rising);
  }
}
