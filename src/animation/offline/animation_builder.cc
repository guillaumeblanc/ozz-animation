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

#include "ozz/animation/offline/animation_builder.h"

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstring>
#include <limits>
#include <numeric>

#include "ozz/animation/offline/raw_animation.h"
#include "ozz/animation/offline/raw_animation_utils.h"
#include "ozz/animation/runtime/animation.h"
#include "ozz/base/containers/vector.h"
#include "ozz/base/encode/group_varint.h"
#include "ozz/base/maths/math_ex.h"
#include "ozz/base/maths/simd_math.h"
#include "ozz/base/memory/allocator.h"

// Internal include file
#define OZZ_INCLUDE_PRIVATE_HEADER  // Allows to include private headers.
#include "animation/runtime/animation_keyframe.h"

namespace ozz {
namespace animation {
namespace offline {
namespace {

template <typename _Key>
struct SortingKey {
  uint16_t track;
  float prev_key_time;
  _Key key;
};

typedef SortingKey<RawAnimation::TranslationKey> SortingTranslationKey;
typedef SortingKey<RawAnimation::RotationKey> SortingQuaternionKey;
typedef SortingKey<RawAnimation::ScaleKey> SortingScaleKey;

// Keyframe sorting. Stores first by time and then track number.
template <typename _Key>
bool SortingKeyLess(const _Key& _left, const _Key& _right) {
  const float time_diff = _left.prev_key_time - _right.prev_key_time;
  return time_diff < 0.f || (time_diff == 0.f && _left.track < _right.track);
}

template <typename _SrcKey, typename _DestTrack>
void PushBackIdentityKey(uint16_t _track, float _time, _DestTrack* _dest) {
  typedef typename _DestTrack::value_type DestKey;
  float prev_time = -1.f;
  if (!_dest->empty() && _dest->back().track == _track) {
    prev_time = _dest->back().key.time;
  }
  const DestKey key = {_track, prev_time, {_time, _SrcKey::identity()}};
  _dest->push_back(key);
}

// Copies a track from a RawAnimation to an Animation.
// Also fixes up the front (t = 0) and back keys (t = duration).
template <typename _SrcTrack, typename _DestTrack>
void CopyRaw(const _SrcTrack& _src, uint16_t _track, float _duration,
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
    for (size_t k = 0; k < _src.size(); ++k) {  // Copies all keys.
      const SrcKey& raw_key = _src[k];
      assert(raw_key.time >= 0 && raw_key.time <= _duration);
      const DestKey key = {_track, prev_time, {raw_key.time, raw_key.value}};
      _dest->push_back(key);
      prev_time = raw_key.time;
    }
    if (_src.back().time - _duration != 0.f) {  // Needs a key at t = _duration.
      const DestKey last = {_track, prev_time, {_duration, _src.back().value}};
      _dest->push_back(last);
    }
  }
  assert(_dest->front().key.time == 0.f &&
         _dest->back().key.time - _duration == 0.f);
}

template <typename _SortingKey, class _Lerp, class _Compare>
void Sort(ozz::vector<_SortingKey>& _src, size_t _num_tracks,
          const _Lerp& _lerp, const _Compare& _comp) {
  // Sorts whole vector
  std::sort(_src.begin(), _src.end(), _comp);

  // Will store last 2 keys (last and penultimate) for a track.
  ozz::vector<std::pair<int, int>> previouses(_num_tracks);
  for (bool loop = true; loop;) {
    loop = false;  // Will be set to true again if vector was changed.

    // Reset all previouses to default.
    const std::pair<int, int> default_previous{-1, -1};
    std::fill(previouses.begin(), previouses.end(), default_previous);

    // Loop through all keys.
    for (size_t i = 0; i < _src.size(); ++i) {
      const uint16_t track = _src[i].track;
      auto& previous = previouses[track];

      // Inject key if distance from previous one is too big to be stored in
      // runtime data structure (aka index to previous frame bigger than
      // kMaxPreviousOffset)
      if (previous.first != -1 &&
          i - previous.first > ozz::animation::internal::kMaxPreviousOffset) {
        assert(previous.second != -1 &&
               "Not possible not to have a valid penultimate key.");
        // Copy as originals are going to be (re)moved.
        _SortingKey last = _src[previous.first];
        const _SortingKey penultimate = _src[previous.second];

        // Prepares new key to insert.
        const _SortingKey insert = {
            track,
            penultimate.key.time,
            {(penultimate.key.time + last.key.time) * .5f,
             _lerp(penultimate.key.value, last.key.value, .5f)}};

        // Removes previous.first key that is changing and needs to be resorted.
        _src.erase(_src.begin() + previous.first);

        // Pushes the new key....
        _src.push_back(insert);

        // ... and the modified one, in the correct order (for the merge).
        last.prev_key_time = insert.key.time;
        _src.push_back(last);

        // Re-sorts vector in place. Last 2 keys where added. Nothing has
        // changed before previous.second.
        std::inplace_merge(_src.begin() + previous.second, _src.end() - 2,
                           _src.end(), _comp);

        // Restart as things have changed behind insertion point and previouses
        // need to be rebuilt.
        loop = true;
        break;
      }

      // Updates previouses
      previous.second = previous.first;
      previous.first = static_cast<int>(i);
    }
  }
}

void CopyTimePoints(const span<const float>& _times, float _inv_duration,
                    const span<float>& _ratios) {
  assert(_times.size() == _ratios.size());
  for (size_t i = 0; i < _times.size(); ++i) {
    _ratios[i] = _times[i] * _inv_duration;
  }
}

uint16_t TimePointToIndex(const span<const float>& _timepoints, float _time) {
  const float* found =
      std::lower_bound(_timepoints.begin(), _timepoints.end(), _time);
  assert(found != _timepoints.end() && *found == _time);
  const ptrdiff_t distance = found - _timepoints.begin();
  assert(distance >= 0 && distance < std::numeric_limits<uint16_t>::max());
  return static_cast<uint16_t>(distance);
}

template <typename _SortingKey, typename _DestKey, typename _Compressor>
void Compress(const span<const float>& _timepoints,
              const span<_SortingKey>& _src, size_t _num_tracks,
              const span<_DestKey> _dest, const Animation::KeyframesCtrl& _base,
              const _Compressor& _compressor) {
  ozz::vector<_DestKey*> previouses(_num_tracks);
  for (size_t i = 0; i < _src.size(); ++i) {
    const _SortingKey& src = _src[i];
    _DestKey& dest_key = _dest[i];

    // Ratio
    const uint16_t ratio = TimePointToIndex(_timepoints, src.key.time);
    if (_timepoints.size() <= std::numeric_limits<uint8_t>::max()) {
      assert(ratio <= std::numeric_limits<uint8_t>::max());
      reinterpret_span<uint8_t>(_base.ratios)[i] = static_cast<uint8_t>(ratio);
    } else {
      reinterpret_span<uint16_t>(_base.ratios)[i] = ratio;
    }

    // Previous
    const ptrdiff_t diff =
        previouses[src.track] ? &dest_key - previouses[src.track] : 0;
    assert(diff < ozz::animation::internal::kMaxPreviousOffset);
    _base.previouses[i] = static_cast<uint16_t>(diff);

    // Value
    _compressor(src.key.value, &dest_key);

    // Stores track position
    previouses[src.track] = &dest_key;
  }
}

void CompressFloat3(const ozz::math::Float3& _src,
                    ozz::animation::internal::Float3Key* _dest) {
  _dest->values[0] = ozz::math::FloatToHalf(_src.x);
  _dest->values[1] = ozz::math::FloatToHalf(_src.y);
  _dest->values[2] = ozz::math::FloatToHalf(_src.z);
}

// Compares float absolute values.
bool LessAbs(float _left, float _right) {
  return std::abs(_left) < std::abs(_right);
}

// Compresses quaternion to ozz::animation::RotationKey format.
// The 3 smallest components of the quaternion are quantized to x bits
// integers, while the largest is recomputed thanks to quaternion
// normalization property (x^2+y^2+z^2+w^2 = 1). Because the 3 components are
// the 3 smallest, their value cannot be greater than sqrt(2)/2. Thus
// quantization quality is improved by pre-multiplying each componenent by
// sqrt(2).
void CompressQuaternion(const ozz::math::Quaternion& _src,
                        ozz::animation::internal::QuaternionKey* _dest) {
  // Finds the largest quaternion component.
  const float quat[4] = {_src.x, _src.y, _src.z, _src.w};
  const ptrdiff_t largest = std::max_element(quat, quat + 4, LessAbs) - quat;
  assert(largest <= 3);

  // Quantize the 3 smallest components on x bits signed integers.
  const float kScale = internal::QuaternionKey::kfScale / math::kSqrt2;
  const float kOffset = -math::kSqrt2_2;
  const int kMapping[4][3] = {{1, 2, 3}, {0, 2, 3}, {0, 1, 3}, {0, 1, 2}};
  const int* map = kMapping[largest];
  const int cpnt[3] = {
      math::Min(static_cast<int>((quat[map[0]] - kOffset) * kScale + .5f),
                internal::QuaternionKey::kiScale),
      math::Min(static_cast<int>((quat[map[1]] - kOffset) * kScale + .5f),
                internal::QuaternionKey::kiScale),
      math::Min(static_cast<int>((quat[map[2]] - kOffset) * kScale + .5f),
                internal::QuaternionKey::kiScale)};

  pack(static_cast<int>(largest), quat[largest] < 0.f, cpnt, _dest);
}  // namespace

// Normalize quaternions. Fixes-up successive opposite quaternions that would
// fail to take the shortest path during the normalized-lerp. Note that keys
// are still sorted per-track at that point, which allows this algorithm to
// process all consecutive keys.
void FixupQuaternions(ozz::vector<SortingQuaternionKey>* _src) {
  size_t track = std::numeric_limits<size_t>::max();
  const math::Quaternion identity = math::Quaternion::identity();
  for (size_t i = 0; i < _src->size(); ++i) {
    SortingQuaternionKey& src = _src->at(i);
    math::Quaternion normalized = NormalizeSafe(src.key.value, identity);
    if (track != _src->at(i).track) {  // First key of the track.
      if (normalized.w < 0.f) {    // .w eq to a dot with identity quaternion.
        normalized = -normalized;  // Q an -Q are the same rotation.
      }
    } else {  // Still on the same track: so fixes-up quaternion.
      const SortingQuaternionKey& prev_src = _src->at(i - 1);
      const math::Float4 prev(prev_src.key.value.x, prev_src.key.value.y,
                              prev_src.key.value.z, prev_src.key.value.w);
      const math::Float4 curr(normalized.x, normalized.y, normalized.z,
                              normalized.w);
      if (Dot(prev, curr) < 0.f) {
        normalized = -normalized;  // Q an -Q are the same rotation.
      }
    }
    // Stores fixed-up quaternion.
    src.key.value = normalized;
    track = src.track;
  }
}

ozz::vector<float> BuildTimePoints(
    ozz::vector<SortingTranslationKey>& _translations,
    ozz::vector<SortingQuaternionKey>& _rotations,
    ozz::vector<SortingScaleKey>& _scales) {
  ozz::vector<float> timepoints;

  for (const auto& _key : _translations) {
    timepoints.push_back(_key.key.time);
  }
  for (const auto& _key : _rotations) {
    timepoints.push_back(_key.key.time);
  }
  for (const auto& _key : _scales) {
    timepoints.push_back(_key.key.time);
  }

  std::sort(timepoints.begin(), timepoints.end());
  timepoints.erase(std::unique(timepoints.begin(), timepoints.end()),
                   timepoints.end());

  return timepoints;
}

struct BuilderIFrame {
  ozz::vector<byte> entries;
  size_t last;
};

template <typename _SortingKey>
BuilderIFrame BuildIFrame(const ozz::span<_SortingKey>& _src, float _time,
                          size_t _num_soa_tracks) {
  BuilderIFrame iframe;

  // Initialize vector with initial cached keys at t=0. Due to sorting, they
  // are the 2nd set keyframes, hence starting from _num_soa_tracks.
  const uint32_t num_entries = static_cast<uint32_t>(_num_soa_tracks);
  ozz::vector<uint32_t> entries(num_entries);

  // The loop can end as soon as it finds a key greater that _time. It
  // will mean that all the keys lower than _time have been processed, meaning
  // all cache entries are up to date.
  for (size_t i = 0, end = _src.size();
       i < end && _src[i].prev_key_time <= _time; ++i) {
    // Stores the last key found for this track.
    entries[_src[i].track] = static_cast<uint32_t>(i);
    iframe.last = i;
  }
  assert(iframe.last >= _num_soa_tracks * 2 - 1);

  // Compress buffer.
  // Entries is a multiple of 4 (number os soa tracks).
  const size_t worst_size = ozz::ComputeGV4WorstBufferSize(make_span(entries));
  iframe.entries.resize(worst_size);
  auto remain =
      ozz::EncodeGV4Stream(make_span(entries), make_span(iframe.entries));
  iframe.entries.resize(iframe.entries.size() - remain.size_bytes());

  return iframe;
}

struct BuilderIFrames {
  ozz::vector<byte> entries;
  ozz::vector<uint32_t> desc;
  float interval = 1.f;
};

// Splits src into parts of similar sizes. The "time" of each part doesn't
// really matter, it's the number of keys that impact performance.
template <typename _SortingKey>
BuilderIFrames BuildIFrames(const ozz::span<_SortingKey>& _src,
                            size_t _num_soa_tracks, float _interval,
                            float _duration) {
  BuilderIFrames iframes;
  if (_num_soa_tracks == 0 || _interval <= 0.f) {
    return iframes;
  }

  const size_t iframes_divs =
      static_cast<size_t>(math::Max(1.f, _duration / _interval));
  for (size_t i = 0; i < iframes_divs; ++i) {
    const float time = _duration * (i + 1) / iframes_divs;
    const auto& iframe = BuildIFrame(_src, time, _num_soa_tracks);

    // Don't need to add an iframe for the first set of keyframes.
    if (iframe.last <= _num_soa_tracks * 2 - 1) {
      continue;
    }

    // Don't need to append an iframe if there's not enough keyframes between
    // two iframes.
    if (!iframes.desc.empty() && iframe.last <= iframes.desc.back()) {
      continue;
    }

    // Pushes offset.
    iframes.desc.push_back(static_cast<uint32_t>(iframes.entries.size()));
    iframes.desc.push_back(static_cast<uint32_t>(iframe.last));

    // Pushes compressed data.
    iframes.entries.insert(iframes.entries.end(), iframe.entries.begin(),
                           iframe.entries.end());
  }

  // Computes actual interval (duration ratio) between iframes.
  const size_t actual_intervals = iframes.desc.size() / 2;
  iframes.interval = iframes.entries.empty() ? 1.f : 1.f / actual_intervals;
  return iframes;
}

void CopyIFrames(const BuilderIFrames& _src, Animation::KeyframesCtrl& _dest) {
  assert(_dest.iframe_entries.size() == _src.entries.size());
  std::copy(_src.entries.begin(), _src.entries.end(),
            _dest.iframe_entries.begin());
  assert(_dest.iframe_desc.size() == _src.desc.size());
  std::copy(_src.desc.begin(), _src.desc.end(), _dest.iframe_desc.begin());
  _dest.iframe_interval = _src.interval;
}
}  // namespace

// Ensures _input's validity and allocates _animation.
// An animation needs to have at least two key frames per joint, the first at
// t = 0 and the last at t = duration. If at least one of those keys are not
// in the RawAnimation then the builder creates it.
unique_ptr<Animation> AnimationBuilder::operator()(
    const RawAnimation& _input) const {
  // Tests _raw_animation validity.
  if (!_input.Validate()) {
    return nullptr;
  }

  // Everything is fine, allocates and fills the animation.
  // Nothing can fail now.
  unique_ptr<Animation> animation = make_unique<Animation>();

  // Sets duration.
  const float duration = _input.duration;
  // A _duration == 0 would create some division by 0 during sampling.
  // Also we need at least to keys with different times, which cannot be done
  // if duration is 0.
  assert(duration > 0.f);  // This case is handled by Validate().
  const float inv_duration = 1.f / _input.duration;
  animation->duration_ = duration;

  // Sets tracks count. Can be safely casted to uint16_t as number of tracks as
  // already been validated.
  const uint16_t num_tracks = static_cast<uint16_t>(_input.num_tracks());
  animation->num_tracks_ = num_tracks;
  const uint16_t num_soa_tracks = Align(num_tracks, 4);

  // Declares and preallocates tracks to sort.
  size_t translations = 0, rotations = 0, scales = 0;
  for (uint16_t i = 0; i < num_tracks; ++i) {
    const RawAnimation::JointTrack& raw_track = _input.tracks[i];
    translations += raw_track.translations.size() + 2;  // +2 because worst case
    rotations += raw_track.rotations.size() + 2;        // needs to add the
    scales += raw_track.scales.size() + 2;              // first and last keys.
  }
  ozz::vector<SortingTranslationKey> sorting_translations;
  sorting_translations.reserve(translations);
  ozz::vector<SortingQuaternionKey> sorting_rotations;
  sorting_rotations.reserve(rotations);
  ozz::vector<SortingScaleKey> sorting_scales;
  sorting_scales.reserve(scales);

  // Filters RawAnimation keys and copies them to the output sorting structure.
  uint16_t i = 0;
  for (; i < num_tracks; ++i) {
    const RawAnimation::JointTrack& raw_track = _input.tracks[i];
    CopyRaw(raw_track.translations, i, duration, &sorting_translations);
    CopyRaw(raw_track.rotations, i, duration, &sorting_rotations);
    CopyRaw(raw_track.scales, i, duration, &sorting_scales);
  }

  // Add enough identity keys to match soa requirements.
  for (; i < num_soa_tracks; ++i) {
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

  FixupQuaternions(&sorting_rotations);

  // Sort animation keys to favor cache coherency.
  Sort(sorting_translations, num_soa_tracks, &LerpTranslation,
       &SortingKeyLess<SortingTranslationKey>);
  Sort(sorting_rotations, num_soa_tracks, &LerpRotation,
       &SortingKeyLess<SortingQuaternionKey>);
  Sort(sorting_scales, num_soa_tracks, &LerpScale,
       &SortingKeyLess<SortingScaleKey>);

  // Get all timepoints. Shall be done on sorting keys as time points might have
  // been added during the process.
  const ozz::vector<float>& time_points =
      BuildTimePoints(sorting_translations, sorting_rotations, sorting_scales);

  // Maximum time points reached.
  if (time_points.size() > std::numeric_limits<uint16_t>::max()) {
    return nullptr;
  }

  // Maximum number of keys reached.
  if (sorting_translations.size() > std::numeric_limits<uint32_t>::max() &&
      sorting_rotations.size() > std::numeric_limits<uint32_t>::max() &&
      sorting_scales.size() > std::numeric_limits<uint32_t>::max()) {
    return nullptr;
  }

  // Build cache snaphots/iframes.
  const auto& translation_ss =
      BuildIFrames(make_span(sorting_translations), num_soa_tracks,
                   iframe_interval, duration);
  const auto& rotation_ss = BuildIFrames(
      make_span(sorting_rotations), num_soa_tracks, iframe_interval, duration);
  const auto& scale_ss = BuildIFrames(make_span(sorting_scales), num_soa_tracks,
                                      iframe_interval, duration);

  // Allocate animation members.
  const Animation::AllocateParams params{
      _input.name.length(),
      time_points.size(),
      sorting_translations.size(),
      sorting_rotations.size(),
      sorting_scales.size(),
      {translation_ss.entries.size(), translation_ss.desc.size()},
      {rotation_ss.entries.size(), rotation_ss.desc.size()},
      {scale_ss.entries.size(), scale_ss.desc.size()}};
  animation->Allocate(params);

  CopyIFrames(translation_ss, animation->translations_ctrl_);
  CopyIFrames(rotation_ss, animation->rotations_ctrl_);
  CopyIFrames(scale_ss, animation->scales_ctrl_);

  // Copy sorted keys to final animation.
  Compress(make_span(time_points), make_span(sorting_translations),
           num_soa_tracks, make_span(animation->translations_values_),
           animation->translations_ctrl_, &CompressFloat3);
  Compress(make_span(time_points), make_span(sorting_rotations), num_soa_tracks,
           make_span(animation->rotations_values_), animation->rotations_ctrl_,
           &CompressQuaternion);
  Compress(make_span(time_points), make_span(sorting_scales), num_soa_tracks,
           make_span(animation->scales_values_), animation->scales_ctrl_,
           &CompressFloat3);

  // Converts timepoints to ratio and copy to animation. Must be done once
  // indices have been set.
  CopyTimePoints(make_span(time_points), inv_duration, animation->timepoints_);

  // Copy animation's name.
  if (animation->name_) {
    strcpy(animation->name_, _input.name.c_str());
  }

  return animation;  // Success.
}
}  // namespace offline
}  // namespace animation
}  // namespace ozz
