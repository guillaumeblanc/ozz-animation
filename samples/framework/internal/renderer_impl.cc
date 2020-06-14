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

#define OZZ_INCLUDE_PRIVATE_HEADER  // Allows to include private headers.

#include "renderer_impl.h"

#include "camera.h"
#include "framework/mesh.h"
#include "icosphere.h"
#include "immediate.h"
#include "ozz/animation/runtime/local_to_model_job.h"
#include "ozz/animation/runtime/skeleton.h"
#include "ozz/animation/runtime/skeleton_utils.h"
#include "ozz/base/log.h"
#include "ozz/base/maths/box.h"
#include "ozz/base/maths/math_ex.h"
#include "ozz/base/maths/simd_math.h"
#include "ozz/base/maths/vec_float.h"
#include "ozz/base/memory/allocator.h"
#include "ozz/base/platform.h"
#include "ozz/geometry/runtime/skinning_job.h"
#include "shader.h"

namespace ozz {
namespace sample {
namespace internal {

namespace {
// A vertex made of positions and normals.
struct VertexPNC {
  math::Float3 pos;
  math::Float3 normal;
  Color color;
};
}  // namespace

RendererImpl::Model::Model() : vbo(0), mode(GL_POINTS), count(0) {}

RendererImpl::Model::~Model() {
  if (vbo) {
    GL(DeleteBuffers(1, &vbo));
    vbo = 0;
  }
}

RendererImpl::RendererImpl(Camera* _camera)
    : camera_(_camera),
      dynamic_array_bo_(0),
      dynamic_index_bo_(0),
      checkered_texture_(0) {}

RendererImpl::~RendererImpl() {
  if (dynamic_array_bo_) {
    GL(DeleteBuffers(1, &dynamic_array_bo_));
    dynamic_array_bo_ = 0;
  }

  if (dynamic_index_bo_) {
    GL(DeleteBuffers(1, &dynamic_index_bo_));
    dynamic_index_bo_ = 0;
  }

  if (checkered_texture_) {
    GL(DeleteTextures(1, &checkered_texture_));
    checkered_texture_ = 0;
  }
}

bool RendererImpl::Initialize() {
  if (!InitOpenGLExtensions()) {
    return false;
  }
  if (!InitPostureRendering()) {
    return false;
  }
  if (!InitCheckeredTexture()) {
    return false;
  }

  // Builds the dynamic vbo
  GL(GenBuffers(1, &dynamic_array_bo_));
  GL(GenBuffers(1, &dynamic_index_bo_));

  // Allocate immediate mode renderer;
  immediate_ = make_unique<GlImmediateRenderer>(this);
  if (!immediate_->Initialize()) {
    return false;
  }

  // Instantiate ambient rendering shader.
  ambient_shader = AmbientShader::Build();
  if (!ambient_shader) {
    return false;
  }

  // Instantiate ambient textured rendering shader.
  ambient_textured_shader = AmbientTexturedShader::Build();
  if (!ambient_textured_shader) {
    return false;
  }

  // Instantiate instanced ambient rendering shader.
  if (GL_ARB_instanced_arrays_supported) {
    ambient_shader_instanced = AmbientShaderInstanced::Build();
    if (!ambient_shader_instanced) {
      return false;
    }
  }

  return true;
}

bool RendererImpl::DrawAxes(const ozz::math::Float4x4& _transform) {
  GlImmediatePC im(immediate_renderer(), GL_LINES, _transform);
  GlImmediatePC::Vertex v = {{0.f, 0.f, 0.f}, {0, 0, 0, 0xff}};

  // X axis (green).
  v.pos[0] = 0.f;
  v.pos[1] = 0.f;
  v.pos[2] = 0.f;
  v.rgba[0] = 0xff;
  v.rgba[1] = 0;
  v.rgba[2] = 0;
  im.PushVertex(v);
  v.pos[0] = 1.f;
  im.PushVertex(v);

  // Y axis (green).
  v.pos[0] = 0.f;
  v.pos[1] = 0.f;
  v.pos[2] = 0.f;
  v.rgba[0] = 0;
  v.rgba[1] = 0xff;
  v.rgba[2] = 0;
  im.PushVertex(v);
  v.pos[1] = 1.f;
  im.PushVertex(v);

  // Z axis (green).
  v.pos[0] = 0.f;
  v.pos[1] = 0.f;
  v.pos[2] = 0.f;
  v.rgba[0] = 0;
  v.rgba[1] = 0;
  v.rgba[2] = 0xff;
  im.PushVertex(v);
  v.pos[2] = 1.f;
  im.PushVertex(v);

  return true;
}

bool RendererImpl::DrawGrid(int _cell_count, float _cell_size) {
  const float extent = _cell_count * _cell_size;
  const float half_extent = extent * 0.5f;
  const ozz::math::Float3 corner(-half_extent, 0, -half_extent);

  GL(DepthMask(GL_FALSE));
  GL(Enable(GL_BLEND));
  GL(BlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));
  GL(Disable(GL_CULL_FACE));
  {
    GlImmediatePC im(immediate_renderer(), GL_TRIANGLE_STRIP,
                     ozz::math::Float4x4::identity());
    GlImmediatePC::Vertex v = {{0.f, 0.f, 0.f}, {0x80, 0xc0, 0xd0, 0xb0}};

    v.pos[0] = corner.x;
    v.pos[1] = corner.y;
    v.pos[2] = corner.z;
    im.PushVertex(v);
    v.pos[2] = corner.z + extent;
    im.PushVertex(v);
    v.pos[0] = corner.x + extent;
    v.pos[2] = corner.z;
    im.PushVertex(v);
    v.pos[2] = corner.z + extent;
    im.PushVertex(v);
  }

  GL(Disable(GL_BLEND));
  GL(Enable(GL_CULL_FACE));

  {
    GlImmediatePC im(immediate_renderer(), GL_LINES,
                     ozz::math::Float4x4::identity());

    // Renders lines along X axis.
    GlImmediatePC::Vertex begin = {{corner.x, corner.y, corner.z},
                                   {0x54, 0x55, 0x50, 0xff}};
    GlImmediatePC::Vertex end = begin;
    end.pos[0] += extent;
    for (int i = 0; i < _cell_count + 1; ++i) {
      im.PushVertex(begin);
      im.PushVertex(end);
      begin.pos[2] += _cell_size;
      end.pos[2] += _cell_size;
    }
    // Renders lines along Z axis.
    begin.pos[0] = corner.x;
    begin.pos[1] = corner.y;
    begin.pos[2] = corner.z;
    end = begin;
    end.pos[2] += extent;
    for (int i = 0; i < _cell_count + 1; ++i) {
      im.PushVertex(begin);
      im.PushVertex(end);
      begin.pos[0] += _cell_size;
      end.pos[0] += _cell_size;
    }
  }

  GL(DepthMask(GL_TRUE));

  return true;
}

// Computes the model space bind pose and renders it.
bool RendererImpl::DrawSkeleton(const ozz::animation::Skeleton& _skeleton,
                                const ozz::math::Float4x4& _transform,
                                bool _draw_joints) {
  using ozz::math::Float4x4;

  const int num_joints = _skeleton.num_joints();
  if (!num_joints) {
    return true;
  }

  // Reallocate matrix array if necessary.
  prealloc_models_.resize(num_joints);

  // Compute model space bind pose.
  ozz::animation::LocalToModelJob job;
  job.input = _skeleton.joint_bind_poses();
  job.output = make_span(prealloc_models_);
  job.skeleton = &_skeleton;
  if (!job.Run()) {
    return false;
  }

  // Forwards to rendering.
  return DrawPosture(_skeleton, job.output, _transform, _draw_joints);
}

bool RendererImpl::InitPostureRendering() {
  const float kInter = .2f;
  {  // Prepares bone mesh.
    const math::Float3 pos[6] = {
        math::Float3(1.f, 0.f, 0.f),     math::Float3(kInter, .1f, .1f),
        math::Float3(kInter, .1f, -.1f), math::Float3(kInter, -.1f, -.1f),
        math::Float3(kInter, -.1f, .1f), math::Float3(0.f, 0.f, 0.f)};
    const math::Float3 normals[8] = {
        Normalize(Cross(pos[2] - pos[1], pos[2] - pos[0])),
        Normalize(Cross(pos[1] - pos[2], pos[1] - pos[5])),
        Normalize(Cross(pos[3] - pos[2], pos[3] - pos[0])),
        Normalize(Cross(pos[2] - pos[3], pos[2] - pos[5])),
        Normalize(Cross(pos[4] - pos[3], pos[4] - pos[0])),
        Normalize(Cross(pos[3] - pos[4], pos[3] - pos[5])),
        Normalize(Cross(pos[1] - pos[4], pos[1] - pos[0])),
        Normalize(Cross(pos[4] - pos[1], pos[4] - pos[5]))};
    const Color white = {0xff, 0xff, 0xff, 0xff};
    const VertexPNC bones[24] = {
        {pos[0], normals[0], white}, {pos[2], normals[0], white},
        {pos[1], normals[0], white}, {pos[5], normals[1], white},
        {pos[1], normals[1], white}, {pos[2], normals[1], white},
        {pos[0], normals[2], white}, {pos[3], normals[2], white},
        {pos[2], normals[2], white}, {pos[5], normals[3], white},
        {pos[2], normals[3], white}, {pos[3], normals[3], white},
        {pos[0], normals[4], white}, {pos[4], normals[4], white},
        {pos[3], normals[4], white}, {pos[5], normals[5], white},
        {pos[3], normals[5], white}, {pos[4], normals[5], white},
        {pos[0], normals[6], white}, {pos[1], normals[6], white},
        {pos[4], normals[6], white}, {pos[5], normals[7], white},
        {pos[4], normals[7], white}, {pos[1], normals[7], white}};

    // Builds and fills the vbo.
    Model& bone = models_[0];
    bone.mode = GL_TRIANGLES;
    bone.count = OZZ_ARRAY_SIZE(bones);
    GL(GenBuffers(1, &bone.vbo));
    GL(BindBuffer(GL_ARRAY_BUFFER, bone.vbo));
    GL(BufferData(GL_ARRAY_BUFFER, sizeof(bones), bones, GL_STATIC_DRAW));
    GL(BindBuffer(GL_ARRAY_BUFFER, 0));  // Unbinds.

    // Init bone shader.
    bone.shader = BoneShader::Build();
    if (!bone.shader) {
      return false;
    }
  }

  {  // Prepares joint mesh.
    const int kNumSlices = 20;
    const int kNumPointsPerCircle = kNumSlices + 1;
    const int kNumPointsYZ = kNumPointsPerCircle;
    const int kNumPointsXY = kNumPointsPerCircle + kNumPointsPerCircle / 4;
    const int kNumPointsXZ = kNumPointsPerCircle;
    const int kNumPoints = kNumPointsXY + kNumPointsXZ + kNumPointsYZ;
    const float kRadius = kInter;  // Radius multiplier.
    const Color red = {0xff, 0xc0, 0xc0, 0xff};
    const Color green = {0xc0, 0xff, 0xc0, 0xff};
    const Color blue = {0xc0, 0xc0, 0xff, 0xff};
    VertexPNC joints[kNumPoints];

    // Fills vertices.
    int index = 0;
    for (int j = 0; j < kNumPointsYZ; ++j) {  // YZ plan.
      float angle = j * math::k2Pi / kNumSlices;
      float s = sinf(angle), c = cosf(angle);
      VertexPNC& vertex = joints[index++];
      vertex.pos = math::Float3(0.f, c * kRadius, s * kRadius);
      vertex.normal = math::Float3(0.f, c, s);
      vertex.color = red;
    }
    for (int j = 0; j < kNumPointsXY; ++j) {  // XY plan.
      float angle = j * math::k2Pi / kNumSlices;
      float s = sinf(angle), c = cosf(angle);
      VertexPNC& vertex = joints[index++];
      vertex.pos = math::Float3(s * kRadius, c * kRadius, 0.f);
      vertex.normal = math::Float3(s, c, 0.f);
      vertex.color = blue;
    }
    for (int j = 0; j < kNumPointsXZ; ++j) {  // XZ plan.
      float angle = j * math::k2Pi / kNumSlices;
      float s = sinf(angle), c = cosf(angle);
      VertexPNC& vertex = joints[index++];
      vertex.pos = math::Float3(c * kRadius, 0.f, -s * kRadius);
      vertex.normal = math::Float3(c, 0.f, -s);
      vertex.color = green;
    }
    assert(index == kNumPoints);

    // Builds and fills the vbo.
    Model& joint = models_[1];
    joint.mode = GL_LINE_STRIP;
    joint.count = OZZ_ARRAY_SIZE(joints);
    GL(GenBuffers(1, &joint.vbo));
    GL(BindBuffer(GL_ARRAY_BUFFER, joint.vbo));
    GL(BufferData(GL_ARRAY_BUFFER, sizeof(joints), joints, GL_STATIC_DRAW));
    GL(BindBuffer(GL_ARRAY_BUFFER, 0));  // Unbinds.

    // Init joint shader.
    joint.shader = JointShader::Build();
    if (!joint.shader) {
      return false;
    }
  }

  return true;
}

bool RendererImpl::InitCheckeredTexture() {
  const int kWidth = 1024;
  const int kCases = 64;

  GL(GenTextures(1, &checkered_texture_));
  GL(BindTexture(GL_TEXTURE_2D, checkered_texture_));
  GL(TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT));
  GL(TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT));
  GL(TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
  GL(TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                   GL_LINEAR_MIPMAP_LINEAR));
  GL(PixelStorei(GL_UNPACK_ALIGNMENT, 1));

