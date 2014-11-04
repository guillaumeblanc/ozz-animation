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

#include "ozz/animation/offline/fbx/fbx.h"
#include "ozz/animation/offline/fbx/fbx_base.h"

#include "ozz/animation/runtime/skeleton.h"

#include "fbxsdk/utils/fbxgeometryconverter.h"

#include "skin_mesh.h"

#include "ozz/base/io/archive.h"
#include "ozz/base/io/stream.h"
#include "ozz/base/memory/allocator.h"
#include "ozz/base/maths/simd_math.h"
#include "ozz/base/maths/math_ex.h"
#include "ozz/base/log.h"
#include "ozz/base/containers/map.h"
#include "ozz/base/containers/vector.h"

#include "ozz/options/options.h"

#include <algorithm>

// Declares command line options.
OZZ_OPTIONS_DECLARE_STRING(file, "Specifies input file.", "", true)
OZZ_OPTIONS_DECLARE_STRING(skeleton, "Specifies the skeleton that the skin is bound to.", "", true)
OZZ_OPTIONS_DECLARE_STRING(skin, "Specifies ozz skin ouput file.", "", true)

bool BuildVertices(FbxMesh* _mesh, ozz::sample::SkinnedMesh::Part* _skinned_mesh_part) {

  // Get the matrices required to transform mesh in the right unit/axis system.
  const FbxAMatrix fbx_point_transform = _mesh->GetNode()->EvaluateGlobalTransform();
  const ozz::math::Float4x4 point_transform = {{
    ozz::math::simd_float4::Load(static_cast<float>(fbx_point_transform[0][0]),
      static_cast<float>(fbx_point_transform[0][1]),
      static_cast<float>(fbx_point_transform[0][2]),
      static_cast<float>(fbx_point_transform[0][3])),
      ozz::math::simd_float4::Load(static_cast<float>(fbx_point_transform[1][0]),
      static_cast<float>(fbx_point_transform[1][1]),
      static_cast<float>(fbx_point_transform[1][2]),
      static_cast<float>(fbx_point_transform[1][3])),
      ozz::math::simd_float4::Load(static_cast<float>(fbx_point_transform[2][0]),
      static_cast<float>(fbx_point_transform[2][1]),
      static_cast<float>(fbx_point_transform[2][2]),
      static_cast<float>(fbx_point_transform[2][3])),
      ozz::math::simd_float4::Load(static_cast<float>(fbx_point_transform[3][0]),
      static_cast<float>(fbx_point_transform[3][1]),
      static_cast<float>(fbx_point_transform[3][2]),
      static_cast<float>(fbx_point_transform[3][3])),
  }};
  ozz::math::Float4x4 vector_transform =
    ozz::math::Transpose(ozz::math::Invert(point_transform));

  const int vertex_count = _mesh->GetControlPointsCount();
  _skinned_mesh_part->positions.resize(vertex_count * 3);
  _skinned_mesh_part->normals.resize(vertex_count * 3);

  // Iterate through all vertices and stores position.
  const FbxVector4* control_points = _mesh->GetControlPoints();
  for (int v = 0; v < vertex_count; ++v) {
    const FbxVector4 in = control_points[v];
    const ozz::math::SimdFloat4 simd_in = ozz::math::simd_float4::Load(
      static_cast<float>(in[0]),
      static_cast<float>(in[1]),
      static_cast<float>(in[2]),
      1.f);
    const ozz::math::SimdFloat4 transformed = point_transform * simd_in;
    ozz::math::Store3PtrU(transformed, &_skinned_mesh_part->positions[v * 3]);
  }

  // Normals could be flipped.
  float ccw_multiplier = _mesh->CheckIfVertexNormalsCCW() ? 1.f : -1.f;

  // If mesh has normals
  if (_mesh->GetElementNormalCount() > 0) {
    const FbxGeometryElementNormal* normal_element = _mesh->GetElementNormal(0);
    const bool indirect =
      normal_element->GetReferenceMode() != FbxLayerElement::eDirect;
    for (int v = 0; v < vertex_count; ++v) {
      const int lv = indirect ? normal_element->GetIndexArray().GetAt(v) : v;
      const FbxVector4 in = normal_element->GetDirectArray().GetAt(lv);
      const ozz::math::SimdFloat4 simd_in = ozz::math::simd_float4::Load(
        static_cast<float>(in[0]) * ccw_multiplier,
        static_cast<float>(in[1]) * ccw_multiplier,
        static_cast<float>(in[2]) * ccw_multiplier,
        0.f);
      const ozz::math::SimdFloat4 transformed =
        ozz::math::Normalize3(vector_transform * simd_in);
      ozz::math::Store3PtrU(transformed, &_skinned_mesh_part->normals[v * 3]);
    }
  } else {
    // Set a default value.
    for (int v = 0; v < vertex_count; ++v) {
      float* out = &_skinned_mesh_part->normals[v * 3];
      out[0] = 0.f;
      out[1] = 1.f;
      out[2] = 0.f;
    }
  }

  // Fails if no vertex in the mesh.
  return _skinned_mesh_part->vertex_count() != 0;
}

