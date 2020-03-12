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

namespace {
template <typename _T>
ozz::Range<char> DispatchRange(const ozz::Range<char>& _buffer, size_t _count,
                               ozz::Range<_T>* _out) {
  *_out = Range<_T>(reinterpret_cast<_T*>(_buffer.begin), _count);
  assert(reinterpret_cast<const char*>(_out->end) <= _buffer.end);
  assert(math::IsAligned(_out->begin, alignof(_T)));
  return Range<char>(reinterpret_cast<char*>(_out->end), _buffer.end);
}
}  // namespace

void Animation::Allocate(size_t _name_len, size_t _translation_count,
                         size_t _rotation_count, size_t _scale_count,
                         size_t _const_translation_soa_count,
                         size_t _const_rotation_soa_count,
                         size_t _const_scale_soa_count, size_t _track_count) {
  num_tracks_ = _track_count;
  const size_t num_soa_tracks_ = (_track_count + 3) / 4;

  // Distributes buffer memory while ensuring proper alignment (serves larger
  // alignment values first).
  static_assert(alignof(Float3ConstKey) >= alignof(QuaternionConstKey) &&
                    alignof(QuaternionConstKey) >= alignof(Float3ConstKey) &&
                    alignof(Float3ConstKey) >= alignof(Float3Key) &&
                    alignof(Float3Key) >= alignof(QuaternionKey) &&
                    alignof(QuaternionKey) >= alignof(Float3Key) &&
                    alignof(Float3Key) >= alignof(uint8_t) &&
                    alignof(uint8_t) >= alignof(char),
                "Must serve larger alignment values first)");

  // Compute overall size and allocate a single buffer for all the data.
  const size_t name_size = _name_len > 0 ? _name_len + 1 : 0;
  const size_t types_size = (num_soa_tracks_ + 3) / 4;  // 2 bits per soa joint
  const size_t buffer_size =
      name_size + _translation_count * sizeof(Float3Key) +
      _rotation_count * sizeof(QuaternionKey) +
      _scale_count * sizeof(Float3Key) +
      _const_translation_soa_count * sizeof(Float3ConstKey) * 4 +
      _const_rotation_soa_count * sizeof(QuaternionConstKey) * 4 +
      _const_scale_soa_count * sizeof(Float3ConstKey) * 4 + types_size * 3;
  Range<char> buffer(
      reinterpret_cast<char*>(memory::default_allocator()->Allocate(
          buffer_size, alignof(Float3Key))),
      buffer_size);

  // Fix up pointers. Serves larger alignment values first.
  buffer = DispatchRange(buffer, _const_translation_soa_count * 4,
                         &const_translations_);
  buffer =
      DispatchRange(buffer, _const_rotation_soa_count * 4, &const_rotations_);
  buffer = DispatchRange(buffer, _const_scale_soa_count * 4, &const_scales_);
  buffer = DispatchRange(buffer, _translation_count, &translations_);
  buffer = DispatchRange(buffer, _rotation_count, &rotations_);
  buffer = DispatchRange(buffer, _scale_count, &scales_);

  // Types
  buffer = DispatchRange(buffer, types_size, &translation_types);
  buffer = DispatchRange(buffer, types_size, &rotation_types);
  buffer = DispatchRange(buffer, types_size, &scale_types);

  // Let name be nullptr if animation has no name. Allows to avoid allocating
  // this buffer in the constructor of empty animations.
  assert(buffer.size() == name_size);
  name_ = name_size == 0 ? nullptr : buffer.begin;
}

void Animation::Deallocate() {
  memory::default_allocator()->Deallocate(const_translations_.begin);

  name_ = nullptr;
  translations_ = {};
  rotations_ = {};
  scales_ = {};
  const_translations_ = {};
  const_rotations_ = {};
  const_scales_ = {};
  translation_types = {};
  rotation_types = {};
  scale_types = {};
}

size_t Animation::size() const {
  const size_t size = sizeof(*this) + translations_.size() + rotations_.size() +
                      scales_.size() + const_translations_.size() +
                      const_rotations_.size() + const_scales_.size() +
                      translation_types.size() + rotation_types.size() +
                      scale_types.size();
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

  const ptrdiff_t const_translation_count = const_translations_.count();
  _archive << static_cast<int32_t>(const_translation_count);
  const ptrdiff_t const_rotation_count = const_rotations_.count();
  _archive << static_cast<int32_t>(const_rotation_count);
  const ptrdiff_t const_scale_count = const_scales_.count();
  _archive << static_cast<int32_t>(const_scale_count);

  _archive << ozz::io::MakeArray(name_, name_len);

  // Animated keys.
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

  // Const keys
  _archive << ozz::io::MakeArray(const_translations_.begin->values,
                                 const_translation_count * 3);
  _archive << ozz::io::MakeArray(const_rotations_.begin->values,
                                 const_rotation_count * 4);
  _archive << ozz::io::MakeArray(const_scales_.begin->values,
                                 const_scale_count * 3);
  // Types
  _archive << ozz::io::MakeArray(translation_types.begin,
                                 translation_types.count());
  _archive << ozz::io::MakeArray(rotation_types.begin, rotation_types.count());
  _archive << ozz::io::MakeArray(scale_types.begin, scale_types.count());
}

void Animation::Load(ozz::io::IArchive& _archive, uint32_t _version) {
  // Destroy animation in case it was already used before.
  Deallocate();
  duration_ = 0.f;
  num_tracks_ = 0;

  // No retro-compatibility with anterior versions.
  if (_version != 7) {
    log::Err() << "Unsupported Animation version " << _version << "."
               << std::endl;
    return;
  }

  _archive >> duration_;

  int32_t num_tracks;
  _archive >> num_tracks;

  int32_t name_len;
  _archive >> name_len;
  int32_t translation_count;
  _archive >> translation_count;
  int32_t rotation_count;
  _archive >> rotation_count;
  int32_t scale_count;
  _archive >> scale_count;
  int32_t const_translation_count;
  _archive >> const_translation_count;
  int32_t const_rotation_count;
  _archive >> const_rotation_count;
  int32_t const_scale_count;
  _archive >> const_scale_count;

  Allocate(name_len, translation_count, rotation_count, scale_count,
           (const_translation_count + 3) / 4, (const_rotation_count + 3) / 4,
           (const_scale_count + 3) / 4, num_tracks);

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

  // Constant
  _archive >> ozz::io::MakeArray(const_translations_.begin->values,
                                 const_translation_count * 3);
  _archive >> ozz::io::MakeArray(const_rotations_.begin->values,
                                 const_rotation_count * 4);
  _archive >>
      ozz::io::MakeArray(const_scales_.begin->values, const_scale_count * 3);

  // Types
  _archive >>
      ozz::io::MakeArray(translation_types.begin, translation_types.count());
  _archive >> ozz::io::MakeArray(rotation_types.begin, rotation_types.count());
  _archive >> ozz::io::MakeArray(scale_types.begin, scale_types.count());
}
}  // namespace animation
}  // namespace ozz