  // Allocates for biggest mip level.
  const size_t buffer_size = 3 * kWidth * kWidth;
  uint8_t* pixels = reinterpret_cast<uint8_t*>(
      memory::default_allocator()->Allocate(buffer_size, 16));

  // Create the checkered pattern on all mip levels.
  int level_width = kWidth;
  for (int level = 0; level_width > 0; ++level, level_width /= 2) {
    if (level_width >= kCases) {
      const int case_width = level_width / kCases;
      for (int j = 0; j < level_width; ++j) {
        const int cpntj = (j / case_width) & 1;
        for (int i = 0; i < kCases; ++i) {
          const int cpnti = i & 1;
          const bool white_case = (cpnti ^ cpntj) != 0;
          const uint8_t cpntr =
              white_case ? 0xff : j * 255 / level_width & 0xff;
          const uint8_t cpntg = white_case ? 0xff : i * 255 / kCases & 0xff;
          const uint8_t cpntb = white_case ? 0xff : 0;

          const int case_start = j * level_width + i * case_width;
          for (int k = case_start; k < case_start + case_width; ++k) {
            pixels[k * 3 + 0] = cpntr;
            pixels[k * 3 + 1] = cpntg;
            pixels[k * 3 + 2] = cpntb;
          }
        }
      }
    } else {
      // Mimaps where width is smaller than the number of cases.
      for (int j = 0; j < level_width; ++j) {
        for (int i = 0; i < level_width; ++i) {
          pixels[(j * level_width + i) * 3 + 0] = 0x7f;
          pixels[(j * level_width + i) * 3 + 1] = 0x7f;
          pixels[(j * level_width + i) * 3 + 2] = 0x7f;
        }
      }
    }

    GL(TexImage2D(GL_TEXTURE_2D, level, GL_RGB, level_width, level_width, 0,
                  GL_RGB, GL_UNSIGNED_BYTE, pixels));
  }

  GL(BindTexture(GL_TEXTURE_2D, 0));
  memory::default_allocator()->Deallocate(pixels);

  return true;
}

namespace {
int DrawPosture_FillUniforms(const ozz::animation::Skeleton& _skeleton,
                             ozz::span<const ozz::math::Float4x4> _matrices,
                             float* _uniforms, int _max_instances) {
  assert(IsAligned(_uniforms, alignof(math::SimdFloat4)));

  // Prepares computation constants.
  const int num_joints = _skeleton.num_joints();
  const span<const int16_t>& parents = _skeleton.joint_parents();

  int instances = 0;
  for (int i = 0; i < num_joints && instances < _max_instances; ++i) {
    // Root isn't rendered.
    const int16_t parent_id = parents[i];
    if (parent_id == ozz::animation::Skeleton::kNoParent) {
      continue;
    }

    // Selects joint matrices.
    const math::Float4x4& parent = _matrices[parent_id];
    const math::Float4x4& current = _matrices[i];

    // Copy parent joint's raw matrix, to render a bone between the parent
    // and current matrix.
    float* uniform = _uniforms + instances * 16;
    math::StorePtr(parent.cols[0], uniform + 0);
    math::StorePtr(parent.cols[1], uniform + 4);
    math::StorePtr(parent.cols[2], uniform + 8);
    math::StorePtr(parent.cols[3], uniform + 12);

    // Set bone direction (bone_dir). The shader expects to find it at index
    // [3,7,11] of the matrix.
    // Index 15 is used to store whether a bone should be rendered,
    // otherwise it's a leaf.
    float bone_dir[4];
    math::StorePtrU(current.cols[3] - parent.cols[3], bone_dir);
    uniform[3] = bone_dir[0];
    uniform[7] = bone_dir[1];
    uniform[11] = bone_dir[2];
    uniform[15] = 1.f;  // Enables bone rendering.

    // Next instance.
    ++instances;
    uniform += 16;

    // Only the joint is rendered for leaves, the bone model isn't.
    if (IsLeaf(_skeleton, i)) {
      // Copy current joint's raw matrix.
      uniform = _uniforms + instances * 16;
      math::StorePtr(current.cols[0], uniform + 0);
      math::StorePtr(current.cols[1], uniform + 4);
      math::StorePtr(current.cols[2], uniform + 8);
      math::StorePtr(current.cols[3], uniform + 12);

      // Re-use bone_dir to fix the size of the leaf (same as previous bone).
      // The shader expects to find it at index [3,7,11] of the matrix.
      uniform[3] = bone_dir[0];
      uniform[7] = bone_dir[1];
      uniform[11] = bone_dir[2];
      uniform[15] = 0.f;  // Disables bone rendering.
      ++instances;
    }
  }

  return instances;
}
}  // namespace

// Draw posture internal non-instanced rendering fall back implementation.
void RendererImpl::DrawPosture_Impl(const ozz::math::Float4x4& _transform,
                                    const float* _uniforms, int _instance_count,
                                    bool _draw_joints) {
  // Loops through models and instances.
  for (int i = 0; i < (_draw_joints ? 2 : 1); ++i) {
    const Model& model = models_[i];

    // Setup model vertex data.
    GL(BindBuffer(GL_ARRAY_BUFFER, model.vbo));

    // Bind shader
    model.shader->Bind(_transform, camera_->view_proj(), sizeof(VertexPNC), 0,
                       sizeof(VertexPNC), 12, sizeof(VertexPNC), 24);

    GL(BindBuffer(GL_ARRAY_BUFFER, 0));

    // Draw loop.
    const GLint joint_uniform = model.shader->joint_uniform();
    for (int j = 0; j < _instance_count; ++j) {
      GL(UniformMatrix4fv(joint_uniform, 1, false, _uniforms + 16 * j));
      GL(DrawArrays(model.mode, 0, model.count));
    }

    model.shader->Unbind();
  }
}

// "Draw posture" internal instanced rendering implementation.
void RendererImpl::DrawPosture_InstancedImpl(
    const ozz::math::Float4x4& _transform, const float* _uniforms,
    int _instance_count, bool _draw_joints) {
  // Maps the dynamic buffer and update it.
  GL(BindBuffer(GL_ARRAY_BUFFER, dynamic_array_bo_));
  const size_t vbo_size = _instance_count * 16 * sizeof(float);
  GL(BufferData(GL_ARRAY_BUFFER, vbo_size, _uniforms, GL_STREAM_DRAW));
  GL(BindBuffer(GL_ARRAY_BUFFER, 0));

  // Renders models.
  for (int i = 0; i < (_draw_joints ? 2 : 1); ++i) {
    const Model& model = models_[i];

    // Setup model vertex data.
    GL(BindBuffer(GL_ARRAY_BUFFER, model.vbo));

    // Bind shader
    model.shader->Bind(_transform, camera_->view_proj(), sizeof(VertexPNC), 0,
                       sizeof(VertexPNC), 12, sizeof(VertexPNC), 24);

    GL(BindBuffer(GL_ARRAY_BUFFER, 0));

    // Setup instanced GL context.
    const GLint joint_attrib = model.shader->joint_instanced_attrib();
    GL(BindBuffer(GL_ARRAY_BUFFER, dynamic_array_bo_));
    GL(EnableVertexAttribArray(joint_attrib + 0));
    GL(EnableVertexAttribArray(joint_attrib + 1));
    GL(EnableVertexAttribArray(joint_attrib + 2));
    GL(EnableVertexAttribArray(joint_attrib + 3));
    GL(VertexAttribDivisor_(joint_attrib + 0, 1));
    GL(VertexAttribDivisor_(joint_attrib + 1, 1));
    GL(VertexAttribDivisor_(joint_attrib + 2, 1));
    GL(VertexAttribDivisor_(joint_attrib + 3, 1));
    GL(VertexAttribPointer(joint_attrib + 0, 4, GL_FLOAT, GL_FALSE,
                           sizeof(math::Float4x4), GL_PTR_OFFSET(0)));
    GL(VertexAttribPointer(joint_attrib + 1, 4, GL_FLOAT, GL_FALSE,
                           sizeof(math::Float4x4), GL_PTR_OFFSET(16)));
    GL(VertexAttribPointer(joint_attrib + 2, 4, GL_FLOAT, GL_FALSE,
                           sizeof(math::Float4x4), GL_PTR_OFFSET(32)));
    GL(VertexAttribPointer(joint_attrib + 3, 4, GL_FLOAT, GL_FALSE,
                           sizeof(math::Float4x4), GL_PTR_OFFSET(48)));
    GL(BindBuffer(GL_ARRAY_BUFFER, 0));

    GL(DrawArraysInstanced_(model.mode, 0, model.count, _instance_count));

    GL(DisableVertexAttribArray(joint_attrib + 0));
    GL(DisableVertexAttribArray(joint_attrib + 1));
    GL(DisableVertexAttribArray(joint_attrib + 2));
    GL(DisableVertexAttribArray(joint_attrib + 3));
    GL(VertexAttribDivisor_(joint_attrib + 0, 0));
    GL(VertexAttribDivisor_(joint_attrib + 1, 0));
    GL(VertexAttribDivisor_(joint_attrib + 2, 0));
    GL(VertexAttribDivisor_(joint_attrib + 3, 0));

    model.shader->Unbind();
  }
}

