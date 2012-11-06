//============================================================================//
// Copyright (c) <2012> <Guillaume Blanc>                                     //
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
//============================================================================//

#include "ozz/animation/animation.h"

#include "ozz/base/io/archive.h"
#include "ozz/base/io/archive_maths.h"
#include "ozz/base/memory/allocator.h"

#include "ozz/animation/key_frame.h"

namespace ozz {
namespace animation {

Animation::Animation()
    : translations_(NULL),
      translations_end_(NULL),
      rotations_(NULL),
      rotations_end_(NULL),
      scales_(NULL),
      scales_end_(NULL),
      duration_(0.f),
      num_tracks_(0) {
}

Animation::~Animation() {
  memory::default_allocator().Deallocate(translations_);
  translations_end_ = translations_ = NULL;
  memory::default_allocator().Deallocate(rotations_);
  rotations_end_ = rotations_ = NULL;
  memory::default_allocator().Deallocate(scales_);
  scales_end_ = scales_ = NULL;
}

std::size_t Animation::size() const {
  const std::ptrdiff_t size =
    sizeof(*this) +
    (translations_end_ - translations_) * sizeof(TranslationKey) +
    (rotations_end_ - rotations_) * sizeof(RotationKey) +
    (scales_end_ - scales_) * sizeof(ScaleKey);
  return size;
}

void Animation::Save(ozz::io::OArchive& _archive) const {
  _archive << duration_;
  _archive << static_cast<int32>(num_tracks_);

  const std::ptrdiff_t translation_count = translations_end_ - translations_;
  _archive << static_cast<int32>(translation_count);
  for (std::ptrdiff_t i = 0; i < translation_count; i++) {
    const TranslationKey& key = translations_[i];
    _archive << static_cast<int32>(key.track);
    _archive << key.time;
    _archive << key.value;
  }
  const std::ptrdiff_t rotation_count = rotations_end_ - rotations_;
  _archive << static_cast<int32>(rotation_count);
  for (std::ptrdiff_t i = 0; i < rotation_count; i++) {
    const RotationKey& key = rotations_[i];
    _archive << static_cast<int32>(key.track);
    _archive << key.time;
    _archive << key.value;
  }
  const std::ptrdiff_t scale_count = scales_end_ - scales_;
  _archive << static_cast<int32>(scale_count);
  for (std::ptrdiff_t i = 0; i < scale_count; i++) {
    const ScaleKey& key = scales_[i];
    _archive << static_cast<int32>(key.track);
    _archive << key.time;
    _archive << key.value;
  }
}

void Animation::Load(ozz::io::IArchive& _archive, ozz::uint32 /*_version*/) {
  _archive >> duration_;
  int32 num_tracks;
  _archive >> num_tracks;
  num_tracks_ = num_tracks;

  int32 translation_count;
  _archive >> translation_count;
  translations_ =
    memory::default_allocator().Allocate<TranslationKey>(translation_count);
  translations_end_ = translations_ + translation_count;
  for (int i = 0; i < translation_count; i++) {
    TranslationKey& key = translations_[i];
    int32 track;
    _archive >> track;
    key.track = track;
    _archive >> key.time;
    _archive >> key.value;
  }
  int32 rotation_count;
  _archive >> rotation_count;
  rotations_ =
    memory::default_allocator().Allocate<RotationKey>(rotation_count);
  rotations_end_ = rotations_ + rotation_count;
  for (int i = 0; i < rotation_count; i++) {
    RotationKey& key = rotations_[i];
    int32 track;
    _archive >> track;
    key.track = track;
    _archive >> key.time;
    _archive >> key.value;
  }
  int32 scale_count;
  _archive >> scale_count;
  scales_ =
    memory::default_allocator().Allocate<ScaleKey>(scale_count);
  scales_end_ = scales_ + scale_count;
  for (int i = 0; i < scale_count; i++) {
    ScaleKey& key = scales_[i];
    int32 track;
    _archive >> track;
    key.track = track;
    _archive >> key.time;
    _archive >> key.value;
  }
}
}  // animation
}  // ozz
