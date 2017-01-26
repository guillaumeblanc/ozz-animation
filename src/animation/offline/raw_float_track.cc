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

#include "ozz/animation/offline/raw_float_track.h"

namespace ozz {
namespace animation {
namespace offline {

RawFloatTrack::RawFloatTrack() : duration(1.f) {}

RawFloatTrack::~RawFloatTrack() {}

namespace {

// Implements key frames' time range and ordering checks.
static bool ValidateKeyframes(const RawFloatTrack::Keyframes& _keyframes,
                              float _duration) {
  float previous_time = -1.f;
  for (size_t k = 0; k < _keyframes.size(); ++k) {
    const float frame_time = _keyframes[k].time;
    // Tests frame's time is in range [0:duration].
    if (frame_time < 0.f || frame_time > _duration) {
      return false;
    }
    // Tests that frames are sorted.
    if (frame_time <= previous_time) {
      return false;
    }
    previous_time = frame_time;
  }
  return true;  // Validated.
}
}  // namespace

bool RawFloatTrack::Validate() const {
  if (duration <= 0.f) {  // Tests duration is valid.
    return false;
  }

  return ValidateKeyframes(keyframes, duration);
}
}  // offline
}  // animation
}  // ozz