// Uses GL_ARB_instanced_arrays_supported as a first choice to render the whole
// skeleton in a single draw call. Does a draw call per joint if no extension
// can help.
bool RendererImpl::DrawPosture(const ozz::animation::Skeleton& _skeleton,
                               ozz::span<const ozz::math::Float4x4> _matrices,
                               const ozz::math::Float4x4& _transform,
                               bool _draw_joints) {
  if (_matrices.size() < static_cast<size_t>(_skeleton.num_joints())) {
    return false;
  }

  // Convert matrices to uniforms.
  const int max_skeleton_pieces = animation::Skeleton::kMaxJoints * 2;
  const size_t max_uniforms_size = max_skeleton_pieces * 2 * 16 * sizeof(float);
  float* uniforms =
      static_cast<float*>(scratch_buffer_.Resize(max_uniforms_size));

  const int instance_count = DrawPosture_FillUniforms(
      _skeleton, _matrices, uniforms, max_skeleton_pieces);
  assert(instance_count <= max_skeleton_pieces);

  if (GL_ARB_instanced_arrays_supported) {
    DrawPosture_InstancedImpl(_transform, uniforms, instance_count,
                              _draw_joints);
  } else {
    DrawPosture_Impl(_transform, uniforms, instance_count, _draw_joints);
  }

  return true;
}

bool RendererImpl::DrawBoxIm(const ozz::math::Box& _box,
                             const ozz::math::Float4x4& _transform,
                             const Color _colors[2]) {
  {  // Filled boxed
    GlImmediatePC im(immediate_renderer(), GL_TRIANGLE_STRIP, _transform);
    GlImmediatePC::Vertex v = {
        {0, 0, 0}, {_colors[0].r, _colors[0].g, _colors[0].b, _colors[0].a}};
    // First 3 cube faces
    v.pos[0] = _box.max.x;
    v.pos[1] = _box.min.y;
    v.pos[2] = _box.min.z;
    im.PushVertex(v);
    v.pos[0] = _box.min.x;
    im.PushVertex(v);
    v.pos[0] = _box.max.x;
    v.pos[1] = _box.max.y;
    im.PushVertex(v);
    v.pos[0] = _box.min.x;
    im.PushVertex(v);
    v.pos[0] = _box.max.x;
    v.pos[2] = _box.max.z;
    im.PushVertex(v);
    v.pos[0] = _box.min.x;
    im.PushVertex(v);
    v.pos[0] = _box.max.x;
    v.pos[1] = _box.min.y;
    im.PushVertex(v);
    v.pos[0] = _box.min.x;
    im.PushVertex(v);
    // Link next 3 cube faces with degenerated triangles.
    im.PushVertex(v);
    v.pos[0] = _box.min.x;
    v.pos[1] = _box.max.y;
    im.PushVertex(v);
    im.PushVertex(v);
    // Last 3 cube faces.
    v.pos[2] = _box.min.z;
    im.PushVertex(v);
    v.pos[1] = _box.min.y;
    v.pos[2] = _box.max.z;
    im.PushVertex(v);
    v.pos[2] = _box.min.z;
    im.PushVertex(v);
    v.pos[0] = _box.max.x;
    v.pos[2] = _box.max.z;
    im.PushVertex(v);
    v.pos[2] = _box.min.z;
    im.PushVertex(v);
    v.pos[1] = _box.max.y;
    v.pos[2] = _box.max.z;
    im.PushVertex(v);
    v.pos[2] = _box.min.z;
    im.PushVertex(v);
  }

  {  // Wireframe boxed
    GlImmediatePC im(immediate_renderer(), GL_LINES, _transform);
    GlImmediatePC::Vertex v = {
        {0, 0, 0}, {_colors[1].r, _colors[1].g, _colors[1].b, _colors[1].a}};
    // First face.
    v.pos[0] = _box.min.x;
    v.pos[1] = _box.min.y;
    v.pos[2] = _box.min.z;
    im.PushVertex(v);
    v.pos[1] = _box.max.y;
    im.PushVertex(v);
    im.PushVertex(v);
    v.pos[0] = _box.max.x;
    im.PushVertex(v);
    im.PushVertex(v);
    v.pos[1] = _box.min.y;
    im.PushVertex(v);
    im.PushVertex(v);
    v.pos[0] = _box.min.x;
    im.PushVertex(v);
    // Second face.
    v.pos[2] = _box.max.z;
    im.PushVertex(v);
    v.pos[1] = _box.max.y;
    im.PushVertex(v);
    im.PushVertex(v);
    v.pos[0] = _box.max.x;
    im.PushVertex(v);
    im.PushVertex(v);
    v.pos[1] = _box.min.y;
    im.PushVertex(v);
    im.PushVertex(v);
    v.pos[0] = _box.min.x;
    im.PushVertex(v);
    // Link faces.
    im.PushVertex(v);
    v.pos[2] = _box.min.z;
    im.PushVertex(v);
    v.pos[1] = _box.max.y;
    im.PushVertex(v);
    v.pos[2] = _box.max.z;
    im.PushVertex(v);
    v.pos[0] = _box.max.x;
    im.PushVertex(v);
    v.pos[2] = _box.min.z;
    im.PushVertex(v);
    v.pos[1] = _box.min.y;
    im.PushVertex(v);
    v.pos[2] = _box.max.z;
    im.PushVertex(v);
  }

  return true;
}

bool RendererImpl::DrawBoxShaded(
    const ozz::math::Box& _box,
    ozz::span<const ozz::math::Float4x4> _transforms, Color _color) {
  // Early out if no instance to render.
  if (_transforms.size() == 0) {
    return true;
  }

  const math::Float3 pos[8] = {
      math::Float3(_box.min.x, _box.min.y, _box.min.z),
      math::Float3(_box.max.x, _box.min.y, _box.min.z),
      math::Float3(_box.max.x, _box.max.y, _box.min.z),
      math::Float3(_box.min.x, _box.max.y, _box.min.z),
      math::Float3(_box.min.x, _box.min.y, _box.max.z),
      math::Float3(_box.max.x, _box.min.y, _box.max.z),
      math::Float3(_box.max.x, _box.max.y, _box.max.z),
      math::Float3(_box.min.x, _box.max.y, _box.max.z)};
  const math::Float3 normals[6] = {
      math::Float3(-1, 0, 0), math::Float3(1, 0, 0),  math::Float3(0, -1, 0),
      math::Float3(0, 1, 0),  math::Float3(0, 0, -1), math::Float3(0, 0, 1)};
  const VertexPNC vertices[36] = {
      {pos[0], normals[4], _color}, {pos[3], normals[4], _color},
      {pos[1], normals[4], _color}, {pos[3], normals[4], _color},
      {pos[2], normals[4], _color}, {pos[1], normals[4], _color},
      {pos[2], normals[3], _color}, {pos[3], normals[3], _color},
      {pos[7], normals[3], _color}, {pos[7], normals[3], _color},
      {pos[6], normals[3], _color}, {pos[2], normals[3], _color},
      {pos[5], normals[5], _color}, {pos[6], normals[5], _color},
      {pos[7], normals[5], _color}, {pos[5], normals[5], _color},
      {pos[7], normals[5], _color}, {pos[4], normals[5], _color},
      {pos[0], normals[2], _color}, {pos[1], normals[2], _color},
      {pos[4], normals[2], _color}, {pos[4], normals[2], _color},
      {pos[1], normals[2], _color}, {pos[5], normals[2], _color},
      {pos[0], normals[0], _color}, {pos[4], normals[0], _color},
      {pos[3], normals[0], _color}, {pos[4], normals[0], _color},
      {pos[7], normals[0], _color}, {pos[3], normals[0], _color},
      {pos[5], normals[1], _color}, {pos[1], normals[1], _color},
      {pos[2], normals[1], _color}, {pos[5], normals[1], _color},
      {pos[2], normals[1], _color}, {pos[6], normals[1], _color}};

  const GLsizei stride = sizeof(VertexPNC);
  const GLsizei positions_offset = 0;
  const GLsizei normals_offset = positions_offset + sizeof(float) * 3;
  const GLsizei colors_offset = normals_offset + sizeof(float) * 3;

  if (GL_ARB_instanced_arrays_supported) {
    // Buffer object will contain vertices and model matrices.
    const size_t bo_size = sizeof(vertices) + _transforms.size_bytes();
    GL(BindBuffer(GL_ARRAY_BUFFER, dynamic_array_bo_));
    GL(BufferData(GL_ARRAY_BUFFER, bo_size, nullptr, GL_STREAM_DRAW));

    // Pushes vertices
    GL(BufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices));
    // Pushes matrices
    const size_t models_offset = sizeof(vertices);
    GL(BufferSubData(GL_ARRAY_BUFFER, models_offset, _transforms.size_bytes(),
                     _transforms.data()));

    ambient_shader_instanced->Bind(models_offset, camera()->view_proj(), stride,
                                   positions_offset, stride, normals_offset,
                                   stride, colors_offset);
    GL(BindBuffer(GL_ARRAY_BUFFER, 0));

    GL(DrawArraysInstanced_(GL_TRIANGLES, 0, OZZ_ARRAY_SIZE(vertices),
                            static_cast<GLsizei>(_transforms.size())));

    // Unbinds.
    ambient_shader_instanced->Unbind();
  } else {
    // Reallocate vertex buffer.
    GL(BindBuffer(GL_ARRAY_BUFFER, dynamic_array_bo_));
    GL(BufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STREAM_DRAW));

    for (size_t i = 0; i < _transforms.size(); i++) {
      const ozz::math::Float4x4& transform = _transforms[i];

      ambient_shader->Bind(transform, camera()->view_proj(), stride,
                           positions_offset, stride, normals_offset, stride,
                           colors_offset);

      // Draws the mesh.
      GL(DrawArrays(GL_TRIANGLES, 0, OZZ_ARRAY_SIZE(vertices)));

      // Unbinds.
      ambient_shader->Unbind();
    }
    GL(BindBuffer(GL_ARRAY_BUFFER, 0));
  }

  return true;
}

// Renders a sphere at a specified location.
bool RendererImpl::DrawSphereIm(float _radius,
                                const ozz::math::Float4x4& _transform,
                                const Color _color) {
  {  // Filled boxed
    const ozz::math::Float4x4& transform =
        Scale(_transform,
              ozz::math::simd_float4::Load(_radius, _radius, _radius, 1.f));
    GlImmediatePC im(immediate_renderer(), GL_TRIANGLES, transform);
    GlImmediatePC::Vertex v = {{0, 0, 0},
                               {_color.r, _color.g, _color.b, _color.a}};

    for (int i = 0; i < icosphere::kNumIndices; ++i) {
      const uint16_t vi = icosphere::kIndices[i];
      v.pos[0] = icosphere::kVertices[vi * 3 + 0];
      v.pos[1] = icosphere::kVertices[vi * 3 + 1];
      v.pos[2] = icosphere::kVertices[vi * 3 + 2];
      im.PushVertex(v);
    }
  }
  return true;
}

