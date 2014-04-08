//============================================================================//
//                                                                            //
// ozz-animation, 3d skeletal animation libraries and tools.                  //
// https://code.google.com/p/ozz-animation/                                   //
//                                                                            //
//----------------------------------------------------------------------------//
//                                                                            //
// Copyright (c) 2012-2014 Guillaume Blanc                                    //
//                                                                            //
// This software is provided 'as-is', without any express or implied          //
// warranty. In no event will the authors be held liable for any damages      //
// arising from the use of this software.                                     //
//                                                                            //
// Permission is granted to anyone to use this software for any purpose,      //
// including commercial applications, and to alter it and redistribute it     //
// freely, subject to the following restrictions:                             //
//                                                                            //
// 1. The origin of this software must not be misrepresented; you must not    //
// claim that you wrote the original software. If you use this software       //
// in a product, an acknowledgment in the product documentation would be      //
// appreciated but is not required.                                           //
//                                                                            //
// 2. Altered source versions must be plainly marked as such, and must not be //
// misrepresented as being the original software.                             //
//                                                                            //
// 3. This notice may not be removed or altered from any source               //
// distribution.                                                              //
//                                                                            //
//============================================================================//

#include "ozz/animation/offline/animation_builder.h"

#include <algorithm>
#include <limits>

#include "ozz/base/containers/vector.h"
#include "ozz/base/memory/allocator.h"
#include "ozz/animation/runtime/animation.h"

// Internal include file
#define OZZ_INCLUDE_PRIVATE_HEADER  // Allows to include private headers.
#include "../runtime/key_frame.h"

