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

#ifndef OZZ_SAMPLES_SKIN_SKIN_MESH_H_
#define OZZ_SAMPLES_SKIN_SKIN_MESH_H_

#include "ozz/base/platform.h"
#include "ozz/base/containers/vector.h"
#include "ozz/base/io/archive_traits.h"

namespace ozz {
namespace sample {

// Defines a mesh with skinning information (joint indices and weights).
// The mesh is subdivided into parts that group vertices according to their
// number of influencing joints. Triangle indices are shared across mesh parts. 
struct SkinnedMesh {
  SkinnedMesh();
  ~SkinnedMesh();

  // Number of triangle indices for the mesh.
  int triangle_index_count() const {
    return static_cast<int>(triangle_indices.size());
  }

  // Number of vertices for all mesh parts.
  int vertex_count() const {
    int vertex_count = 0;
    for (size_t i = 0; i < parts.size(); ++i) {
      vertex_count += parts[i].vertex_count();
    }
    return vertex_count;
  }

  // Maximum number of joints influences for all mesh parts.
  int max_influences_count() const {
    int max_influences_count = 0;
    for (size_t i = 0; i < parts.size(); ++i) {
      const int influences_count = parts[i].influences_count();
      max_influences_count = influences_count > max_influences_count ?
        influences_count : max_influences_count;
    }
    return max_influences_count;
  }

  // Defines a portion of the mesh. A mesh is subdivided in sets of vertices
  // with the same number of joint influences.
  struct Part {
    int vertex_count() const {
      return static_cast<int>(positions.size()) / 3;
    }

    int influences_count() const {
      const int _vertex_count = vertex_count();
      if (_vertex_count == 0) {
        return 0;
      }
      return static_cast<int>(joint_indices.size()) / _vertex_count;
    }

    ozz::Vector<float>::Std positions;
    ozz::Vector<float>::Std normals;
    ozz::Vector<uint16_t>::Std joint_indices;
    ozz::Vector<float>::Std joint_weights;
  };
  ozz::Vector<Part>::Std parts;

  // Triangles indices. Indexed vertex are shared across all parts.
  ozz::Vector<uint16_t>::Std triangle_indices;
};
}  // sample

namespace io {
OZZ_IO_TYPE_TAG("ozz-sample-SkinnedMesh", sample::SkinnedMesh)
OZZ_IO_TYPE_NOT_VERSIONABLE(sample::SkinnedMesh)

template <>
void Save(OArchive& _archive,
          const sample::SkinnedMesh* _meshes,
          size_t _count);

template <>
void Load(IArchive& _archive,
          sample::SkinnedMesh* _meshes,
          size_t _count,
          uint32_t _version);
}  // io
}  // ozz

#endif  // OZZ_SAMPLES_SKIN_SKIN_MESH_H_