// Renders shaded spheres at specified locations.
bool RendererImpl::DrawSphereShaded(
    float _radius, ozz::span<const ozz::math::Float4x4> _transforms,
    Color _color) {
  // Early out if no instance to render.
  if (_transforms.size() == 0) {
    return true;
  }

  ozz::math::SimdFloat4 radius =
      ozz::math::simd_float4::Load(_radius, _radius, _radius, 1.f);

  // Setup indices
  GL(BindBuffer(GL_ELEMENT_ARRAY_BUFFER, dynamic_index_bo_));
  GL(BufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(icosphere::kIndices),
                icosphere::kIndices, GL_STREAM_DRAW));

  // Vertices
  const GLsizei positions_offset = 0;
  const GLsizei positions_stride = sizeof(float) * 3;
  const GLsizei normals_offset =
      positions_offset;  // Normals and positions are the same.
  const GLsizei normals_stride = positions_stride;
  const GLsizei colors_offset = sizeof(icosphere::kVertices);

  if (GL_ARB_instanced_arrays_supported) {
    const GLsizei colors_stride = 0;
    const GLsizei colors_size = sizeof(uint8_t) * 4;
    const GLsizei models_offset = sizeof(icosphere::kVertices) + colors_size;
    const GLsizei bo_size =
        models_offset + static_cast<GLsizei>(_transforms.size_bytes());

    GL(BindBuffer(GL_ARRAY_BUFFER, dynamic_array_bo_));
    GL(BufferData(GL_ARRAY_BUFFER, bo_size, nullptr, GL_STREAM_DRAW));
    GL(BufferSubData(GL_ARRAY_BUFFER, positions_offset,
                     sizeof(icosphere::kVertices), icosphere::kVertices));
    GL(BufferSubData(GL_ARRAY_BUFFER, colors_offset, colors_size, &_color));

    ozz::math::Float4x4* models = static_cast<ozz::math::Float4x4*>(
        scratch_buffer_.Resize(_transforms.size_bytes()));
    for (size_t i = 0; i < _transforms.size(); ++i) {
      models[i] = Scale(_transforms[i], radius);
    }
    GL(BufferSubData(GL_ARRAY_BUFFER, models_offset, _transforms.size_bytes(),
                     models));

    ambient_shader_instanced->Bind(models_offset, camera()->view_proj(),
                                   positions_stride, positions_offset,
                                   normals_stride, normals_offset,
                                   colors_stride, colors_offset);

    static_assert(sizeof(icosphere::kIndices[0]) == 2,
                  "Indices must be 2 bytes");
    GL(DrawElementsInstanced_(GL_TRIANGLES, OZZ_ARRAY_SIZE(icosphere::kIndices),
                              GL_UNSIGNED_SHORT, 0,
                              static_cast<GLsizei>(_transforms.size())));

    // Unbinds.
    ambient_shader_instanced->Unbind();
  } else {
    // OpenGL doesn't support 0 stride (without glVertexAttribDivisor
    // extension), so we must copy a color for each vertex.
    const GLsizei colors_stride = sizeof(uint8_t) * 4;
    const GLsizei colors_size = colors_stride * icosphere::kNumVertices;
    const GLsizei bo_size = sizeof(icosphere::kVertices) + colors_size;

    // Reallocate vertex buffer.
    GL(BindBuffer(GL_ARRAY_BUFFER, dynamic_array_bo_));
    GL(BufferData(GL_ARRAY_BUFFER, bo_size, nullptr, GL_STREAM_DRAW));
    GL(BufferSubData(GL_ARRAY_BUFFER, positions_offset,
                     sizeof(icosphere::kVertices), icosphere::kVertices));
    Color* colors = static_cast<Color*>(scratch_buffer_.Resize(colors_size));
    for (int i = 0; i < icosphere::kNumVertices; ++i) {
      colors[i] = _color;
    }
    GL(BufferSubData(GL_ARRAY_BUFFER, colors_offset, colors_size, colors));

    for (size_t i = 0; i < _transforms.size(); i++) {
      const ozz::math::Float4x4& transform = Scale(_transforms[i], radius);

      ambient_shader->Bind(transform, camera()->view_proj(), positions_stride,
                           positions_offset, normals_stride, normals_offset,
                           colors_stride, colors_offset);

      static_assert(sizeof(icosphere::kIndices[0]) == 2,
                    "Indices must be 2 bytes");
      GL(DrawElements(GL_TRIANGLES, OZZ_ARRAY_SIZE(icosphere::kIndices),
                      GL_UNSIGNED_SHORT, 0));

      // Unbinds.
      ambient_shader->Unbind();
    }
    GL(BindBuffer(GL_ARRAY_BUFFER, 0));
    GL(BindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0));
  }

  return true;
}

bool RendererImpl::DrawSegment(const math::Float3& _begin,
                               const math::Float3& _end, Color _color,
                               const ozz::math::Float4x4& _transform) {
  const math::Float3 dir(_end - _begin);
  return DrawVectors(ozz::span<const float>(&_begin.x, 3), 12,
                     ozz::span<const float>(&dir.x, 3), 12, 1, 1.f, _color,
                     _transform);
}

bool RendererImpl::DrawVectors(ozz::span<const float> _positions,
                               size_t _positions_stride,
                               ozz::span<const float> _directions,
                               size_t _directions_stride, int _num_vectors,
                               float _vector_length, Color _color,
                               const ozz::math::Float4x4& _transform) {
  // Invalid range length.
  if (PointerStride(_positions.begin(), _positions_stride * _num_vectors) >
          _positions.end() ||
      PointerStride(_directions.begin(), _directions_stride * _num_vectors) >
          _directions.end()) {
    return false;
  }

  GlImmediatePC im(immediate_renderer(), GL_LINES, _transform);
  GlImmediatePC::Vertex v = {{0, 0, 0},
                             {_color.r, _color.g, _color.b, _color.a}};

  for (int i = 0; i < _num_vectors; ++i) {
    const float* position =
        PointerStride(_positions.data(), _positions_stride * i);
    v.pos[0] = position[0];
    v.pos[1] = position[1];
    v.pos[2] = position[2];
    im.PushVertex(v);

    const float* direction =
        PointerStride(_directions.data(), _directions_stride * i);
    v.pos[0] = position[0] + direction[0] * _vector_length;
    v.pos[1] = position[1] + direction[1] * _vector_length;
    v.pos[2] = position[2] + direction[2] * _vector_length;
    im.PushVertex(v);
  }

  return true;
}

bool RendererImpl::DrawBinormals(
    ozz::span<const float> _positions, size_t _positions_stride,
    ozz::span<const float> _normals, size_t _normals_stride,
    ozz::span<const float> _tangents, size_t _tangents_stride,
    ozz::span<const float> _handenesses, size_t _handenesses_stride,
    int _num_vectors, float _vector_length, Color _color,
    const ozz::math::Float4x4& _transform) {
  // Invalid range length.
  if (PointerStride(_positions.begin(), _positions_stride * _num_vectors) >
          _positions.end() ||
      PointerStride(_normals.begin(), _normals_stride * _num_vectors) >
          _normals.end() ||
      PointerStride(_tangents.begin(), _tangents_stride * _num_vectors) >
          _tangents.end() ||
      PointerStride(_handenesses.begin(), _handenesses_stride * _num_vectors) >
          _handenesses.end()) {
    return false;
  }

  GlImmediatePC im(immediate_renderer(), GL_LINES, _transform);
  GlImmediatePC::Vertex v = {{0, 0, 0},
                             {_color.r, _color.g, _color.b, _color.a}};

  for (int i = 0; i < _num_vectors; ++i) {
    const float* position =
        PointerStride(_positions.data(), _positions_stride * i);
    v.pos[0] = position[0];
    v.pos[1] = position[1];
    v.pos[2] = position[2];
    im.PushVertex(v);

    // Compute binormal.
    const float* p_normal = PointerStride(_normals.data(), _normals_stride * i);
    const ozz::math::Float3 normal(p_normal[0], p_normal[1], p_normal[2]);
    const float* p_tangent =
        PointerStride(_tangents.data(), _tangents_stride * i);
    const ozz::math::Float3 tangent(p_tangent[0], p_tangent[1], p_tangent[2]);
    const float* p_handedness =
        PointerStride(_handenesses.data(), _handenesses_stride * i);
    // Handedness is used to flip binormal.
    const ozz::math::Float3 binormal = Cross(normal, tangent) * p_handedness[0];

    v.pos[0] = position[0] + binormal.x * _vector_length;
    v.pos[1] = position[1] + binormal.y * _vector_length;
    v.pos[2] = position[2] + binormal.z * _vector_length;
    im.PushVertex(v);
  }
  return true;
}

