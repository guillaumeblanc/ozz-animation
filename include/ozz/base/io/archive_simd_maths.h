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

#ifndef OZZ_OZZ_BASE_IO_ARCHIVE_SIMD_MATHS_H_
#define OZZ_OZZ_BASE_IO_ARCHIVE_SIMD_MATHS_H_

#include "ozz/base/platform.h"
#include "ozz/base/io/archive_traits.h"
#include "ozz/base/maths/simd_math.h"

namespace ozz {
namespace io {
OZZ_IO_TYPE_NOT_VERSIONABLE(math::SimdFloat4)
template <>
void Save(OArchive& _archive,
          const math::SimdFloat4* _values,
          std::size_t _count);
template <>
void Load(IArchive& _archive,
          math::SimdFloat4* _values,
          std::size_t _count,
          uint32 /*_version*/);

OZZ_IO_TYPE_NOT_VERSIONABLE(math::SimdInt4)
template <>
void Save(OArchive& _archive,
          const math::SimdInt4* _values,
          std::size_t _count);
template <>
void Load(IArchive& _archive,
          math::SimdInt4* _values,
          std::size_t _count,
          uint32 /*_version*/);

OZZ_IO_TYPE_NOT_VERSIONABLE(math::Float4x4)
template <>
void Save(OArchive& _archive,
          const math::Float4x4* _values,
          std::size_t _count);
template <>
void Load(IArchive& _archive,
          math::Float4x4* _values,
          std::size_t _count,
          uint32 /*_version*/);
}  // io
}  // ozz
#endif  // OZZ_OZZ_BASE_IO_ARCHIVE_SIMD_MATHS_H_