namespace {

// Define a per vertex skin attributes mapping.
struct SkinMapping {
  uint16_t index;
  float weight;
};

typedef ozz::Vector<SkinMapping>::Std SkinMappings;
typedef ozz::Vector<SkinMappings>::Std VertexSkinMappings;

// Sort highest weight first.
bool SortInfluenceWeights(const SkinMapping& _left, const SkinMapping& _right) {
  return _left.weight > _right.weight;
}
}  // namespace

bool BuildSkin(FbxMesh* _mesh,
               const ozz::animation::Skeleton& _skeleton,
               ozz::sample::SkinnedMesh::Part* _skinned_mesh_part) {
  assert(_skinned_mesh_part->vertex_count() != 0);

  const int skin_count = _mesh->GetDeformerCount(FbxDeformer::eSkin);
  if (skin_count == 0) {
    ozz::log::Err() << "No skin found." << std::endl;
    return false;
  }
  if (skin_count > 1) {
    ozz::log::Err() <<
      "More than one skin found, only the first one will be processed." <<
      std::endl;
    return false;
  }

  // Get skinning indices and weights.
  FbxSkin* deformer = static_cast<FbxSkin*>(_mesh->GetDeformer(0, FbxDeformer::eSkin));
  FbxSkin::EType skinning_type = deformer->GetSkinningType();
  if (skinning_type != FbxSkin::eRigid &&
      skinning_type != FbxSkin::eLinear) {
        ozz::log::Err() << "Unsupported skinning type" << std::endl;
    return false;
  }

  // Builds joints names map.
  typedef ozz::CStringMap<uint16_t>::Std JointsMap;
  JointsMap joints_map;
  for (int i = 0; i < _skeleton.num_joints(); ++i) {
    joints_map[_skeleton.joint_names()[i]] = static_cast<uint16_t>(i);
  }

  // Resize to the number of vertices
  const size_t vertex_count = _skinned_mesh_part->vertex_count();
  VertexSkinMappings vertex_skin_mappings;
  vertex_skin_mappings.resize(vertex_count);

  int cluster_count = deformer->GetClusterCount();
  for (int c = 0; c < cluster_count; ++c)
  {
    const FbxCluster* cluster = deformer->GetCluster(c);
    const FbxNode* node = cluster->GetLink();
    if (!node) {
      continue;
    }

    // Get corresponding joint index;
    JointsMap::const_iterator it = joints_map.find(node->GetName());
    if (it == joints_map.end()) {
      ozz::log::Err() << "Required joint " << node->GetName() <<
        " not found in skeleton." << std::endl;
      return false;
    }
    const uint16_t joint = it->second;

    // Affect joint to all vertices of the cluster.
    const int ctrl_point_index_count = cluster->GetControlPointIndicesCount();
    const int* ctrl_point_indices = cluster->GetControlPointIndices();
    const double* ctrl_point_weights = cluster->GetControlPointWeights();
    for (int v = 0; v < ctrl_point_index_count; ++v) {
      const SkinMapping mapping = {
        joint, static_cast<float>(ctrl_point_weights[v])
      };

      // Sometimes, the mesh can have less points than at the time of the
      // skinning because a smooth operator was active when skinning but has
      // been deactivated during export.
      const size_t vertex_index = ctrl_point_indices[v];
      if (vertex_index < vertex_count && mapping.weight != 0.f) {
        vertex_skin_mappings[vertex_index].push_back(mapping);
      }
    }
  }

  // Sort joint indexes according to weights.
  // Also deduce max number of indices per vertex.
  size_t max_influences = 0;
  for (size_t i = 0; i < vertex_count; ++i) {
    VertexSkinMappings::reference inv = vertex_skin_mappings[i];
    max_influences = ozz::math::Max(max_influences, inv.size());
    std::sort(inv.begin(), inv.end(), &SortInfluenceWeights);
  }

  // Allocates indices and weights.
  _skinned_mesh_part->joint_indices.resize(vertex_count * max_influences);
  _skinned_mesh_part->joint_weights.resize(vertex_count * max_influences);

  // Build output vertices data.
  bool vertex_isnt_influenced = false;
  for (size_t i = 0; i < vertex_count; ++i) {
    VertexSkinMappings::const_reference inv = vertex_skin_mappings[i];
    uint16_t* indices = &_skinned_mesh_part->joint_indices[i * max_influences];
    float* weights = &_skinned_mesh_part->joint_weights[i * max_influences];

    // Stores joint's indices and weights.
    size_t influence_count = inv.size();
    if (influence_count > 0) {
      size_t j = 0;
      for (; j < influence_count; ++j) {
        indices[j] = inv[j].index;
        weights[j] = inv[j].weight;
      }
    } else {
      // No joint influencing this vertex.
      vertex_isnt_influenced = true;
    }

    // Set unused indices and weights.
    for (size_t j = influence_count; j < max_influences; ++j) {
      indices[j] = 0;
      weights[j] = 0.f;
    }
  }

  if (vertex_isnt_influenced) {
    ozz::log::Err() << "At least one vertex isn't influenced by any joints."
      << std::endl;
  }

  return !vertex_isnt_influenced;
}

