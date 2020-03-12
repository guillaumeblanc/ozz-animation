//----------------------------------------------------------------------------//
//                                                                            //
// ozz-animation is hosted at http://github.com/guillaumeblanc/ozz-animation  //
// and distributed under the MIT License (MIT).                               //
//                                                                            //
// Copyright (c) 2019 Guillaume Blanc                                         //
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

#include "ozz/base/containers/vector.h"
#include "ozz/base/memory/allocator.h"

#include "ozz/base/maths/math_ex.h"
#include "ozz/base/maths/simd_math.h"

#include "ozz/animation/offline/raw_animation.h"

#include "ozz/animation/runtime/animation.h"

// Internal include file
#define OZZ_INCLUDE_PRIVATE_HEADER  // Allows to include private headers.
#include "animation/runtime/animation_keyframe.h"

namespace ozz {
namespace animation {
namespace offline {
namespace {

struct SortingTranslationKey {
  uint16_t track;
  float prev_key_time;
  RawAnimation::TranslationKey key;
};

struct SortingRotationKey {
  uint16_t track;
  float prev_key_time;
  RawAnimation::RotationKey key;
};

struct SortingScaleKey {
  uint16_t track;
  float prev_key_time;
  RawAnimation::ScaleKey key;
};

// Keyframe sorting. Stores first by time and then track number.
template <typename _Key>
bool SortingKeyLess(const _Key& _left, const _Key& _right) {
  return _left.prev_key_time < _right.prev_key_time ||
         (_left.prev_key_time == _right.prev_key_time &&
          _left.track < _right.track);
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

  // TODO assert type is animated

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
    if (_src.back().time != _duration) {  // Needs a key at t = _duration.
      const DestKey last = {_track, prev_time, {_duration, _src.back().value}};
      _dest->push_back(last);
    }
  }
  assert(_dest->front().key.time == 0.f && _dest->back().key.time == _duration);
}

void CopyAnimated(ozz::vector<SortingTranslationKey>* _src,
                  ozz::Range<Float3Key>* _dest, float _inv_duration) {
  // Sort animation keys to favor cache coherency.
  std::sort(&_src->front(), (&_src->back()) + 1,
            &SortingKeyLess<SortingTranslationKey>);

  // Fills output.
  const SortingTranslationKey* src = &_src->front();
  for (size_t i = 0; i < _src->size(); ++i) {
    Float3Key& key = _dest->begin[i];
    key.ratio = src[i].key.time * _inv_duration;
    key.track = src[i].track;
    key.value[0] = ozz::math::FloatToHalf(src[i].key.value.x);
    key.value[1] = ozz::math::FloatToHalf(src[i].key.value.y);
    key.value[2] = ozz::math::FloatToHalf(src[i].key.value.z);
  }
}

// TODO factorize
void CopyAnimated(ozz::vector<SortingScaleKey>* _src,
                  ozz::Range<Float3Key>* _dest, float _inv_duration) {
  // Sort animation keys to favor cache coherency.
  std::sort(&_src->front(), (&_src->back()) + 1,
            &SortingKeyLess<SortingScaleKey>);

  // Fills output.
  const SortingScaleKey* src = &_src->front();
  for (size_t i = 0; i < _src->size(); ++i) {
    Float3Key& key = _dest->begin[i];
    key.ratio = src[i].key.time * _inv_duration;
    key.track = src[i].track;
    key.value[0] = ozz::math::FloatToHalf(src[i].key.value.x);
    key.value[1] = ozz::math::FloatToHalf(src[i].key.value.y);
    key.value[2] = ozz::math::FloatToHalf(src[i].key.value.z);
  }
}

// Compares float absolute values.
bool LessAbs(float _left, float _right) {
  return std::abs(_left) < std::abs(_right);
}

// Compresses quaternion to ozz::animation::RotationKey format.
// The 3 smallest components of the quaternion are quantized to 16 bits
// integers, while the largest is recomputed thanks to quaternion normalization
// property (x^2+y^2+z^2+w^2 = 1). Because the 3 components are the 3 smallest,
// their value cannot be greater than sqrt(2)/2. Thus quantization quality is
// improved by pre-multiplying each componenent by sqrt(2).
void CompressQuat(const ozz::math::Quaternion& _src,
                  ozz::animation::QuaternionKey* _dest) {
  // Finds the largest quaternion component.
  const float quat[4] = {_src.x, _src.y, _src.z, _src.w};
  const size_t largest = std::max_element(quat, quat + 4, LessAbs) - quat;
  assert(largest <= 3);
  _dest->largest = largest & 0x3;

  // Stores the sign of the largest component.
  _dest->sign = quat[largest] < 0.f;

  // Quantize the 3 smallest components on 16 bits signed integers.
  const float kFloat2Int = 32767.f * math::kSqrt2;
  const int kMapping[4][3] = {{1, 2, 3}, {0, 2, 3}, {0, 1, 3}, {0, 1, 2}};
  const int* map = kMapping[largest];
  const int a = static_cast<int>(floor(quat[map[0]] * kFloat2Int + .5f));
  const int b = static_cast<int>(floor(quat[map[1]] * kFloat2Int + .5f));
  const int c = static_cast<int>(floor(quat[map[2]] * kFloat2Int + .5f));
  _dest->value[0] = math::Clamp(-32767, a, 32767) & 0xffff;
  _dest->value[1] = math::Clamp(-32767, b, 32767) & 0xffff;
  _dest->value[2] = math::Clamp(-32767, c, 32767) & 0xffff;
}

// Specialize for rotations in order to normalize quaternions.
// Consecutive opposite quaternions are also fixed up in order to avoid checking
// for the smallest path during the NLerp runtime algorithm.
void CopyAnimated(ozz::vector<SortingRotationKey>* _src,
                  ozz::Range<QuaternionKey>* _dest, float _inv_duration) {
  const size_t src_count = _src->size();

  // Normalize quaternions.
  // Also fixes-up successive opposite quaternions that would fail to take the
  // shortest path during the normalized-lerp.
  // Note that keys are still sorted per-track at that point, which allows this
  // algorithm to process all consecutive keys.
  size_t track = std::numeric_limits<size_t>::max();
  const math::Quaternion identity = math::Quaternion::identity();
  SortingRotationKey* src = &_src->front();
  for (size_t i = 0; i < src_count; ++i) {
    math::Quaternion normalized = NormalizeSafe(src[i].key.value, identity);
    if (track != src[i].track) {   // First key of the track.
      if (normalized.w < 0.f) {    // .w eq to a dot with identity quaternion.
        normalized = -normalized;  // Q an -Q are the same rotation.
      }
    } else {  // Still on the same track: so fixes-up quaternion.
      const math::Float4 prev(src[i - 1].key.value.x, src[i - 1].key.value.y,
                              src[i - 1].key.value.z, src[i - 1].key.value.w);
      const math::Float4 curr(normalized.x, normalized.y, normalized.z,
                              normalized.w);
      if (Dot(prev, curr) < 0.f) {
        normalized = -normalized;  // Q an -Q are the same rotation.
      }
    }
    // Stores fixed-up quaternion.
    src[i].key.value = normalized;
    track = src[i].track;
  }

  // Sort.
  std::sort(array_begin(*_src), array_end(*_src),
            &SortingKeyLess<SortingRotationKey>);

  // Fills rotation keys output.
  for (size_t i = 0; i < src_count; ++i) {
    const SortingRotationKey& skey = src[i];
    QuaternionKey& dkey = _dest->begin[i];
    dkey.ratio = skey.key.time * _inv_duration;
    dkey.track = skey.track;

    // Compress quaternion to destination container.
    CompressQuat(skey.key.value, &dkey);
  }
}

template <typename _Dest, typename _Getter>
void CopyConstant(const ozz::Range<const RawAnimation::JointTrack>& _tracks,
                  const ozz::Range<const TrackType>& _types,
                  const ozz::Range<_Dest>& _dest, const _Getter& _getter) {
  const size_t num_tracks = _tracks.count();
  if (num_tracks == 0) {
    return;
  }
  _Dest* dest = _dest.begin;
  for (size_t i = 0; i < num_tracks; ++i) {
    if (_types[i / 4] != TrackType::kConstant) {
      continue;
    }
    *dest = _getter(_tracks[i]);
    ++dest;
  }

  const size_t last_track = num_tracks - 1;
  if (_types[last_track / 4] == TrackType::kConstant) {
    for (size_t i = num_tracks; i < math::Align(num_tracks, 4); ++i) {
      *dest = _getter(_tracks[last_track]);  // Reuse last valid key
      ++dest;
    }
  }
  assert(dest == _dest.end);
}

void CopyTypes(const ozz::vector<TrackType>& _src,
               const Range<uint8_t>& _dest) {
  assert((_src.size() + 3) / 4 == _dest.count());
  memset(_dest.begin, 0, _dest.size());
  for (size_t i = 0; i < _src.size(); ++i) {
    const size_t byte_offset = i / 4;
    const size_t bit_offset = (i - byte_offset * 4) * 2;
    _dest[byte_offset] |= static_cast<uint8_t>(_src[i]) << bit_offset;
  }
}

template <typename _Track>
TrackType GetTrackType(const _Track& _track) {
  const size_t size = _track.size();
  if (size == 0) {
    return TrackType::kIdentity;
  } else if (size == 1) {
    return TrackType::kConstant;
  }
  return TrackType::kAnimated;
}

// Group types 4 by 4 to match with SoA data structure.
// The common minimum denominator for each group of 4 is chosen.
ozz::vector<TrackType> ToSoaTypes(const ozz::vector<TrackType>& _types) {
  ozz::vector<TrackType> soa;
  const size_t size = _types.size();
  for (size_t i = 0; i < math::Align(size, 4); i += 4) {
    TrackType ltypes[4] = {TrackType::kIdentity, TrackType::kIdentity,
                           TrackType::kIdentity, TrackType::kIdentity};
    // Finds minimum denominator.
    for (size_t j = 0; j < 4; ++j) {
      if (i + j < size) {
        ltypes[j] = _types[i + j];
      }
    }
    const TrackType minimum = *std::min_element(ltypes, ltypes + 4);
    soa.push_back(minimum);
  }
  return soa;
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
  const float inv_duration = 1.f / _input.duration;
  animation->duration_ = duration;
  // A _duration == 0 would create some division by 0 during sampling.
  // Also we need at least to keys with different times, which cannot be done
  // if duration is 0.
  assert(duration > 0.f);  // This case is handled by Validate().

  // Sets tracks count. Can be safely casted to uint16_t as number of tracks as
  // already been validated.
  const size_t num_tracks = static_cast<size_t>(_input.num_tracks());
  const size_t num_soa_tracks = (num_tracks + 3) / 4;

  ozz::vector<TrackType> translation_types(num_soa_tracks * 4,
                                           TrackType::kIdentity);
  ozz::vector<TrackType> rotation_types(num_soa_tracks * 4,
                                        TrackType::kIdentity);
  ozz::vector<TrackType> scale_types(num_soa_tracks * 4, TrackType::kIdentity);
  for (size_t i = 0; i < num_tracks; ++i) {
    const RawAnimation::JointTrack& raw_track = _input.tracks[i];
    translation_types[i] = GetTrackType(raw_track.translations);
    rotation_types[i] = GetTrackType(raw_track.rotations);
    scale_types[i] = GetTrackType(raw_track.scales);
  }

  // Group types 4 by 4 to match with SoA SamplingJob requirement.
  translation_types = ToSoaTypes(translation_types);
  rotation_types = ToSoaTypes(rotation_types);
  scale_types = ToSoaTypes(scale_types);

  ozz::vector<SortingTranslationKey> sorting_translations;
  ozz::vector<SortingRotationKey> sorting_rotations;
  ozz::vector<SortingScaleKey> sorting_scales;

  // Filters RawAnimation keys and copies them to the output sorting structure.
  uint16_t translation_index = 0;
  uint16_t rotation_index = 0;
  uint16_t scale_index = 0;
  for (size_t i = 0; i < num_tracks; ++i) {
    const RawAnimation::JointTrack& raw_track = _input.tracks[i];
    if (translation_types[i / 4] == TrackType::kAnimated) {
      CopyRaw(raw_track.translations, translation_index++, duration,
              &sorting_translations);
    }
    if (rotation_types[i / 4] == TrackType::kAnimated) {
      CopyRaw(raw_track.rotations, rotation_index++, duration,
              &sorting_rotations);
    }
    if (scale_types[i / 4] == TrackType::kAnimated) {
      CopyRaw(raw_track.scales, scale_index++, duration, &sorting_scales);
    }
  }

  // Copy constant tracks.
  const size_t const_translation_soa_count = std::count(
      translation_types.begin(), translation_types.end(), TrackType::kConstant);
  const size_t const_rotation_soa_count = std::count(
      rotation_types.begin(), rotation_types.end(), TrackType::kConstant);
  const size_t const_scale_soa_count =
      std::count(scale_types.begin(), scale_types.end(), TrackType::kConstant);

  // Allocate animation members.
  animation->Allocate(_input.name.length(), sorting_translations.size(),
                      sorting_rotations.size(), sorting_scales.size(),
                      const_translation_soa_count, const_rotation_soa_count,
                      const_scale_soa_count, num_tracks);

  // Copy sorted keys to final animation.
  CopyAnimated(&sorting_translations, &animation->translations_, inv_duration);
  CopyAnimated(&sorting_rotations, &animation->rotations_, inv_duration);
  CopyAnimated(&sorting_scales, &animation->scales_, inv_duration);

  // Copy constant keys
  CopyConstant(make_range(_input.tracks), make_range(translation_types),
               animation->const_translations_,
               [](const RawAnimation::JointTrack& _track) -> math::Float3 {
                 return _track.translations.size() != 0
                            ? _track.translations[0].value
                            : math::Float3(0.f);
               });
  CopyConstant(make_range(_input.tracks), make_range(rotation_types),
               animation->const_rotations_,
               [](const RawAnimation::JointTrack& _track) -> math::Quaternion {
                 return _track.rotations.size() != 0
                            ? _track.rotations[0].value
                            : math::Quaternion::identity();
               });
  CopyConstant(make_range(_input.tracks), make_range(scale_types),
               animation->const_scales_,
               [](const RawAnimation::JointTrack& _track) -> math::Float3 {
                 return _track.scales.size() != 0 ? _track.scales[0].value
                                                  : math::Float3(1.f);
               });

  CopyTypes(translation_types, animation->translation_types);
  CopyTypes(rotation_types, animation->rotation_types);
  CopyTypes(scale_types, animation->scale_types);

  // Copy animation's name.
  if (animation->name_) {
    strcpy(animation->name_, _input.name.c_str());
  }
  return animation;  // Success.
}
}  // namespace offline
}  // namespace animation
}  // namespace ozz
