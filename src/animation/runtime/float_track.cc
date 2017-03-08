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

#include "ozz/animation/runtime/float_track.h"

#include "ozz/base/io/archive.h"
#include "ozz/base/log.h"
#include "ozz/base/memory/allocator.h"
#include "ozz/base/maths/math_ex.h"

#include <cassert>

namespace ozz {
namespace animation {

FloatTrack::FloatTrack() {}

FloatTrack::~FloatTrack() { Deallocate(); }

void FloatTrack::Allocate(size_t _keys_count) {
  // Distributes buffer memory while ensuring proper alignment (serves larger
  // alignment values first).
  assert(times_.Size() == 0 && values_.Size() == 0);

  // Compute overall size and allocate a single buffer for all the data.
  const size_t buffer_size = _keys_count * sizeof(float) +  // times
                             _keys_count * sizeof(float);   // values
  char* buffer = reinterpret_cast<char*>(
      memory::default_allocator()->Allocate(buffer_size, OZZ_ALIGN_OF(float)));

  // Fix up pointers
  times_.begin = reinterpret_cast<float*>(buffer);
  assert(math::IsAligned(times_.begin, OZZ_ALIGN_OF(float)));
  buffer += _keys_count * sizeof(float);
  times_.end = reinterpret_cast<float*>(buffer);
  
  values_.begin = reinterpret_cast<float*>(buffer);
  assert(math::IsAligned(times_.begin, OZZ_ALIGN_OF(float)));
  buffer += _keys_count * sizeof(float);
  values_.end = reinterpret_cast<float*>(buffer);
}

void FloatTrack::Deallocate() {
  // Deallocate everything at once.
  memory::default_allocator()->Deallocate(times_.begin);

  times_.Clear();
  values_.Clear();
}

size_t FloatTrack::size() const {
  const size_t size = sizeof(*this);
  return size;
}

void FloatTrack::Save(ozz::io::OArchive& /*_archive*/) const {
}

void FloatTrack::Load(ozz::io::IArchive& /*_archive*/, uint32_t _version) {
  // Destroy animation in case it was already used before.
  Deallocate();

  if (_version > 1) {
    log::Err() << "Unsupported FloatTrack version " << _version << "."
               << std::endl;
    return;
  }
}
}  // animation
}  // ozz
