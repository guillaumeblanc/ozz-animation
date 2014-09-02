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

#include "ozz/animation/runtime/animation.h"

#include "ozz/base/io/archive.h"
#include "ozz/base/maths/math_archive.h"
#include "ozz/base/memory/allocator.h"

// Internal include file
#define OZZ_INCLUDE_PRIVATE_HEADER  // Allows to include private headers.
#include "../runtime/key_frame.h"

namespace ozz {
namespace animation {

Animation::Animation()
    : duration_(0.f),
      num_tracks_(0) {
}

Animation::~Animation() {
  Destroy();
}

void Animation::Destroy() {
  memory::Allocator* allocator = memory::default_allocator();
  allocator->Deallocate(translations_);
  translations_.begin = NULL; translations_.end = NULL;
  allocator->Deallocate(rotations_);
  rotations_.begin = NULL; rotations_.end = NULL;
  allocator->Deallocate(scales_);
  scales_.begin = NULL; scales_.end = NULL;

  duration_ = 0.f;
  num_tracks_ = 0;
}

size_t Animation::size() const {
  const ptrdiff_t size =
    sizeof(*this) +
    (translations_.Size()) * sizeof(TranslationKey) +
    (rotations_.Size()) * sizeof(RotationKey) +
    (scales_.Size()) * sizeof(ScaleKey);
  return size;
}

void Animation::Save(ozz::io::OArchive& _archive) const {
  _archive << duration_;
  _archive << static_cast<int32_t>(num_tracks_);

  const ptrdiff_t translation_count = translations_.Size();
  _archive << static_cast<int32_t>(translation_count);
  for (ptrdiff_t i = 0; i < translation_count; ++i) {
    const TranslationKey& key = translations_.begin[i];
    _archive << static_cast<int32_t>(key.track);
    _archive << key.time;
    _archive << key.value;
  }

  const ptrdiff_t rotation_count = rotations_.Size();
  _archive << static_cast<int32_t>(rotation_count);
  for (ptrdiff_t i = 0; i < rotation_count; ++i) {
    const RotationKey& key = rotations_.begin[i];
    _archive << static_cast<int32_t>(key.track);
    _archive << key.time;
    _archive << key.value;
  }

  const ptrdiff_t scale_count = scales_.Size();
  _archive << static_cast<int32_t>(scale_count);
  for (ptrdiff_t i = 0; i < scale_count; ++i) {
    const ScaleKey& key = scales_.begin[i];
    _archive << static_cast<int32_t>(key.track);
    _archive << key.time;
    _archive << key.value;
  }
}

void Animation::Load(ozz::io::IArchive& _archive, uint32_t _version) {
  (void)_version;

  // Destroy animation in case it was already used before.
  Destroy();

  memory::Allocator* allocator = memory::default_allocator();

  _archive >> duration_;

  int32_t num_tracks;
  _archive >> num_tracks;
  num_tracks_ = num_tracks;

  int32_t translation_count;
  _archive >> translation_count;
  translations_ = allocator->AllocateRange<TranslationKey>(translation_count);
  for (int i = 0; i < translation_count; ++i) {
    TranslationKey& key = translations_.begin[i];
    int32_t track;
    _archive >> track;
    key.track = track;
    _archive >> key.time;
    _archive >> key.value;
  }
  int32_t rotation_count;
  _archive >> rotation_count;
  rotations_ = allocator->AllocateRange<RotationKey>(rotation_count);
  for (int i = 0; i < rotation_count; ++i) {
    RotationKey& key = rotations_.begin[i];
    int32_t track;
    _archive >> track;
    key.track = track;
    _archive >> key.time;
    _archive >> key.value;
  }
  int32_t scale_count;
  _archive >> scale_count;
  scales_ = allocator->AllocateRange<ScaleKey>(scale_count);
  for (int i = 0; i < scale_count; ++i) {
    ScaleKey& key = scales_.begin[i];
    int32_t track;
    _archive >> track;
    key.track = track;
    _archive >> key.time;
    _archive >> key.value;
  }
}
}  // animation
}  // ozz