namespace {
const uint8_t kDefaultColorsArray[][4] = {
    {255, 255, 255, 255}, {255, 255, 255, 255}, {255, 255, 255, 255},
    {255, 255, 255, 255}, {255, 255, 255, 255}, {255, 255, 255, 255},
    {255, 255, 255, 255}, {255, 255, 255, 255}, {255, 255, 255, 255},
    {255, 255, 255, 255}, {255, 255, 255, 255}, {255, 255, 255, 255},
    {255, 255, 255, 255}, {255, 255, 255, 255}, {255, 255, 255, 255},
    {255, 255, 255, 255}, {255, 255, 255, 255}, {255, 255, 255, 255},
    {255, 255, 255, 255}, {255, 255, 255, 255}, {255, 255, 255, 255},
    {255, 255, 255, 255}, {255, 255, 255, 255}, {255, 255, 255, 255},
    {255, 255, 255, 255}, {255, 255, 255, 255}, {255, 255, 255, 255},
    {255, 255, 255, 255}, {255, 255, 255, 255}, {255, 255, 255, 255},
    {255, 255, 255, 255}, {255, 255, 255, 255}, {255, 255, 255, 255},
    {255, 255, 255, 255}, {255, 255, 255, 255}, {255, 255, 255, 255},
    {255, 255, 255, 255}, {255, 255, 255, 255}, {255, 255, 255, 255},
    {255, 255, 255, 255}, {255, 255, 255, 255}, {255, 255, 255, 255},
    {255, 255, 255, 255}, {255, 255, 255, 255}, {255, 255, 255, 255},
    {255, 255, 255, 255}, {255, 255, 255, 255}, {255, 255, 255, 255},
    {255, 255, 255, 255}, {255, 255, 255, 255}, {255, 255, 255, 255},
    {255, 255, 255, 255}, {255, 255, 255, 255}, {255, 255, 255, 255},
    {255, 255, 255, 255}, {255, 255, 255, 255}, {255, 255, 255, 255},
    {255, 255, 255, 255}, {255, 255, 255, 255}, {255, 255, 255, 255},
    {255, 255, 255, 255}, {255, 255, 255, 255}, {255, 255, 255, 255},
    {255, 255, 255, 255}, {255, 255, 255, 255}, {255, 255, 255, 255},
    {255, 255, 255, 255}, {255, 255, 255, 255}, {255, 255, 255, 255},
    {255, 255, 255, 255}, {255, 255, 255, 255}, {255, 255, 255, 255}};

const float kDefaultNormalsArray[][3] = {
    {0.f, 1.f, 0.f}, {0.f, 1.f, 0.f}, {0.f, 1.f, 0.f}, {0.f, 1.f, 0.f},
    {0.f, 1.f, 0.f}, {0.f, 1.f, 0.f}, {0.f, 1.f, 0.f}, {0.f, 1.f, 0.f},
    {0.f, 1.f, 0.f}, {0.f, 1.f, 0.f}, {0.f, 1.f, 0.f}, {0.f, 1.f, 0.f},
    {0.f, 1.f, 0.f}, {0.f, 1.f, 0.f}, {0.f, 1.f, 0.f}, {0.f, 1.f, 0.f},
    {0.f, 1.f, 0.f}, {0.f, 1.f, 0.f}, {0.f, 1.f, 0.f}, {0.f, 1.f, 0.f},
    {0.f, 1.f, 0.f}, {0.f, 1.f, 0.f}, {0.f, 1.f, 0.f}, {0.f, 1.f, 0.f},
    {0.f, 1.f, 0.f}, {0.f, 1.f, 0.f}, {0.f, 1.f, 0.f}, {0.f, 1.f, 0.f},
    {0.f, 1.f, 0.f}, {0.f, 1.f, 0.f}, {0.f, 1.f, 0.f}, {0.f, 1.f, 0.f},
    {0.f, 1.f, 0.f}, {0.f, 1.f, 0.f}, {0.f, 1.f, 0.f}, {0.f, 1.f, 0.f},
    {0.f, 1.f, 0.f}, {0.f, 1.f, 0.f}, {0.f, 1.f, 0.f}, {0.f, 1.f, 0.f},
    {0.f, 1.f, 0.f}, {0.f, 1.f, 0.f}, {0.f, 1.f, 0.f}, {0.f, 1.f, 0.f},
    {0.f, 1.f, 0.f}, {0.f, 1.f, 0.f}, {0.f, 1.f, 0.f}, {0.f, 1.f, 0.f},
    {0.f, 1.f, 0.f}, {0.f, 1.f, 0.f}, {0.f, 1.f, 0.f}, {0.f, 1.f, 0.f},
    {0.f, 1.f, 0.f}, {0.f, 1.f, 0.f}, {0.f, 1.f, 0.f}, {0.f, 1.f, 0.f},
    {0.f, 1.f, 0.f}, {0.f, 1.f, 0.f}, {0.f, 1.f, 0.f}, {0.f, 1.f, 0.f},
    {0.f, 1.f, 0.f}, {0.f, 1.f, 0.f}, {0.f, 1.f, 0.f}, {0.f, 1.f, 0.f}};

const float kDefaultUVsArray[][2] = {
    {0.f, 0.f}, {0.f, 0.f}, {0.f, 0.f}, {0.f, 0.f}, {0.f, 0.f}, {0.f, 0.f},
    {0.f, 0.f}, {0.f, 0.f}, {0.f, 0.f}, {0.f, 0.f}, {0.f, 0.f}, {0.f, 0.f},
    {0.f, 0.f}, {0.f, 0.f}, {0.f, 0.f}, {0.f, 0.f}, {0.f, 0.f}, {0.f, 0.f},
    {0.f, 0.f}, {0.f, 0.f}, {0.f, 0.f}, {0.f, 0.f}, {0.f, 0.f}, {0.f, 0.f},
    {0.f, 0.f}, {0.f, 0.f}, {0.f, 0.f}, {0.f, 0.f}, {0.f, 0.f}, {0.f, 0.f},
    {0.f, 0.f}, {0.f, 0.f}, {0.f, 0.f}, {0.f, 0.f}, {0.f, 0.f}, {0.f, 0.f},
    {0.f, 0.f}, {0.f, 0.f}, {0.f, 0.f}, {0.f, 0.f}, {0.f, 0.f}, {0.f, 0.f},
    {0.f, 0.f}, {0.f, 0.f}, {0.f, 0.f}, {0.f, 0.f}, {0.f, 0.f}, {0.f, 0.f},
    {0.f, 0.f}, {0.f, 0.f}, {0.f, 0.f}, {0.f, 0.f}, {0.f, 0.f}, {0.f, 0.f},
    {0.f, 0.f}, {0.f, 0.f}, {0.f, 0.f}, {0.f, 0.f}, {0.f, 0.f}, {0.f, 0.f},
    {0.f, 0.f}, {0.f, 0.f}, {0.f, 0.f}, {0.f, 0.f}};
}  // namespace

bool RendererImpl::DrawMesh(const Mesh& _mesh,
                            const ozz::math::Float4x4& _transform,
                            const Options& _options) {
  if (_options.wireframe) {
#ifndef EMSCRIPTEN
    GL(PolygonMode(GL_FRONT_AND_BACK, GL_LINE));
#endif  // EMSCRIPTEN
  }

  const int vertex_count = _mesh.vertex_count();
  const GLsizei positions_offset = 0;
  const GLsizei positions_stride =
      sizeof(float) * ozz::sample::Mesh::Part::kPositionsCpnts;
  const GLsizei positions_size = vertex_count * positions_stride;
  const GLsizei normals_offset = positions_offset + positions_size;
  const GLsizei normals_stride =
      sizeof(float) * ozz::sample::Mesh::Part::kNormalsCpnts;
  const GLsizei normals_size = vertex_count * normals_stride;
  // Colors will be filled with white if _options.colors is false.
  const GLsizei colors_offset = normals_offset + normals_size;
  const GLsizei colors_stride =
      sizeof(uint8_t) * ozz::sample::Mesh::Part::kColorsCpnts;
  const GLsizei colors_size = vertex_count * colors_stride;
  // Uvs are skipped if _options.texture is false.
  const GLsizei uvs_offset = colors_offset + colors_size;
  const GLsizei uvs_stride =
      _options.texture ? sizeof(float) * ozz::sample::Mesh::Part::kUVsCpnts : 0;
  const GLsizei uvs_size = vertex_count * uvs_stride;

  // Reallocate vertex buffer.
  const GLsizei vbo_size =
      positions_size + normals_size + colors_size + uvs_size;
  GL(BindBuffer(GL_ARRAY_BUFFER, dynamic_array_bo_));
  GL(BufferData(GL_ARRAY_BUFFER, vbo_size, nullptr, GL_STREAM_DRAW));

  // Iterate mesh parts and fills vbo.
  size_t vertex_offset = 0;
  for (size_t i = 0; i < _mesh.parts.size(); ++i) {
    const Mesh::Part& part = _mesh.parts[i];
    const size_t part_vertex_count = part.vertex_count();

    // Handles positions.
    GL(BufferSubData(
        GL_ARRAY_BUFFER, positions_offset + vertex_offset * positions_stride,
        part_vertex_count * positions_stride, array_begin(part.positions)));

    // Handles normals.
    const size_t part_normal_count =
        part.normals.size() / ozz::sample::Mesh::Part::kNormalsCpnts;
    if (part_vertex_count == part_normal_count) {
      // Optimal path used when the right number of normals is provided.
      GL(BufferSubData(
          GL_ARRAY_BUFFER, normals_offset + vertex_offset * normals_stride,
          part_normal_count * normals_stride, array_begin(part.normals)));
    } else {
      // Un-optimal path used when the right number of normals is not provided.
      static_assert(sizeof(kDefaultNormalsArray[0]) == normals_stride,
                    "Stride mismatch");
      for (size_t j = 0; j < part_vertex_count;
           j += OZZ_ARRAY_SIZE(kDefaultNormalsArray)) {
        const size_t this_loop_count = math::Min(
            OZZ_ARRAY_SIZE(kDefaultNormalsArray), part_vertex_count - j);
        GL(BufferSubData(GL_ARRAY_BUFFER,
                         normals_offset + (vertex_offset + j) * normals_stride,
                         normals_stride * this_loop_count,
                         kDefaultNormalsArray));
      }
    }

    // Handles colors.
    const size_t part_color_count =
        part.colors.size() / ozz::sample::Mesh::Part::kColorsCpnts;
    if (_options.colors && part_vertex_count == part_color_count) {
      // Optimal path used when the right number of colors is provided.
      GL(BufferSubData(
          GL_ARRAY_BUFFER, colors_offset + vertex_offset * colors_stride,
          part_color_count * colors_stride, array_begin(part.colors)));
    } else {
      // Un-optimal path used when the right number of colors is not provided.
      static_assert(sizeof(kDefaultColorsArray[0]) == colors_stride,
                    "Stride mismatch");
      for (size_t j = 0; j < part_vertex_count;
           j += OZZ_ARRAY_SIZE(kDefaultColorsArray)) {
        const size_t this_loop_count = math::Min(
            OZZ_ARRAY_SIZE(kDefaultColorsArray), part_vertex_count - j);
        GL(BufferSubData(GL_ARRAY_BUFFER,
                         colors_offset + (vertex_offset + j) * colors_stride,
                         colors_stride * this_loop_count, kDefaultColorsArray));
      }
    }

    // Handles uvs.
    if (_options.texture) {
      const size_t part_uvs_count =
          part.uvs.size() / ozz::sample::Mesh::Part::kUVsCpnts;
      if (part_vertex_count == part_uvs_count) {
        // Optimal path used when the right number of uvs is provided.
        GL(BufferSubData(GL_ARRAY_BUFFER,
                         uvs_offset + vertex_offset * uvs_stride,
                         part_uvs_count * uvs_stride, array_begin(part.uvs)));
      } else {
        // Un-optimal path used when the right number of uvs is not provided.
        assert(sizeof(kDefaultUVsArray[0]) == uvs_stride);
        for (size_t j = 0; j < part_vertex_count;
             j += OZZ_ARRAY_SIZE(kDefaultUVsArray)) {
          const size_t this_loop_count = math::Min(
              OZZ_ARRAY_SIZE(kDefaultUVsArray), part_vertex_count - j);
          GL(BufferSubData(GL_ARRAY_BUFFER,
                           uvs_offset + (vertex_offset + j) * uvs_stride,
                           uvs_stride * this_loop_count, kDefaultUVsArray));
        }
      }
    }

    // Computes next loop offset.
    vertex_offset += part_vertex_count;
  }

  // Binds shader with this array buffer, depending on rendering options.
  Shader* shader = nullptr;
  if (_options.texture) {
    ambient_textured_shader->Bind(_transform, camera()->view_proj(),
                                  positions_stride, positions_offset,
                                  normals_stride, normals_offset, colors_stride,
                                  colors_offset, uvs_stride, uvs_offset);
    shader = ambient_textured_shader.get();

    // Binds default texture
    GL(BindTexture(GL_TEXTURE_2D, checkered_texture_));
  } else {
    ambient_shader->Bind(_transform, camera()->view_proj(), positions_stride,
                         positions_offset, normals_stride, normals_offset,
                         colors_stride, colors_offset);
    shader = ambient_shader.get();
  }

  // Maps the index dynamic buffer and update it.
  GL(BindBuffer(GL_ELEMENT_ARRAY_BUFFER, dynamic_index_bo_));
  const Mesh::TriangleIndices& indices = _mesh.triangle_indices;
  GL(BufferData(GL_ELEMENT_ARRAY_BUFFER,
                indices.size() * sizeof(Mesh::TriangleIndices::value_type),
                array_begin(indices), GL_STREAM_DRAW));

  // Draws the mesh.
  static_assert(sizeof(Mesh::TriangleIndices::value_type) == 2,
                "Expects 2 bytes indices.");
  GL(DrawElements(GL_TRIANGLES, static_cast<GLsizei>(indices.size()),
                  GL_UNSIGNED_SHORT, 0));

  // Unbinds.
  GL(BindBuffer(GL_ARRAY_BUFFER, 0));
  GL(BindTexture(GL_TEXTURE_2D, 0));
  GL(BindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0));
  shader->Unbind();

  if (_options.wireframe) {
#ifndef EMSCRIPTEN
    GL(PolygonMode(GL_FRONT_AND_BACK, GL_FILL));
#endif  // EMSCRIPTEN
  }

  // Renders debug normals.
  if (_options.normals) {
    for (size_t i = 0; i < _mesh.parts.size(); ++i) {
      const Mesh::Part& part = _mesh.parts[i];
      DrawVectors(make_span(part.positions),
                  ozz::sample::Mesh::Part::kPositionsCpnts * sizeof(float),
                  make_span(part.normals),
                  ozz::sample::Mesh::Part::kNormalsCpnts * sizeof(float),
                  part.vertex_count(), .03f, ozz::sample::kGreen, _transform);
    }
  }

  // Renders debug tangents.
  if (_options.tangents) {
    for (size_t i = 0; i < _mesh.parts.size(); ++i) {
      const Mesh::Part& part = _mesh.parts[i];
      if (part.normals.size() != 0) {
        DrawVectors(make_span(part.positions),
                    ozz::sample::Mesh::Part::kPositionsCpnts * sizeof(float),
                    make_span(part.tangents),
                    ozz::sample::Mesh::Part::kTangentsCpnts * sizeof(float),
                    part.vertex_count(), .03f, ozz::sample::kRed, _transform);
      }
    }
  }

  // Renders debug binormals.
  if (_options.binormals) {
    for (size_t i = 0; i < _mesh.parts.size(); ++i) {
      const Mesh::Part& part = _mesh.parts[i];
      if (part.normals.size() != 0 && part.tangents.size() != 0) {
        DrawBinormals(
            make_span(part.positions),
            ozz::sample::Mesh::Part::kPositionsCpnts * sizeof(float),
            make_span(part.normals),
            ozz::sample::Mesh::Part::kNormalsCpnts * sizeof(float),
            make_span(part.tangents),
            ozz::sample::Mesh::Part::kTangentsCpnts * sizeof(float),
            ozz::span<const float>(&part.tangents[3], part.tangents.size()),
            ozz::sample::Mesh::Part::kTangentsCpnts * sizeof(float),
            part.vertex_count(), .03f, ozz::sample::kBlue, _transform);
      }
    }
  }

  return true;
}

