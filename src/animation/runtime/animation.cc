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

#include "ozz/animation/runtime/animation.h"

#include "ozz/base/io/archive.h"
#include "ozz/base/maths/math_archive.h"
#include "ozz/base/memory/allocator.h"

#include <cstring>
#include <cassert>

// Internal include file
#define OZZ_INCLUDE_PRIVATE_HEADER  // Allows to include private headers.
#include "animation/runtime/animation_keyframe.h"

namespace ozz {
namespace animation {

Animation::Animation()
    : duration_(0.f),
      num_tracks_(0),
      name_(NULL) {
}

Animation::~Animation() {
  Deallocate();
}

void Animation::Allocate(size_t name_len, size_t _translation_count,
                         size_t _rotation_count, size_t _scale_count) {
  assert(name_ == NULL && translations_.Size() == 0 && rotations_.Size() == 0 &&
         scales_.Size() == 0);

  memory::Allocator* allocator = memory::default_allocator();

  if (name_len > 0) {  // NULL name_ is supported.
    name_ = allocator->Allocate<char>(name_len + 1);
  }
  translations_ = allocator->AllocateRange<TranslationKey>(_translation_count);
  rotations_ = allocator->AllocateRange<RotationKey>(_rotation_count);
  scales_ = allocator->AllocateRange<ScaleKey>(_scale_count);
}

void Animation::Deallocate() {
  memory::Allocator* allocator = memory::default_allocator();

  allocator->Deallocate(name_);
  name_ = NULL;
  allocator->Deallocate(translations_);
  allocator->Deallocate(rotations_);
  allocator->Deallocate(scales_);
}

size_t Animation::size() const {
  const size_t size =
    sizeof(*this) + translations_.Size() + rotations_.Size() + scales_.Size();
  return size;
}

void Animation::Save(ozz::io::OArchive& _archive) const {
  _archive << duration_;
  _archive << static_cast<int32_t>(num_tracks_);

  const size_t name_len = name_ ? std::strlen(name_) : 0;
  _archive << static_cast<int32_t>(name_len);

  const ptrdiff_t translation_count = translations_.Count();
  _archive << static_cast<int32_t>(translation_count);
  const ptrdiff_t rotation_count = rotations_.Count();
  _archive << static_cast<int32_t>(rotation_count);
  const ptrdiff_t scale_count = scales_.Count();
  _archive << static_cast<int32_t>(scale_count);

  _archive << ozz::io::MakeArray(name_, name_len);

  for (ptrdiff_t i = 0; i < translation_count; ++i) {
    const TranslationKey& key = translations_.begin[i];
    _archive << key.time;
    _archive << key.track;
    _archive << ozz::io::MakeArray(key.value);
  }

  for (ptrdiff_t i = 0; i < rotation_count; ++i) {
    const RotationKey& key = rotations_.begin[i];
    _archive << key.time;
    uint16_t track = key.track;
    _archive << track;
    uint8_t largest = key.largest;
    _archive << largest;
    bool sign = key.sign;
    _archive << sign;
    _archive << ozz::io::MakeArray(key.value);
  }

  for (ptrdiff_t i = 0; i < scale_count; ++i) {
    const ScaleKey& key = scales_.begin[i];
    _archive << key.time;
    _archive << key.track;
    _archive << ozz::io::MakeArray(key.value);
  }
}

void Animation::Load(ozz::io::IArchive& _archive, uint32_t _version) {

  // Destroy animation in case it was already used before.
  Deallocate();
  duration_ = 0.f;
  num_tracks_ = 0;

  // No retro-compatibility with anterior versions.
  if (_version != 4) {
    assert(false && "Unsupported version for Animation object type.");
    return;
  }

  _archive >> duration_;

  int32_t num_tracks;
  _archive >> num_tracks;
  num_tracks_ = num_tracks;

  int32_t name_len;
  _archive >> name_len;
  int32_t translation_count;
  _archive >> translation_count;
  int32_t rotation_count;
  _archive >> rotation_count;
  int32_t scale_count;
  _archive >> scale_count;

  Allocate(name_len, translation_count, rotation_count, scale_count);

  if (name_) {  // NULL name_ is supported.
    _archive >> ozz::io::MakeArray(name_, name_len);
    name_[name_len] = 0;
  }

  for (int i = 0; i < translation_count; ++i) {
    TranslationKey& key = translations_.begin[i];
    _archive >> key.time;
    _archive >> key.track;
    _archive >> ozz::io::MakeArray(key.value);
  }

  for (int i = 0; i < rotation_count; ++i) {
    RotationKey& key = rotations_.begin[i];
    _archive >> key.time;
    uint16_t track;
    _archive >> track;
    key.track = track;
    uint8_t largest;
    _archive >> largest;
    key.largest = largest & 3;
    bool sign;
    _archive >> sign;
    key.sign = sign & 1;
    _archive >> ozz::io::MakeArray(key.value);
  }

  for (int i = 0; i < scale_count; ++i) {
    ScaleKey& key = scales_.begin[i];
    _archive >> key.time;
    _archive >> key.track;
    _archive >> ozz::io::MakeArray(key.value);
  }
}
}  // animation
}  // ozz
