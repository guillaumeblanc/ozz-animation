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

#include "ozz/animation/runtime/track.h"

#include "ozz/base/io/archive.h"
#include "ozz/base/log.h"
#include "ozz/base/maths/math_ex.h"
#include "ozz/base/memory/allocator.h"

#include <cassert>

namespace ozz {
namespace animation {

namespace internal {

template <typename _ValueType>
Track<_ValueType>::Track() {}

template <typename _ValueType>
Track<_ValueType>::~Track() {
  Deallocate();
}

template <typename _ValueType>
void Track<_ValueType>::Allocate(size_t _keys_count) {
  assert(times_.Size() == 0 && values_.Size() == 0);

  // Distributes buffer memory while ensuring proper alignment (serves larger
  // alignment values first).
  OZZ_STATIC_ASSERT(OZZ_ALIGN_OF(_ValueType) >= OZZ_ALIGN_OF(float));

  // Compute overall size and allocate a single buffer for all the data.
  const size_t buffer_size = _keys_count * sizeof(float) +  // times
                             _keys_count * sizeof(_ValueType);   // values
  char* buffer = reinterpret_cast<char*>(
      memory::default_allocator()->Allocate(buffer_size, OZZ_ALIGN_OF(_ValueType)));

  // Fix up pointers. Serves larger alignment values first.
  values_.begin = reinterpret_cast<_ValueType*>(buffer);
  assert(math::IsAligned(times_.begin, OZZ_ALIGN_OF(_ValueType)));
  buffer += _keys_count * sizeof(_ValueType);
  values_.end = reinterpret_cast<_ValueType*>(buffer);

  times_.begin = reinterpret_cast<float*>(buffer);
  assert(math::IsAligned(times_.begin, OZZ_ALIGN_OF(float)));
  buffer += _keys_count * sizeof(float);
  times_.end = reinterpret_cast<float*>(buffer);
}

template <typename _ValueType>
void Track<_ValueType>::Deallocate() {
  // Deallocate everything at once.
  memory::default_allocator()->Deallocate(values_.begin);

  times_.Clear();
  values_.Clear();
}

template <typename _ValueType>
size_t Track<_ValueType>::size() const {
  const size_t size = sizeof(*this) + times_.Size() + values_.Size();
  return size;
}

template <typename _ValueType>
void Track<_ValueType>::Save(ozz::io::OArchive& /*_archive*/) const {}

template <typename _ValueType>
void Track<_ValueType>::Load(ozz::io::IArchive& /*_archive*/, uint32_t _version) {
  // Destroy animation in case it was already used before.
  Deallocate();

  if (_version > 1) {
    log::Err() << "Unsupported Track version " << _version << "." << std::endl;
    return;
  }
}

// Explicitly instantiate supported tracks.
template class Track<float>;
template class Track<math::Float2>;
template class Track<math::Float3>;
template class Track<math::Quaternion>;

}  // internal
}  // animation
}  // ozz
