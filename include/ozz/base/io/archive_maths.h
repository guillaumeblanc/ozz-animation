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

#ifndef OZZ_OZZ_BASE_IO_ARCHIVE_MATHS_H_
#define OZZ_OZZ_BASE_IO_ARCHIVE_MATHS_H_

#include "ozz/base/platform.h"
#include "ozz/base/io/archive_traits.h"

namespace ozz {
namespace math {
struct Float2;
struct Float3;
struct Float4;
struct Quaternion;
struct Transform;
struct Box;
struct RectFloat;
struct RectInt;
}  // math
namespace io {
OZZ_IO_TYPE_NOT_VERSIONABLE(math::Float2)
template <>
void Save(OArchive& _archive,
          const math::Float2* _values,
          std::size_t _count);
template <>
void Load(IArchive& _archive,
          math::Float2* _values,
          std::size_t _count,
          uint32 /*_version*/);

OZZ_IO_TYPE_NOT_VERSIONABLE(math::Float3)
template <>
void Save(OArchive& _archive,
          const math::Float3* _values,
          std::size_t _count);
template <>
void Load(IArchive& _archive,
          math::Float3* _values,
          std::size_t _count,
          uint32 /*_version*/);

OZZ_IO_TYPE_NOT_VERSIONABLE(math::Float4)
template <>
void Save(OArchive& _archive,
          const math::Float4* _values,
          std::size_t _count);
template <>
void Load(IArchive& _archive,
          math::Float4* _values,
          std::size_t _count,
          uint32 /*_version*/);

OZZ_IO_TYPE_NOT_VERSIONABLE(math::Quaternion)
template <>
void Save(OArchive& _archive,
          const math::Quaternion* _values,
          std::size_t _count);
template <>
void Load(IArchive& _archive,
          math::Quaternion* _values,
          std::size_t _count,
          uint32 /*_version*/);

OZZ_IO_TYPE_NOT_VERSIONABLE(math::Transform)
template <>
void Save(OArchive& _archive,
          const math::Transform* _values,
          std::size_t _count);
template <>
void Load(IArchive& _archive,
          math::Transform* _values,
          std::size_t _count,
          uint32 /*_version*/);

OZZ_IO_TYPE_NOT_VERSIONABLE(math::Box)
template <>
void Save(OArchive& _archive,
          const math::Box* _values,
          std::size_t _count);
template <>
void Load(IArchive& _archive,
          math::Box* _values,
          std::size_t _count,
          uint32 /*_version*/);

OZZ_IO_TYPE_NOT_VERSIONABLE(math::RectFloat)
template <>
void Save(OArchive& _archive,
          const math::RectFloat* _values,
          std::size_t _count);
template <>
void Load(IArchive& _archive,
          math::RectFloat* _values,
          std::size_t _count,
          uint32 /*_version*/);

OZZ_IO_TYPE_NOT_VERSIONABLE(math::RectInt)
template <>
void Save(OArchive& _archive,
          const math::RectInt* _values,
          std::size_t _count);
template <>
void Load(IArchive& _archive,
          math::RectInt* _values,
          std::size_t _count,
          uint32 /*_version*/);
}  // io
}  // ozz
#endif  // OZZ_OZZ_BASE_IO_ARCHIVE_MATHS_H_
