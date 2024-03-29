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

#include "ozz/animation/offline/raw_track_utils.h"

#include <algorithm>
#include <limits>

// Needs runtime track to access TrackPolicy.
#include "ozz/animation/runtime/track.h"

namespace ozz {
namespace animation {

using internal::TrackPolicy;

namespace offline {

namespace {

// Less comparator, used by search algorithm to walk through track sorted
// keyframes
template <typename _Key>
bool TrackLess(const _Key& _left, const _Key& _right) {
  return _left.ratio < _right.ratio;
}

template <typename _Key>
typename _Key::ValueType TrackLerp(const _Key& _left, const _Key& _right,
                                   float _alpha) {
  if (_left.interpolation == RawTrackInterpolation::kStep && _alpha < 1.f) {
    return _left.value;
  }
  return TrackPolicy<typename _Key::ValueType>::Lerp(_left.value, _right.value,
                                                     _alpha);
}

template <>
math::Quaternion TrackLerp(const RawQuaternionTrack::Keyframe& _left,
                           const RawQuaternionTrack::Keyframe& _right,
                           float _alpha) {
  if (_left.interpolation == RawTrackInterpolation::kStep && _alpha < 1.f) {
    return _left.value;
  }
  // Fix quaternion interpolation to always use the shortest path.
  const auto& lq = _left.value;
  auto rq = _right.value;
  const float dot = lq.x * rq.x + lq.y * rq.y + lq.z * rq.z + lq.w * rq.w;
  if (dot < 0.f) {
    rq = -rq;
  }
  return TrackPolicy<math::Quaternion>::Lerp(lq, rq, _alpha);
}

template <typename _Keyframes>
typename _Keyframes::value_type::ValueType _SampleTrack(
    const _Keyframes& _keyframes, float _ratio) {
  using Keyframe = typename _Keyframes::value_type;
  if (_keyframes.size() == 0) {
    // Return identity if there's no key for this track.
    return TrackPolicy<typename Keyframe::ValueType>::identity();
  } else if (_ratio <= _keyframes.front().ratio) {
    // Returns the first keyframe if _time is before the first keyframe.
    return _keyframes.front().value;
  } else if (_ratio >= _keyframes.back().ratio) {
    // Returns the last keyframe if _time is before the last keyframe.
    return _keyframes.back().value;
  } else {
    // Needs to interpolate the 2 keyframes before and after _time.
    assert(_keyframes.size() >= 2);

    // First find the 2 keys.
    typename _Keyframes::value_type cmp;
    cmp.ratio = _ratio;
    auto it = std::lower_bound(array_begin(_keyframes), array_end(_keyframes),
                               cmp, TrackLess<Keyframe>);
    assert(it > array_begin(_keyframes) && it < array_end(_keyframes));

    // Then interpolate them at t = _time.
    const auto& right = it[0];
    const auto& left = it[-1];
    const float alpha = (_ratio - left.ratio) / (right.ratio - left.ratio);
    return TrackLerp(left, right, alpha);
  }
}
}  // namespace

template <typename _RawTrack>
bool SampleTrack(const _RawTrack& _track, float _ratio,
                 typename _RawTrack::ValueType* _value) {
  if (!_track.Validate()) {
    return false;
  }

  *_value = _SampleTrack(_track.keyframes, _ratio);
  return true;
}

// Explicitly instantiate supported raw tracks sampling functions.
template OZZ_ANIMOFFLINE_DLL bool SampleTrack(const RawFloatTrack& _track,
                                              float _ratio, float* _value);
template OZZ_ANIMOFFLINE_DLL bool SampleTrack(const RawFloat2Track& _track,
                                              float _ratio,
                                              math::Float2* _value);
template OZZ_ANIMOFFLINE_DLL bool SampleTrack(const RawFloat3Track& _track,
                                              float _ratio,
                                              math::Float3* _value);
template OZZ_ANIMOFFLINE_DLL bool SampleTrack(const RawFloat4Track& _track,
                                              float _ratio,
                                              math::Float4* _value);
template OZZ_ANIMOFFLINE_DLL bool SampleTrack(const RawQuaternionTrack& _track,
                                              float _ratio,
                                              math::Quaternion* _value);

}  // namespace offline
}  // namespace animation
}  // namespace ozz