namespace ozz {
namespace animation {
namespace offline {

RawAnimation::RawAnimation()
  : duration(1.f) {
}

RawAnimation::~RawAnimation() {
}

namespace {

// Implements key frames' time range and ordering checks.
// See AnimationBuilder::Create for more details.
template <typename _Key>
static bool ValidateTrack(const typename ozz::Vector<_Key>::Std& _track,
                          float _duration) {
  float previous_time = -1.f;
  for (std::size_t k = 0; k < _track.size(); ++k) {
    const float frame_time = _track[k].time;
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

struct SortingTranslationKey {
  unsigned int track;
  float prev_key_time;
  RawAnimation::TranslationKey key;
};

struct SortingRotationKey {
  unsigned int track;
  float prev_key_time;
  RawAnimation::RotationKey key;
};

struct SortingScaleKey {
  unsigned int track;
  float prev_key_time;
  RawAnimation::ScaleKey key;
};

// Keyframe sorting. Stores first by time and then track number.
template<typename _Key>
bool SortingKeyLess(const _Key& _left, const _Key& _right) {
  return _left.prev_key_time < _right.prev_key_time
         || (_left.prev_key_time == _right.prev_key_time
             && _left.track < _right.track);
}

template<typename _SrcKey, typename _DestTrack>
void PushBackIdentityKey(unsigned int _track, float _time, _DestTrack* _dest) {
  typedef typename _DestTrack::value_type DestKey;
  float prev_time = -1.f;
  if (!_dest->empty() && _dest->back().track == _track) {
    prev_time =  _dest->back().key.time;
  }
  const DestKey key = {_track, prev_time, {_time, _SrcKey::identity()}};
  _dest->push_back(key);
}

// Copies a track from a RawAnimation to an Animation.
// Also fixes up the front (t = 0) and back keys (t = duration).
template<typename _SrcTrack, typename _DestTrack>
void CopyRaw(const _SrcTrack& _src, unsigned int _track, float _duration,
             _DestTrack* _dest) {
  typedef typename _SrcTrack::value_type SrcKey;
  typedef typename _DestTrack::value_type DestKey;

  if (_src.size() == 0) {  // Adds 2 new keys.
    PushBackIdentityKey<SrcKey, _DestTrack>(_track, 0.f, _dest);
    PushBackIdentityKey<SrcKey, _DestTrack>(_track, _duration, _dest);
  } else if (_src.size() == 1) {  // Adds 1 new key.
    const SrcKey& raw_key = _src.front();
    assert(raw_key.time >= 0 && raw_key.time <= _duration);
    const DestKey first = {_track, -1.f, {0.f, raw_key.value}};
      _dest->push_back(first);
    const DestKey last = {_track, 0.f, {_duration, raw_key.value}};
      _dest->push_back(last);
  } else {  // Copies all keys, and fixes up first and last keys.
    float prev_time = -1.f;
    if (_src.front().time != 0.f) {  // Needs a key at t = 0.f.
      const DestKey first = {_track, prev_time, {0.f, _src.front().value}};
      _dest->push_back(first);
      prev_time = 0.f;
    }
    for (std::size_t k = 0; k < _src.size(); ++k) {  // Copies all keys.
      const SrcKey& raw_key = _src[k];
      assert(raw_key.time >= 0 && raw_key.time <= _duration);
      const DestKey key = {_track, prev_time, {raw_key.time, raw_key.value}};
      _dest->push_back(key);
      prev_time = raw_key.time;
    }
    if (_src.back().time != _duration) {  // Needs a key at t = _duration.
      const DestKey last = {
        _track, prev_time, {_duration, _src.back().value}};
      _dest->push_back(last);
    }
  }
  assert(_dest->front().key.time == 0.f && _dest->back().key.time == _duration);
}

template<typename _SrcKey, typename _DestKey>
ozz::Range<_DestKey> CopyToAnimation(typename ozz::Vector<_SrcKey>::Std* _src) {
  const std::size_t src_count = _src->size();
  if (!src_count) {
    return ozz::Range<_DestKey>();
  }

  // Sort animation keys to favor cache coherency.
  std::sort(&_src->front(), (&_src->back()) + 1, &SortingKeyLess<_SrcKey>);

  // Fills output.
  ozz::Range<_DestKey> dest =
    memory::default_allocator()->AllocateRange<_DestKey>(src_count);
  const _SrcKey* src = &_src->front();
  for (std::size_t i = 0; i < src_count; ++i) {
    dest.begin[i].track = src[i].track;
    dest.begin[i].time = src[i].key.time;
    dest.begin[i].value = src[i].key.value;
  }
  return dest;
}

// Specialize for rotations in order to normalize quaternions.
// Consecutive opposite quatenions are also fixed up in order to avoid checking
// for the smallest path during the NLerp runtime algorithm .
template<>
ozz::Range<RotationKey> CopyToAnimation<SortingRotationKey, RotationKey>(
  ozz::Vector<SortingRotationKey>::Std* _src) {
  const std::size_t src_count = _src->size();
  if (!src_count) {
    return ozz::Range<RotationKey>();
  }

  // Normalize quaternions.
  // Also fixes-up successive opposite quaternions that would fail to take the
  // shortest path during the normalized-lerp.
  // Note that keys are still sorted per-track at that point, which allows this
  // algorithm to process all consecutive keys.
  std::size_t track = std::numeric_limits<std::size_t>::max();
  const math::Quaternion identity = math::Quaternion::identity();
  SortingRotationKey* src = &_src->front();
  for (std::size_t i = 0; i < src_count; ++i) {
    math::Quaternion normalized = NormalizeSafe(src[i].key.value, identity);
    if (track != src[i].track) {  // First key of the track.
      if (normalized.w < 0.f) {  // .w eq to a dot with identity quaternion.
        normalized = -normalized;  // Q an -Q are the same rotation.
      }
    } else {  // Still on the same track: so fixes-up quaternion.
      const math::Float4 prev(src[i - 1].key.value.x, src[i - 1].key.value.y,
                              src[i - 1].key.value.z, src[i - 1].key.value.w);
      const math::Float4 curr(normalized.x, normalized.y,
                              normalized.z, normalized.w);
      if (Dot(prev, curr) < 0.f) {
        normalized = -normalized;  // Q an -Q are the same rotation.
      }
    }
    // Stores fixed-up quaternion.
    src[i].key.value = normalized;
    track = src[i].track;
  }

  // Sort.
  std::sort(array_begin(*_src),
            array_end(*_src),
            &SortingKeyLess<SortingRotationKey>);

  // Fills output.
  ozz::Range<RotationKey> dest =
    memory::default_allocator()->AllocateRange<RotationKey>(src_count);
  for (size_t i = 0; i < src_count; ++i) {
    dest.begin[i].track = src[i].track;
    dest.begin[i].time = src[i].key.time;
    dest.begin[i].value = src[i].key.value;
  }
  return dest;
}
}  // namespace

bool RawAnimation::Validate() const {
  if (duration <= 0.f) {  // Tests duration is valid.
    return false;
  }
  // Ensures that all key frames' time are valid, ie: in a strict ascending
  // order and within range [0,duration].
  for (std::size_t j = 0; j < tracks.size(); ++j) {
    const RawAnimation::JointTrack& track = tracks[j];
    if (!ValidateTrack<TranslationKey>(track.translations, duration) ||
        !ValidateTrack<RotationKey>(track.rotations, duration) || 
        !ValidateTrack<ScaleKey>(track.scales, duration)) {
      return false;
    }
  }

  return true;  // *this is valid.
}

// Ensures _input's validity and allocates _animation.
// An animation needs to have at least two key frames per joint, the first at
// t = 0 and the last at t = duration. If at least one of those keys are not
// in the RawAnimation then the builder creates it.
Animation* AnimationBuilder::operator()(const RawAnimation& _input) const {
  // Tests _raw_animation validity.
  if (!_input.Validate()) {
    return NULL;
  }

  // Everything is fine, allocates and fills the animation.
  // Nothing can fail now.
  Animation* animation = memory::default_allocator()->New<Animation>();

  // Sets duration.
  const float duration = _input.duration;
  animation->duration_ = duration;
  // A _duration == 0 would create some division by 0 during sampling.
  // Also we need at least to keys with different times, which cannot be done
  // if duration is 0.
  assert(duration > 0.f);  // This cas must be handled by Validate().

  // Sets tracks count.
  const int num_tracks = _input.num_tracks();
  animation->num_tracks_ = num_tracks;
  const int num_soa_tracks = math::Align(num_tracks, 4);

  // Declares and preallocates tracks to sort.
  std::size_t translations = 0, rotations = 0, scales = 0;
  for (int i = 0; i < num_tracks; ++i) {
    const RawAnimation::JointTrack& raw_track =  _input.tracks[i];
    translations += raw_track.translations.size() + 2;  // +2 because worst case
    rotations += raw_track.rotations.size() + 2;        // needs to add the
    scales += raw_track.scales.size() + 2;              // first and last keys.
  }
  ozz::Vector<SortingTranslationKey>::Std sorting_translations;
  sorting_translations.reserve(translations);
  ozz::Vector<SortingRotationKey>::Std sorting_rotations;
  sorting_rotations.reserve(rotations);
  ozz::Vector<SortingScaleKey>::Std sorting_scales;
  sorting_scales.reserve(scales);

  // Filters RawAnimation keys and copies them to the output sorting structure.
  for (int i = 0; i < num_tracks; ++i) {
    const RawAnimation::JointTrack& raw_track = _input.tracks[i];
    CopyRaw(raw_track.translations, i, duration, &sorting_translations);
    CopyRaw(raw_track.rotations, i, duration, &sorting_rotations);
    CopyRaw(raw_track.scales, i, duration, &sorting_scales);
  }

  // Add enough identity keys to match soa requirements.
  for (int i = num_tracks; i < num_soa_tracks; ++i) {
    typedef RawAnimation::TranslationKey SrcTKey;
    PushBackIdentityKey<SrcTKey>(i, 0.f, &sorting_translations);
    PushBackIdentityKey<SrcTKey>(i, duration, &sorting_translations);

    typedef RawAnimation::RotationKey SrcRKey;
    PushBackIdentityKey<SrcRKey>(i, 0.f, &sorting_rotations);
    PushBackIdentityKey<SrcRKey>(i, duration, &sorting_rotations);

    typedef RawAnimation::ScaleKey SrcSKey;
    PushBackIdentityKey<SrcSKey>(i, 0.f, &sorting_scales);
    PushBackIdentityKey<SrcSKey>(i, duration, &sorting_scales);
  }

  // Copy sorted keys to final animation.
  animation->translations_ =
    CopyToAnimation<SortingTranslationKey, TranslationKey>(
      &sorting_translations);
  animation->rotations_ = CopyToAnimation<SortingRotationKey, RotationKey>(
    &sorting_rotations);
  animation->scales_ = CopyToAnimation<SortingScaleKey, ScaleKey>(
    &sorting_scales);

  return animation;  // Success.
}
}  // offline
}  // animation
}  // ozz
