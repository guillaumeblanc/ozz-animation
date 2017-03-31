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

#include "ozz/animation/runtime/track_sampling_job.h"
#include "ozz/animation/runtime/track.h"
#include "ozz/base/maths/math_ex.h"

#include <algorithm>
#include <cassert>

namespace ozz {
namespace animation {
namespace internal {

template <typename _Track>
TrackSamplingJob<_Track>::TrackSamplingJob()
    : time(0.f), track(NULL), result(NULL) {}

template <typename _Track>
bool TrackSamplingJob<_Track>::Validate() const {
  bool success = true;
  success &= result != NULL;
  success &= track != NULL;
  return success;
}

template <typename _Track>
bool TrackSamplingJob<_Track>::Run() const {
  if (!Validate()) {
    return false;
  }

  // Clamps time in range [0,1].
  const float clamped_time = math::Clamp(0.f, time, 1.f);

  // Search keyframes to interpolate.
  const Range<const float> times = track->times();
  const Range<const typename _Track::ValueType> values = track->values();
  assert(times.Count() == values.Count());

  // Search for the first key frame with a time value greater than input time.
  const float* ptk1 = std::upper_bound(times.begin, times.end, clamped_time);

  // upper_bound returns "end" if time == end, so patch the selected
  // keyframe (ptk1) to be able enter the lerp algorithm.
  if (ptk1 == times.end) {
    --ptk1;
  }

  // Lerp relevant keys.
  const float tk0 = ptk1[-1];
  const float tk1 = ptk1[0];
  assert(clamped_time >= tk0 && tk0 != tk1);
  const typename _Track::ValueType* pvk1 = values.begin + (ptk1 - times.begin);
  const typename _Track::ValueType vk0 = pvk1[-1];
  const typename _Track::ValueType vk1 = pvk1[0];
  const float alpha = (clamped_time - tk0) / (tk1 - tk0);
  *result = math::Lerp(vk0, vk1, alpha);

  return true;
}

// Explicitly instantiate supported raw tracks.
template struct TrackSamplingJob<FloatTrack>;
template struct TrackSamplingJob<Float2Track>;
template struct TrackSamplingJob<Float3Track>;
template struct TrackSamplingJob<QuaternionTrack>;
}  // internal
}  // animation
}  // ozz
