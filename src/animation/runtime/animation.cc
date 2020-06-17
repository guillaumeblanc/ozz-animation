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

void Animation::Allocate(const AllocateParams& _params) {
  // Distributes buffer memory while ensuring proper alignment (serves larger
  // alignment values first).
  static_assert(alignof(float) >= alignof(uint16_t) &&
                    alignof(Float3Key) >= alignof(QuaternionKey) &&
                    alignof(QuaternionKey) >= alignof(Float3Key) &&
                    alignof(Float3Key) >= alignof(uint16_t) &&
                    alignof(float) >= alignof(char),
                "Must serve larger alignment values first)");

  assert(timepoints_.empty() && "Animation must be unallocated");

  // Compute overall size and allocate a single buffer for all the data.
  assert(_params.timepoints <= std::numeric_limits<uint16_t>::max());
  const size_t sizeof_ratio =
      _params.timepoints <= std::numeric_limits<uint8_t>::max()
          ? sizeof(uint8_t)
          : sizeof(uint16_t);
  const size_t buffer_size =
      (_params.name_len > 0 ? _params.name_len + 1 : 0) +
      _params.timepoints * sizeof(float) +
      _params.translations * (sizeof(Float3Key) + sizeof_ratio) +
      _params.rotations * (sizeof(QuaternionKey) + sizeof_ratio) +
      _params.scales * (sizeof(Float3Key) + sizeof_ratio);
  span<char> buffer = {static_cast<char*>(memory::default_allocator()->Allocate(
                           buffer_size, alignof(Float3Key))),
                       buffer_size};

  // Fix up pointers. Serves larger alignment values first.
  timepoints_ = fill_span<float>(buffer, _params.timepoints);
  translations_.values = fill_span<Float3Key>(buffer, _params.translations);
  rotations_.values = fill_span<QuaternionKey>(buffer, _params.rotations);
  scales_.values = fill_span<Float3Key>(buffer, _params.scales);
  translations_.ratios =
      fill_span<char>(buffer, _params.translations * sizeof_ratio);
  rotations_.ratios = fill_span<char>(buffer, _params.rotations * sizeof_ratio);
  scales_.ratios = fill_span<char>(buffer, _params.scales * sizeof_ratio);

  // Let name be nullptr if animation has no name. Allows to avoid allocating
  // this buffer in the constructor of empty animations.
  name_ = _params.name_len > 0
              ? fill_span<char>(buffer, _params.name_len + 1).data()
              : nullptr;

  assert(buffer.empty() && "Whole buffer should be consumned");
}

void Animation::Deallocate() {
  memory::default_allocator()->Deallocate(
      as_writable_bytes(timepoints_).data());

  name_ = nullptr;
  timepoints_ = {};
  translations_ = {};
  rotations_ = {};
  scales_ = {};
}

size_t Animation::size() const {
  const size_t size = sizeof(*this) + timepoints_.size_bytes() +
                      translations_.size_bytes() + rotations_.size_bytes() +
                      scales_.size_bytes();
  return size;
}

void Animation::Save(ozz::io::OArchive& _archive) const {
  _archive << duration_;
  _archive << static_cast<uint32_t>(num_tracks_);

  const size_t name_len = name_ ? std::strlen(name_) : 0;
  _archive << static_cast<uint32_t>(name_len);

  const ptrdiff_t timepoints_count = timepoints_.size();
  _archive << static_cast<uint32_t>(timepoints_count);
  const ptrdiff_t translation_count = translations_.values.size();
  _archive << static_cast<uint32_t>(translation_count);
  const ptrdiff_t rotation_count = rotations_.values.size();
  _archive << static_cast<uint32_t>(rotation_count);
  const ptrdiff_t scale_count = scales_.values.size();
  _archive << static_cast<uint32_t>(scale_count);

  _archive << ozz::io::MakeArray(name_, name_len);

  _archive << ozz::io::MakeArray(timepoints_);

  _archive << ozz::io::MakeArray(translations_.ratios);
  for (const Float3Key& key : translations_.values) {
    _archive << key.previous;
    _archive << ozz::io::MakeArray(key.value);
  }

  _archive << ozz::io::MakeArray(rotations_.ratios);
  for (const QuaternionKey& key : rotations_.values) {
    const uint16_t previous = key.previous;
    _archive << previous;
    const uint8_t largest = key.largest;
    _archive << largest;
    const bool sign = key.sign;
    _archive << sign;
    _archive << ozz::io::MakeArray(key.value);
  }

  _archive << ozz::io::MakeArray(scales_.ratios);
  for (const Float3Key& key : scales_.values) {
    _archive << key.previous;
    _archive << ozz::io::MakeArray(key.value);
  }
}

void Animation::Load(ozz::io::IArchive& _archive, uint32_t _version) {
  // Destroy animation in case it was already used before.
  Deallocate();
  duration_ = 0.f;
  num_tracks_ = 0;

  // No retro-compatibility with anterior versions.
  if (_version != 8) {
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

  const AllocateParams params{name_len, timepoints_count, translation_count,
                              rotation_count, scale_count};
  Allocate(params);

  if (name_) {  // nullptr name_ is supported.
    _archive >> ozz::io::MakeArray(name_, name_len);
    name_[name_len] = 0;
  }

  _archive >> ozz::io::MakeArray(timepoints_);

  _archive >> ozz::io::MakeArray(translations_.ratios);
  for (Float3Key& key : translations_.values) {
    _archive >> key.previous;
    _archive >> ozz::io::MakeArray(key.value);
  }

  _archive >> ozz::io::MakeArray(rotations_.ratios);
  for (QuaternionKey& key : rotations_.values) {
    uint16_t previous;
    _archive >> previous;
    key.previous = previous;
    uint8_t largest;
    _archive >> largest;
    key.largest = largest & 3;
    bool sign;
    _archive >> sign;
    key.sign = sign & 1;
    _archive >> ozz::io::MakeArray(key.value);
  }

  _archive >> ozz::io::MakeArray(scales_.ratios);
  for (Float3Key& key : scales_.values) {
    _archive >> key.previous;
    _archive >> ozz::io::MakeArray(key.value);
  }

  // size_t kframes = translations_.size() + rotations_.size() + scales_.size();
  // size_t sizeb = size();
  // size_t sizeo = sizeb + kframes * 2 - timepoints_.size();
  // size_t sizen = sizeb - kframes;
  // assert(sizeo < sizen);
}
}  // namespace animation
}  // namespace ozz
