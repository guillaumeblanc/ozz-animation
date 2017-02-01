---
title: Geometry runtime
layout: full
keywords: runtime,optimize,sample,sampling,blend,blending,soa,sse,job,batch,skinning,matrix,palette,cpu
order: 50
---

{% include links.jekyll %}

ozz-geometry runtime data structures
====================================

ozz-geometry doesn't provide nor impose a format for geometry processing.

ozz-geometry runtime jopbs
==========================

`ozz::geometry::SkinningJob`
----------------------------

The `ozz::geometry::SkinningJob` is part of ozz_geometry library in ozz::geometry namespace and provides per-vertex matrix palette skinning job implementation. Skinning is the process of creating the association of skeleton joints with some vertices of a mesh. Portions of the mesh's skin can normally be associated with multiple joints, each one having a weight. The sum of the weights for a vertex is equal to 1. To calculate the final position of the vertex, each joint transformation is applied to the vertex position, scaled by its corresponding weight. This algorithm is called matrix palette skinning because of the set of joint transformations (stored as transform matrices) form a palette for the skin vertex to choose from.

This job iterates and transforms vertices (points and vectors) provided as input using the matrix palette skinning algorithm. The implementation supports any number of joints influences per vertex, and can transform up to one point (vertex position) and two vectors (vertex normal and tangent) per loop (aka per vertex). It assumes bi-normals aren't needed as they can be rebuilt from the normal and tangent with a lower cost than skinning (a single cross product). Input and output buffers must be provided with a stride value (aka the number of bytes from a vertex to the next). This allows the job to support vertices packed as array-of-structs (array of vertices with positions, normals...) or struct-of-arrays (buffers of points, buffer of normals...).

Note that ozz does not propose any production ready tool to load meshes or any mesh format. See fbx2skin from ozz sample framework for an example of how to import a skinned mesh from fbx.

The skinning job optimizes every code path at maximum. The per vertex loop is indeed not the same depending on the number of joints influencing a vertex (or if there are normals to transform). To maximize performances, application should partition its vertices based on their number of joints influences, and run a different job for every vertices set. Joint matrices are accessed using the per-vertex joints indices provided as input. These matrices must be pre-multiplied with the inverse of the skeleton bind-pose model-space matrices. This allows to transform vertices to joints local space. In case of non-uniform-scale matrices, the job proposes to transform vectors using an optional set of matrices, whose are usually inverse transpose of joints matrices (see http://www.glprogramming.com/red/appendixf.html). This code path is less efficient than the one without this matrices set, and should only be used when input matrices have non uniform scaling or shearing.

- __Inputs__
  - `int vertex_count`: Number of vertices to transform. All input and output arrays must store at least this number of vertices.
  - `int influences_count`: Maximum number of joints influencing each vertex. The number of influences drives how `joint_indices` and `joint_weights` are sampled. `influences_count` joint indices are red from `joint_indices` for each vertex, ```influences_count - 1``` joint weights are red from `joint_weights` for each vertex. The weight of the last joint is restored (as weights are normalized).
  - `Range<const math::Float4x4> joint_matrices`: Array of matrices for each joint. Joint are indexed through indices array.
  - `Range<const math::Float4x4> joint_inverse_transpose_matrices`: Optional array of inverse transposed matrices for each joint. If provided, this array is used to transform vectors (normals and tangents), otherwise joint_matrices array is used. Transforming vectors requires a special attention when the transformation matrix has scaling or shearing. In this case the right transformation is done by the inverse transpose of the transformation that transforms points. Any rotation matrix is good though. These matrices are optional as they might by costly to compute, and also fall into a more costly code path in the skinning algorithm. 
  - `Range<const uint16_t> joint_indices`: Array of joints indices. This array is used to indexes matrices in joints array. Each vertex has `influences_count` number of indices, meaning that the size of this array must be at least `influences_count * vertex_count`.
  - `size_t joint_indices_stride`: This is the stride (offset) in byte in joint_indices buffer to move from one vertex to the next.
  - `Range<const float> joint_weights`: Array of joints weights. This array is used to associate a weight to every joint that influences a vertex. The number of weights required per vertex is ```influences_count - 1```. The weight for the last joint (for each vertex) is restored at runtime thanks to the fact that the sum of the weights for each vertex is 1. Each vertex has ```influences_count - 1``` number of weights, meaning that the size of this array must be at least ```(influences_count - 1) * vertex_count```.
  - `size_t joint_weights_stride`: This is the stride (offset) in byte in `joint_weights` buffer to move from one vertex to the next.
  - `Range<const float> in_positions`: Input vertex positions array made of 3 float values per vertex. Array length must be at least ```vertex_count * in_positions_stride```.
  - `size_t in_positions_stride`: This is the stride (offset) in byte in in_positions buffer to move from one vertex to the next.
  - ...
  - Remaining inputs are optional normal and tangent buffers. Note that tangents buffer cannot be transformed if normals aren't. If this case is required, tangents should be provided in the normals buffer, as they are transformed the same way. 
- __Processing__
  - Validates job.
  - Deduce optimal skinning algorithm based on job inputs (influences, inverse transpose matrices, normals...) and run it.
  - For each vertex
    - Computes the transform matrix based on joint indices and weights.
    - If a joint_inverse_transpose_matrices is provided, computes the normal and tangent transform matrix.
    - Load, transform (based on vertex transformation matrix) and store vertex in the output buffers.
- __Outputs__
  - `Range<float> out_positions`: Output vertex positions (3 float values per vertex) array. Array length must be at least ```vertex_count * out_positions_stride```.
  - `size_t out_positions_stride`: This is the stride (offset) in byte in out_positions buffer to move from one vertex to the next.
  - ...
  - Remaining outputs are optional normal and tangent buffers. Note that this buffers must be provided if there corresponding input one are.
See ozz sample framework for an example of how to use this job. 