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

#include "ozz/base/maths/math_archive.h"

#include <cassert>

#include "ozz/base/io/archive.h"
#include "ozz/base/maths/vec_float.h"
#include "ozz/base/maths/quaternion.h"
#include "ozz/base/maths/transform.h"
#include "ozz/base/maths/box.h"
#include "ozz/base/maths/rect.h"

namespace ozz {
namespace io {
template <>
void Save(OArchive& _archive,
          const math::Float2* _values,
          size_t _count) {
  _archive << MakeArray(&_values->x, 2 * _count);
}
template <>
void Load(IArchive& _archive,
          math::Float2* _values,
          size_t _count,
          uint32_t _version) {
  (void)_version;
  _archive >> MakeArray(&_values->x, 2 * _count);
}

template <>
void Save(OArchive& _archive,
          const math::Float3* _values,
          size_t _count) {
  _archive << MakeArray(&_values->x, 3 * _count);
}
template <>
void Load(IArchive& _archive,
          math::Float3* _values,
          size_t _count,
          uint32_t _version) {
  (void)_version;
  _archive >> MakeArray(&_values->x, 3 * _count);
}

template <>
void Save(OArchive& _archive,
          const math::Float4* _values,
          size_t _count) {
  _archive << MakeArray(&_values->x, 4 * _count);
}
template <>
void Load(IArchive& _archive,
          math::Float4* _values,
          size_t _count,
          uint32_t _version) {
  (void)_version;
  _archive >> MakeArray(&_values->x, 4 * _count);
}

template <>
void Save(OArchive& _archive,
          const math::Quaternion* _values,
          size_t _count) {
  _archive << MakeArray(&_values->x, 4 * _count);
}
template <>
void Load(IArchive& _archive,
          math::Quaternion* _values,
          size_t _count,
          uint32_t _version) {
  (void)_version;
  _archive >> MakeArray(&_values->x, 4 * _count);
}

template <>
void Save(OArchive& _archive,
          const math::Transform* _values,
          size_t _count) {
  _archive << MakeArray(&_values->translation.x, 10 * _count);
}
template <>
void Load(IArchive& _archive,
          math::Transform* _values,
          size_t _count,
          uint32_t _version) {
  (void)_version;
  _archive >> MakeArray(&_values->translation.x, 10 * _count);
}

template <>
void Save(OArchive& _archive,
          const math::Box* _values,
          size_t _count) {
  _archive << MakeArray(&_values->min.x, 6 * _count);
}
template <>
void Load(IArchive& _archive,
          math::Box* _values,
          size_t _count,
          uint32_t _version) {
  (void)_version;
  _archive >> MakeArray(&_values->min.x, 6 * _count);
}

template <>
void Save(OArchive& _archive,
          const math::RectFloat* _values,
          size_t _count) {
  _archive << MakeArray(&_values->left, 4 * _count);
}
template <>
void Load(IArchive& _archive,
          math::RectFloat* _values,
          size_t _count,
          uint32_t _version) {
  (void)_version;
  _archive >> MakeArray(&_values->left, 4 * _count);
}

template <>
void Save(OArchive& _archive,
          const math::RectInt* _values,
          size_t _count) {
  _archive << MakeArray(&_values->left, 4 * _count);
}
template <>
void Load(IArchive& _archive,
          math::RectInt* _values,
          size_t _count,
          uint32_t _version) {
  (void)_version;
  _archive >> MakeArray(&_values->left, 4 * _count);
}
}  // io
}  // ozz