bool BuildTriangleIndices(FbxMesh* _mesh,
                          ozz::sample::SkinnedMesh* _skinned_mesh) {
  //  Builds triangle indices.
  const int index_count = _mesh->GetPolygonVertexCount();
  _skinned_mesh->triangle_indices.resize(index_count);

  const int* indices = _mesh->GetPolygonVertices();
  for(int p = 0; p < index_count; ++p) {
    _skinned_mesh->triangle_indices[p] = static_cast<uint16_t>(indices[p]);
  }
  return true;
}

bool SplitParts(const ozz::sample::SkinnedMesh& _skinned_mesh,
                ozz::sample::SkinnedMesh* _partitionned_mesh) {
  assert(_skinned_mesh.parts.size() == 1);
  assert(_partitionned_mesh->parts.size() == 0);

  const ozz::sample::SkinnedMesh::Part& in_part = _skinned_mesh.parts.front();
  const size_t vertex_count = in_part.vertex_count();

  // Creates one mesh part per influence.
  const int max_influences = in_part.influences_count();

  // Bucket-sort vertices per influence count.
  typedef ozz::Vector<ozz::Vector<size_t>::Std>::Std BuckedVertices;
  BuckedVertices bucked_vertices;
  bucked_vertices.resize(max_influences);
  if (max_influences > 1) {
    for (size_t i = 0; i < vertex_count; ++i) {
      const float* weights = &in_part.joint_weights[i * max_influences];
      int j = 0;
      for (; j < max_influences && weights[j] > 0.f; ++j) {
      }
      const int influences = j - 1;
      bucked_vertices[influences].push_back(i);
    }
  } else {
    for (size_t i = 0; i < vertex_count; ++i) {
      bucked_vertices[0].push_back(i);
    }
  }

  // Group vertices if there's not enough of them for a given part. This allows to
  // limit SkinningJob fix cost overhead.
  const size_t kMinBucketSize = 10;

  for (size_t i = 0; i < bucked_vertices.size() - 1; ++i) {
    BuckedVertices::reference bucket = bucked_vertices[i];
    if (bucket.size() < kMinBucketSize) {
      // Transfers vertices to next bucket if there aren't enough.
      BuckedVertices::reference next_bucket = bucked_vertices[i + 1];
      next_bucket.reserve(next_bucket.size() + bucket.size());
      for (size_t j = 0; j < bucket.size(); ++j) {
        next_bucket.push_back(bucket[j]);
      }
      bucket.clear();
    }
  }

  // Fills mesh parts.
  _partitionned_mesh->parts.reserve(max_influences);
  for (int i = 0; i < max_influences; ++i) {

    const ozz::Vector<size_t>::Std& bucket = bucked_vertices[i];
    const size_t bucket_vertex_count = bucket.size();
    if (bucket_vertex_count == 0) {
      // No Mesh part if no vertices.
      continue;
    }

    // Adds a new part.
    _partitionned_mesh->parts.resize(_partitionned_mesh->parts.size() + 1);
    ozz::sample::SkinnedMesh::Part& out_part = _partitionned_mesh->parts.back();

    // Resize output part.
    const int influences = i + 1;
    out_part.positions.resize(bucket_vertex_count * 3);
    out_part.normals.resize(bucket_vertex_count * 3);
    out_part.joint_indices.resize(bucket_vertex_count * influences);
    out_part.joint_weights.resize(bucket_vertex_count * influences);

    // Fills output of this part.
    for (size_t j = 0; j < bucket_vertex_count; ++j) {
      // Fills positions.
      const size_t bucket_vertex_index = bucket[j];
      const float* in_pos = &in_part.positions[bucket_vertex_index * 3];
      float* out_pos = &out_part.positions[j * 3];
      out_pos[0] = in_pos[0];
      out_pos[1] = in_pos[1];
      out_pos[2] = in_pos[2];
      // Fills normals.
      const float* in_normal = &in_part.normals[bucket_vertex_index * 3];
      float* out_normal = &out_part.normals[j * 3];
      out_normal[0] = in_normal[0];
      out_normal[1] = in_normal[1];
      out_normal[2] = in_normal[2];
      // Fills joints indices.
      const uint16_t* in_indices =
        &in_part.joint_indices[bucket_vertex_index * max_influences];
      uint16_t* out_indices =
        &out_part.joint_indices[j * influences];
      for (int k = 0; k < influences; ++k) {
        out_indices[k] = in_indices[k];
      }
      // Fills weights. Note that there's no weight if there's only one joint
      // influencing a vertex.
      if (influences > 1) {
        const float* in_weights =
          &in_part.joint_weights[bucket_vertex_index * max_influences];
        float* out_weights =
          &out_part.joint_weights[j * influences];
        for (int k = 0; k < influences; ++k) {
          out_weights[k] = in_weights[k];
        }
      }
    }
  }

  // Builds a vertex remapping table to help rebuild triangle indices.
  ozz::Vector<uint16_t>::Std vertices_remap;
  vertices_remap.resize(vertex_count);
  uint16_t processed_vertices = 0;
  for (size_t i = 0; i < bucked_vertices.size(); ++i) {
    const ozz::Vector<size_t>::Std& bucket = bucked_vertices[i];
    const uint16_t bucket_vertex_count = static_cast<uint16_t>(bucket.size());
    for (uint16_t j = 0; j < bucket_vertex_count; ++j) {
      vertices_remap[bucket[j]] = j + processed_vertices;
    }
    processed_vertices += bucket_vertex_count;
  }

  // Remaps triangle indices, using vertex mapping table.
  const size_t index_count = _skinned_mesh.triangle_indices.size();
  _partitionned_mesh->triangle_indices.resize(index_count);
  for (size_t i = 0; i < index_count; ++i) {
    _partitionned_mesh->triangle_indices[i] =
      vertices_remap[_skinned_mesh.triangle_indices[i]];
  }

  return true;
}

