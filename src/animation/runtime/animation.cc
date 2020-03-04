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

#include "ozz/animation/runtime/animation.h"

#include <cassert>
#include <cstring>

#include "ozz/base/io/archive.h"
#include "ozz/base/log.h"
#include "ozz/base/maths/math_archive.h"
#include "ozz/base/maths/math_ex.h"
#include "ozz/base/memory/allocator.h"

// Internal include file
#define OZZ_INCLUDE_PRIVATE_HEADER  // Allows to include private headers.
#include "animation/runtime/animation_keyframe.h"

namespace ozz {
namespace animation {

Animation::Animation() : duration_(0.f), num_tracks_(0), name_(nullptr) {}

Animation::~Animation() { Deallocate(); }

void Animation::Allocate(size_t _name_len, size_t _translation_count,
                         size_t _rotation_count, size_t _scale_count,
                         uint16_t _track_count) {
  num_tracks_ = _track_count;

  // Distributes buffer memory while ensuring proper alignment (serves larger
  // alignment values first).
  static_assert(alignof(Float3Key) >= alignof(QuaternionKey) &&
                    alignof(QuaternionKey) >= alignof(Float3Key) &&
                    alignof(Float3Key) >= alignof(uint8_t) &&
                    alignof(uint8_t) >= alignof(char),
                "Must serve larger alignment values first)");

  assert(name_ == nullptr && translations_.size() == 0 &&
         rotations_.size() == 0 && scales_.size() == 0);

  // Compute overall size and allocate a single buffer for all the data.
  const size_t buffer_size =
      (_name_len > 0 ? _name_len + 1 : 0) +
      _translation_count * sizeof(Float3Key) +
      _rotation_count * sizeof(QuaternionKey) +
      _scale_count * sizeof(Float3Key) +
      ((_track_count + 3) / 4) * 3;  // 2bits bet joint TRS
  char* buffer = reinterpret_cast<char*>(
      memory::default_allocator()->Allocate(buffer_size, alignof(Float3Key)));

  // Fix up pointers. Serves larger alignment values first.
  translations_.begin = reinterpret_cast<Float3Key*>(buffer);
  assert(math::IsAligned(translations_.begin, alignof(Float3Key)));
  buffer += _translation_count * sizeof(Float3Key);
  translations_.end = reinterpret_cast<Float3Key*>(buffer);

  rotations_.begin = reinterpret_cast<QuaternionKey*>(buffer);
  assert(math::IsAligned(rotations_.begin, alignof(QuaternionKey)));
  buffer += _rotation_count * sizeof(QuaternionKey);
  rotations_.end = reinterpret_cast<QuaternionKey*>(buffer);

  scales_.begin = reinterpret_cast<Float3Key*>(buffer);
  assert(math::IsAligned(scales_.begin, alignof(Float3Key)));
  buffer += _scale_count * sizeof(Float3Key);
  scales_.end = reinterpret_cast<Float3Key*>(buffer);

  // Types
  translation_types.begin = reinterpret_cast<uint8_t*>(buffer);
  assert(math::IsAligned(translation_types.begin, alignof(uint8_t)));
  buffer += ((_track_count + 3) / 4) * sizeof(uint8_t);
  translation_types.end = reinterpret_cast<uint8_t*>(buffer);

  rotation_types.begin = reinterpret_cast<uint8_t*>(buffer);
  assert(math::IsAligned(rotation_types.begin, alignof(uint8_t)));
  buffer += ((_track_count + 3) / 4) * sizeof(uint8_t);
  rotation_types.end = reinterpret_cast<uint8_t*>(buffer);

  scale_types.begin = reinterpret_cast<uint8_t*>(buffer);
  assert(math::IsAligned(scale_types.begin, alignof(uint8_t)));
  buffer += ((_track_count + 3) / 4) * sizeof(uint8_t);
  scale_types.end = reinterpret_cast<uint8_t*>(buffer);

  // Let name be nullptr if animation has no name. Allows to avoid allocating
  // this buffer in the constructor of empty animations.
  name_ = reinterpret_cast<char*>(_name_len > 0 ? buffer : nullptr);
  assert(math::IsAligned(name_, alignof(char)));
}

void Animation::Deallocate() {
  memory::default_allocator()->Deallocate(translations_.begin);

  name_ = nullptr;
  translations_ = ozz::Range<Float3Key>();
  rotations_ = ozz::Range<QuaternionKey>();
  scales_ = ozz::Range<Float3Key>();
}

size_t Animation::size() const {
  const size_t size =
      sizeof(*this) + translations_.size() + rotations_.size() + scales_.size();
  return size;
}

void Animation::Save(ozz::io::OArchive& _archive) const {
  _archive << duration_;
  _archive << static_cast<int32_t>(num_tracks_);

  const size_t name_len = name_ ? std::strlen(name_) : 0;
  _archive << static_cast<int32_t>(name_len);

  const ptrdiff_t translation_count = translations_.count();
  _archive << static_cast<int32_t>(translation_count);
  const ptrdiff_t rotation_count = rotations_.count();
  _archive << static_cast<int32_t>(rotation_count);
  const ptrdiff_t scale_count = scales_.count();
  _archive << static_cast<int32_t>(scale_count);

  _archive << ozz::io::MakeArray(name_, name_len);

  for (ptrdiff_t i = 0; i < translation_count; ++i) {
    const Float3Key& key = translations_.begin[i];
    _archive << key.ratio;
    _archive << key.track;
    _archive << ozz::io::MakeArray(key.value);
  }

  for (ptrdiff_t i = 0; i < rotation_count; ++i) {
    const QuaternionKey& key = rotations_.begin[i];
    _archive << key.ratio;
    uint16_t track = key.track;
    _archive << track;
    uint8_t largest = key.largest;
    _archive << largest;
    bool sign = key.sign;
    _archive << sign;
    _archive << ozz::io::MakeArray(key.value);
  }

  for (ptrdiff_t i = 0; i < scale_count; ++i) {
    const Float3Key& key = scales_.begin[i];
    _archive << key.ratio;
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
  if (_version != 6) {
    log::Err() << "Unsupported Animation version " << _version << "."
               << std::endl;
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

  Allocate(name_len, translation_count, rotation_count, scale_count,
           num_tracks);

  if (name_) {  // nullptr name_ is supported.
    _archive >> ozz::io::MakeArray(name_, name_len);
    name_[name_len] = 0;
  }

  for (int i = 0; i < translation_count; ++i) {
    Float3Key& key = translations_.begin[i];
    _archive >> key.ratio;
    _archive >> key.track;
    _archive >> ozz::io::MakeArray(key.value);
  }

  for (int i = 0; i < rotation_count; ++i) {
    QuaternionKey& key = rotations_.begin[i];
    _archive >> key.ratio;
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
    Float3Key& key = scales_.begin[i];
    _archive >> key.ratio;
    _archive >> key.track;
    _archive >> ozz::io::MakeArray(key.value);
  }
}
}  // namespace animation
}  // namespace ozz
