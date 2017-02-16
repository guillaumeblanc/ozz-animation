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

#include <cassert>
#include <cmath>
#include <cfloat>

#include "ozz/base/memory/allocator.h"

#include "ozz/animation/offline/raw_float_track.h"

#include "ozz/animation/runtime/float_track.h"

namespace ozz {
namespace animation {
namespace offline {

namespace {
void PatchBeginEndKeys(const RawFloatTrack& _input,
                       RawFloatTrack::Keyframes* keyframes) {
  if (_input.keyframes.empty()) {
    const RawFloatTrack::Keyframe begin = {RawFloatTrack::kLinear, 0.f, 0.f};
    keyframes->push_back(begin);
    const RawFloatTrack::Keyframe end = {RawFloatTrack::kLinear, 1.f, 0.f};
    keyframes->push_back(end);
  } else if (_input.keyframes.size() == 1) {
    const RawFloatTrack::Keyframe& src_key = _input.keyframes.front();
    const RawFloatTrack::Keyframe begin = {RawFloatTrack::kLinear, 0.f,
                                           src_key.value};
    keyframes->push_back(begin);
    const RawFloatTrack::Keyframe end = {RawFloatTrack::kLinear, 1.f, src_key.value};
    keyframes->push_back(end);
  } else {
    // Copy all source data.
    if (_input.keyframes.front().time != 0.f) {
      const RawFloatTrack::Keyframe& src_key = _input.keyframes.front();
      const RawFloatTrack::Keyframe begin = {RawFloatTrack::kLinear, 0.f,
                                             src_key.value};
      keyframes->push_back(begin);
    }
    for (size_t i = 0; i < _input.keyframes.size(); ++i) {
      keyframes->push_back(_input.keyframes[i]);
    }
    if (_input.keyframes.back().time != 1.f) {
      const RawFloatTrack::Keyframe& src_key = _input.keyframes.back();
      const RawFloatTrack::Keyframe end = {RawFloatTrack::kLinear,
                                           1.f, src_key.value};
      keyframes->push_back(end);
    }
  }
}

void Linearize(RawFloatTrack::Keyframes* keyframes) {
  assert(keyframes->size() >= 2);

  // Loop through kStep keys and create an extra key right before the next
  // "official" one such that interpolating this two keys will simulate a kStep
  // behavior.
  // Note that interpolation mode of the last key has no impact.
  for (RawFloatTrack::Keyframes::iterator it = keyframes->begin();
       it != keyframes->end() - 1; ++it) {
    const RawFloatTrack::Keyframe& src_key = *it;

    if (src_key.interpolation == RawFloatTrack::kStep) {
      // Pick a time right before the next key frame.
      const RawFloatTrack::Keyframe& src_next_key = *(it + 1);

      // FLT_EPSILON is the smallest such that 1.0+FLT_EPSILON != 1.0.
      // Key time being in range [0, 1], FLT_EPSILON works.
      // nextafterf(src_next_key.time, -1.f) would be a better option, but it
      // isn't available for all compilers (MSVC 11).
      const float new_key_time = src_next_key.time - FLT_EPSILON;

      const RawFloatTrack::Keyframe new_key = {RawFloatTrack::kLinear,
                                               new_key_time, src_key.value};

      it = keyframes->insert(it + 1, new_key);
    } else {
      assert(src_key.interpolation == RawFloatTrack::kLinear);
    }
  }

  // Patch last key as its interpolation mode has no impact.
  keyframes->back().interpolation = RawFloatTrack::kLinear;

  assert(keyframes->front().time >= 0.f);
  assert(keyframes->back().time <= 1.f);
}
}  // namespace

// Ensures _input's validity and allocates _animation.
// An animation needs to have at least two key frames per joint, the first at
// t = 0 and the last at t = 1. If at least one of those keys are not
// in the RawAnimation then the builder creates it.
FloatTrack* FloatTrackBuilder::operator()(const RawFloatTrack& _input) const {
  // Tests _raw_animation validity.
  if (!_input.Validate()) {
    return NULL;
  }

  // Everything is fine, allocates and fills the animation.
  // Nothing can fail now.
  FloatTrack* track = memory::default_allocator()->New<FloatTrack>();

  // Copy data to temporary prepared data structure
  RawFloatTrack::Keyframes keyframes;
  // Guessing a worst size to avoid realloc.
  const size_t worst_size =
      _input.keyframes.size() * 2 +  // * 2 in case all keys are kStep
      2;                             // + 2 for first and last keys
  keyframes.reserve(worst_size);

  PatchBeginEndKeys(_input, &keyframes);
  Linearize(&keyframes);

  // Allocates output track.
  track->Allocate(keyframes.size());

  // Copy all keys to output.
  for (size_t i = 0; i < keyframes.size(); ++i) {
    const RawFloatTrack::Keyframe& src_key = keyframes[i];
    track->times()[i] = src_key.time;
    track->values()[i] = src_key.value;
  }
  /*
    // Copy animation's name.
    strcpy(animation->name_, _input.name.c_str());
    */
  return track;  // Success.
}
}  // offline
}  // animation
}  // ozz
