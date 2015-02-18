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

#ifndef OZZ_OZZ_GEOMETRY_RUNTIME_SKINNING_JOB_H_
#define OZZ_OZZ_GEOMETRY_RUNTIME_SKINNING_JOB_H_

#include "ozz/base/platform.h"

namespace ozz {
namespace math { struct Float4x4; }
namespace geometry {

// Provides per-vertex matrix palette skinning job implementation.
// Skinning is the process of creating the association of skeleton joints with
// some vertices of a mesh. Portions of the mesh's skin can normally be
// associated with multiple joints, each one having a weight. The sum of the
// weights for a vertex is equal to 1. To calculate the final position of the
// vertex, each joint transformation is applied to the vertex position, scaled
// by its corresponding weight. This algorithm is called matrix palette skinning
// because of the set of joint transformations (stored as transform matrices)
// form a palette for the skin vertex to choose from.
//
// This job iterates and transforms vertices (points and vectors) provided as
// input using the matrix palette skinning algorithm. The implementation
// supports any number of joints influences per vertex, and can transform one
// point (vertex position) and two vectors (vertex normal and tangent) per loop
// (aka vertex). It assumes bi-normals aren't needed as they can be rebuilt
// from the normal and tangent with a lower cost than skinning (a single cross
// product).
// Input and output buffers must be provided with a stride value (aka the
// number of bytes from a vertex to the next). This allows the job to support
// vertices packed as array-of-structs (array of vertices with positions,
// normals...) or struct-of-arrays (buffers of points, buffer of normals...).
// The skinning job optimizes every code path at maximum. The loop is indeed not
// the same depending on the number of joints influencing a vertex (or if there
// are normals to transform). To maximize performances, application should
// partition its vertices based on their number of joints influences, and call
// a different job for every vertices set.
// Joint matrices are accessed using the per-vertex joints indices provided as
// input. These matrices must be pre-multiplied with the inverse of the skeleton
// bind-pose matrices. This allows to transform vertices to joints local space. 
// In case of non-uniform-scale matrices, the job proposes to transform vectors
// using an optional set of matrices, whose are usually inverse transpose of
// joints matrices (see http://www.glprogramming.com/red/appendixf.html). This
// code path is less efficient than the one without this matrices set, and
// should only be used when input matrices have non uniform scaling or shearing.
// The job does not owned the buffers (in/output) and will thus not delete them
// during job's destruction.
struct SkinningJob {
  // Default constructor, initializes default values.
  SkinningJob();

  // Validates job parameters.
  // Returns true for a valid job, false otherwise:
  // - if any range is invalid. See each range description.
  // - if normals are provided but positions aren't.
  // - if tangents are provided but normals aren't.
  // - if no output is provided while an input is. For example, if input normals
  // are provided, then output normals must also.
  bool Validate() const;

  // Runs job's skinning task.
  // The job is validated before any operation is performed, see Validate() for
  // more details.
  // Returns false if *this job is not valid.
  bool Run() const;

  // Number of vertices to transform. All input and output arrays must store at
  // least this number of vertices.
  int vertex_count;

  // Maximum number of joints influencing each vertex. Must be greater than 0.
  // The number of influences drives how joint_indices and joint_weights are
  // sampled:
  // - influences_count joint indices are red from joint_indices for each
  // vertex.
  // - influences_count - 1 joint weights are red from joint_weightrs for each
  // vertex. The weight of the last joint is restored (weights are normalized).
  int influences_count;

  // Array of matrices for each joint. Joint are indexed through indices array.
  Range<const math::Float4x4> joint_matrices;

  // Optional array of inverse transposed matrices for each joint. If provided,
  // this array is used to transform vectors (normals and tangents), otherwise
  // joint_matrices array is used.
  // As explained here (http://www.glprogramming.com/red/appendixf.html) in the,
  // red book, transforming normals requires a special attention when the
  // transformation matrix has scaling or shearing. In this case the right
  // transformation is done by the inverse transpose of the transformation that
  // transforms points. Any rotation matrix is good though.
  // These matrices are optional as they might by costly to compute, and also
  // fall into a more costly code path in the skinning algorithm. 
  Range<const math::Float4x4> joint_inverse_transpose_matrices;

  // Array of joints indices. This array is used to indexes matrices in joints
  // array.
  // Each vertex has influences_max number of indices, meaning that the size of
  // this array must be at least influences_max * vertex_count.
  Range<const uint16_t> joint_indices;
  size_t joint_indices_stride;

  // Array of joints weights. This array is used to associate a weight to every
  // joint that influences a vertex. The number of weights required per vertex
  // is "influences_max - 1". The weight for the last joint (for each vertex) is
  // restored at runtime thanks to the fact that the sum of the weights for each
  // vertex is 1.
  // Each vertex has (influences_max - 1) number of weights, meaning that the
  // size of this array must be at least (influences_max - 1)* vertex_count.
  Range<const float> joint_weights;
  size_t joint_weights_stride;

  // Input vertex positions array (3 float values per vertex) and stride (number
  // of bytes between each position).
  // Array length must be at least vertex_count * in_positions_stride.
  Range<const float> in_positions;
  size_t in_positions_stride;

  // Input vertex normals (3 float values per vertex) array and stride (number
  // of bytes between each normal).
  // Array length must be at least vertex_count * in_normals_stride.
  Range<const float> in_normals;
  size_t in_normals_stride;

  // Input vertex tangents (3 float values per vertex) array and stride (number
  // of bytes between each tangent).
  // Array length must be at least vertex_count * in_tangents_stride.
  Range<const float> in_tangents;
  size_t in_tangents_stride;

  // Output vertex positions (3 float values per vertex) array and stride
  // (number of bytes between each position).
  // Array length must be at least vertex_count * out_positions_stride.
  Range<float> out_positions;
  size_t out_positions_stride;

  // Output vertex normals (3 float values per vertex) array and stride (number
  // of bytes between each normal).
  // Note that output normals are not normalized by the skinning job. This task
  // should be handled by the application, who knows if transform matrices have
  // uniform scale, and if normals are re-normalized later in the rendering
  // pipeline (shader vertex transformation stage).  
  // Array length must be at least vertex_count * out_normals_stride.
  Range<float> out_normals;
  size_t out_normals_stride;

  // Output vertex positions (3 float values per vertex) array and stride
  // (number of bytes between each tangent).
  // Like normals, Note that output tangents are not normalized by the skinning
  // job.
  // Array length must be at least vertex_count * out_tangents_stride.
  Range<float> out_tangents;
  size_t out_tangents_stride;
};
}  // geometry
}  // ozz
#endif  // OZZ_OZZ_GEOMETRY_RUNTIME_SKINNING_JOB_H_
