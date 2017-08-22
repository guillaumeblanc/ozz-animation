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
#include "ozz/animation/runtime/track.h"

#include <algorithm>
#include <cassert>

namespace ozz {
namespace animation {

FloatTrackTriggeringJob::FloatTrackTriggeringJob()
    : from(0.f), to(0.f), threshold(0.f), track(NULL), edges(NULL) {}

bool FloatTrackTriggeringJob::Validate() const {
  bool valid = true;
  valid &= track != NULL;
  valid &= edges != NULL;
  return valid;
}

bool FloatTrackTriggeringJob::Run() const {
  if (!Validate()) {
    return false;
  }

  // Triggering can only happen in a valid range of time.
  if (from == to) {
    edges->Clear();
    return true;
  }

  // Search keyframes to interpolate.
  const Range<const float> times = track->times();
  const Range<const float> values = track->values();
  const Range<const uint8_t> steps = track->steps();
  assert(times.Count() == values.Count());

  // from and to are exchanged if time is going backward, so the algorithm
  // remains the same. Output is reversed after on exit though.
  const bool forward = to > from;
  const float rfrom = forward ? from : to;
  const float rto = forward ? to : from;

  // If from is >= 1.f, then we consider as within a loop, meaning we should
  // detect edges between the last and first keys.
  float prev_loop_val = values[values.Count() - 1];

  // Loops in the global evaluation range, divided in local subranges [0,1].
  Edge* edges_end = edges->begin;
  for (float rcurr = floorf(rfrom), lcurr = rfrom - rcurr;
       rcurr < rto;    // Up to reaching the end time.
       rcurr += 1.f,   // Next loop.
       lcurr = 0.f) {  // Always starts from 0 (local) on the following loops

    const float lto = math::Min(rto - rcurr, 1.f);
    assert(lcurr >= 0.f && lcurr <= 1.f && lto > lcurr && lto <= 1.f);

    // Finds iteration starting point. Key at t=0 is the first key.
    const float* ptfrom =
        lcurr == 0.f ? times.begin
                     : std::lower_bound(times.begin + 1, times.end, lcurr);

    for (size_t i = ptfrom - times.begin; i < times.Count(); ++i) {
      // Find relevant keyframes value around i.
      const float vk0 = i == 0 ? prev_loop_val : values[i - 1];
      const float vk1 = values[i];

      // Stores value for next outer loop.
      prev_loop_val = vk1;

      bool rising = false;
      bool detected = false;
      if (vk0 < threshold && vk1 >= threshold) {
        // Rising edge
        rising = forward;
        detected = true;
      } else if (vk0 >= threshold && vk1 < threshold) {
        // Falling edge
        rising = !forward;
        detected = true;
      }

      if (detected) {
        Edge edge;
        edge.rising = rising;

        const bool step = (steps[i / 8] & (1 << (i & 7))) != 0;
        if (step) {
          edge.time = times[i];
        } else {
          assert(vk0 != vk1);

          // Finds where the curve crosses threshold value.
          // This is the lerp equation, where we know the result and look for
          // alpha, aka unlerp.
          const float alpha = (threshold - vk0) / (vk1 - vk0);

          // Remaps to keyframes actual times.
          const float tk0 = times[i - 1];
          const float tk1 = times[i];
          const float time = math::Lerp(tk0, tk1, alpha);

          // Set found edge time.
          edge.time = time;
        }

        // Pushes the new edge only if it's in the input range.
        if (edge.time >= lcurr && (edge.time < lto || lto == 1.f)) {
          // Prevents output buffer overflow.
          if (edges_end == edges->end) {
            // edges buffer is full, returning false allows the user to know
            // that output buffer range was too small.
            return false;
          }
          // Convert local loop time to global time space.
          edge.time += rcurr;

          // Pushes new edge to output.
          *(edges_end++) = edge;
        }
      }

      // If "to" is higher that the current frame, we can stop iterating.
      if (times[i] >= lto) {
        break;
      }
    }
  }

  // Reverse if backward.
  // Edge* new_end = edges->begin + current_edge;
  if (!forward) {
    std::reverse(edges->begin, edges_end);
  }

  // Adjust output length.
  edges->end = edges_end;

  return true;
}
}  // namespace animation
}  // namespace ozz
