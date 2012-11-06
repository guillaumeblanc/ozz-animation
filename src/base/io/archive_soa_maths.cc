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

#include "ozz/base/io/archive_maths.h"

#include <cassert>

#include "ozz/base/io/archive.h"
#include "ozz/base/maths/soa_float.h"
#include "ozz/base/maths/soa_quaternion.h"
#include "ozz/base/maths/soa_float4x4.h"
#include "ozz/base/maths/soa_transform.h"

namespace ozz {
namespace io {
template <>
void Save(OArchive& _archive,
          const math::SoaFloat2* _values,
          std::size_t _count) {
  _archive << MakeArray(reinterpret_cast<const float*>(&_values->x),
                        2 * 4 * _count);
}
template <>
void Load(IArchive& _archive,
          math::SoaFloat2* _values,
          std::size_t _count,
          uint32 /*_version*/) {
  _archive >> MakeArray(reinterpret_cast<float*>(&_values->x),
                        2 * 4 * _count);
}

template <>
void Save(OArchive& _archive,
          const math::SoaFloat3* _values,
          std::size_t _count) {
  _archive << MakeArray(reinterpret_cast<const float*>(&_values->x),
                        3 * 4 * _count);
}
template <>
void Load(IArchive& _archive,
          math::SoaFloat3* _values,
          std::size_t _count,
          uint32 /*_version*/) {
  _archive >> MakeArray(reinterpret_cast<float*>(&_values->x),
                        3 * 4 * _count);
}

template <>
void Save(OArchive& _archive,
          const math::SoaFloat4* _values,
          std::size_t _count) {
  _archive << MakeArray(reinterpret_cast<const float*>(&_values->x),
                        4 * 4 *_count);
}
template <>
void Load(IArchive& _archive,
          math::SoaFloat4* _values,
          std::size_t _count,
          uint32 /*_version*/) {
  _archive >> MakeArray(reinterpret_cast<float*>(&_values->x),
                        4 * 4 * _count);
}

template <>
void Save(OArchive& _archive,
          const math::SoaQuaternion* _values,
          std::size_t _count) {
  _archive << MakeArray(reinterpret_cast<const float*>(&_values->x),
                        4 * 4 * _count);
}
template <>
void Load(IArchive& _archive,
          math::SoaQuaternion* _values,
          std::size_t _count,
          uint32 /*_version*/) {
  _archive >> MakeArray(reinterpret_cast<float*>(&_values->x),
                        4 * 4 * _count);
}

template <>
void Save(OArchive& _archive,
          const math::SoaFloat4x4* _values,
          std::size_t _count) {
  _archive << MakeArray(reinterpret_cast<const float*>(&_values->cols[0].x),
                        4 * 4 * 4 *_count);
}
template <>
void Load(IArchive& _archive,
          math::SoaFloat4x4* _values,
          std::size_t _count,
          uint32 /*_version*/) {
  _archive >> MakeArray(reinterpret_cast<float*>(&_values->cols[0].x),
                        4 * 4 * 4 * _count);
}

template <>
void Save(OArchive& _archive,
          const math::SoaTransform* _values,
          std::size_t _count) {
  _archive << MakeArray(reinterpret_cast<const float*>(&_values->translation.x),
                        10 * 4 *_count);
}
template <>
void Load(IArchive& _archive,
          math::SoaTransform* _values,
          std::size_t _count,
          uint32 /*_version*/) {
  _archive >> MakeArray(reinterpret_cast<float*>(&_values->translation.x),
                        10 * 4 * _count);
}
}  // io
}  // ozz
