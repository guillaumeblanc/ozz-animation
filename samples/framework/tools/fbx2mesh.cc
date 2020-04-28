//----------------------------------------------------------------------------//
//                                                                            //
// ozz-animation is hosted at http://github.com/guillaumeblanc/ozz-animation  //
// and distributed under the MIT License (MIT).                               //
//                                                                            //
// Copyright (c) Guillaume Blanc                                              //
//                                                                            //
// Permission is hereby granted, free of charge, to any person obtaining a    //
// copy of this software and associated documentation files (the "Software"), //
// to deal in the Software without restriction, including without limitation  //
// the rights to use, copy, modify, merge, publish, distribute, sublicense,   //
// and/or sell copies of the Software, and to permit persons to whom the      //
// Software is furnished to do so, subject to the following conditions:       //
//                                                                            //
// The above copyright notice and this permission notice shall be included in //
// all copies or substantial portions of the Software.                        //
//                                                                            //
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR //
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,   //
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL    //
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER //
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING    //
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER        //
// DEALINGS IN THE SOFTWARE.                                                  //
//                                                                            //
//----------------------------------------------------------------------------//

#include "framework/mesh.h"

#include "ozz/animation/offline/fbx/fbx.h"

#include "ozz/animation/runtime/skeleton.h"

#include "ozz/base/containers/map.h"
#include "ozz/base/containers/vector.h"
#include "ozz/base/io/archive.h"
#include "ozz/base/io/stream.h"
#include "ozz/base/log.h"
#include "ozz/base/maths/math_ex.h"
#include "ozz/base/maths/simd_math.h"
#include "ozz/base/memory/allocator.h"

#include "ozz/options/options.h"

#include <algorithm>
#include <limits>

#include "fbxsdk/utils/fbxgeometryconverter.h"

// Declares command line options.
OZZ_OPTIONS_DECLARE_STRING(file, "Specifies input file.", "", true)
OZZ_OPTIONS_DECLARE_STRING(skeleton,
                           "Specifies the skeleton that the skin is bound to.",
                           "", true)
OZZ_OPTIONS_DECLARE_STRING(mesh, "Specifies ozz mesh output file.", "", true)
OZZ_OPTIONS_DECLARE_BOOL(split,
                         "Split the skinned mesh into parts (number of joint "
                         "influences per vertex).",
                         true, false)
OZZ_OPTIONS_DECLARE_INT(
    max_influences,
    "Maximum number of joint influences per vertex (0 means no limitation).", 0,
    false)

namespace {

// Control point to vertex buffer remapping.
typedef ozz::vector<uint16_t> ControlPointRemap;
typedef ozz::vector<ControlPointRemap> ControlPointsRemap;

// Triangle indices naive sort function.
int SortTriangles(const void* _left, const void* _right) {
  const uint16_t* left = static_cast<const uint16_t*>(_left);
  const uint16_t* right = static_cast<const uint16_t*>(_right);
  return (left[0] + left[1] + left[2]) - (right[0] + right[1] + right[2]);
}

// Generic function that gets an element from a layer.
template <typename _Element>
bool GetElement(const _Element& _layer, int _vertex_id, int _control_point,
                typename _Element::ArrayElementType* _out) {
  assert(_out);

  int direct_array_id;
  switch (_layer.GetMappingMode()) {
    case FbxGeometryElement::eByControlPoint: {
      switch (_layer.GetReferenceMode()) {
        case FbxGeometryElement::eDirect: {
          direct_array_id = _control_point;
          break;
        }
        case FbxGeometryElement::eIndexToDirect: {
          direct_array_id = _layer.GetIndexArray().GetAt(_control_point);
          break;
        }
        default:
          return false;  // Unhandled reference mode.
      }
      break;
    }
    case FbxGeometryElement::eByPolygonVertex: {
      switch (_layer.GetReferenceMode()) {
        case FbxGeometryElement::eDirect: {
          direct_array_id = _vertex_id;
          break;
        }
        case FbxGeometryElement::eIndexToDirect: {
          direct_array_id = _layer.GetIndexArray().GetAt(_vertex_id);
          break;
        }
        default:
          return false;  // Unhandled reference mode.
      }
      break;
    }
    default:
      return false;  // Unhandled mapping mode.
  }

  // Extract data from the layer direct array.
  *_out = _layer.GetDirectArray().GetAt(direct_array_id);

  return true;
}

// Compare raw elements. Returns true if all elements from _a and _b are equals.
template <typename _T>
bool Compare(const _T* _a, const _T* _b, size_t _count) {
  size_t i = 0;
  for (; i < _count && _a[i] == _b[i]; ++i)
    ;
  return i == _count;
}
}  // namespace

