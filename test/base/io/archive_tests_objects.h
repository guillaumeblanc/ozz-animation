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

#ifndef OZZ_TEST_BASE_IO_ARCHIVE_TESTS_OBJECTS_H_
#define OZZ_TEST_BASE_IO_ARCHIVE_TESTS_OBJECTS_H_

#include "ozz/base/platform.h"
#include "ozz/base/io/archive_traits.h"

namespace ozz {
namespace io {
class OArchive;
class IArchive;
}  // io
}  // ozz

struct Intrusive {
  explicit Intrusive(int32_t _i = 12) :
    i(_i) {
  }
  void Save(ozz::io::OArchive& _archive) const;
  void Load(ozz::io::IArchive& _archive, uint32_t _version);
  int32_t i;
};

struct Extrusive {
  uint64_t i;
};

namespace ozz {
namespace io {
// Give Intrusive type a version.
OZZ_IO_TYPE_VERSION(46, Intrusive)

// Extrusive is not versionable.
OZZ_IO_TYPE_NOT_VERSIONABLE(Extrusive)

// Specializes Extrusive type external Save and Load functions.
template <>
void Save(OArchive& _archive, const Extrusive* _test, size_t _count);
template <>
void Load(IArchive& _archive,
          Extrusive* _test,
          size_t _count,
          uint32_t _version);
}  // ozz
}  // io

class Tagged1 {
 public:
  void Save(ozz::io::OArchive& _archive) const;
  void Load(ozz::io::IArchive& _archive, uint32_t _version);
};

class Tagged2 {
 public:
  void Save(ozz::io::OArchive& _archive) const;
  void Load(ozz::io::IArchive& _archive, uint32_t _version);
};

namespace ozz {
namespace io {
OZZ_IO_TYPE_NOT_VERSIONABLE(Tagged1)
OZZ_IO_TYPE_TAG("tagged1", Tagged1)
OZZ_IO_TYPE_NOT_VERSIONABLE(Tagged2)
OZZ_IO_TYPE_TAG("tagged2", Tagged2)
}  // ozz
}  // io
#endif  // OZZ_TEST_BASE_IO_ARCHIVE_TESTS_OBJECTS_H_