bool RendererImpl::DrawSkinnedMesh(
    const Mesh& _mesh, const span<math::Float4x4> _skinning_matrices,
    const ozz::math::Float4x4& _transform, const Options& _options) {
  // Forward to DrawMesh function is skinning is disabled.
  if (_options.skip_skinning) {
    return DrawMesh(_mesh, _transform, _options);
  }

  if (_options.wireframe) {
#ifndef EMSCRIPTEN
    GL(PolygonMode(GL_FRONT_AND_BACK, GL_LINE));
#endif  // EMSCRIPTEN
  }

  const int vertex_count = _mesh.vertex_count();

  // Positions and normals are interleaved to improve caching while executing
  // skinning job.
  const GLsizei positions_offset = 0;
  const GLsizei normals_offset = sizeof(float) * 3;
  const GLsizei tangents_offset = sizeof(float) * 6;
  const GLsizei positions_stride = sizeof(float) * 9;
  const GLsizei normals_stride = positions_stride;
  const GLsizei tangents_stride = positions_stride;
  const GLsizei skinned_data_size = vertex_count * positions_stride;

  // Colors and uvs are contiguous. They aren't transformed, so they can be
  // directly copied from source mesh which is non-interleaved as-well.
  // Colors will be filled with white if _options.colors is false.
  // UVs will be skipped if _options.textured is false.
  const GLsizei colors_offset = skinned_data_size;
  const GLsizei colors_stride = sizeof(uint8_t) * 4;
  const GLsizei colors_size = vertex_count * colors_stride;
  const GLsizei uvs_offset = colors_offset + colors_size;
  const GLsizei uvs_stride = _options.texture ? sizeof(float) * 2 : 0;
  const GLsizei uvs_size = vertex_count * uvs_stride;
  const GLsizei fixed_data_size = colors_size + uvs_size;

  // Reallocate vertex buffer.
  const GLsizei vbo_size = skinned_data_size + fixed_data_size;
  void* vbo_map = scratch_buffer_.Resize(vbo_size);

  // Iterate mesh parts and fills vbo.
  // Runs a skinning job per mesh part. Triangle indices are shared
  // across parts.
  size_t processed_vertex_count = 0;
  for (size_t i = 0; i < _mesh.parts.size(); ++i) {
    const ozz::sample::Mesh::Part& part = _mesh.parts[i];

    // Skip this iteration if no vertex.
    const size_t part_vertex_count = part.positions.size() / 3;
    if (part_vertex_count == 0) {
      continue;
    }

    // Fills the job.
    ozz::geometry::SkinningJob skinning_job;
    skinning_job.vertex_count = static_cast<int>(part_vertex_count);
    const int part_influences_count = part.influences_count();

    // Clamps joints influence count according to the option.
    skinning_job.influences_count = part_influences_count;

    // Setup skinning matrices, that came from the animation stage before being
    // multiplied by inverse model-space bind-pose.
    skinning_job.joint_matrices = _skinning_matrices;

    // Setup joint's indices.
    skinning_job.joint_indices = make_span(part.joint_indices);
    skinning_job.joint_indices_stride =
        sizeof(uint16_t) * part_influences_count;

    // Setup joint's weights.
    if (part_influences_count > 1) {
      skinning_job.joint_weights = make_span(part.joint_weights);
      skinning_job.joint_weights_stride =
          sizeof(float) * (part_influences_count - 1);
    }

    // Setup input positions, coming from the loaded mesh.
    skinning_job.in_positions = make_span(part.positions);
    skinning_job.in_positions_stride =
        sizeof(float) * ozz::sample::Mesh::Part::kPositionsCpnts;

    // Setup output positions, coming from the rendering output mesh buffers.
    // We need to offset the buffer every loop.
    float* out_positions_begin = reinterpret_cast<float*>(ozz::PointerStride(
        vbo_map, positions_offset + processed_vertex_count * positions_stride));
    float* out_positions_end = ozz::PointerStride(
        out_positions_begin, part_vertex_count * positions_stride);
    skinning_job.out_positions = {out_positions_begin, out_positions_end};
    skinning_job.out_positions_stride = positions_stride;

    // Setup normals if input are provided.
    float* out_normal_begin = reinterpret_cast<float*>(ozz::PointerStride(
        vbo_map, normals_offset + processed_vertex_count * normals_stride));
    float* out_normal_end = ozz::PointerStride(
        out_normal_begin, part_vertex_count * normals_stride);

    if (part.normals.size() / ozz::sample::Mesh::Part::kNormalsCpnts ==
        part_vertex_count) {
      // Setup input normals, coming from the loaded mesh.
      skinning_job.in_normals = make_span(part.normals);
      skinning_job.in_normals_stride =
          sizeof(float) * ozz::sample::Mesh::Part::kNormalsCpnts;

      // Setup output normals, coming from the rendering output mesh buffers.
      // We need to offset the buffer every loop.
      skinning_job.out_normals = {out_normal_begin, out_normal_end};
      skinning_job.out_normals_stride = normals_stride;
    } else {
      // Fills output with default normals.
      for (float* normal = out_normal_begin; normal < out_normal_end;
           normal = ozz::PointerStride(normal, normals_stride)) {
        normal[0] = 0.f;
        normal[1] = 1.f;
        normal[2] = 0.f;
      }
    }

    // Setup tangents if input are provided.
    float* out_tangent_begin = reinterpret_cast<float*>(ozz::PointerStride(
        vbo_map, tangents_offset + processed_vertex_count * tangents_stride));
    float* out_tangent_end = ozz::PointerStride(
        out_tangent_begin, part_vertex_count * tangents_stride);

    if (part.tangents.size() / ozz::sample::Mesh::Part::kTangentsCpnts ==
        part_vertex_count) {
      // Setup input tangents, coming from the loaded mesh.
      skinning_job.in_tangents = make_span(part.tangents);
      skinning_job.in_tangents_stride =
          sizeof(float) * ozz::sample::Mesh::Part::kTangentsCpnts;

      // Setup output tangents, coming from the rendering output mesh buffers.
      // We need to offset the buffer every loop.
      skinning_job.out_tangents = {out_tangent_begin, out_tangent_end};
      skinning_job.out_tangents_stride = tangents_stride;
    } else {
      // Fills output with default tangents.
      for (float* tangent = out_tangent_begin; tangent < out_tangent_end;
           tangent = ozz::PointerStride(tangent, tangents_stride)) {
        tangent[0] = 1.f;
        tangent[1] = 0.f;
        tangent[2] = 0.f;
      }
    }

    // Execute the job, which should succeed unless a parameter is invalid.
    if (!skinning_job.Run()) {
      return false;
    }

    // Renders debug normals.
    if (_options.normals && skinning_job.out_normals.size() > 0) {
      DrawVectors(skinning_job.out_positions, skinning_job.out_positions_stride,
                  skinning_job.out_normals, skinning_job.out_normals_stride,
                  skinning_job.vertex_count, .03f, ozz::sample::kGreen,
                  _transform);
    }

    // Renders debug tangents.
    if (_options.tangents && skinning_job.out_tangents.size() > 0) {
      DrawVectors(skinning_job.out_positions, skinning_job.out_positions_stride,
                  skinning_job.out_tangents, skinning_job.out_tangents_stride,
                  skinning_job.vertex_count, .03f, ozz::sample::kRed,
                  _transform);
    }

    // Renders debug binormals.
    if (_options.binormals && skinning_job.out_normals.size() > 0 &&
        skinning_job.out_tangents.size() > 0) {
      DrawBinormals(skinning_job.out_positions,
                    skinning_job.out_positions_stride, skinning_job.out_normals,
                    skinning_job.out_normals_stride, skinning_job.out_tangents,
                    skinning_job.out_tangents_stride,
                    ozz::span<const float>(skinning_job.in_tangents.begin() + 3,
                                           skinning_job.in_tangents.end() + 3),
                    skinning_job.in_tangents_stride, skinning_job.vertex_count,
                    .03f, ozz::sample::kBlue, _transform);
    }

    // Handles colors which aren't affected by skinning.
    if (_options.colors &&
        part_vertex_count ==
            part.colors.size() / ozz::sample::Mesh::Part::kColorsCpnts) {
      // Optimal path used when the right number of colors is provided.
      memcpy(
          ozz::PointerStride(
              vbo_map, colors_offset + processed_vertex_count * colors_stride),
          array_begin(part.colors), part_vertex_count * colors_stride);
    } else {
      // Un-optimal path used when the right number of colors is not provided.
      static_assert(sizeof(kDefaultColorsArray[0]) == colors_stride,
                    "Stride mismatch");

      for (size_t j = 0; j < part_vertex_count;
           j += OZZ_ARRAY_SIZE(kDefaultColorsArray)) {
        const size_t this_loop_count = math::Min(
            OZZ_ARRAY_SIZE(kDefaultColorsArray), part_vertex_count - j);
        memcpy(ozz::PointerStride(
                   vbo_map, colors_offset +
                                (processed_vertex_count + j) * colors_stride),
               kDefaultColorsArray, colors_stride * this_loop_count);
      }
    }

    // Copies uvs which aren't affected by skinning.
    if (_options.texture) {
      if (part_vertex_count ==
          part.uvs.size() / ozz::sample::Mesh::Part::kUVsCpnts) {
        // Optimal path used when the right number of uvs is provided.
        memcpy(ozz::PointerStride(
                   vbo_map, uvs_offset + processed_vertex_count * uvs_stride),
               array_begin(part.uvs), part_vertex_count * uvs_stride);
      } else {
        // Un-optimal path used when the right number of uvs is not provided.
        assert(sizeof(kDefaultUVsArray[0]) == uvs_stride);
        for (size_t j = 0; j < part_vertex_count;
             j += OZZ_ARRAY_SIZE(kDefaultUVsArray)) {
          const size_t this_loop_count = math::Min(
              OZZ_ARRAY_SIZE(kDefaultUVsArray), part_vertex_count - j);
          memcpy(ozz::PointerStride(
                     vbo_map,
                     uvs_offset + (processed_vertex_count + j) * uvs_stride),
                 kDefaultUVsArray, uvs_stride * this_loop_count);
        }
      }
    }

    // Some more vertices were processed.
    processed_vertex_count += part_vertex_count;
  }

  // Updates dynamic vertex buffer with skinned data.
  GL(BindBuffer(GL_ARRAY_BUFFER, dynamic_array_bo_));
  GL(BufferData(GL_ARRAY_BUFFER, vbo_size, nullptr, GL_STREAM_DRAW));
  GL(BufferSubData(GL_ARRAY_BUFFER, 0, vbo_size, vbo_map));

  // Binds shader with this array buffer, depending on rendering options.
  Shader* shader = nullptr;
  if (_options.texture) {
    ambient_textured_shader->Bind(_transform, camera()->view_proj(),
                                  positions_stride, positions_offset,
                                  normals_stride, normals_offset, colors_stride,
                                  colors_offset, uvs_stride, uvs_offset);
    shader = ambient_textured_shader.get();

    // Binds default texture
    GL(BindTexture(GL_TEXTURE_2D, checkered_texture_));
  } else {
    ambient_shader->Bind(_transform, camera()->view_proj(), positions_stride,
                         positions_offset, normals_stride, normals_offset,
                         colors_stride, colors_offset);
    shader = ambient_shader.get();
  }

  // Maps the index dynamic buffer and update it.
  GL(BindBuffer(GL_ELEMENT_ARRAY_BUFFER, dynamic_index_bo_));
  const Mesh::TriangleIndices& indices = _mesh.triangle_indices;
  GL(BufferData(GL_ELEMENT_ARRAY_BUFFER,
                indices.size() * sizeof(Mesh::TriangleIndices::value_type),
                array_begin(indices), GL_STREAM_DRAW));

  // Draws the mesh.
  static_assert(sizeof(Mesh::TriangleIndices::value_type) == 2,
                "Expects 2 bytes indices.");
  GL(DrawElements(GL_TRIANGLES, static_cast<GLsizei>(indices.size()),
                  GL_UNSIGNED_SHORT, 0));

  // Unbinds.
  GL(BindBuffer(GL_ARRAY_BUFFER, 0));
  GL(BindTexture(GL_TEXTURE_2D, 0));
  GL(BindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0));
  shader->Unbind();

  if (_options.wireframe) {
#ifndef EMSCRIPTEN
    GL(PolygonMode(GL_FRONT_AND_BACK, GL_FILL));
#endif  // EMSCRIPTEN
  }

  return true;
}

