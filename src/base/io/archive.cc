//============================================================================//
//                                                                            //
// ozz-animation, 3d skeletal animation libraries and tools.                  //
// https://code.google.com/p/ozz-animation/                                   //
//                                                                            //
//----------------------------------------------------------------------------//
//                                                                            //
// Copyright (c) 2012-2015 Guillaume Blanc                                    //
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

#include "ozz/base/io/archive.h"

#include <cassert>

namespace ozz {
namespace io {

// OArchive implementation.

OArchive::OArchive(Stream* _stream, Endianness _endianness)
    : stream_(_stream),
      endian_swap_(_endianness != GetNativeEndianness()) {
  assert(stream_ && stream_->opened() &&
         L"_stream argument must point a valid opened stream.");
  // Save as a single byte as it does not need to be swapped.
  uint8_t endianness = static_cast<uint8_t>(_endianness);
  *this << endianness;
}

// IArchive implementation.

IArchive::IArchive(Stream* _stream)
    : stream_(_stream),
      endian_swap_(false) {
  assert(stream_ && stream_->opened() &&
         L"_stream argument must point a valid opened stream.");
  // Endianness was saved as a single byte, as it does not need to be swapped.
  uint8_t endianness;
  *this >> endianness;
  endian_swap_ = endianness != GetNativeEndianness();
}
}  // io
}  // ozz
