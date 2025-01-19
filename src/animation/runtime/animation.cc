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

#include "ozz/animation/runtime/animation.h"

#include <cassert>
#include <cstring>
#include <limits>

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

Animation::Animation(Animation&& _other) { *this = std::move(_other); }

Animation& Animation::operator=(Animation&& _other) {
  std::swap(allocation_, _other.allocation_);
  std::swap(duration_, _other.duration_);
  std::swap(num_tracks_, _other.num_tracks_);
  std::swap(name_, _other.name_);
  std::swap(timepoints_, _other.timepoints_);
  std::swap(translations_ctrl_, _other.translations_ctrl_);
  std::swap(rotations_ctrl_, _other.rotations_ctrl_);
  std::swap(scales_ctrl_, _other.scales_ctrl_);
  std::swap(translations_values_, _other.translations_values_);
  std::swap(rotations_values_, _other.rotations_values_);
  std::swap(scales_values_, _other.scales_values_);

  return *this;
}

Animation::~Animation() { Deallocate(); }

void Animation::Allocate(const AllocateParams& _params) {
  // Distributes buffer memory while ensuring proper alignment (serves larger
  // alignment values first).
  static_assert(
      alignof(float) >= alignof(uint32_t) &&
          alignof(uint32_t) >= alignof(uint16_t) &&
          alignof(uint16_t) >= alignof(internal::Float3Key) &&
          alignof(internal::Float3Key) >= alignof(internal::QuaternionKey) &&
          alignof(internal::QuaternionKey) >= alignof(char),
      "Must serve larger alignment values first)");

  assert(allocation_ == nullptr && "Already allocated");

  // Compute overall size and allocate a single buffer for all the data.
  assert(_params.timepoints <= std::numeric_limits<uint16_t>::max());
  const size_t sizeof_ratio =
      _params.timepoints <= std::numeric_limits<uint8_t>::max()
          ? sizeof(uint8_t)
          : sizeof(uint16_t);
  const size_t sizeof_previous = sizeof(uint16_t);
  const size_t buffer_size =
      (_params.name_len > 0 ? _params.name_len + 1 : 0) +
      _params.timepoints * sizeof(float) +
      _params.translations *
          (sizeof(internal::Float3Key) + sizeof_ratio + sizeof_previous) +
      _params.rotations *
          (sizeof(internal::QuaternionKey) + sizeof_ratio + sizeof_previous) +
      _params.scales *
          (sizeof(internal::Float3Key) + sizeof_ratio + sizeof_previous) +
      _params.translation_iframes.entries * sizeof(byte) +
      _params.translation_iframes.offsets * sizeof(uint32_t) +
      _params.rotation_iframes.entries * sizeof(byte) +
      _params.rotation_iframes.offsets * sizeof(uint32_t) +
      _params.scale_iframes.entries * sizeof(byte) +
      _params.scale_iframes.offsets * sizeof(uint32_t);

  // Allocate whole buffer
  auto* allocator = memory::default_allocator();
  allocation_ = allocator->Allocate(buffer_size, alignof(float));
  span<byte> buffer = {static_cast<byte*>(allocation_), buffer_size};

  // Fix up pointers. Serves larger alignment values first.

  // 32b alignment
  timepoints_ = fill_span<float>(buffer, _params.timepoints);
  translations_ctrl_.iframe_desc =
      fill_span<uint32_t>(buffer, _params.translation_iframes.offsets);
  rotations_ctrl_.iframe_desc =
      fill_span<uint32_t>(buffer, _params.rotation_iframes.offsets);
  scales_ctrl_.iframe_desc =
      fill_span<uint32_t>(buffer, _params.scale_iframes.offsets);

  // 16b alignment
  translations_ctrl_.previouses =
      fill_span<uint16_t>(buffer, _params.translations);
  rotations_ctrl_.previouses = fill_span<uint16_t>(buffer, _params.rotations);
  scales_ctrl_.previouses = fill_span<uint16_t>(buffer, _params.scales);
  translations_values_ =
      fill_span<internal::Float3Key>(buffer, _params.translations);
  rotations_values_ =
      fill_span<internal::QuaternionKey>(buffer, _params.rotations);
  scales_values_ = fill_span<internal::Float3Key>(buffer, _params.scales);

  // 16b / 8b alignment
  translations_ctrl_.ratios =
      fill_span<byte>(buffer, _params.translations * sizeof_ratio);
  rotations_ctrl_.ratios =
      fill_span<byte>(buffer, _params.rotations * sizeof_ratio);
  scales_ctrl_.ratios = fill_span<byte>(buffer, _params.scales * sizeof_ratio);

  // 8b alignment

  // iframe_entries are compressed with gv4, they must not be at the end of the
  // buffer, as gv4 will access 3 bytes further than compressed entries.
  translations_ctrl_.iframe_entries =
      fill_span<byte>(buffer, _params.translation_iframes.entries);
  rotations_ctrl_.iframe_entries =
      fill_span<byte>(buffer, _params.rotation_iframes.entries);
  scales_ctrl_.iframe_entries =
      fill_span<byte>(buffer, _params.scale_iframes.entries);

  // Let name be nullptr if animation has no name. Allows to avoid allocating
  // this buffer in the constructor of empty animations.
  name_ = _params.name_len > 0
              ? fill_span<char>(buffer, _params.name_len + 1).data()
              : nullptr;

  assert(buffer.empty() && "Whole buffer should be consumed");
}