// Helper macro used to initialize extension function pointer.
#define OZZ_INIT_GL_EXT_N(_fct, _fct_name, _fct_type, _success)               \
  do {                                                                        \
    _fct = reinterpret_cast<_fct_type>(glfwGetProcAddress(_fct_name));        \
    if (_fct == nullptr) {                                                    \
      log::Err() << "Unable to install " _fct_name " function." << std::endl; \
      _success &= false;                                                      \
    }                                                                         \
                                                                              \
  } while (void(0), 0)

#define OZZ_INIT_GL_EXT(_fct, _fct_type, _success) \
  OZZ_INIT_GL_EXT_N(_fct, #_fct, _fct_type, _success)

bool RendererImpl::InitOpenGLExtensions() {
  bool optional_success = true;
  bool success = true;  // aka mandatory extensions
#ifdef OZZ_GL_VERSION_1_5_EXT
  OZZ_INIT_GL_EXT(glBindBuffer, PFNGLBINDBUFFERPROC, success);
  OZZ_INIT_GL_EXT(glDeleteBuffers, PFNGLDELETEBUFFERSPROC, success);
  OZZ_INIT_GL_EXT(glGenBuffers, PFNGLGENBUFFERSPROC, success);
  OZZ_INIT_GL_EXT(glIsBuffer, PFNGLISBUFFERPROC, success);
  OZZ_INIT_GL_EXT(glBufferData, PFNGLBUFFERDATAPROC, success);
  OZZ_INIT_GL_EXT(glBufferSubData, PFNGLBUFFERSUBDATAPROC, success);
  OZZ_INIT_GL_EXT(glMapBuffer, PFNGLMAPBUFFERPROC, optional_success);
  OZZ_INIT_GL_EXT(glUnmapBuffer, PFNGLUNMAPBUFFERPROC, optional_success);
  OZZ_INIT_GL_EXT(glGetBufferParameteriv, PFNGLGETBUFFERPARAMETERIVPROC,
                  success);
#endif  // OZZ_GL_VERSION_1_5_EXT

#ifdef OZZ_GL_VERSION_2_0_EXT
  OZZ_INIT_GL_EXT(glAttachShader, PFNGLATTACHSHADERPROC, success);
  OZZ_INIT_GL_EXT(glBindAttribLocation, PFNGLBINDATTRIBLOCATIONPROC, success);
  OZZ_INIT_GL_EXT(glCompileShader, PFNGLCOMPILESHADERPROC, success);
  OZZ_INIT_GL_EXT(glCreateProgram, PFNGLCREATEPROGRAMPROC, success);
  OZZ_INIT_GL_EXT(glCreateShader, PFNGLCREATESHADERPROC, success);
  OZZ_INIT_GL_EXT(glDeleteProgram, PFNGLDELETEPROGRAMPROC, success);
  OZZ_INIT_GL_EXT(glDeleteShader, PFNGLDELETESHADERPROC, success);
  OZZ_INIT_GL_EXT(glDetachShader, PFNGLDETACHSHADERPROC, success);
  OZZ_INIT_GL_EXT(glDisableVertexAttribArray, PFNGLDISABLEVERTEXATTRIBARRAYPROC,
                  success);
  OZZ_INIT_GL_EXT(glEnableVertexAttribArray, PFNGLENABLEVERTEXATTRIBARRAYPROC,
                  success);
  OZZ_INIT_GL_EXT(glGetActiveAttrib, PFNGLGETACTIVEATTRIBPROC, success);
  OZZ_INIT_GL_EXT(glGetActiveUniform, PFNGLGETACTIVEUNIFORMPROC, success);
  OZZ_INIT_GL_EXT(glGetAttachedShaders, PFNGLGETATTACHEDSHADERSPROC, success);
  OZZ_INIT_GL_EXT(glGetAttribLocation, PFNGLGETATTRIBLOCATIONPROC, success);
  OZZ_INIT_GL_EXT(glGetProgramiv, PFNGLGETPROGRAMIVPROC, success);
  OZZ_INIT_GL_EXT(glGetProgramInfoLog, PFNGLGETPROGRAMINFOLOGPROC, success);
  OZZ_INIT_GL_EXT(glGetShaderiv, PFNGLGETSHADERIVPROC, success);
  OZZ_INIT_GL_EXT(glGetShaderInfoLog, PFNGLGETSHADERINFOLOGPROC, success);
  OZZ_INIT_GL_EXT(glGetShaderSource, PFNGLGETSHADERSOURCEPROC, success);
  OZZ_INIT_GL_EXT(glGetUniformLocation, PFNGLGETUNIFORMLOCATIONPROC, success);
  OZZ_INIT_GL_EXT(glGetUniformfv, PFNGLGETUNIFORMFVPROC, success);
  OZZ_INIT_GL_EXT(glGetUniformiv, PFNGLGETUNIFORMIVPROC, success);
  OZZ_INIT_GL_EXT(glGetVertexAttribfv, PFNGLGETVERTEXATTRIBFVPROC, success);
  OZZ_INIT_GL_EXT(glGetVertexAttribiv, PFNGLGETVERTEXATTRIBIVPROC, success);
  OZZ_INIT_GL_EXT(glGetVertexAttribPointerv, PFNGLGETVERTEXATTRIBPOINTERVPROC,
                  success);
  OZZ_INIT_GL_EXT(glIsProgram, PFNGLISPROGRAMPROC, success);
  OZZ_INIT_GL_EXT(glIsShader, PFNGLISSHADERPROC, success);
  OZZ_INIT_GL_EXT(glLinkProgram, PFNGLLINKPROGRAMPROC, success);
  OZZ_INIT_GL_EXT(glShaderSource, PFNGLSHADERSOURCEPROC, success);
  OZZ_INIT_GL_EXT(glUseProgram, PFNGLUSEPROGRAMPROC, success);
  OZZ_INIT_GL_EXT(glUniform1f, PFNGLUNIFORM1FPROC, success);
  OZZ_INIT_GL_EXT(glUniform2f, PFNGLUNIFORM2FPROC, success);
  OZZ_INIT_GL_EXT(glUniform3f, PFNGLUNIFORM3FPROC, success);
  OZZ_INIT_GL_EXT(glUniform4f, PFNGLUNIFORM4FPROC, success);
  OZZ_INIT_GL_EXT(glUniform1i, PFNGLUNIFORM1IPROC, success);
  OZZ_INIT_GL_EXT(glUniform2i, PFNGLUNIFORM2IPROC, success);
  OZZ_INIT_GL_EXT(glUniform3i, PFNGLUNIFORM3IPROC, success);
  OZZ_INIT_GL_EXT(glUniform4i, PFNGLUNIFORM4IPROC, success);
  OZZ_INIT_GL_EXT(glUniform1fv, PFNGLUNIFORM1FVPROC, success);
  OZZ_INIT_GL_EXT(glUniform2fv, PFNGLUNIFORM2FVPROC, success);
  OZZ_INIT_GL_EXT(glUniform3fv, PFNGLUNIFORM3FVPROC, success);
  OZZ_INIT_GL_EXT(glUniform4fv, PFNGLUNIFORM4FVPROC, success);
  OZZ_INIT_GL_EXT(glUniformMatrix2fv, PFNGLUNIFORMMATRIX2FVPROC, success);
  OZZ_INIT_GL_EXT(glUniformMatrix3fv, PFNGLUNIFORMMATRIX3FVPROC, success);
  OZZ_INIT_GL_EXT(glUniformMatrix4fv, PFNGLUNIFORMMATRIX4FVPROC, success);
  OZZ_INIT_GL_EXT(glValidateProgram, PFNGLVALIDATEPROGRAMPROC, success);
  OZZ_INIT_GL_EXT(glVertexAttrib1f, PFNGLVERTEXATTRIB1FPROC, success);
  OZZ_INIT_GL_EXT(glVertexAttrib1fv, PFNGLVERTEXATTRIB1FVPROC, success);
  OZZ_INIT_GL_EXT(glVertexAttrib2f, PFNGLVERTEXATTRIB2FPROC, success);
  OZZ_INIT_GL_EXT(glVertexAttrib2fv, PFNGLVERTEXATTRIB2FVPROC, success);
  OZZ_INIT_GL_EXT(glVertexAttrib3f, PFNGLVERTEXATTRIB3FPROC, success);
  OZZ_INIT_GL_EXT(glVertexAttrib3fv, PFNGLVERTEXATTRIB3FVPROC, success);
  OZZ_INIT_GL_EXT(glVertexAttrib4f, PFNGLVERTEXATTRIB4FPROC, success);
  OZZ_INIT_GL_EXT(glVertexAttrib4fv, PFNGLVERTEXATTRIB4FVPROC, success);
  OZZ_INIT_GL_EXT(glVertexAttribPointer, PFNGLVERTEXATTRIBPOINTERPROC, success);
#endif  // OZZ_GL_VERSION_2_0_EXT
  if (!success) {
    log::Err() << "Failed to initialize all mandatory GL extensions."
               << std::endl;
    return false;
  }
  if (!optional_success) {
    log::Log() << "Failed to initialize some optional GL extensions."
               << std::endl;
  }

  GL_ARB_instanced_arrays_supported =
      glfwExtensionSupported("GL_ARB_instanced_arrays") != 0;
  if (GL_ARB_instanced_arrays_supported) {
    log::Log() << "Optional GL_ARB_instanced_arrays extensions found."
               << std::endl;
    success = true;
    OZZ_INIT_GL_EXT_N(glVertexAttribDivisor_, "glVertexAttribDivisorARB",
                      PFNGLVERTEXATTRIBDIVISORARBPROC, success);
    OZZ_INIT_GL_EXT_N(glDrawArraysInstanced_, "glDrawArraysInstancedARB",
                      PFNGLDRAWARRAYSINSTANCEDARBPROC, success);
    OZZ_INIT_GL_EXT_N(glDrawElementsInstanced_, "glDrawElementsInstancedARB",
                      PFNGLDRAWELEMENTSINSTANCEDARBPROC, success);
    if (!success) {
      log::Err()
          << "Failed to setup GL_ARB_instanced_arrays, feature is disabled."
          << std::endl;
      GL_ARB_instanced_arrays_supported = false;
    }
  } else {
    log::Log() << "Optional GL_ARB_instanced_arrays extensions not found."
               << std::endl;
  }
  return true;
}

RendererImpl::ScratchBuffer::ScratchBuffer() : buffer_(nullptr), size_(0) {}

RendererImpl::ScratchBuffer::~ScratchBuffer() {
  memory::default_allocator()->Deallocate(buffer_);
}

void* RendererImpl::ScratchBuffer::Resize(size_t _size) {
  if (_size > size_) {
    size_ = _size;
    memory::default_allocator()->Deallocate(buffer_);
    buffer_ = memory::default_allocator()->Allocate(_size, 16);
  }
  return buffer_;
}
}  // namespace internal
}  // namespace sample
}  // namespace ozz

