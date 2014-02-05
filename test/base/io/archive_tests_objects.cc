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

#include "archive_tests_objects.h"

#include "gtest/gtest.h"

#include "ozz/base/io/archive.h"

void Intrusive::Save(ozz::io::OArchive& _archive) const {
  _archive << i;
}

void Intrusive::Load(ozz::io::IArchive& _archive, ozz::uint32 _version) {
  EXPECT_EQ(_version, 46u);
  _archive >> i;
}

namespace ozz {
namespace io {

// Specializes Extrusive type external Save and Load functions.
template <>
void Save(OArchive& _archive, const Extrusive* _test, std::size_t _count) {
  _archive << ozz::io::MakeArray(&_test->i, _count);
}
template <>
void Load(IArchive& _archive,
          Extrusive* _test,
          std::size_t _count,
          uint32 _version) {
  EXPECT_EQ(_version, 0u);
  _archive >> ozz::io::MakeArray(&_test->i, _count);
}
}  // ozz
}  // io

void Tagged1::Save(ozz::io::OArchive& /*_archive*/) const {
}
void Tagged1::Load(ozz::io::IArchive& /*_archive*/, ozz::uint32 /*_version*/) {
}
void Tagged2::Save(ozz::io::OArchive& /*_archive*/) const {
}
void Tagged2::Load(ozz::io::IArchive& /*_archive*/, ozz::uint32 /*_version*/) {
}
