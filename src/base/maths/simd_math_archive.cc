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

#include "ozz/base/maths/simd_math_archive.h"

#include <cassert>

#include "ozz/base/io/archive.h"

namespace ozz {
namespace io {
template <>
void Save(OArchive& _archive,
          const math::SimdFloat4* _values,
          size_t _count) {
  _archive << MakeArray(reinterpret_cast<const float*>(_values), 4 * _count);
}
template <>
void Load(IArchive& _archive,
          math::SimdFloat4* _values,
          size_t _count,
          uint32_t _version) {
  (void)_version;
  _archive >> MakeArray(reinterpret_cast<float*>(_values), 4 * _count);
}

template <>
void Save(OArchive& _archive,
          const math::SimdInt4* _values,
          size_t _count) {
  _archive << MakeArray(reinterpret_cast<const int*>(_values), 4 * _count);
}
template <>
void Load(IArchive& _archive,
          math::SimdInt4* _values,
          size_t _count,
          uint32_t _version) {
  (void)_version;
  _archive >> MakeArray(reinterpret_cast<int*>(_values), 4 * _count);
}

template <>
void Save(OArchive& _archive,
          const math::Float4x4* _values,
          size_t _count) {
  _archive << MakeArray(reinterpret_cast<const float*>(_values), 16 * _count);
}
template <>
void Load(IArchive& _archive,
          math::Float4x4* _values,
          size_t _count,
          uint32_t _version) {
  (void)_version;
  _archive >> MakeArray(reinterpret_cast<float*>(_values), 16 * _count);
}
}  // io
}  // ozz