bool BuildVertices(FbxMesh* _fbx_mesh,
                   ozz::animation::offline::fbx::FbxSystemConverter* _converter,
                   ControlPointsRemap* _remap,
                   ozz::sample::Mesh* _output_mesh) {
  // This function treat all layers like if they were using mapping mode
  // eByPolygonVertex. This allow to use a single code path for all mapping
  // modes. It requires one more pass (compare to eByControlPoint mode), which
  // is to weld vertices with identical positions, normals, uvs...

  // Allocates control point to polygon remapping.
  const int ctrl_point_count = _fbx_mesh->GetControlPointsCount();
  _remap->resize(ctrl_point_count);

  // Regenerate normals if they're not available.
  if (!_fbx_mesh->GenerateNormals(false,     // overwrite
                                  true,      // by ctrl point
                                  false)) {  // clockwise
    return false;
  }

  assert(_fbx_mesh->GetElementNormalCount() > 0);
  const FbxGeometryElementNormal* element_normals =
      _fbx_mesh->GetElementNormal(0);
  assert(element_normals);

  // Checks uvs availability.
  const FbxGeometryElementUV* element_uvs = nullptr;
  if (_fbx_mesh->GetElementUVCount() > 0) {
    element_uvs = _fbx_mesh->GetElementUV(0);
  }

  // Checks tangents availability.
  const FbxGeometryElementTangent* element_tangents = nullptr;
  if (element_uvs) {  // UVs are needed to generate tangents.
    // Regenerate tangents if they're not available.
    if (!_fbx_mesh->GenerateTangentsData(0, false)) {
      return false;
    }
  }
  if (_fbx_mesh->GetElementTangentCount() > 0) {
    element_tangents = _fbx_mesh->GetElementTangent(0);
  }

  // Checks vertex colors availability.
  const FbxGeometryElementVertexColor* element_colors = nullptr;
  if (_fbx_mesh->GetElementVertexColorCount() > 0) {
    element_colors = _fbx_mesh->GetElementVertexColor(0);
  }

  // Computes worst vertex count case. Needs to allocate 3 vertices per polygon,
  // as they should all be triangles.
  const int polygon_count = _fbx_mesh->GetPolygonCount();
  int vertex_count = _fbx_mesh->GetPolygonCount() * 3;

  // Reserve vertex buffers. Real size is unknown as redundant vertices will be
  // rejected.
  ozz::sample::Mesh::Part& part = _output_mesh->parts[0];
  part.positions.reserve(vertex_count *
                         ozz::sample::Mesh::Part::kPositionsCpnts);
  part.normals.reserve(vertex_count * ozz::sample::Mesh::Part::kNormalsCpnts);
  if (element_tangents) {
    part.tangents.reserve(vertex_count *
                          ozz::sample::Mesh::Part::kTangentsCpnts);
  }
  if (element_uvs) {
    part.uvs.reserve(vertex_count * ozz::sample::Mesh::Part::kUVsCpnts);
  }
  if (element_colors) {
    part.colors.reserve(vertex_count * ozz::sample::Mesh::Part::kColorsCpnts);
  }

  // Resize triangle indices, as their size is known.
  _output_mesh->triangle_indices.resize(vertex_count);

  // Iterate all polygons and stores ctrl point to polygon mappings.
  int vertex_id = 0;
  for (int p = 0; p < polygon_count; ++p) {
    assert(_fbx_mesh->GetPolygonSize(p) == 3 &&
           "Mesh must have been triangulated.");

    for (int v = 0; v < 3; ++v, ++vertex_id) {
      // Get control point.
      const int ctrl_point = _fbx_mesh->GetPolygonVertex(p, v);
      assert(ctrl_point >= 0);
      ControlPointRemap& remap = _remap->at(ctrl_point);

      // Get vertex position.
      const ozz::math::Float3 position =
          _converter->ConvertPoint(_fbx_mesh->GetControlPoints()[ctrl_point]);

      // Get vertex normal.
      FbxVector4 src_normal(0.f, 1.f, 0.f, 0.f);
      if (!GetElement(*element_normals, vertex_id, ctrl_point, &src_normal)) {
        return false;
      }
      const ozz::math::Float3 normal = NormalizeSafe(
          _converter->ConvertVector(src_normal), ozz::math::Float3::y_axis());

      // Get vertex tangent.
      FbxVector4 src_tangent(1.f, 0.f, 0.f, 0.f);
      if (element_tangents) {
        if (!GetElement(*element_tangents, vertex_id, ctrl_point,
                        &src_tangent)) {
          return false;
        }
      }
      const ozz::math::Float3 tangent3 = NormalizeSafe(
          _converter->ConvertVector(src_tangent), ozz::math::Float3::x_axis());
      const ozz::math::Float4 tangent(tangent3,
                                      static_cast<float>(src_tangent[3]));

      // Get vertex uv.
      FbxVector2 src_uv;
      if (element_uvs) {
        if (!GetElement(*element_uvs, vertex_id, ctrl_point, &src_uv)) {
          return false;
        }
      } else {
        src_uv = FbxVector2(0., 0.);
      }
      const ozz::math::Float2 uv(static_cast<float>(src_uv[0]),
                                 static_cast<float>(src_uv[1]));

      // Get vertex colors.
      FbxColor src_color;
      if (element_colors) {
        if (!GetElement(*element_colors, vertex_id, ctrl_point, &src_color)) {
          return false;
        }
      } else {
        src_color = FbxColor(1., 1., 1., 1.);
      }
      const uint8_t color[4] = {
          static_cast<uint8_t>(
              ozz::math::Clamp(0., src_color.mRed * 255., 255.)),
          static_cast<uint8_t>(
              ozz::math::Clamp(0., src_color.mGreen * 255., 255.)),
          static_cast<uint8_t>(
              ozz::math::Clamp(0., src_color.mBlue * 255., 255.)),
          static_cast<uint8_t>(
              ozz::math::Clamp(0., src_color.mAlpha * 255., 255.)),
      };

      // Check for vertex redundancy, only with other points that share the same
      // control point.
      int redundant_with = -1;
      for (size_t r = 0; r < remap.size(); ++r) {
        const int to_test = remap[r];

        // Check for identical normals.
        if (!Compare(
                &normal.x,
                &part.normals[to_test * ozz::sample::Mesh::Part::kNormalsCpnts],
                ozz::sample::Mesh::Part::kNormalsCpnts)) {
          continue;  // Next vertex.
        }

        // Check for identical uvs.
        if (element_uvs) {
          if (!Compare(&uv.x,
                       &part.uvs[to_test * ozz::sample::Mesh::Part::kUVsCpnts],
                       ozz::sample::Mesh::Part::kUVsCpnts)) {
            continue;  // Next vertex.
          }
        }

        // Check for identical colors.
        if (element_colors) {
          if (!Compare(
                  color,
                  &part.colors[to_test * ozz::sample::Mesh::Part::kColorsCpnts],
                  ozz::sample::Mesh::Part::kColorsCpnts)) {
            continue;  // Next vertex.
          }
        }

        // Check for identical tangents.
        if (element_tangents) {
          if (!Compare(&tangent.x,
                       &part.tangents[to_test *
                                      ozz::sample::Mesh::Part::kTangentsCpnts],
                       ozz::sample::Mesh::Part::kColorsCpnts)) {
            continue;  // Next vertex.
          }
        }

        // This vertex is redundant with an existing one.
        redundant_with = to_test;
        break;
      }

      if (redundant_with >= 0) {
        assert(redundant_with <= std::numeric_limits<uint16_t>::max());

        // Reuse existing vertex.
        _output_mesh->triangle_indices[p * 3 + v] =
            static_cast<uint16_t>(redundant_with);
      } else {
        // Detect triangle indices overflow.
        if ((part.positions.size() / 3) >
            std::numeric_limits<uint16_t>::max()) {
          ozz::log::Err() << "Mesh uses too many vertices (> "
                          << std::numeric_limits<uint16_t>::max()
                          << ") to fit in the index "
                             "buffer."
                          << std::endl;
          return false;
        }

        // Deduce this vertex offset in the output vertex buffer.
        uint16_t vertex_index = static_cast<uint16_t>(
            part.positions.size() / ozz::sample::Mesh::Part::kPositionsCpnts);

        // Build triangle indices.
        _output_mesh->triangle_indices[p * 3 + v] = vertex_index;

        // Stores vertex offset in the output vertex buffer.
        remap.push_back(vertex_index);

        // Push vertex data.
        part.positions.push_back(position.x);
        part.positions.push_back(position.y);
        part.positions.push_back(position.z);
        part.normals.push_back(normal.x);
        part.normals.push_back(normal.y);
        part.normals.push_back(normal.z);
        if (element_uvs) {
          part.uvs.push_back(uv.x);
          part.uvs.push_back(uv.y);
        }
        if (element_tangents) {
          part.tangents.push_back(tangent.x);
          part.tangents.push_back(tangent.y);
          part.tangents.push_back(tangent.z);
          part.tangents.push_back(tangent.w);
        }
        if (element_colors) {
          part.colors.push_back(color[0]);
          part.colors.push_back(color[1]);
          part.colors.push_back(color[2]);
          part.colors.push_back(color[3]);
        }
      }
    }
  }

  // Sorts triangle indices to optimize vertex cache.
  std::qsort(array_begin(_output_mesh->triangle_indices),
             _output_mesh->triangle_indices.size() / 3, sizeof(uint16_t) * 3,
             &SortTriangles);

  return true;
}