void Animation::Deallocate() {
  memory::default_allocator()->Deallocate(allocation_);
  allocation_ = nullptr;
}

size_t Animation::size() const {
  const size_t size =
      sizeof(*this) + timepoints_.size_bytes() +
      translations_ctrl_.size_bytes() + rotations_ctrl_.size_bytes() +
      scales_ctrl_.size_bytes() + translations_values_.size_bytes() +
      rotations_values_.size_bytes() + scales_values_.size_bytes();
  return size;
}
}  // namespace animation

namespace io {
OZZ_IO_TYPE_NOT_VERSIONABLE(animation::Animation::KeyframesCtrl)
template <>
struct Extern<animation::Animation::KeyframesCtrl> {
  static void Save(OArchive& _archive,
                   const animation::Animation::KeyframesCtrl* _components,
                   size_t _count) {
    for (size_t i = 0; i < _count; ++i) {
      const animation::Animation::KeyframesCtrl& component = _components[i];
      _archive << ozz::io::MakeArray(component.ratios);
      _archive << ozz::io::MakeArray(component.previouses);
      _archive << ozz::io::MakeArray(component.iframe_entries);
      _archive << ozz::io::MakeArray(component.iframe_desc);
      _archive << component.iframe_interval;
    }
  }
  static void Load(IArchive& _archive,
                   animation::Animation::KeyframesCtrl* _components,
                   size_t _count, uint32_t _version) {
    (void)_version;
    for (size_t i = 0; i < _count; ++i) {
      animation::Animation::KeyframesCtrl& component = _components[i];
      _archive >> ozz::io::MakeArray(component.ratios);
      _archive >> ozz::io::MakeArray(component.previouses);
      _archive >> ozz::io::MakeArray(component.iframe_entries);
      _archive >> ozz::io::MakeArray(component.iframe_desc);
      _archive >> component.iframe_interval;
    }
  }
};

OZZ_IO_TYPE_NOT_VERSIONABLE(animation::internal::Float3Key)
template <>
struct Extern<animation::internal::Float3Key> {
  static void Save(OArchive& _archive,
                   const animation::internal::Float3Key* _keys, size_t _count) {
    _archive << ozz::io::MakeArray(_keys->values,
                                   OZZ_ARRAY_SIZE(_keys->values) * _count);
  }
  static void Load(IArchive& _archive, animation::internal::Float3Key* _keys,
                   size_t _count, uint32_t _version) {
    (void)_version;
    _archive >> ozz::io::MakeArray(_keys->values,
                                   OZZ_ARRAY_SIZE(_keys->values) * _count);
  }
};
OZZ_IO_TYPE_NOT_VERSIONABLE(animation::internal::QuaternionKey)
template <>
struct Extern<animation::internal::QuaternionKey> {
  static void Save(OArchive& _archive,
                   const animation::internal::QuaternionKey* _keys,
                   size_t _count) {
    _archive << ozz::io::MakeArray(_keys->values,
                                   OZZ_ARRAY_SIZE(_keys->values) * _count);
  }
  static void Load(IArchive& _archive,
                   animation::internal::QuaternionKey* _keys, size_t _count,
                   uint32_t _version) {
    (void)_version;
    _archive >> ozz::io::MakeArray(_keys->values,
                                   OZZ_ARRAY_SIZE(_keys->values) * _count);
  }
};
}  // namespace io
namespace animation {
void Animation::Save(ozz::io::OArchive& _archive) const {
  _archive << duration_;
  _archive << static_cast<uint32_t>(num_tracks_);

  const size_t name_len = name_ ? std::strlen(name_) : 0;
  _archive << static_cast<uint32_t>(name_len);

  const size_t timepoints_count = timepoints_.size();
  _archive << static_cast<uint32_t>(timepoints_count);
  const size_t translation_count = translations_values_.size();
  _archive << static_cast<uint32_t>(translation_count);
  const size_t rotation_count = rotations_values_.size();
  _archive << static_cast<uint32_t>(rotation_count);
  const size_t scale_count = scales_values_.size();
  _archive << static_cast<uint32_t>(scale_count);
  const size_t t_iframe_entries_count =
      translations_ctrl_.iframe_entries.size();
  _archive << static_cast<uint32_t>(t_iframe_entries_count);
  const size_t t_iframe_desc_count = translations_ctrl_.iframe_desc.size();
  _archive << static_cast<uint32_t>(t_iframe_desc_count);
  const size_t r_iframe_entries_count = rotations_ctrl_.iframe_entries.size();
  _archive << static_cast<uint32_t>(r_iframe_entries_count);
  const size_t r_iframe_desc_count = rotations_ctrl_.iframe_desc.size();
  _archive << static_cast<uint32_t>(r_iframe_desc_count);
  const size_t s_iframe_entries_count = scales_ctrl_.iframe_entries.size();
  _archive << static_cast<uint32_t>(s_iframe_entries_count);
  const size_t s_iframe_desc_count = scales_ctrl_.iframe_desc.size();
  _archive << static_cast<uint32_t>(s_iframe_desc_count);

  _archive << ozz::io::MakeArray(name_, name_len);
  _archive << ozz::io::MakeArray(timepoints_);

  _archive << translations_ctrl_;
  _archive << io::MakeArray(translations_values_);
  _archive << rotations_ctrl_;
  _archive << io::MakeArray(rotations_values_);
  _archive << scales_ctrl_;
  _archive << io::MakeArray(scales_values_);
}

void Animation::Load(ozz::io::IArchive& _archive, uint32_t _version) {
  // Destroy animation in case it was already used before.
  Deallocate();
  duration_ = 0.f;
  num_tracks_ = 0;

  // No retro-compatibility with anterior versions.
  if (_version != 7) {
    log::Err() << "Unsupported animation version " << _version << "."
               << std::endl;
    return;
  }

  _archive >> duration_;

  uint32_t num_tracks;
  _archive >> num_tracks;
  num_tracks_ = num_tracks;

  uint32_t name_len;
  _archive >> name_len;
  uint32_t timepoints_count;
  _archive >> timepoints_count;
  uint32_t translation_count;
  _archive >> translation_count;
  uint32_t rotation_count;
  _archive >> rotation_count;
  uint32_t scale_count;
  _archive >> scale_count;
  uint32_t t_iframe_entries_count;
  _archive >> t_iframe_entries_count;
  uint32_t t_iframe_desc_count;
  _archive >> t_iframe_desc_count;
  uint32_t r_iframe_entries_count;
  _archive >> r_iframe_entries_count;
  uint32_t r_iframe_desc_count;
  _archive >> r_iframe_desc_count;
  uint32_t s_iframe_entries_count;
  _archive >> s_iframe_entries_count;
  uint32_t s_iframe_desc_count;
  _archive >> s_iframe_desc_count;

  const AllocateParams params{name_len,
                              timepoints_count,
                              translation_count,
                              rotation_count,
                              scale_count,
                              {t_iframe_entries_count, t_iframe_desc_count},
                              {r_iframe_entries_count, r_iframe_desc_count},
                              {s_iframe_entries_count, s_iframe_desc_count}};
  Allocate(params);

  if (name_) {  // nullptr name_ is supported.
    _archive >> ozz::io::MakeArray(name_, name_len);
    name_[name_len] = 0;
  }

  _archive >> ozz::io::MakeArray(timepoints_);

  _archive >> translations_ctrl_;
  _archive >> io::MakeArray(translations_values_);
  _archive >> rotations_ctrl_;
  _archive >> io::MakeArray(rotations_values_);
  _archive >> scales_ctrl_;
  _archive >> io::MakeArray(scales_values_);
}
}  // namespace animation
}  // namespace ozz