bool StripWeights(ozz::sample::SkinnedMesh* _mesh) {
  for (size_t i = 0; i < _mesh->parts.size(); ++i) {
    ozz::sample::SkinnedMesh::Part& part = _mesh->parts[i];
    const int influence_count = part.influences_count();
    const int vertex_count = part.vertex_count();
    if (influence_count == 1) {
      part.joint_weights.clear();
    } else {
      for (int j = vertex_count - 1; j >= 0; --j) {
        part.joint_weights.erase(
          part.joint_weights.begin() + (j + 1) * influence_count - 1);
      }
    }
    assert(static_cast<int>(part.joint_weights.size()) ==
           vertex_count * (influence_count - 1));
  }

  return true;
}

int main(int _argc, const char** _argv) {
  // Parses arguments.
  ozz::options::ParseResult parse_result = ozz::options::ParseCommandLine(
    _argc, _argv,
    "1.1",
    "Imports a skin from a fbx file and converts it to ozz binary format");
  if (parse_result != ozz::options::kSuccess) {
    return parse_result == ozz::options::kExitSuccess ?
      EXIT_SUCCESS : EXIT_FAILURE;
  }
  
  // Opens skeleton file.
  ozz::animation::Skeleton skeleton;
  {
    ozz::log::Out() << "Loading skeleton archive " << OPTIONS_skeleton.value() <<
      "." << std::endl;
    ozz::io::File file(OPTIONS_skeleton.value(), "rb");
    if (!file.opened()) {
      ozz::log::Err() << "Failed to open skeleton file " <<
        OPTIONS_skeleton.value() << "." << std::endl;
      return EXIT_FAILURE;
    }
    ozz::io::IArchive archive(&file);
    if (!archive.TestTag<ozz::animation::Skeleton>()) {
      ozz::log::Err() << "Failed to load skeleton instance from file " <<
        OPTIONS_skeleton.value() << "." << std::endl;
      return EXIT_FAILURE;
    }

    // Once the tag is validated, reading cannot fail.
    archive >> skeleton;
  }

  // Import Fbx content.
  ozz::animation::offline::fbx::FbxManagerInstance fbx_manager;
  ozz::animation::offline::fbx::FbxDefaultIOSettings settings(fbx_manager);
  ozz::animation::offline::fbx::FbxSceneLoader scene_loader(OPTIONS_file, "", fbx_manager, settings);
  if (!scene_loader.scene()) {
    ozz::log::Err() << "Failed to import file " << OPTIONS_file.value() << "." <<
      std::endl;
    return EXIT_FAILURE;
  }

  if (scene_loader.scene()->GetSrcObjectCount<FbxMesh>() == 0) {
    ozz::log::Err() << "No mesh to process in this file: " <<
      OPTIONS_file.value() << "." << std::endl;
    return EXIT_FAILURE;
  } else if (scene_loader.scene()->GetSrcObjectCount<FbxMesh>() > 1) {
    ozz::log::Err() << "There's more than one mesh in the file: " <<
      OPTIONS_file.value() << ". Only the first one will be processed." <<
      std::endl;
  }

  { // Triangulate the scene.
    FbxGeometryConverter converter(fbx_manager);
    if (!converter.Triangulate(scene_loader.scene(), true)) {
      ozz::log::Err() << "Failed to triangulating meshes." << std::endl;
      return EXIT_FAILURE;
    }
  }

  FbxMesh* mesh = scene_loader.scene()->GetSrcObject<FbxMesh>(0);

  ozz::sample::SkinnedMesh skinned_mesh;
  skinned_mesh.parts.resize(1);
  ozz::sample::SkinnedMesh::Part& skinned_mesh_part = skinned_mesh.parts[0];
  if (!BuildVertices(mesh, &skinned_mesh_part)) {
    return EXIT_FAILURE;
  }

  if (!BuildSkin(mesh, skeleton, &skinned_mesh_part)) {
    return EXIT_FAILURE;
  }

  if (!BuildTriangleIndices(mesh, &skinned_mesh)) {
    return EXIT_FAILURE;
  }

  ozz::sample::SkinnedMesh partitioned_meshes;
  if (!SplitParts(skinned_mesh, &partitioned_meshes)) {
    return EXIT_FAILURE;
  }

  if (!StripWeights(&partitioned_meshes)) {
    return EXIT_FAILURE;
  }

  // Opens output file.
  ozz::io::File skin_file(OPTIONS_skin, "wb");
  if (!skin_file.opened()) {
    ozz::log::Err() << "Failed to open output file: " << OPTIONS_skin.value() <<
      std::endl;
    return EXIT_FAILURE;
  }

  { // Serialize the partitioned mesh.
    ozz::io::OArchive archive(&skin_file);
    archive << partitioned_meshes;
  }

  return EXIT_SUCCESS;
}
