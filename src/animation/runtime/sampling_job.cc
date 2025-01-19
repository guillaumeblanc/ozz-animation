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

#include "ozz/animation/runtime/sampling_job.h"

#include <algorithm>
#include <cassert>
#include <limits>

#include "ozz/animation/runtime/animation.h"
#include "ozz/base/encode/group_varint.h"
#include "ozz/base/maths/math_constant.h"
#include "ozz/base/maths/math_ex.h"
#include "ozz/base/maths/soa_transform.h"
#include "ozz/base/memory/allocator.h"

// Internal include file
#define OZZ_INCLUDE_PRIVATE_HEADER  // Allows to include private headers.
#include "animation/runtime/animation_keyframe.h"

namespace ozz {
namespace animation {

namespace internal {
struct InterpSoaFloat3 {
  math::SimdFloat4 ratio[2];
  math::SoaFloat3 value[2];
};
struct InterpSoaQuaternion {
  math::SimdFloat4 ratio[2];
  math::SoaQuaternion value[2];
};
}  // namespace internal

bool SamplingJob::Validate() const {
  // Don't need any early out, as jobs are valid in most of the performance
  // critical cases.
  // Tests are written in multiple lines in order to avoid branches.
  bool valid = true;

  // Test for nullptr pointers.
  if (!animation || !context) {
    return false;
  }
  valid &= !output.empty();

  const int num_soa_tracks = animation->num_soa_tracks();

  // Tests context size.
  valid &= context->max_soa_tracks() >= num_soa_tracks;

  return valid;
}

namespace {
inline uint32_t TrackForward(const ozz::span<const uint32_t> _cache,
                             const ozz::span<const uint16_t>& _previouses,
                             uint32_t _key, uint32_t _last_track,
                             uint32_t _num_tracks) {
  assert(_key < _previouses.size());
  assert(_last_track >= 0 && _last_track < _num_tracks);

  const uint32_t target = _key - _previouses[_key];
  for (uint32_t entry = _last_track; entry < _num_tracks; ++entry) {
    if (_cache[entry] == target) {
      return entry;
    }
  }
  for (uint32_t entry = 0;; ++entry) {
    if (_cache[entry] == target) {
      return entry;
    }
    assert(entry < _last_track && "Previous track should be in cache");
  }
}

inline uint32_t TrackBackward(const ozz::span<const uint32_t> _cache,
                              uint32_t _target, uint32_t _last_track,
                              uint32_t _num_tracks) {
  assert(_last_track >= 0 && _last_track < _num_tracks);

  for (uint32_t entry = _last_track;; --entry) {
    if (_cache[entry] == _target) {
      return entry;
    }
    if (entry == 0) {
      break;
    }
  }
  for (uint32_t entry = _num_tracks - 1;; --entry) {
    if (_cache[entry] == _target) {
      return entry;
    }
    assert(entry > _last_track && "Previous track should be in cache");
  }
}

inline float KeyRatio(const ozz::span<const float>& _timepoints,
                      const ozz::span<const byte>& _ratios, size_t _at) {
  if (_timepoints.size() <= std::numeric_limits<uint8_t>::max()) {
    return _timepoints[reinterpret_span<const uint8_t>(_ratios)[_at]];
  } else {
    return _timepoints[reinterpret_span<const uint16_t>(_ratios)[_at]];
  }
}

inline ozz::math::SimdFloat4 KeysRatio(
    const ozz::span<const float>& _timepoints,
    const ozz::span<const byte>& _ratios,
    const ozz::span<const uint32_t>& _ats) {
  if (_timepoints.size() <= std::numeric_limits<uint8_t>::max()) {
    const auto& ratios = reinterpret_span<const uint8_t>(_ratios);
    return ozz::math::simd_float4::Load(
        _timepoints[ratios[_ats[0]]], _timepoints[ratios[_ats[1]]],
        _timepoints[ratios[_ats[2]]], _timepoints[ratios[_ats[3]]]);
  } else {
    const auto& ratios = reinterpret_span<const uint16_t>(_ratios);
    return ozz::math::simd_float4::Load(
        _timepoints[ratios[_ats[0]]], _timepoints[ratios[_ats[1]]],
        _timepoints[ratios[_ats[2]]], _timepoints[ratios[_ats[3]]]);
  }
}

inline uint32_t InitializeCache(const Animation::KeyframesCtrlConst& _ctrl,
                                size_t _iframe,
                                const ozz::span<uint32_t>& _entries) {
  if (_iframe > 0) {
    // Initializes cache entries from a compressed cache iframe.
    size_t iframe = (_iframe - 1) * 2;
    const size_t offset = _ctrl.iframe_desc[iframe];
    ozz::DecodeGV4Stream(_ctrl.iframe_entries.subspan(
                             offset, _ctrl.iframe_entries.size() - offset),
                         _entries);

    // Find "next" keyframe, aka the one after the last cached one.
    return _ctrl.iframe_desc[iframe + 1] + 1;
  } else {
    // Initializes cache entries with the first 2nd sets of key frames. The
    // sorting algorithm ensures that the first 2 key frames of a track are
    // consecutive.
    const uint32_t num_tracks = static_cast<uint32_t>(_entries.size());
    for (uint32_t i = 0; i < num_tracks; ++i) {
      _entries[i] = i + num_tracks;
    }

    // Next is set to the next unprocessed keyframe
    return num_tracks * 2;
  }
}

// Outdates all entries. It's important to only flag valid soa entries as this
// is the exit condition of other algorithms.
inline void OutdateCache(const ozz::span<byte>& _outdated,
                         size_t _num_soa_tracks) {
  const size_t num_outdated_flags = (_num_soa_tracks + 7) / 8;
  size_t i = 0;
  for (; i < num_outdated_flags - 1; ++i) {
    _outdated[i] = 0xff;
  }
  _outdated[i] = 0xff >> (num_outdated_flags * 8 - _num_soa_tracks);
}

// Loops through the sorted key frames and update cache structure.
void UpdateCache(float _ratio, float _previous_ratio, size_t _num_soa_tracks,
                 const ozz::span<const float>& _timepoints,
                 const Animation::KeyframesCtrlConst& _ctrl,
                 SamplingJob::Context::Cache& _cache) {
  assert(_num_soa_tracks > 0);
  const uint32_t num_tracks = static_cast<uint32_t>(_num_soa_tracks * 4);
  assert(_ctrl.previouses.begin() + num_tracks * 2 <= _ctrl.previouses.end());
  const uint32_t num_keys = static_cast<uint32_t>(_ctrl.previouses.size());

  uint32_t next = _cache.next;
  assert(next == 0 || (next >= num_tracks * 2 && next <= num_keys));

  // Initialize cache if needed.
  const float delta = _ratio - _previous_ratio;
  if (next == 0 || std::abs(delta) > _ctrl.iframe_interval / 2.f) {
    int iframe = -1;
    if (!_ctrl.iframe_desc.empty()) {
      // First time, or fast seeking into animation.
      // Finds the closest iframe to the expected _ratio.
      iframe = static_cast<int>(.5f + _ratio / _ctrl.iframe_interval);
    } else if (next == 0 || delta < 0.f) {
      // This handles the cases:
      // - First time, and no iframe
      // - Time is going backward, seeking toward 0, and no iframe is defined.
      // In this case it can still be valuable to reset cache to animation
      // begining.
      iframe = 0;
    }

    // Seek to defined keyframe
    if (iframe >= 0) {
      next = InitializeCache(_ctrl, iframe, _cache.entries.first(num_tracks));
      assert(next >= num_tracks * 2 && next <= num_keys);

      // Cache was overwritten, all entries must be flagged as outdated.
      OutdateCache(_cache.outdated, _num_soa_tracks);
    }
  }

  // Reading forward.
  // Iterates while the cache is not updated with previous key required for
  // interpolation at _ratio. Thanks to the keyframe sorting, the loop can end
  // as soon as it finds a key greater that _ratio. It will mean that all the
  // keys lower than _ratio have been processed, meaning all cache entries are
  // up to date.
  uint32_t track = 0;
  for (; next < num_keys && KeyRatio(_timepoints, _ctrl.ratios,
                                     next - _ctrl.previouses[next]) <= _ratio;
       ++next) {
    // Finds track index.
    track =
        TrackForward(_cache.entries, _ctrl.previouses, next, track, num_tracks);
    assert(_cache.entries[track] == next - _ctrl.previouses[next] &&
           "Wrong cache entry.");

    // Flag this soa entry as outdated.
    _cache.outdated[track / 32] |= 1 << ((track & 0x1f) / 4);

    // Updates cache.
    _cache.entries[track] = next;
  }

  // Rewinds.
  // Checks if the time of the penultimate key is greater than _ratio, in which
  // case we need to rewind.
  for (; KeyRatio(_timepoints, _ctrl.ratios,
                  (next - 1) - _ctrl.previouses[next - 1]) > _ratio;
       --next) {
    assert(next - 1 >= num_tracks * 2);

    // Finds track index.
    track = TrackBackward(_cache.entries, next - 1, track, num_tracks);

    // Flag this soa entry as outdated.
    _cache.outdated[track / 32] |= 1 << ((track & 0x1f) / 4);

    // Updates cache.
    assert(_cache.entries[track] == next - 1);
    const uint32_t previous = _ctrl.previouses[_cache.entries[track]];
    assert(_cache.entries[track] >= previous + num_tracks);
    _cache.entries[track] -= previous;
  }

  // Updates next output.
  assert(next >= num_tracks * 2 && next <= num_keys);
  _cache.next = next;
}

template <typename _CompressedKey, typename _DecompressedKey,
          typename _Decompress>
inline void Decompress(size_t _num_soa_tracks,
                       const ozz::span<const float>& _timepoints,
                       const Animation::KeyframesCtrlConst& _ctrl,
                       const ozz::span<const _CompressedKey>& _compressed,
                       const SamplingJob::Context::Cache& _cache,
                       const ozz::span<_DecompressedKey>& _decompressed,
                       const _Decompress& _decompress) {
  const size_t num_outdated_flags = (_num_soa_tracks + 7) / 8;
  for (size_t j = 0; j < num_outdated_flags; ++j) {
    byte outdated = _cache.outdated[j];  // Copy outdated flag
    _cache.outdated[j] = 0;  // Reset outdated entries as all will be processed.
    for (size_t i = j * 8; outdated != 0; ++i, outdated >>= 1) {
      if (!(outdated & 1)) {
        continue;
      }

      // Get cache sub part matching this outdated soa entry.
      const auto& rights = _cache.entries.subspan(i * 4, 4);

      // Left side keys can be found from right ones as we know the offset from
      // right to left (_previouses).
      const uint32_t lefts[4] = {rights[0] - _ctrl.previouses[rights[0]],
                                 rights[1] - _ctrl.previouses[rights[1]],
                                 rights[2] - _ctrl.previouses[rights[2]],
                                 rights[3] - _ctrl.previouses[rights[3]]};

      // Decompress left side keyframes and store them in soa structures.
      const _CompressedKey& k00 = _compressed[lefts[0]];
      const _CompressedKey& k10 = _compressed[lefts[1]];
      const _CompressedKey& k20 = _compressed[lefts[2]];
      const _CompressedKey& k30 = _compressed[lefts[3]];
      _decompressed[i].ratio[0] = KeysRatio(_timepoints, _ctrl.ratios, lefts);
      _decompress(k00, k10, k20, k30, &_decompressed[i].value[0]);

      // Decompress right side keyframes and store them in soa structures.
      const _CompressedKey& k01 = _compressed[rights[0]];
      const _CompressedKey& k11 = _compressed[rights[1]];
      const _CompressedKey& k21 = _compressed[rights[2]];
      const _CompressedKey& k31 = _compressed[rights[3]];
      _decompressed[i].ratio[1] = KeysRatio(_timepoints, _ctrl.ratios, rights);
      _decompress(k01, k11, k21, k31, &_decompressed[i].value[1]);
    }
  }
}

inline void DecompressFloat3(const internal::Float3Key& _k0,
                             const internal::Float3Key& _k1,
                             const internal::Float3Key& _k2,
                             const internal::Float3Key& _k3,
                             math::SoaFloat3* _soa_float3) {
  _soa_float3->x = math::HalfToFloat(math::simd_int4::Load(
      _k0.values[0], _k1.values[0], _k2.values[0], _k3.values[0]));
  _soa_float3->y = math::HalfToFloat(math::simd_int4::Load(
      _k0.values[1], _k1.values[1], _k2.values[1], _k3.values[1]));
  _soa_float3->z = math::HalfToFloat(math::simd_int4::Load(
      _k0.values[2], _k1.values[2], _k2.values[2], _k3.values[2]));
}

// Defines a mapping table that defines components assignation in the output
// quaternion.
static constexpr uint8_t kCpntMapping[4][4] = {
    {0, 0, 1, 2}, {0, 0, 1, 2}, {0, 1, 0, 2}, {0, 1, 2, 0}};

inline void DecompressQuaternion(const internal::QuaternionKey& _k0,
                                 const internal::QuaternionKey& _k1,
                                 const internal::QuaternionKey& _k2,
                                 const internal::QuaternionKey& _k3,
                                 math::SoaQuaternion* _quaternion) {
  int largests[4], signs[4], values[4][3];
  internal::unpack(_k0, largests[0], signs[0], values[0]);
  internal::unpack(_k1, largests[1], signs[1], values[1]);
  internal::unpack(_k2, largests[2], signs[2], values[2]);
  internal::unpack(_k3, largests[3], signs[3], values[3]);

  // Selects proper mapping for each key.
  const uint8_t* m0 = kCpntMapping[largests[0]];
  const uint8_t* m1 = kCpntMapping[largests[1]];
  const uint8_t* m2 = kCpntMapping[largests[2]];
  const uint8_t* m3 = kCpntMapping[largests[3]];

  // Prepares an array of input values, according to the mapping required to
  // restore quaternion largest component.
  alignas(16) int cmp_keys[4][4] = {
      {values[0][m0[0]], values[1][m1[0]], values[2][m2[0]], values[3][m3[0]]},
      {values[0][m0[1]], values[1][m1[1]], values[2][m2[1]], values[3][m3[1]]},
      {values[0][m0[2]], values[1][m1[2]], values[2][m2[2]], values[3][m3[2]]},
      {values[0][m0[3]], values[1][m1[3]], values[2][m2[3]], values[3][m3[3]]},
  };

  // Rebuilds quaternion from quantized values.
  const math::SimdFloat4 kScale =
      math::simd_float4::Load1(math::kSqrt2 / internal::QuaternionKey::kfScale);
  const math::SimdFloat4 kOffset = math::simd_float4::Load1(-math::kSqrt2_2);
  math::SimdFloat4 cpnt[4] = {
      kScale * math::simd_float4::FromInt(
                   math::simd_int4::LoadPtr(cmp_keys[0])) +
          kOffset,
      kScale * math::simd_float4::FromInt(
                   math::simd_int4::LoadPtr(cmp_keys[1])) +
          kOffset,
      kScale * math::simd_float4::FromInt(
                   math::simd_int4::LoadPtr(cmp_keys[2])) +
          kOffset,
      kScale * math::simd_float4::FromInt(
                   math::simd_int4::LoadPtr(cmp_keys[3])) +
          kOffset};

  // Zeroed largest components so they're not part of the dot.
  const math::SimdInt4 mask_f000 = math::simd_int4::mask_f000();
  const math::SimdInt4 mask_0f00 = math::simd_int4::mask_0f00();
  const math::SimdInt4 mask_00f0 = math::simd_int4::mask_00f0();
  const math::SimdInt4 mask_000f = math::simd_int4::mask_000f();
  cpnt[largests[0]] = math::AndNot(cpnt[largests[0]], mask_f000);
  cpnt[largests[1]] = math::AndNot(cpnt[largests[1]], mask_0f00);
  cpnt[largests[2]] = math::AndNot(cpnt[largests[2]], mask_00f0);
  cpnt[largests[3]] = math::AndNot(cpnt[largests[3]], mask_000f);

  // Get back length of 4th component. Favors performance over accuracy by using
  // x * RSqrtEst(x) instead of Sqrt(x).
  // ww0 cannot be 0 because we 're recomputing the largest component.
  const math::SimdFloat4 dot = cpnt[0] * cpnt[0] + cpnt[1] * cpnt[1] +
                               cpnt[2] * cpnt[2] + cpnt[3] * cpnt[3];
  // dot cannot be >= 1, because it does not include the largest component.
  const math::SimdFloat4 ww0 = math::simd_float4::one() - dot;
  const math::SimdFloat4 w0 = ww0 * math::RSqrtEst(ww0);

  // Re-applies 4th component's sign.
  const math::SimdInt4 sign = math::ShiftL(
      math::simd_int4::Load(signs[0], signs[1], signs[2], signs[3]), 31);
  const math::SimdFloat4 restored = math::Or(w0, sign);

  // Re-injects the largest component inside the SoA structure.
  // Note that largest component is already 0.
  cpnt[largests[0]] =
      math::Or(cpnt[largests[0]], math::And(restored, mask_f000));
  cpnt[largests[1]] =
      math::Or(cpnt[largests[1]], math::And(restored, mask_0f00));
  cpnt[largests[2]] =
      math::Or(cpnt[largests[2]], math::And(restored, mask_00f0));
  cpnt[largests[3]] =
      math::Or(cpnt[largests[3]], math::And(restored, mask_000f));

  // Stores result.
  _quaternion->x = cpnt[0];
  _quaternion->y = cpnt[1];
  _quaternion->z = cpnt[2];
  _quaternion->w = cpnt[3];
}

void Interpolates(float _anim_ratio, size_t _num_soa_tracks,
                  const span<const internal::InterpSoaFloat3>& _translations,
                  const span<const internal::InterpSoaQuaternion>& _rotations,
                  const span<const internal::InterpSoaFloat3>& _scales,
                  const span<math::SoaTransform>& _output) {
  const math::SimdFloat4 anim_ratio = math::simd_float4::Load1(_anim_ratio);
  for (size_t i = 0; i < _num_soa_tracks; ++i) {
    // Prepares interpolation coefficients.
    const internal::InterpSoaFloat3& t = _translations[i];
    const math::SimdFloat4 t_ratio =
        (anim_ratio - t.ratio[0]) * math::RcpEst(t.ratio[1] - t.ratio[0]);
    const internal::InterpSoaQuaternion& r = _rotations[i];
    const math::SimdFloat4 r_ratio =
        (anim_ratio - r.ratio[0]) * math::RcpEst(r.ratio[1] - r.ratio[0]);
    const internal::InterpSoaFloat3& s = _scales[i];
    const math::SimdFloat4 s_ratio =
        (anim_ratio - s.ratio[0]) * math::RcpEst(s.ratio[1] - s.ratio[0]);

    // Processes interpolations.
    // The lerp of the rotation uses the shortest path, because opposed
    // quaternions were negated during animation build stage (see
    // AnimationBuilder).
    _output[i].translation = Lerp(t.value[0], t.value[1], t_ratio);
    _output[i].rotation = NLerpEst(r.value[0], r.value[1], r_ratio);
    _output[i].scale = Lerp(s.value[0], s.value[1], s_ratio);
  }
}
}  // namespace

bool SamplingJob::Run() const {
  if (!Validate()) {
    return false;
  }

  // Checked during validation
  assert(context->max_soa_tracks() >= animation->num_soa_tracks());

  // Early out if animation contains no joint.
  const size_t num_soa_tracks =
      static_cast<size_t>(animation->num_soa_tracks());
  if (num_soa_tracks <= 0) {
    return true;
  }

  // Clamps ratio in range [0,duration].
  const float clamped_ratio = math::Clamp(0.f, ratio, 1.f);

  // Step the context to this potentially new animation and ratio.
  const float previous_ratio = context->Step(*animation, clamped_ratio);

  // Update cache with animation keyframe indexes for t = ratio.
  // Decompresses outdated soa hot values.

  // Translations
  const Animation::KeyframesCtrlConst& translations_ctrl =
      animation->translations_ctrl();
  UpdateCache(clamped_ratio, previous_ratio, num_soa_tracks,
              animation->timepoints(), translations_ctrl,
              context->translations_cache_);
  Decompress(num_soa_tracks, animation->timepoints(), translations_ctrl,
             animation->translations_values(), context->translations_cache_,
             context->translations_, &DecompressFloat3);

  // Rotations
  const Animation::KeyframesCtrlConst& rotations_ctrl =
      animation->rotations_ctrl();
  UpdateCache(clamped_ratio, previous_ratio, num_soa_tracks,
              animation->timepoints(), rotations_ctrl,
              context->rotations_cache_);
  Decompress(num_soa_tracks, animation->timepoints(), rotations_ctrl,
             animation->rotations_values(), context->rotations_cache_,
             context->rotations_, &DecompressQuaternion);

  // Scales
  const Animation::KeyframesCtrlConst& scales_ctrl = animation->scales_ctrl();
  UpdateCache(clamped_ratio, previous_ratio, num_soa_tracks,
              animation->timepoints(), scales_ctrl, context->scales_cache_);
  Decompress(num_soa_tracks, animation->timepoints(), scales_ctrl,
             animation->scales_values(), context->scales_cache_,
             context->scales_, &DecompressFloat3);

  // Only interp as much as we have output for.
  const size_t num_soa_interp_tracks = math::Min(output.size(), num_soa_tracks);

  // Interpolates soa hot data.
  Interpolates(clamped_ratio, num_soa_interp_tracks, context->translations_,
               context->rotations_, context->scales_, output);

  return true;
}

SamplingJob::Context::Context() : max_soa_tracks_(0) { Invalidate(); }

SamplingJob::Context::Context(int _max_tracks) : max_soa_tracks_(0) {
  Resize(_max_tracks);
}

SamplingJob::Context::~Context() { Deallocate(); }

void SamplingJob::Context::Deallocate() {
  memory::default_allocator()->Deallocate(allocation_);
  allocation_ = nullptr;
}

void SamplingJob::Context::Resize(int _max_tracks) {
  using internal::InterpSoaFloat3;
  using internal::InterpSoaQuaternion;

  // Reset existing data.
  Invalidate();
  Deallocate();

  // Updates maximum supported soa tracks.
  max_soa_tracks_ = (math::Max(0, _max_tracks) + 3) / 4;
  const size_t max_soa_tracks = static_cast<size_t>(max_soa_tracks_);

  // Allocate all context data at once in a single allocation.
  // Alignment is guaranteed because memory is dispatch from the highest
  // alignment requirement (Soa data: SimdFloat4) to the lowest (outdated
  // flag: unsigned char).

  // Computes allocation size.
  const size_t max_tracks = max_soa_tracks * 4;
  const size_t num_outdated = (max_soa_tracks + 7) / 8;
  const size_t size =
      sizeof(InterpSoaFloat3) * max_soa_tracks +
      sizeof(InterpSoaQuaternion) * max_soa_tracks +
      sizeof(InterpSoaFloat3) * max_soa_tracks +
      sizeof(uint32_t) * max_tracks * 3 +  // trans + rot + scale.
      sizeof(uint8_t) * 3 * num_outdated;

  // Allocates all at once.
  auto* allocator = memory::default_allocator();
  allocation_ = allocator->Allocate(size, alignof(InterpSoaFloat3));
  span<byte> buffer = {static_cast<byte*>(allocation_), size};

  // Distributes buffer memory while ensuring proper alignment (serves larger
  // alignment values first).
  static_assert(alignof(InterpSoaFloat3) >= alignof(InterpSoaQuaternion) &&
                    alignof(InterpSoaQuaternion) >= alignof(InterpSoaFloat3) &&
                    alignof(InterpSoaFloat3) >= alignof(uint32_t) &&
                    alignof(uint32_t) >= alignof(byte),
                "Must serve larger alignment values first)");

  translations_ = fill_span<InterpSoaFloat3>(buffer, max_soa_tracks);
  rotations_ = fill_span<InterpSoaQuaternion>(buffer, max_soa_tracks);
  scales_ = fill_span<InterpSoaFloat3>(buffer, max_soa_tracks);

  translations_cache_.entries = fill_span<uint32_t>(buffer, max_tracks);
  rotations_cache_.entries = fill_span<uint32_t>(buffer, max_tracks);
  scales_cache_.entries = fill_span<uint32_t>(buffer, max_tracks);

  translations_cache_.outdated = fill_span<byte>(buffer, num_outdated);
  rotations_cache_.outdated = fill_span<byte>(buffer, num_outdated);
  scales_cache_.outdated = fill_span<byte>(buffer, num_outdated);

  assert(buffer.empty());
}

float SamplingJob::Context::Step(const Animation& _animation, float _ratio) {
  // The cache is invalidated if animation has changed...
  if (animation_ != &_animation) {
    Invalidate();
    animation_ = &_animation;
  }
  const float previous_ratio = ratio_;
  ratio_ = _ratio;
  return previous_ratio;
}

void SamplingJob::Context::Invalidate() {
  animation_ = nullptr;
  ratio_ = 0.f;
  translations_cache_.next = 0;
  rotations_cache_.next = 0;
  scales_cache_.next = 0;
}
}  // namespace animation
}  // namespace ozz
