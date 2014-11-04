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

#include "ozz/animation/offline/raw_skeleton.h"

#include "ozz/base/io/archive.h"
#include "ozz/base/maths/math_archive.h"

#include "ozz/base/containers/string_archive.h"
#include "ozz/base/containers/vector_archive.h"

namespace ozz {
namespace io {

template <>
void Save(OArchive& _archive,
          const animation::offline::RawSkeleton* _skeletons,
          size_t _count) {
  for (size_t i = 0; i < _count; ++i) {
    const animation::offline::RawSkeleton& skeleton = _skeletons[i];
    _archive << skeleton.roots;
  }
}

template <>
void Load(IArchive& _archive,
          animation::offline::RawSkeleton* _skeletons,
          size_t _count,
          uint32_t _version) {
  (void)_version;
  for (size_t i = 0; i < _count; ++i) {
    animation::offline::RawSkeleton& skeleton = _skeletons[i];
    _archive >> skeleton.roots;
  }
}

// RawSkeleton::Joint' version can be declared locally as it will be saved from
// this cpp file only.
OZZ_IO_TYPE_VERSION(1, animation::offline::RawSkeleton::Joint)

template <>
void Save(OArchive& _archive,
          const animation::offline::RawSkeleton::Joint* _joints,
          size_t _count) {
  for (size_t i = 0; i < _count; ++i) {
    const animation::offline::RawSkeleton::Joint& joint = _joints[i];
    _archive << joint.name;
    _archive << joint.transform;
    _archive << joint.children;
  }
}

template <>
void Load(IArchive& _archive,
          animation::offline::RawSkeleton::Joint* _joints,
          size_t _count,
          uint32_t _version) {
  (void)_version;
  for (size_t i = 0; i < _count; ++i) {
    animation::offline::RawSkeleton::Joint& joint = _joints[i];
    _archive >> joint.name;
    _archive >> joint.transform;
    _archive >> joint.children;
  }
}
}  // io
}  // ozz
