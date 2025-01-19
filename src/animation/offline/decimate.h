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

#ifndef OZZ_ANIMATION_OFFLINE_DECIMATE_H_
#define OZZ_ANIMATION_OFFLINE_DECIMATE_H_

#ifndef OZZ_INCLUDE_PRIVATE_HEADER
#error "This header is private, it cannot be included from public headers."
#endif  // OZZ_INCLUDE_PRIVATE_HEADER

#include <cassert>

#include "ozz/base/containers/stack.h"
#include "ozz/base/containers/vector.h"

namespace ozz {
namespace animation {
namespace offline {

// Decimation algorithm based on Ramer-Douglas-Peucker.
// https://en.wikipedia.org/wiki/Ramer%E2%80%93Douglas%E2%80%93Peucker_algorithm
// _Track must have std::vector interface.
// Adapter must have the following interface:
// struct Adapter {
//  bool Decimable(const Key&) const;
//  Key Lerp(const Key& _left, const Key& _right, const Key& _ref) const;
//  float Distance(const Value& _a, const Value& _b) const;
// };
template <typename _Track, typename _Adapter>
_Track Decimate(const _Track& _src, const _Adapter& _adapter,
                float _tolerance) {
  _Track output;
  if (_src.size() < 2) {  // Nothing to decimate.
    output = _src;
  } else {
    // Stack of segments to process.
    typedef std::pair<size_t, size_t> Segment;
    ozz::stack<Segment> segments;

    // Bit vector of all points to included.
    ozz::vector<bool> included(_src.size(), false);

    // Pushes segment made from first and last points.
    segments.push(Segment(0, _src.size() - 1));
    included[0] = true;
    included[_src.size() - 1] = true;

    // Empties segments stack.
    while (!segments.empty()) {
      // Pops next segment to process.
      const Segment segment = segments.top();
      segments.pop();

      // Looks for the furthest point from the segment.
      float max = -1.f;
      size_t candidate = segment.first;
      typename _Track::const_reference left = _src[segment.first];
      typename _Track::const_reference right = _src[segment.second];
      for (size_t i = segment.first + 1; i < segment.second; ++i) {
        assert(!included[i] &&
               "Included points should be processed once only.");
        typename _Track::const_reference test = _src[i];
        if (!_adapter.Decimable(test)) {
          candidate = i;
          break;
        } else {
          const float distance = _adapter.Distance(
              _adapter.Lerp(left, right, test).value, test.value);
          if (distance > _tolerance && distance > max) {
            max = distance;
            candidate = i;
          }
        }
      }

      // If found, include the point and pushes the 2 new segments (before and
      // after the new point).
      if (candidate != segment.first) {
        included[candidate] = true;
        if (candidate - segment.first > 1) {
          segments.push(Segment(segment.first, candidate));
        }
        if (segment.second - candidate > 1) {
          segments.push(Segment(candidate, segment.second));
        }
      }
    }

    // Copy all included points.
    for (size_t i = 0; i < _src.size(); ++i) {
      if (included[i]) {
        output.push_back(_src[i]);
      }
    }
  }

  // RDP algo ends with a minimum of 2 points (first and last).
  // Removes last keys if track is constant or identity.
  while (!output.empty()) {
    const bool last_key = output.size() == 1;
    typename _Track::const_reference back = output.back();
    if (!last_key && !_adapter.Decimable(back)) {
      break;  // Not allowed, only meaningful if not last key
    }
    const auto& penultimate =
        last_key ? _Adapter::identity() : output[output.size() - 2].value;
    const float distance = _adapter.Distance(penultimate, back.value);
    if (distance > _tolerance) {
      break;  // Too far, not decimable
    }
    // Decimation is possible, remove last key.
    output.pop_back();
  }

  return output;
}
}  // namespace offline
}  // namespace animation
}  // namespace ozz
#endif  // OZZ_ANIMATION_OFFLINE_DECIMATE_H_
