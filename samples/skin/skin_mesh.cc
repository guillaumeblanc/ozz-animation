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

#include "skin_mesh.h"

#include "ozz/base/memory/allocator.h"
#include "ozz/base/containers/vector_archive.h"

#include "ozz/base/io/archive.h"

#include "ozz/base/maths/math_archive.h"
#include "ozz/base/maths/simd_math_archive.h"

namespace ozz {
namespace sample {
SkinnedMesh::SkinnedMesh() {
}

SkinnedMesh::~SkinnedMesh() {
}
}  // sample

namespace io {

OZZ_IO_TYPE_NOT_VERSIONABLE(sample::SkinnedMesh::Part)

template <>
void Save(OArchive& _archive,
          const sample::SkinnedMesh::Part* _parts,
          size_t _count) {
  for (size_t i = 0; i < _count; ++i) {
    const sample::SkinnedMesh::Part& part = _parts[i];
    _archive << part.positions;
    _archive << part.normals;
    _archive << part.joint_indices;
    _archive << part.joint_weights;
  }
}

template <>
void Load(IArchive& _archive,
          sample::SkinnedMesh::Part* _parts,
          size_t _count,
          uint32_t _version) {
  (void)_version;
  for (size_t i = 0; i < _count; ++i) {
    sample::SkinnedMesh::Part& part = _parts[i];
    _archive >> part.positions;
    _archive >> part.normals;
    _archive >> part.joint_indices;
    _archive >> part.joint_weights;
  }
}

template <>
void Save(OArchive& _archive,
          const sample::SkinnedMesh* _meshes,
          size_t _count) {
  for (size_t i = 0; i < _count; ++i) {
    const sample::SkinnedMesh& mesh = _meshes[i];
    _archive << mesh.parts;
    _archive << mesh.triangle_indices;
    _archive << mesh.inverse_bind_poses;
  }
}

template <>
void Load(IArchive& _archive,
          sample::SkinnedMesh* _meshes,
          size_t _count,
          uint32_t _version) {
  (void)_version;
  for (size_t i = 0; i < _count; ++i) {
    sample::SkinnedMesh& mesh = _meshes[i];
    _archive >> mesh.parts;
    _archive >> mesh.triangle_indices;
    _archive >> mesh.inverse_bind_poses;
  }
}
}  // io
}  // ozz
