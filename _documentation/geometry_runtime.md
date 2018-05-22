---
title: Geometry runtime
layout: full
keywords: runtime,optimize,soa,sse,job,batch,skinning,matrix,palette,cpu,opengl,directx,webgl
order: 70
---

{% include links.jekyll %}

ozz-geometry runtime data structures
====================================

ozz-geometry doesn't provide nor impose a format for geometry processing. Jobs relies on float buffers and strides in order to be mesh data format agnostic.

ozz-geometry runtime jops
==========================

`ozz::geometry::SkinningJob`
----------------------------

The `ozz::geometry::SkinningJob` is part of ozz_geometry library in ozz::geometry namespace and provides per-vertex matrix palette skinning job implementation. Skinning is the process of creating the association of skeleton joints with some vertices of a mesh. Portions of the mesh's skin can normally be associated with multiple joints, each one having a weight. The sum of the weights for a vertex is equal to 1. To calculate the final position of the vertex, each joint transformation is applied to the vertex position, scaled by its corresponding weight. This algorithm is called matrix palette skinning because of the set of joint transformations (stored as transform matrices) form a palette for the skin vertex to choose from.

This job iterates and transforms vertices (points and vectors) provided as input using the matrix palette skinning algorithm. The implementation supports any number of joints influences per vertex, and can transform up to one point (vertex position) and two vectors (vertex normal and tangent) per loop (aka per vertex). It assumes bi-normals aren't needed as they can be rebuilt from the normal and tangent with a lower cost than skinning (a single cross product). Input and output buffers must be provided with a stride value (aka the number of bytes from a vertex to the next). This allows the job to support vertices packed as array-of-structs (array of vertices with positions, normals...) or struct-of-arrays (buffers of points, buffer of normals...).

Note that ozz does not propose any production ready tool to load meshes or any mesh format. See fbx2skin from ozz sample framework for an example of how to import a skinned mesh from fbx.

The skinning job optimizes every code path at maximum. The per vertex loop is indeed not the same depending on the number of joints influencing a vertex (or if there are normals to transform). To maximize performances, application should partition its vertices based on their number of joints influences, and run a different job for every vertices set. Joint matrices are accessed using the per-vertex joints indices provided as input. These matrices must be pre-multiplied with the inverse of the skeleton bind-pose model-space matrices. This allows to transform vertices to joints local space. In case of non-uniform-scale matrices, the job proposes to transform vectors using an optional set of matrices, whose are usually inverse transpose of joints matrices (see http://www.glprogramming.com/red/appendixf.html). This code path is less efficient than the one without this matrices set, and should only be used when input matrices have non uniform scaling or shearing.

See ozz sample framework for an example of how to use this job. 