// Helper macro used to declare extension function pointer.
#define OZZ_DECL_GL_EXT(_fct, _fct_type) _fct_type _fct = nullptr

#ifdef OZZ_GL_VERSION_1_5_EXT
OZZ_DECL_GL_EXT(glBindBuffer, PFNGLBINDBUFFERPROC);
OZZ_DECL_GL_EXT(glDeleteBuffers, PFNGLDELETEBUFFERSPROC);
OZZ_DECL_GL_EXT(glGenBuffers, PFNGLGENBUFFERSPROC);
OZZ_DECL_GL_EXT(glIsBuffer, PFNGLISBUFFERPROC);
OZZ_DECL_GL_EXT(glBufferData, PFNGLBUFFERDATAPROC);
OZZ_DECL_GL_EXT(glBufferSubData, PFNGLBUFFERSUBDATAPROC);
OZZ_DECL_GL_EXT(glGetBufferSubData, PFNGLGETBUFFERSUBDATAPROC);
OZZ_DECL_GL_EXT(glMapBuffer, PFNGLMAPBUFFERPROC);
OZZ_DECL_GL_EXT(glUnmapBuffer, PFNGLUNMAPBUFFERPROC);
OZZ_DECL_GL_EXT(glGetBufferParameteriv, PFNGLGETBUFFERPARAMETERIVPROC);
OZZ_DECL_GL_EXT(glGetBufferPointerv, PFNGLGETBUFFERPOINTERVPROC);
#endif  // OZZ_GL_VERSION_1_5_EXT

#ifdef OZZ_GL_VERSION_2_0_EXT
OZZ_DECL_GL_EXT(glAttachShader, PFNGLATTACHSHADERPROC);
OZZ_DECL_GL_EXT(glBindAttribLocation, PFNGLBINDATTRIBLOCATIONPROC);
OZZ_DECL_GL_EXT(glCompileShader, PFNGLCOMPILESHADERPROC);
OZZ_DECL_GL_EXT(glCreateProgram, PFNGLCREATEPROGRAMPROC);
OZZ_DECL_GL_EXT(glCreateShader, PFNGLCREATESHADERPROC);
OZZ_DECL_GL_EXT(glDeleteProgram, PFNGLDELETEPROGRAMPROC);
OZZ_DECL_GL_EXT(glDeleteShader, PFNGLDELETESHADERPROC);
OZZ_DECL_GL_EXT(glDetachShader, PFNGLDETACHSHADERPROC);
OZZ_DECL_GL_EXT(glDisableVertexAttribArray, PFNGLDISABLEVERTEXATTRIBARRAYPROC);
OZZ_DECL_GL_EXT(glEnableVertexAttribArray, PFNGLENABLEVERTEXATTRIBARRAYPROC);
OZZ_DECL_GL_EXT(glGetActiveAttrib, PFNGLGETACTIVEATTRIBPROC);
OZZ_DECL_GL_EXT(glGetActiveUniform, PFNGLGETACTIVEUNIFORMPROC);
OZZ_DECL_GL_EXT(glGetAttachedShaders, PFNGLGETATTACHEDSHADERSPROC);
OZZ_DECL_GL_EXT(glGetAttribLocation, PFNGLGETATTRIBLOCATIONPROC);
OZZ_DECL_GL_EXT(glGetProgramiv, PFNGLGETPROGRAMIVPROC);
OZZ_DECL_GL_EXT(glGetProgramInfoLog, PFNGLGETPROGRAMINFOLOGPROC);
OZZ_DECL_GL_EXT(glGetShaderiv, PFNGLGETSHADERIVPROC);
OZZ_DECL_GL_EXT(glGetShaderInfoLog, PFNGLGETSHADERINFOLOGPROC);
OZZ_DECL_GL_EXT(glGetShaderSource, PFNGLGETSHADERSOURCEPROC);
OZZ_DECL_GL_EXT(glGetUniformLocation, PFNGLGETUNIFORMLOCATIONPROC);
OZZ_DECL_GL_EXT(glGetUniformfv, PFNGLGETUNIFORMFVPROC);
OZZ_DECL_GL_EXT(glGetUniformiv, PFNGLGETUNIFORMIVPROC);
OZZ_DECL_GL_EXT(glGetVertexAttribdv, PFNGLGETVERTEXATTRIBDVPROC);
OZZ_DECL_GL_EXT(glGetVertexAttribfv, PFNGLGETVERTEXATTRIBFVPROC);
OZZ_DECL_GL_EXT(glGetVertexAttribiv, PFNGLGETVERTEXATTRIBIVPROC);
OZZ_DECL_GL_EXT(glGetVertexAttribPointerv, PFNGLGETVERTEXATTRIBPOINTERVPROC);
OZZ_DECL_GL_EXT(glIsProgram, PFNGLISPROGRAMPROC);
OZZ_DECL_GL_EXT(glIsShader, PFNGLISSHADERPROC);
OZZ_DECL_GL_EXT(glLinkProgram, PFNGLLINKPROGRAMPROC);
OZZ_DECL_GL_EXT(glShaderSource, PFNGLSHADERSOURCEPROC);
OZZ_DECL_GL_EXT(glUseProgram, PFNGLUSEPROGRAMPROC);
OZZ_DECL_GL_EXT(glUniform1f, PFNGLUNIFORM1FPROC);
OZZ_DECL_GL_EXT(glUniform2f, PFNGLUNIFORM2FPROC);
OZZ_DECL_GL_EXT(glUniform3f, PFNGLUNIFORM3FPROC);
OZZ_DECL_GL_EXT(glUniform4f, PFNGLUNIFORM4FPROC);
OZZ_DECL_GL_EXT(glUniform1i, PFNGLUNIFORM1IPROC);
OZZ_DECL_GL_EXT(glUniform2i, PFNGLUNIFORM2IPROC);
OZZ_DECL_GL_EXT(glUniform3i, PFNGLUNIFORM3IPROC);
OZZ_DECL_GL_EXT(glUniform4i, PFNGLUNIFORM4IPROC);
OZZ_DECL_GL_EXT(glUniform1fv, PFNGLUNIFORM1FVPROC);
OZZ_DECL_GL_EXT(glUniform2fv, PFNGLUNIFORM2FVPROC);
OZZ_DECL_GL_EXT(glUniform3fv, PFNGLUNIFORM3FVPROC);
OZZ_DECL_GL_EXT(glUniform4fv, PFNGLUNIFORM4FVPROC);
OZZ_DECL_GL_EXT(glUniformMatrix2fv, PFNGLUNIFORMMATRIX2FVPROC);
OZZ_DECL_GL_EXT(glUniformMatrix3fv, PFNGLUNIFORMMATRIX3FVPROC);
OZZ_DECL_GL_EXT(glUniformMatrix4fv, PFNGLUNIFORMMATRIX4FVPROC);
OZZ_DECL_GL_EXT(glValidateProgram, PFNGLVALIDATEPROGRAMPROC);
OZZ_DECL_GL_EXT(glVertexAttrib1f, PFNGLVERTEXATTRIB1FPROC);
OZZ_DECL_GL_EXT(glVertexAttrib1fv, PFNGLVERTEXATTRIB1FVPROC);
OZZ_DECL_GL_EXT(glVertexAttrib2f, PFNGLVERTEXATTRIB2FPROC);
OZZ_DECL_GL_EXT(glVertexAttrib2fv, PFNGLVERTEXATTRIB2FVPROC);
OZZ_DECL_GL_EXT(glVertexAttrib3f, PFNGLVERTEXATTRIB3FPROC);
OZZ_DECL_GL_EXT(glVertexAttrib3fv, PFNGLVERTEXATTRIB3FVPROC);
OZZ_DECL_GL_EXT(glVertexAttrib4f, PFNGLVERTEXATTRIB4FPROC);
OZZ_DECL_GL_EXT(glVertexAttrib4fv, PFNGLVERTEXATTRIB4FVPROC);
OZZ_DECL_GL_EXT(glVertexAttribPointer, PFNGLVERTEXATTRIBPOINTERPROC);
#endif  // OZZ_GL_VERSION_2_0_EXT

bool GL_ARB_instanced_arrays_supported = false;
OZZ_DECL_GL_EXT(glVertexAttribDivisor_, PFNGLVERTEXATTRIBDIVISORARBPROC);
OZZ_DECL_GL_EXT(glDrawArraysInstanced_, PFNGLDRAWARRAYSINSTANCEDARBPROC);
OZZ_DECL_GL_EXT(glDrawElementsInstanced_, PFNGLDRAWELEMENTSINSTANCEDARBPROC);
