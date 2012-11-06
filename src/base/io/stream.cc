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

#include "ozz/base/io/stream.h"

#include <cstdio>
#include <limits>
#include <cstring>
#include <cassert>

#include "ozz/base/memory/allocator.h"
#include "ozz/base/maths/math_ex.h"

namespace ozz {
namespace io {

// Starts File implementation.

File::File(const char* _filename, const char* _mode)
    : file_(std::fopen(_filename, _mode)) {
}

File::File(void* _file)
    : file_(_file) {
}

File::~File() {
  Close();
}

void File::Close() {
  if (file_) {
    std::FILE* file = reinterpret_cast<std::FILE*>(file_);
    std::fclose(file);
    file_ = NULL;
  }
}

bool File::opened() const {
  return file_ != NULL;
}

std::size_t File::Read(void* _buffer, std::size_t _size) {
  std::FILE* file = reinterpret_cast<std::FILE*>(file_);
  return std::fread(_buffer, 1, _size, file);
}

std::size_t File::Write(const void* _buffer, std::size_t _size) {
  std::FILE* file = reinterpret_cast<std::FILE*>(file_);
  return std::fwrite(_buffer, 1, _size, file);
}

int File::Seek(int _offset, Origin _origin) {
  int origins[] = {SEEK_CUR, SEEK_END, SEEK_SET};
  if (_origin >= static_cast<int>(OZZ_ARRAY_SIZE(origins))) {
    return -1;
  }
  std::FILE* file = reinterpret_cast<std::FILE*>(file_);
  return std::fseek(file, _offset, origins[_origin]);
}

int File::Tell() const {
  std::FILE* file = reinterpret_cast<std::FILE*>(file_);
  return std::ftell(file);
}

// Starts MemoryStream implementation.
const std::size_t MemoryStream::kBufferSizeIncrement = 16<<10;
const std::size_t MemoryStream::kMaxSize = std::numeric_limits<int>::max();

MemoryStream::MemoryStream()
    : buffer_(NULL),
      alloc_size_(0),
      end_(0),
      tell_(0) {
}

MemoryStream::~MemoryStream() {
  ozz::memory::default_allocator().Deallocate(buffer_);
  buffer_ = NULL;
}

bool MemoryStream::opened() const {
  return true;
}

std::size_t MemoryStream::Read(void* _buffer, std::size_t _size) {
  // A read cannot set file position beyond the end of the file.
  // A read cannot exceed the maximum Stream size.
  if (tell_ > end_ || _size > kMaxSize) {
    return 0;
  }

  const int read_size = math::Min(end_ - tell_, static_cast<int>(_size));
  std::memcpy(_buffer, buffer_ + tell_, read_size);
  tell_ += read_size;
  return read_size;
}

std::size_t MemoryStream::Write(const void* _buffer, std::size_t _size) {
  if (_size > kMaxSize ||
      tell_ > static_cast<int>(kMaxSize - _size)) {
    // A write cannot exceed the maximum Stream size.
    return 0;
  }
  if (tell_ > end_) {
    // The fseek() function shall allow the file-position indicator to be set
    // beyond the end of existing data in the file. If data is later written at
    // this point, subsequent reads of data in the gap shall return bytes with
    // the value 0 until data is actually written into the gap.
    if (!Resize(tell_)) {
      return 0;
    }
    // Fills the gap with 0's.
    const std::size_t gap = tell_ - end_;
    std::memset(buffer_ + end_, 0, gap);
    end_ = tell_;
  }

  const int size = static_cast<int>(_size);
  const int tell_end = tell_ + size;
  if (Resize(tell_end)) {
    end_ = math::Max(tell_end, end_);
    std::memcpy(buffer_ + tell_, _buffer, _size);
    tell_ += size;
    return _size;
  }
  return 0;
}

int MemoryStream::Seek(int _offset, Origin _origin) {
  int origin;
  switch (_origin) {
    case kCurrent: origin = tell_; break;
    case kEnd: origin = end_; break;
    case kSet: origin = 0; break;
    default: return -1;
  }

  // Exit if seeking before file begin or beyond max file size.
  if (origin < -_offset ||  
      (_offset > 0 && origin > static_cast<int>(kMaxSize - _offset))) {
    return -1;
  }

  // So tell_ is moved but end_ pointer is not moved until something is later
  // written.
  tell_ = origin + _offset;
  return 0;
}

int MemoryStream::Tell() const {
  return tell_;
}

bool MemoryStream::Resize(std::size_t _size) {
  if (_size > alloc_size_) {
    // Resize to the next multiple of kBufferSizeIncrement, requires
    // kBufferSizeIncrement to be a power of 2.
    OZZ_STATIC_ASSERT(
      (MemoryStream::kBufferSizeIncrement & (kBufferSizeIncrement-1)) == 0);

    alloc_size_ = ozz::math::Align(_size, kBufferSizeIncrement);
    buffer_ = ozz::memory::default_allocator().Reallocate(buffer_, alloc_size_);
  }
  return _size == 0 || buffer_ != NULL;
}
}  // io
}  // ozz
