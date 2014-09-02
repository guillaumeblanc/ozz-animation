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

#include "ozz/animation/runtime/sampling_job.h"

#include <cassert>

#include "ozz/base/maths/soa_transform.h"
#include "ozz/base/memory/allocator.h"
#include "ozz/animation/runtime/animation.h"

// Internal include file
#define OZZ_INCLUDE_PRIVATE_HEADER  // Allows to include private headers.
#include "../runtime/key_frame.h"

namespace ozz {
namespace animation {

namespace internal {
struct InterpSoaTranslation {
  math::SimdFloat4 time[2];
  math::SoaFloat3 value[2];
};
struct InterpSoaRotation {
  math::SimdFloat4 time[2];
  math::SoaQuaternion value[2];
};
struct InterpSoaScale {
  math::SimdFloat4 time[2];
  math::SoaFloat3 value[2];
};
}  // internal
bool SamplingJob::Validate() const {
  // Don't need any early out, as jobs are valid in most of the performance
  // critical cases.
  // Tests are written in multiple lines in order to avoid branches.
  bool valid = true;

  // Test for NULL pointers.
  if (!animation || !cache) {
    return false;
  }
  valid &= output.begin != NULL;

  // Tests output range, implicitly tests output.end != NULL.
  const ptrdiff_t num_soa_tracks = animation->num_soa_tracks();
  valid &= output.end - output.begin >= num_soa_tracks;

  // Tests cache size.
  valid &= cache->max_soa_tracks() >= num_soa_tracks;

  return valid;
}

namespace {
// Loops through the sorted key frames and update cache structure.
template<typename _Key>
void UpdateKeys(float _time, int _num_soa_tracks,
                ozz::Range<const _Key> _keys,
                int* _cursor,
                int* _cache, unsigned char* _outdated) {
    assert(_num_soa_tracks >= 1);
    const int num_tracks = _num_soa_tracks * 4;
    assert(_keys.begin + num_tracks * 2 <= _keys.end);

    const _Key* cursor = &_keys.begin[*_cursor];
    if (!*_cursor) {
      // Initializes interpolated entries with the first 2 sets of key frames.
      // The sorting algorithm ensures that the first 2 key frames of a track
      // are consecutive.
      for (int i = 0; i < _num_soa_tracks; ++i) {
        const int in_index0 = i * 4;  // * soa size
        const int in_index1 = in_index0 + num_tracks;  // 2nd row.
        const int out_index = i * 4 * 2;  // * soa size * 2 keys
        _cache[out_index + 0] = in_index0 + 0;
        _cache[out_index + 1] = in_index1 + 0;
        _cache[out_index + 2] = in_index0 + 1;
        _cache[out_index + 3] = in_index1 + 1;
        _cache[out_index + 4] = in_index0 + 2;
        _cache[out_index + 5] = in_index1 + 2;
        _cache[out_index + 6] = in_index0 + 3;
        _cache[out_index + 7] = in_index1 + 3;
      }
      cursor = _keys.begin + num_tracks * 2;  // New cursor position.

      // All entries are outdated. It cares to only flag valid soa entries as
      // this is the exit condition of other algorithms.
      const int num_outdated_flags = (_num_soa_tracks + 7) / 8;
      for (int i = 0; i < num_outdated_flags - 1; ++i) {
        _outdated[i] = 0xff;
      }
      _outdated[num_outdated_flags - 1] =
        0xff >> (num_outdated_flags * 8 - _num_soa_tracks);
    } else {
      assert(cursor >= _keys.begin + num_tracks * 2 && cursor <= _keys.end);
    }

    // Search for the keys that matches _time.
    // Iterates while the cache is not updated with left and right keys required
    // for interpolation at time _time, for all tracks. Thanks to the keyframe
    // sorting, the loop can end as soon as it finds a key greater that _time.
    // It will mean that all the keys lower than _time have been processed,
    // meaning all cache entries are updated. 
    while (cursor < _keys.end &&
           _keys.begin[_cache[cursor->track * 2 + 1]].time <= _time) {
      // Flag this soa entry as outdated.
      _outdated[cursor->track / 32] |= 1 << ((cursor->track & 0x1f) / 4);
      // Updates cache.
      const int base = cursor->track * 2;
      _cache[base] = _cache[base + 1];
      _cache[base + 1] = static_cast<int>(cursor - _keys.begin);
      // Process next key.
      ++cursor;
    }
    assert(cursor <= _keys.end);

    // Updates cursor output.
    *_cursor = static_cast<int>(cursor - _keys.begin);
}

void UpdateSoaTranslations(int _num_soa_tracks,
                           ozz::Range<const TranslationKey> _keys,
                           const int* _interp,
                           unsigned char* _outdated,
                           internal::InterpSoaTranslation* soa_translations_) {
  const int num_outdated_flags = (_num_soa_tracks + 7) / 8;
  for (int j = 0; j < num_outdated_flags; ++j) {
    unsigned char outdated = _outdated[j];
    _outdated[j] = 0;  // Reset outdated entries as all will be processed.
    for (int i = j * 8; outdated; ++i, outdated >>= 1) {
      if (!(outdated & 1)) {
        continue;
      }
      const int base = i * 4 * 2;
      const int interp00 = _interp[base + 0];
      const int interp01 = _interp[base + 1];
      const int interp10 = _interp[base + 2];
      const int interp11 = _interp[base + 3];
      const int interp20 = _interp[base + 4];
      const int interp21 = _interp[base + 5];
      const int interp30 = _interp[base + 6];
      const int interp31 = _interp[base + 7];

      math::SimdFloat4 time0[4];
      math::SimdFloat4 value0[4];
      time0[0] = math::simd_float4::LoadXPtrU(&_keys.begin[interp00].time);
      value0[0] = math::simd_float4::Load3PtrU(&_keys.begin[interp00].value.x);
      time0[1] = math::simd_float4::LoadXPtrU(&_keys.begin[interp10].time);
      value0[1] = math::simd_float4::Load3PtrU(&_keys.begin[interp10].value.x);
      time0[2] = math::simd_float4::LoadXPtrU(&_keys.begin[interp20].time);
      value0[2] = math::simd_float4::Load3PtrU(&_keys.begin[interp20].value.x);
      time0[3] = math::simd_float4::LoadXPtrU(&_keys.begin[interp30].time);
      value0[3] = math::simd_float4::Load3PtrU(&_keys.begin[interp30].value.x);
      math::Transpose4x1(time0, &soa_translations_[i].time[0]);
      math::Transpose4x3(value0, &soa_translations_[i].value[0].x);

      math::SimdFloat4 time1[4];
      math::SimdFloat4 value1[4];
      time1[0] = math::simd_float4::LoadXPtrU(&_keys.begin[interp01].time);
      value1[0] = math::simd_float4::Load3PtrU(&_keys.begin[interp01].value.x);
      time1[1] = math::simd_float4::LoadXPtrU(&_keys.begin[interp11].time);
      value1[1] = math::simd_float4::Load3PtrU(&_keys.begin[interp11].value.x);
      time1[2] = math::simd_float4::LoadXPtrU(&_keys.begin[interp21].time);
      value1[2] = math::simd_float4::Load3PtrU(&_keys.begin[interp21].value.x);
      time1[3] = math::simd_float4::LoadXPtrU(&_keys.begin[interp31].time);
      value1[3] = math::simd_float4::Load3PtrU(&_keys.begin[interp31].value.x);
      math::Transpose4x1(time1, &soa_translations_[i].time[1]);
      math::Transpose4x3(value1, &soa_translations_[i].value[1].x);
    }
  }
}

void UpdateSoaRotations(int _num_soa_tracks,
                        ozz::Range<const RotationKey> _keys,
                        const int* _interp,
                        unsigned char* _outdated,
                        internal::InterpSoaRotation* soa_rotations_) {
  const int num_outdated_flags = (_num_soa_tracks + 7) / 8;
  for (int j = 0; j < num_outdated_flags; ++j) {
    unsigned char outdated = _outdated[j];
    _outdated[j] = 0;  // Reset outdated entries as all will be processed.
    for (int i = j * 8; outdated; ++i, outdated >>= 1) {
      if (!(outdated & 1)) {
        continue;
      }
      const int base = i * 4 * 2;
      const int interp00 = _interp[base + 0];
      const int interp01 = _interp[base + 1];
      const int interp10 = _interp[base + 2];
      const int interp11 = _interp[base + 3];
      const int interp20 = _interp[base + 4];
      const int interp21 = _interp[base + 5];
      const int interp30 = _interp[base + 6];
      const int interp31 = _interp[base + 7];

      math::SimdFloat4 time0[4];
      math::SimdFloat4 value0[4];
      time0[0] = math::simd_float4::LoadXPtrU(&_keys.begin[interp00].time);
      value0[0] = math::simd_float4::LoadPtrU(&_keys.begin[interp00].value.x);
      time0[1] = math::simd_float4::LoadXPtrU(&_keys.begin[interp10].time);
      value0[1] = math::simd_float4::LoadPtrU(&_keys.begin[interp10].value.x);
      time0[2] = math::simd_float4::LoadXPtrU(&_keys.begin[interp20].time);
      value0[2] = math::simd_float4::LoadPtrU(&_keys.begin[interp20].value.x);
      time0[3] = math::simd_float4::LoadXPtrU(&_keys.begin[interp30].time);
      value0[3] = math::simd_float4::LoadPtrU(&_keys.begin[interp30].value.x);
      math::Transpose4x1(time0, &soa_rotations_[i].time[0]);
      math::Transpose4x4(value0, &soa_rotations_[i].value[0].x);

      math::SimdFloat4 time1[4];
      math::SimdFloat4 value1[4];
      time1[0] = math::simd_float4::LoadXPtrU(&_keys.begin[interp01].time);
      value1[0] = math::simd_float4::LoadPtrU(&_keys.begin[interp01].value.x);
      time1[1] = math::simd_float4::LoadXPtrU(&_keys.begin[interp11].time);
      value1[1] = math::simd_float4::LoadPtrU(&_keys.begin[interp11].value.x);
      time1[2] = math::simd_float4::LoadXPtrU(&_keys.begin[interp21].time);
      value1[2] = math::simd_float4::LoadPtrU(&_keys.begin[interp21].value.x);
      time1[3] = math::simd_float4::LoadXPtrU(&_keys.begin[interp31].time);
      value1[3] = math::simd_float4::LoadPtrU(&_keys.begin[interp31].value.x);
      math::Transpose4x1(time1, &soa_rotations_[i].time[1]);
      math::Transpose4x4(value1, &soa_rotations_[i].value[1].x);
    }
  }
}

void UpdateSoaScales(int _num_soa_tracks,
                     ozz::Range<const ScaleKey> _keys,
                     const int* _interp,
                     unsigned char* _outdated,
                     internal::InterpSoaScale* soa_scales_) {
  const int num_outdated_flags = (_num_soa_tracks + 7) / 8;
  for (int j = 0; j < num_outdated_flags; ++j) {
    unsigned char outdated = _outdated[j];
    _outdated[j] = 0;  // Reset outdated entries as all will be processed.
    for (int i = j * 8; outdated; ++i, outdated >>= 1) {
      if (!(outdated & 1)) {
        continue;
      }
      const int base = i * 4 * 2;
      const int interp00 = _interp[base + 0];
      const int interp01 = _interp[base + 1];
      const int interp10 = _interp[base + 2];
      const int interp11 = _interp[base + 3];
      const int interp20 = _interp[base + 4];
      const int interp21 = _interp[base + 5];
      const int interp30 = _interp[base + 6];
      const int interp31 = _interp[base + 7];

      math::SimdFloat4 time0[4];
      math::SimdFloat4 value0[4];
      time0[0] = math::simd_float4::LoadXPtrU(&_keys.begin[interp00].time);
      value0[0] = math::simd_float4::Load3PtrU(&_keys.begin[interp00].value.x);
      time0[1] = math::simd_float4::LoadXPtrU(&_keys.begin[interp10].time);
      value0[1] = math::simd_float4::Load3PtrU(&_keys.begin[interp10].value.x);
      time0[2] = math::simd_float4::LoadXPtrU(&_keys.begin[interp20].time);
      value0[2] = math::simd_float4::Load3PtrU(&_keys.begin[interp20].value.x);
      time0[3] = math::simd_float4::LoadXPtrU(&_keys.begin[interp30].time);
      value0[3] = math::simd_float4::Load3PtrU(&_keys.begin[interp30].value.x);
      math::Transpose4x1(time0, &soa_scales_[i].time[0]);
      math::Transpose4x3(value0, &soa_scales_[i].value[0].x);

      math::SimdFloat4 time1[4];
      math::SimdFloat4 value1[4];
      time1[0] = math::simd_float4::LoadXPtrU(&_keys.begin[interp01].time);
      value1[0] = math::simd_float4::Load3PtrU(&_keys.begin[interp01].value.x);
      time1[1] = math::simd_float4::LoadXPtrU(&_keys.begin[interp11].time);
      value1[1] = math::simd_float4::Load3PtrU(&_keys.begin[interp11].value.x);
      time1[2] = math::simd_float4::LoadXPtrU(&_keys.begin[interp21].time);
      value1[2] = math::simd_float4::Load3PtrU(&_keys.begin[interp21].value.x);
      time1[3] = math::simd_float4::LoadXPtrU(&_keys.begin[interp31].time);
      value1[3] = math::simd_float4::Load3PtrU(&_keys.begin[interp31].value.x);
      math::Transpose4x1(time1, &soa_scales_[i].time[1]);
      math::Transpose4x3(value1, &soa_scales_[i].value[1].x);
    }
  }
}

void Interpolates(float _anim_time,
                  int _num_soa_tracks,
                  const internal::InterpSoaTranslation* _translations,
                  const internal::InterpSoaRotation* _rotations,
                  const internal::InterpSoaScale* _scales,
                  math::SoaTransform* _output) {
    const math::SimdFloat4 anim_time = math::simd_float4::Load1(_anim_time);
    for (int i = 0; i < _num_soa_tracks; ++i) {
      // Prepares interpolation coefficients.
      const math::SimdFloat4 interp_t_time =
        (anim_time - _translations[i].time[0]) *
        math::RcpEst(_translations[i].time[1] - _translations[i].time[0]);
      const math::SimdFloat4 interp_r_time =
        (anim_time - _rotations[i].time[0]) *
        math::RcpEst(_rotations[i].time[1] - _rotations[i].time[0]);
      const math::SimdFloat4 interp_s_time =
        (anim_time - _scales[i].time[0]) *
        math::RcpEst(_scales[i].time[1] - _scales[i].time[0]);

      // Processes interpolations.
      // The lerp of the rotation uses the shortest path, because opposed
      // quaternions were negated during animation build stage (AnimationBuilder).
      _output[i].translation = Lerp(
        _translations[i].value[0], _translations[i].value[1], interp_t_time);
      _output[i].rotation = NLerpEst(
        _rotations[i].value[0], _rotations[i].value[1], interp_r_time);
      _output[i].scale = Lerp(
        _scales[i].value[0], _scales[i].value[1], interp_s_time);
    }
}
}  // namespace

SamplingJob::SamplingJob()
    : time(0.f),
      animation(NULL),
      cache(NULL) {
}

bool SamplingJob::Run() const {
  if (!Validate()) {
    return false;
  }

  const int num_soa_tracks = animation->num_soa_tracks();
  if (num_soa_tracks == 0) {  // Early out if animation contains no joint.
    return true;
  }

  // Clamps time in range [0,duration].
  const float anim_time = math::Clamp(0.f, time, animation->duration());

  // Step the cache to this potentially new animation and time.
  assert(cache->max_soa_tracks() >= num_soa_tracks);
  cache->Step(*animation, anim_time);

  // Fetch key frames from the animation to the cache a t = anim_time.
  // Then updates outdated soa hot values.
  UpdateKeys(anim_time, num_soa_tracks,
             animation->translations(),
             &cache->translation_cursor_,
             cache->translation_keys_,
             cache->outdated_translations_);
  UpdateSoaTranslations(num_soa_tracks,
                        animation->translations(),
                        cache->translation_keys_,
                        cache->outdated_translations_,
                        cache->soa_translations_);

  UpdateKeys(anim_time, num_soa_tracks,
             animation->rotations(),
             &cache->rotation_cursor_,
             cache->rotation_keys_,
             cache->outdated_rotations_);
  UpdateSoaRotations(num_soa_tracks,
                     animation->rotations(),
                     cache->rotation_keys_,
                     cache->outdated_rotations_,
                     cache->soa_rotations_);

  UpdateKeys(anim_time, num_soa_tracks,
             animation->scales(),
             &cache->scale_cursor_,
             cache->scale_keys_,
             cache->outdated_scales_);
  UpdateSoaScales(num_soa_tracks,
                  animation->scales(),
                  cache->scale_keys_,
                  cache->outdated_scales_,
                  cache->soa_scales_);

  // Interpolates soa hot data.
  Interpolates(anim_time,
               num_soa_tracks,
               cache->soa_translations_,
               cache->soa_rotations_,
               cache->soa_scales_,
               output.begin);

  return true;
}

SamplingCache::SamplingCache(int _max_tracks)
    : animation_(NULL),
    time_(0.f),
    max_soa_tracks_((_max_tracks + 3) / 4),
    soa_translations_(NULL),
    soa_rotations_(NULL),
    soa_scales_(NULL),
    translation_keys_(NULL),
    rotation_keys_(NULL),
    scale_keys_(NULL),
    translation_cursor_(0),
    rotation_cursor_(0),
    scale_cursor_(0),
    outdated_translations_(NULL),
    outdated_rotations_(NULL),
    outdated_scales_(NULL) {
  using internal::InterpSoaTranslation;
  using internal::InterpSoaRotation;
  using internal::InterpSoaScale;

  // Allocate all cache data at once in a single allocation.
  // Alignment is guaranteed because memory is dispatch from the highest
  // alignment requirement (Soa data: SimdFloat4) to the lowest (outdated
  // flag: unsigned char).

  // Computes allocation size.
  const size_t max_tracks = max_soa_tracks_ * 4;
  const size_t num_outdated = (max_soa_tracks_ + 7) / 8;
  const size_t size =
    sizeof(InterpSoaTranslation) * max_soa_tracks_  +
    sizeof(InterpSoaRotation) * max_soa_tracks_ +
    sizeof(InterpSoaScale) *max_soa_tracks_ +
    sizeof(int) * max_tracks * 2 * 3 +  // 2 keys * (trans + rot + scale).
    sizeof(unsigned char) * 3 * num_outdated;

  // Allocates all at once.
  memory::Allocator* allocator = memory::default_allocator();
  char* alloc_begin = reinterpret_cast<char*>(
    allocator->Allocate(size, AlignOf<InterpSoaTranslation>::value));
  char* alloc_cursor = alloc_begin;

  // Dispatches allocated memory, from the highest alignment requirement to the
  // lowest.
  soa_translations_ = reinterpret_cast<InterpSoaTranslation*>(alloc_cursor);
  alloc_cursor += sizeof(InterpSoaTranslation) * max_soa_tracks_;
  soa_rotations_ = reinterpret_cast<InterpSoaRotation*>(alloc_cursor);
  alloc_cursor += sizeof(InterpSoaRotation) * max_soa_tracks_;
  soa_scales_ = reinterpret_cast<InterpSoaScale*>(alloc_cursor);
  alloc_cursor += sizeof(InterpSoaScale) * max_soa_tracks_;

  translation_keys_ = reinterpret_cast<int*>(alloc_cursor);
  alloc_cursor += sizeof(int) * max_tracks * 2;
  rotation_keys_ = reinterpret_cast<int*>(alloc_cursor);
  alloc_cursor += sizeof(int) * max_tracks * 2;
  scale_keys_ = reinterpret_cast<int*>(alloc_cursor);
  alloc_cursor += sizeof(int) * max_tracks * 2;

  outdated_translations_ = reinterpret_cast<unsigned char*>(alloc_cursor);
  alloc_cursor += sizeof(unsigned char) * num_outdated;
  outdated_rotations_ = reinterpret_cast<unsigned char*>(alloc_cursor);
  alloc_cursor += sizeof(unsigned char) * num_outdated;
  outdated_scales_ = reinterpret_cast<unsigned char*>(alloc_cursor);
  alloc_cursor += sizeof(unsigned char) * num_outdated;

  assert(alloc_cursor == alloc_begin + size);
}

SamplingCache::~SamplingCache() {
  // Deallocates everything at once.
  memory::default_allocator()->Deallocate(soa_translations_);
}

void SamplingCache::Step(const Animation& _animation, float _time) {
  // The cache is invalidated if animation has changed or if it is being rewind.
  if (animation_ != &_animation || _time < time_) {
    animation_ = &_animation;
    translation_cursor_ = 0;
    rotation_cursor_ = 0;
    scale_cursor_ = 0;
  }
  time_ = _time;
}

void SamplingCache::Invalidate() {
  animation_ = NULL;
  time_ = 0.f;
  translation_cursor_ = 0;
  rotation_cursor_ = 0;
  scale_cursor_ = 0;
}
}  // animation
}  // ozz
