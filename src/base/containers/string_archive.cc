//============================================================================//
//                                                                            //
// ozz-animation, 3d skeletal animation libraries and tools.                  //
// https://code.google.com/p/ozz-animation/                                   //
//                                                                            //
//----------------------------------------------------------------------------//
//                                                                            //
// Copyright (c) 2012-2014 Guillaume Blanc                                    //
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

#include "ozz/base/containers/string_archive.h"

#include "ozz/base/io/archive.h"
#include "ozz/base/maths/math_ex.h"

namespace ozz {
namespace io {
template <>
void Save(OArchive& _archive,
          const String::Std* _values,
          size_t _count) {
 for (size_t i = 0; i < _count; i++) {
   const ozz::String::Std& string = _values[i];

   // Get size excluding null terminating character.
   uint32_t size = static_cast<uint32_t>(string.size());
   _archive << size;
   _archive << ozz::io::MakeArray(string.c_str(), size);
 }
}

template <>
void Load(IArchive& _archive,
          String::Std* _values,
          size_t _count,
          uint32_t _version) {
  (void)_version;
  for (size_t i = 0; i < _count; i++) {
    ozz::String::Std& string = _values[i];

    // Ensure an existing string is reseted.
    string.clear();

    uint32_t size;
    _archive >> size;
    string.reserve(size);

    // Prepares temporary buffer used for reading.
    char buffer[128];
    for (size_t to_read = size; to_read != 0;) {
      // Read from the archive to the local temporary buffer.
      const size_t to_read_this_loop = math::Min(to_read, OZZ_ARRAY_SIZE(buffer));
      _archive >> ozz::io::MakeArray(buffer, to_read_this_loop);
      to_read -= to_read_this_loop;

      // Append to the string.
      string.append(buffer, to_read_this_loop);
    }
  }
}
}  // io
}  // ozz
