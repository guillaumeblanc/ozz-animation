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

#ifndef OZZ_OZZ_BASE_CONTAINERS_STRING_ARCHIVE_H_
#define OZZ_OZZ_BASE_CONTAINERS_STRING_ARCHIVE_H_

#include "ozz/base/platform.h"
#include "ozz/base/containers/string.h"
#include "ozz/base/io/archive_traits.h"

namespace ozz {
namespace io {
class IArchive;
class OArchive;

OZZ_IO_TYPE_NOT_VERSIONABLE(ozz::String::Std)

template <>
void Save(OArchive& _archive,
          const ozz::String::Std* _values,
          size_t _count);

template <>
void Load(IArchive& _archive,
          ozz::String::Std* _values,
          size_t _count,
          uint32_t _version);
}  // io
}  // ozz
#endif  // OZZ_OZZ_BASE_CONTAINERS_STRING_ARCHIVE_H_