namespace {

// Define a per vertex skin attributes mapping.
struct SkinMapping {
  uint16_t index;
  float weight;
};

typedef ozz::vector<SkinMapping> SkinMappings;
typedef ozz::vector<SkinMappings> VertexSkinMappings;

// Sort highest weight first.
bool SortInfluenceWeights(const SkinMapping& _left, const SkinMapping& _right) {
  return _left.weight > _right.weight;
}
}  // namespace

bool BuildSkin(FbxMesh* _fbx_mesh,
               ozz::animation::offline::fbx::FbxSystemConverter* _converter,
               const ControlPointsRemap& _remap,
               const ozz::animation::Skeleton& _skeleton,
               ozz::sample::Mesh* _output_mesh) {
  assert(_output_mesh->parts.size() == 1 &&
         _output_mesh->parts[0].vertex_count() != 0);
  ozz::sample::Mesh::Part& part = _output_mesh->parts[0];

  const int skin_count = _fbx_mesh->GetDeformerCount(FbxDeformer::eSkin);
  if (skin_count == 0) {
    ozz::log::Err() << "No skin found." << std::endl;
    return false;
  }
  if (skin_count > 1) {
    ozz::log::Log()
        << "More than one skin found, only the first one will be processed."
        << std::endl;
  }

  // Get skinning indices and weights.
  FbxSkin* deformer =
      static_cast<FbxSkin*>(_fbx_mesh->GetDeformer(0, FbxDeformer::eSkin));
  FbxSkin::EType skinning_type = deformer->GetSkinningType();
  if (skinning_type != FbxSkin::eRigid && skinning_type != FbxSkin::eLinear) {
    ozz::log::Err() << "Unsupported skinning type" << std::endl;
    return false;
  }

  // Builds joints names map.
  typedef ozz::cstring_map<uint16_t> JointsMap;
  JointsMap joints_map;
  for (int i = 0; i < _skeleton.num_joints(); ++i) {
    joints_map[_skeleton.joint_names()[i]] = static_cast<uint16_t>(i);
  }

  // Resize inverse bind pose matrices and set all to identity.
  _output_mesh->inverse_bind_poses.resize(_skeleton.num_joints());
  for (int i = 0; i < _skeleton.num_joints(); ++i) {
    _output_mesh->inverse_bind_poses[i] = ozz::math::Float4x4::identity();
  }

  // Resize to the number of vertices
  const size_t vertex_count = part.vertex_count();
  VertexSkinMappings vertex_skin_mappings;
  vertex_skin_mappings.resize(vertex_count);

  // Computes geometry matrix.
  const FbxAMatrix geometry_matrix(
      _fbx_mesh->GetNode()->GetGeometricTranslation(FbxNode::eSourcePivot),
      _fbx_mesh->GetNode()->GetGeometricRotation(FbxNode::eSourcePivot),
      _fbx_mesh->GetNode()->GetGeometricScaling(FbxNode::eSourcePivot));

  const int cluster_count = deformer->GetClusterCount();
  for (int cl = 0; cl < cluster_count; ++cl) {
    const FbxCluster* cluster = deformer->GetCluster(cl);
    const FbxNode* node = cluster->GetLink();
    if (!node) {
      ozz::log::Log() << "No node linked to cluster " << cluster->GetName()
                      << "." << std::endl;
      continue;
    }

    const FbxCluster::ELinkMode mode = cluster->GetLinkMode();
    if (mode != FbxCluster::eNormalize) {
      ozz::log::Err() << "Unsupported link mode for joint " << node->GetName()
                      << "." << std::endl;
      return false;
    }

    // Get corresponding joint index;
    JointsMap::const_iterator it = joints_map.find(node->GetName());
    if (it == joints_map.end()) {
      ozz::log::Err() << "Required joint " << node->GetName()
                      << " not found in provided skeleton." << std::endl;
      return false;
    }
    const uint16_t joint = it->second;

    // Computes joint's inverse bind-pose matrix.
    FbxAMatrix transform_matrix;
    cluster->GetTransformMatrix(transform_matrix);
    transform_matrix *= geometry_matrix;

    FbxAMatrix transform_link_matrix;
    cluster->GetTransformLinkMatrix(transform_link_matrix);

    const FbxAMatrix inverse_bind_pose =
        transform_link_matrix.Inverse() * transform_matrix;

    // Stores inverse transformation.
    _output_mesh->inverse_bind_poses[joint] =
        _converter->ConvertMatrix(inverse_bind_pose);

    // Affect joint to all vertices of the cluster.
    const int ctrl_point_index_count = cluster->GetControlPointIndicesCount();

    const int* ctrl_point_indices = cluster->GetControlPointIndices();
    const double* ctrl_point_weights = cluster->GetControlPointWeights();
    for (int cpi = 0; cpi < ctrl_point_index_count; ++cpi) {
      const SkinMapping mapping = {joint,
                                   static_cast<float>(ctrl_point_weights[cpi])};
      if (mapping.weight <= 0.f) {
        continue;
      }

      const int ctrl_point = ctrl_point_indices[cpi];
      assert(ctrl_point < static_cast<int>(_remap.size()));

      // remap.size() can be 0, skinned control point might not be used by any
      // polygon of the mesh. Sometimes, the mesh can have less points than at
      // the time of the skinning because a smooth operator was active when
      // skinning but has been deactivated during export.
      const ControlPointRemap& remap = _remap[ctrl_point];
      for (size_t v = 0; v < remap.size(); ++v) {
        vertex_skin_mappings[remap[v]].push_back(mapping);
      }
    }
  }

  // Sort joint indexes according to weights.
  // Also deduce max number of indices per vertex.
  size_t max_influences = 0;
  for (size_t i = 0; i < vertex_count; ++i) {
    VertexSkinMappings::reference inv = vertex_skin_mappings[i];

    // Updates max_influences.
    max_influences = ozz::math::Max(max_influences, inv.size());

    // Normalize weights.
    float sum = 0.f;
    for (size_t j = 0; j < inv.size(); ++j) {
      sum += inv[j].weight;
    }
    const float inv_sum = 1.f / (sum != 0.f ? sum : 1.f);
    for (size_t j = 0; j < inv.size(); ++j) {
      inv[j].weight *= inv_sum;
    }

    // Sort weights, bigger ones first, so that lowest one can be filtered out.
    std::sort(inv.begin(), inv.end(), &SortInfluenceWeights);
  }

  // Allocates indices and weights.
  part.joint_indices.resize(vertex_count * max_influences);
  part.joint_weights.resize(vertex_count * max_influences);

  // Build output vertices data.
  bool vertex_isnt_influenced = false;
  for (size_t i = 0; i < vertex_count; ++i) {
    VertexSkinMappings::const_reference inv = vertex_skin_mappings[i];
    uint16_t* indices = &part.joint_indices[i * max_influences];
    float* weights = &part.joint_weights[i * max_influences];

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

// Limits the number of joints influencing a vertex.
bool LimitInfluences(ozz::sample::Mesh& _skinned_mesh, int _limit) {
  assert(_skinned_mesh.parts.size() == 1);

  ozz::sample::Mesh::Part& in_part = _skinned_mesh.parts.front();

  // Check if it's actually needed to limit the number of influences.
  const int max_influences = in_part.influences_count();
  assert(max_influences > 0);
  if (max_influences <= _limit) {
    return true;
  }

  // Iterate all vertices to remove unwanted weights and renormalizes.
  // Note that weights are already sorted, so the last ones are the less
  // influencing.
  const size_t vertex_count = in_part.vertex_count();
  for (size_t i = 0, offset = 0; i < vertex_count; ++i, offset += _limit) {
    // Remove exceeding influences
    for (int j = 0; j < _limit; ++j) {
      in_part.joint_indices[offset + j] =
          in_part.joint_indices[i * max_influences + j];
      in_part.joint_weights[offset + j] =
          in_part.joint_weights[i * max_influences + j];
    }
    // Renormalizes weights.
    float sum = 0.f;
    for (int j = 0; j < _limit; ++j) {
      sum += in_part.joint_weights[offset + j];
    }
    for (int j = 0; j < _limit; ++j) {
      in_part.joint_weights[offset + j] *= 1.f / sum;
    }
  }

  // Resizes data
  in_part.joint_indices.resize(vertex_count * _limit);
  in_part.joint_weights.resize(vertex_count * _limit);
  return true;
}

// Finds used joints and remaps joint indices to the minimal range.
// The mesh might not use all skeleton joints, so this function remaps joint
// indices to the subset of used joints. It also reorders inverse bin pose
// matrices.
bool RemapIndices(ozz::sample::Mesh* _skinned_mesh) {
  assert(_skinned_mesh->parts.size() == 1);

  ozz::sample::Mesh::Part& in_part = _skinned_mesh->parts.front();
  assert(in_part.influences_count() > 0);

  // Collects all unique indices.
  ozz::sample::Mesh::Part::JointIndices local_indices = in_part.joint_indices;
  std::sort(local_indices.begin(), local_indices.end());
  local_indices.erase(std::unique(local_indices.begin(), local_indices.end()),
                      local_indices.end());

  // Build mapping table of mesh original joints to the new ones. Unused joints
  // are set to 0.
  ozz::sample::Mesh::Part::JointIndices original_remap(
      _skinned_mesh->num_joints(), 0);
  for (size_t i = 0; i < local_indices.size(); ++i) {
    original_remap[local_indices[i]] =
        static_cast<ozz::sample::Mesh::Part::JointIndices::value_type>(i);
  }

  // Reset all joints in the mesh.
  for (size_t i = 0; i < in_part.joint_indices.size(); ++i) {
    in_part.joint_indices[i] = original_remap[in_part.joint_indices[i]];
  }

  // Builds joint mapping for the mesh.
  _skinned_mesh->joint_remaps = local_indices;

  // Remaps bind poses and removes unused joints.
  for (size_t i = 0; i < local_indices.size(); ++i) {
    _skinned_mesh->inverse_bind_poses[i] =
        _skinned_mesh->inverse_bind_poses[local_indices[i]];
  }
  _skinned_mesh->inverse_bind_poses.resize(local_indices.size());

  return true;
}

// Split the skinned mesh into parts. For each part, all vertices has the same
// number of influencing joints.
bool SplitParts(const ozz::sample::Mesh& _skinned_mesh,
                ozz::sample::Mesh* _partitionned_mesh) {
  assert(_skinned_mesh.parts.size() == 1);
  assert(_partitionned_mesh->parts.size() == 0);

  const ozz::sample::Mesh::Part& in_part = _skinned_mesh.parts.front();
  const size_t vertex_count = in_part.vertex_count();

  // Creates one mesh part per influence.
  const int max_influences = in_part.influences_count();
  assert(max_influences > 0);

  // Bucket-sort vertices per influence count.
  typedef ozz::vector<ozz::vector<size_t>> BuckedVertices;
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

  // Group vertices if there's not enough of them for a given part. This allows
  // to
  // limit SkinningJob fix cost overhead.
  const size_t kMinBucketSize = 32;

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
    const ozz::vector<size_t>& bucket = bucked_vertices[i];
    const size_t bucket_vertex_count = bucket.size();
    if (bucket_vertex_count == 0) {
      // No Mesh part if no vertices.
      continue;
    }

    // Adds a new part.
    _partitionned_mesh->parts.resize(_partitionned_mesh->parts.size() + 1);
    ozz::sample::Mesh::Part& out_part = _partitionned_mesh->parts.back();

    // Resize output part.
    const int influences = i + 1;
    out_part.positions.resize(bucket_vertex_count *
                              ozz::sample::Mesh::Part::kPositionsCpnts);
    out_part.normals.resize(bucket_vertex_count *
                            ozz::sample::Mesh::Part::kNormalsCpnts);
    if (in_part.uvs.size()) {
      out_part.uvs.resize(bucket_vertex_count *
                          ozz::sample::Mesh::Part::kUVsCpnts);
    }
    if (in_part.colors.size()) {
      out_part.colors.resize(bucket_vertex_count *
                             ozz::sample::Mesh::Part::kColorsCpnts);
    }
    if (in_part.tangents.size()) {
      out_part.tangents.resize(bucket_vertex_count *
                               ozz::sample::Mesh::Part::kTangentsCpnts);
    }
    out_part.joint_indices.resize(bucket_vertex_count * influences);
    out_part.joint_weights.resize(bucket_vertex_count * influences);

    // Fills output of this part.
    for (size_t j = 0; j < bucket_vertex_count; ++j) {
      const size_t bucket_vertex_index = bucket[j];

      // Fills positions.
      float* out_pos =
          &out_part.positions[j * ozz::sample::Mesh::Part::kPositionsCpnts];
      const float* in_pos =
          &in_part.positions[bucket_vertex_index *
                             ozz::sample::Mesh::Part::kPositionsCpnts];
      out_pos[0] = in_pos[0];
      out_pos[1] = in_pos[1];
      out_pos[2] = in_pos[2];

      // Fills normals.
      float* out_normal =
          &out_part.normals[j * ozz::sample::Mesh::Part::kNormalsCpnts];
      const float* in_normal =
          &in_part.normals[bucket_vertex_index *
                           ozz::sample::Mesh::Part::kNormalsCpnts];
      out_normal[0] = in_normal[0];
      out_normal[1] = in_normal[1];
      out_normal[2] = in_normal[2];

      // Fills uvs.
      if (in_part.uvs.size()) {
        float* out_uv = &out_part.uvs[j * ozz::sample::Mesh::Part::kUVsCpnts];
        const float* in_uv =
            &in_part
                 .uvs[bucket_vertex_index * ozz::sample::Mesh::Part::kUVsCpnts];
        out_uv[0] = in_uv[0];
        out_uv[1] = in_uv[1];
      }
      // Fills colors.
      if (in_part.colors.size()) {
        uint8_t* out_color =
            &out_part.colors[j * ozz::sample::Mesh::Part::kColorsCpnts];
        const uint8_t* in_color =
            &in_part.colors[bucket_vertex_index *
                            ozz::sample::Mesh::Part::kColorsCpnts];
        out_color[0] = in_color[0];
        out_color[1] = in_color[1];
        out_color[2] = in_color[2];
        out_color[3] = in_color[3];
      }
      // Fills tangents.
      if (in_part.tangents.size()) {
        float* out_tangent =
            &out_part.tangents[j * ozz::sample::Mesh::Part::kTangentsCpnts];
        const float* in_tangent =
            &in_part.tangents[bucket_vertex_index *
                              ozz::sample::Mesh::Part::kTangentsCpnts];
        out_tangent[0] = in_tangent[0];
        out_tangent[1] = in_tangent[1];
        out_tangent[2] = in_tangent[2];
        out_tangent[3] = in_tangent[3];
      }

      // Fills joints indices.
      const uint16_t* in_indices =
          &in_part.joint_indices[bucket_vertex_index * max_influences];
      uint16_t* out_indices = &out_part.joint_indices[j * influences];
      for (int k = 0; k < influences; ++k) {
        out_indices[k] = in_indices[k];
      }

      // Fills weights. Note that there's no weight if there's only one joint
      // influencing a vertex.
      if (influences > 1) {
        const float* in_weights =
            &in_part.joint_weights[bucket_vertex_index * max_influences];
        float* out_weights = &out_part.joint_weights[j * influences];
        for (int k = 0; k < influences; ++k) {
          out_weights[k] = in_weights[k];
        }
      }
    }
  }

  // Builds a vertex remapping table to help rebuild triangle indices.
  ozz::vector<uint16_t> vertices_remap;
  vertices_remap.resize(vertex_count);
  uint16_t processed_vertices = 0;
  for (size_t i = 0; i < bucked_vertices.size(); ++i) {
    const ozz::vector<size_t>& bucket = bucked_vertices[i];
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

  // Copy bind pose matrices
  _partitionned_mesh->inverse_bind_poses = _skinned_mesh.inverse_bind_poses;
  _partitionned_mesh->joint_remaps = _skinned_mesh.joint_remaps;

  return true;
}

// Removes the less significant weight, which is recomputed at runtime (sum of
// weights equals 1).
bool StripWeights(ozz::sample::Mesh* _mesh) {
  for (size_t i = 0; i < _mesh->parts.size(); ++i) {
    ozz::sample::Mesh::Part& part = _mesh->parts[i];
    const int influence_count = part.influences_count();
    const int vertex_count = part.vertex_count();
    if (influence_count <= 1) {
      part.joint_weights.clear();
    } else {
      const ozz::vector<float> copy = part.joint_weights;
      part.joint_weights.clear();
      part.joint_weights.reserve(vertex_count * (influence_count - 1));

      for (int j = 0; j < vertex_count; ++j) {
        for (int k = 0; k < influence_count - 1; ++k) {
          part.joint_weights.push_back(copy[j * influence_count + k]);
        }
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
      _argc, _argv, "1.1",
      "Imports a skin from a fbx file and converts it to ozz binary format");
  if (parse_result != ozz::options::kSuccess) {
    return parse_result == ozz::options::kExitSuccess ? EXIT_SUCCESS
                                                      : EXIT_FAILURE;
  }

  // Opens skeleton file.
  ozz::animation::Skeleton skeleton;
  {
    ozz::log::Out() << "Loading skeleton archive " << OPTIONS_skeleton.value()
                    << "." << std::endl;
    ozz::io::File file(OPTIONS_skeleton.value(), "rb");
    if (!file.opened()) {
      ozz::log::Err() << "Failed to open skeleton file "
                      << OPTIONS_skeleton.value() << "." << std::endl;
      return EXIT_FAILURE;
    }
    ozz::io::IArchive archive(&file);
    if (!archive.TestTag<ozz::animation::Skeleton>()) {
      ozz::log::Err() << "Failed to load skeleton instance from file "
                      << OPTIONS_skeleton.value() << "." << std::endl;
      return EXIT_FAILURE;
    }

    // Once the tag is validated, reading cannot fail.
    archive >> skeleton;
  }

  // Import Fbx content.
  ozz::animation::offline::fbx::FbxManagerInstance fbx_manager;
  ozz::animation::offline::fbx::FbxDefaultIOSettings settings(fbx_manager);
  ozz::animation::offline::fbx::FbxSceneLoader scene_loader(
      OPTIONS_file, "", fbx_manager, settings);
  if (!scene_loader.scene()) {
    ozz::log::Err() << "Failed to import file " << OPTIONS_file.value() << "."
                    << std::endl;
    return EXIT_FAILURE;
  }

  const int num_meshes = scene_loader.scene()->GetSrcObjectCount<FbxMesh>();
  if (num_meshes == 0) {
    ozz::log::Err() << "No mesh to process in this file: "
                    << OPTIONS_file.value() << "." << std::endl;
    return EXIT_FAILURE;
  } else if (num_meshes > 1) {
    ozz::log::Err() << "There's more than one mesh in the file: "
                    << OPTIONS_file.value() << ". All (" << num_meshes
                    << ") meshes will be concatenated to the output file."
                    << std::endl;
  }

  {  // Clean and triangulates the scene.
    ozz::log::LogV() << "Triangulating scene." << std::endl;
    FbxGeometryConverter converter(fbx_manager);
    converter.RemoveBadPolygonsFromMeshes(scene_loader.scene());
    if (!converter.Triangulate(scene_loader.scene(), true)) {
      ozz::log::Err() << "Failed to triangulating meshes." << std::endl;
      return EXIT_FAILURE;
    }
  }

  // Take all meshes
  ozz::vector<ozz::sample::Mesh> meshes;
  meshes.resize(num_meshes);

  for (int m = 0; m < num_meshes; ++m) {
    FbxMesh* mesh = scene_loader.scene()->GetSrcObject<FbxMesh>(m);

    // Allocates output mesh.
    ozz::sample::Mesh& output_mesh = meshes[m];
    output_mesh.parts.resize(1);

    ControlPointsRemap remap;
    if (!BuildVertices(mesh, scene_loader.converter(), &remap, &output_mesh)) {
      ozz::log::Err() << "Failed to read vertices." << std::endl;
      return EXIT_FAILURE;
    }

    // Finds skinning informations
    if (mesh->GetDeformerCount(FbxDeformer::eSkin) > 0) {
      if (!BuildSkin(mesh, scene_loader.converter(), remap, skeleton,
                     &output_mesh)) {
        ozz::log::Err() << "Failed to read skinning data." << std::endl;
        return EXIT_FAILURE;
      }

      // Limiting number of joint influences per vertex.
      if (OPTIONS_max_influences > 0) {
        ozz::sample::Mesh partitioned_meshes;
        if (!LimitInfluences(output_mesh, OPTIONS_max_influences)) {
          ozz::log::Err() << "Failed to limit number of joint influences."
                          << std::endl;
          return EXIT_FAILURE;
        }
      }

      // Remap joint indices. The mesh might not use all skeleton joints, so
      // this function remaps joint indices to the subset of used joints. It
      // also reoders inverse bin pose matrices.
      if (!RemapIndices(&output_mesh)) {
        ozz::log::Err() << "Failed to remap joint indices." << std::endl;
        return EXIT_FAILURE;
      }

      // Split the mesh if option is true (default)
      if (OPTIONS_split) {
        ozz::sample::Mesh partitioned_meshes;
        if (!SplitParts(output_mesh, &partitioned_meshes)) {
          ozz::log::Err() << "Failed to partitioned meshes." << std::endl;
          return EXIT_FAILURE;
        }

        // Copy partitioned mesh back to the output.
        output_mesh = partitioned_meshes;
      }

      if (!StripWeights(&output_mesh)) {
        ozz::log::Err() << "Failed to strip weights." << std::endl;
        return EXIT_FAILURE;
      }

      assert(OPTIONS_max_influences <= 0 ||
             output_mesh.max_influences_count() <= OPTIONS_max_influences);
    }
  }

  // Opens output file.
  ozz::io::File mesh_file(OPTIONS_mesh, "wb");
  if (!mesh_file.opened()) {
    ozz::log::Err() << "Failed to open output file: " << OPTIONS_mesh.value()
                    << std::endl;
    return EXIT_FAILURE;
  }

  {
    // Serialize the partitioned meshes.
    // They aren't serialized as a vector/array as we don't know how they are
    // going to be read.
    ozz::io::OArchive archive(&mesh_file);
    for (size_t m = 0; m < meshes.size(); ++m) {
      archive << meshes[m];
    }
  }

  ozz::log::Log() << "Mesh binary archive successfully outputted for file "
                  << OPTIONS_file.value() << "." << std::endl;

  return EXIT_SUCCESS;
}
