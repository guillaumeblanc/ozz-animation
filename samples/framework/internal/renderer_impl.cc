//----------------------------------------------------------------------------//
//                                                                            //
// ozz-animation is hosted at http://github.com/guillaumeblanc/ozz-animation  //
// and distributed under the MIT License (MIT).                               //
//                                                                            //
// Copyright (c) 2015 Guillaume Blanc                                         //
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

#include "ozz/animation/runtime/local_to_model_job.h"
#include "ozz/animation/runtime/skeleton.h"

#include "ozz/geometry/runtime/skinning_job.h"

#include "ozz/base/log.h"

#include "ozz/base/platform.h"

#include "ozz/base/maths/math_ex.h"
#include "ozz/base/maths/simd_math.h"
#include "ozz/base/maths/vec_float.h"
#include "ozz/base/maths/box.h"

#include "ozz/base/memory/allocator.h"

#include "framework/mesh.h"

#include "camera.h"
#include "immediate.h"
#include "shader.h"

namespace ozz {
namespace sample {
namespace internal {

namespace {
// A vertex made of positions and normals.
struct VertexPNC {
  math::Float3 pos;
  math::Float3 normal;
  Renderer::Color color;
};
}  // namespace

RendererImpl::Model::Model()
    : vbo(0),
      mode(GL_POINTS),
      count(0),
      shader(NULL) {
}

RendererImpl::Model::~Model() {
  if (vbo) {
    GL(DeleteBuffers(1, &vbo));
    vbo = 0;
  }
  if (shader) {
    memory::default_allocator()->Delete(shader);
    shader = NULL;
  }
}

RendererImpl::RendererImpl(Camera* _camera)
    : camera_(_camera),
      dynamic_array_bo_(0),
      dynamic_index_bo_(0),
      immediate_(NULL),
      mesh_shader_(NULL) {
}

RendererImpl::~RendererImpl() {
  memory::Allocator* allocator = memory::default_allocator();

  allocator->Deallocate(prealloc_models_);

  if(dynamic_array_bo_) {
    GL(DeleteBuffers(1, &dynamic_array_bo_));
    dynamic_array_bo_ = 0;
  }

  if(dynamic_index_bo_) {
    GL(DeleteBuffers(1, &dynamic_index_bo_));
    dynamic_index_bo_ = 0;
  }

  allocator->Delete(immediate_);
  immediate_ = NULL;

  allocator->Delete(mesh_shader_);
  mesh_shader_ = NULL;
}

bool RendererImpl::Initialize() {
  if (!InitOpenGLExtensions()) {
    return false;
  }
  if (!InitPostureRendering()) {
    return false;
  }

  // Builds the dynamic vbo
  GL(GenBuffers(1, &dynamic_array_bo_));
  GL(GenBuffers(1, &dynamic_index_bo_));

  // Allocate immediate mode renderer;
  immediate_ = memory::default_allocator()->New<GlImmediateRenderer>(this);
  if (!immediate_->Initialize()) {
    return false;
  }

  // Instantiate mesh rendering shader.
  mesh_shader_ = AmbientShader::Build();
  if (!mesh_shader_) {
    return false;
  }

  return true;
}

void RendererImpl::DrawAxes(const ozz::math::Float4x4& _transform) {

  GlImmediatePC im(immediate_renderer(), GL_LINES,_transform);
  GlImmediatePC::Vertex v = {{0.f, 0.f, 0.f}, {0, 0, 0, 0xff}};

  // X axis (green).
  v.pos[0] = 0.f; v.pos[1] = 0.f; v.pos[2] = 0.f;
  v.rgba[0] = 0xff; v.rgba[1] = 0; v.rgba[2] = 0;
  im.PushVertex(v);
  v.pos[0] = 1.f;
  im.PushVertex(v);

  // Y axis (green).
  v.pos[0] = 0.f; v.pos[1] = 0.f; v.pos[2] = 0.f;
  v.rgba[0] = 0; v.rgba[1] = 0xff; v.rgba[2] = 0;
  im.PushVertex(v);
  v.pos[1] = 1.f;
  im.PushVertex(v);

  // Z axis (green).
  v.pos[0] = 0.f; v.pos[1] = 0.f; v.pos[2] = 0.f;
  v.rgba[0] = 0; v.rgba[1] = 0; v.rgba[2] = 0xff;
  im.PushVertex(v);
  v.pos[2] = 1.f;
  im.PushVertex(v);
}

void RendererImpl::DrawGrid(int _cell_count, float _cell_size) {
  const float extent = _cell_count * _cell_size;
  const float half_extent = extent * 0.5f;
  const ozz::math::Float3 corner(-half_extent, 0, -half_extent);

  GL(Enable(GL_BLEND));
  GL(BlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));
  GL(Disable(GL_CULL_FACE));
  {
    GlImmediatePC im(immediate_renderer(),
                     GL_TRIANGLE_STRIP,
                     ozz::math::Float4x4::identity());
    GlImmediatePC::Vertex v = {{0.f, 0.f, 0.f}, {0x80, 0xc0, 0xd0, 0xb0}};

    v.pos[0] = corner.x; v.pos[1] = corner.y; v.pos[2] = corner.z;
    im.PushVertex(v);
    v.pos[2] = corner.z + extent;
    im.PushVertex(v);
    v.pos[0] = corner.x + extent; v.pos[2] = corner.z;
    im.PushVertex(v);
    v.pos[2] = corner.z + extent;
    im.PushVertex(v);
  }
  GL(Disable(GL_BLEND));
  GL(Enable(GL_CULL_FACE));

  {
    GlImmediatePC im(immediate_renderer(),
                     GL_LINES,
                     ozz::math::Float4x4::identity());

    // Renders lines along X axis.
    GlImmediatePC::Vertex begin = {{corner.x, corner.y, corner.z}, {0xb0, 0xb0, 0xb0, 0xff}};
    GlImmediatePC::Vertex end = begin; end.pos[0] += extent;
    for (int i = 0; i < _cell_count + 1; ++i) {
      im.PushVertex(begin);
      im.PushVertex(end);
      begin.pos[2] += _cell_size;
      end.pos[2] += _cell_size;
    }
    // Renders lines along Z axis.
    begin.pos[0] = corner.x; begin.pos[1] = corner.y; begin.pos[2] = corner.z;
    end = begin; end.pos[2] += extent;
    for (int i = 0; i < _cell_count + 1; ++i) {
      im.PushVertex(begin);
      im.PushVertex(end);
      begin.pos[0] += _cell_size;
      end.pos[0] += _cell_size;
    }
  }
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
  if (prealloc_models_.Size() < num_joints * sizeof(ozz::math::Float4x4)) {
    ozz::memory::Allocator* allocator = ozz::memory::default_allocator();
    prealloc_models_ =
      allocator->AllocateRange<ozz::math::Float4x4>(_skeleton.num_joints());
  }
  if (!prealloc_models_.begin) {
    return false;
  }
  
  // Compute model space bind pose.
  ozz::animation::LocalToModelJob job;
  job.input = _skeleton.bind_pose();
  job.output = prealloc_models_;
  job.skeleton = &_skeleton;
  if (!job.Run()) {
    return false;
  }

  // Forwards to rendering.
  return DrawPosture(_skeleton, prealloc_models_, _transform, _draw_joints);
}

bool RendererImpl::InitPostureRendering() {
  const float kInter = .2f;
  {  // Prepares bone mesh.
    const math::Float3 pos[6] = {
      math::Float3(1.f, 0.f, 0.f),
      math::Float3(kInter, .1f, .1f),
      math::Float3(kInter, .1f, -.1f),
      math::Float3(kInter, -.1f, -.1f),
      math::Float3(kInter, -.1f, .1f),
      math::Float3(0.f, 0.f, 0.f)};
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
    for (int j = 0; j < kNumPointsYZ; ++j) { // YZ plan.
      float angle = j * math::k2Pi / kNumSlices;
      float s = sinf(angle), c = cosf(angle);
      VertexPNC& vertex = joints[index++];
      vertex.pos = math::Float3(0.f, c * kRadius, s * kRadius);
      vertex.normal = math::Float3(0.f, c, s);
      vertex.color = red;
    }
    for (int j = 0; j < kNumPointsXY; ++j) { // XY plan.
      float angle = j * math::k2Pi / kNumSlices;
      float s = sinf(angle), c = cosf(angle);
      VertexPNC& vertex = joints[index++];
      vertex.pos = math::Float3(s * kRadius, c * kRadius, 0.f);
      vertex.normal = math::Float3(s, c, 0.f);
      vertex.color = blue;
    }
    for (int j = 0; j < kNumPointsXZ; ++j) { // XZ plan.
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

namespace {
int DrawPosture_FillUniforms(const ozz::animation::Skeleton& _skeleton,
                             ozz::Range<const ozz::math::Float4x4> _matrices,
                             float* _uniforms,
                             int _max_instances) {
    assert(math::IsAligned(_uniforms, AlignOf<math::SimdFloat4>::value));

    // Prepares computation constants.
    const int num_joints = _skeleton.num_joints();
    const ozz::animation::Skeleton::JointProperties* properties =
      _skeleton.joint_properties().begin;

    int instances = 0;
    for (int i = 0; i < num_joints && instances < _max_instances; ++i) {

      // Selects parent matrix, which is identity in case of a root.
      const int parent_id = properties[i].parent;
      if (parent_id == ozz::animation::Skeleton::kNoParentIndex) {
        continue;
      }

      // Selects joint matrices.
      const math::Float4x4& parent = _matrices.begin[parent_id];
      const math::Float4x4& current = _matrices.begin[i];

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
      if (properties[i].is_leaf) {
        // Copy current joint's raw matrix.
        float* uniform = _uniforms + instances * 16;
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
                                    const float* _uniforms,
                                    int _instance_count, bool _draw_joints) {
  // Loops through models and instances.
  for (int i = 0; i < (_draw_joints ? 2 : 1); ++i) {
    const Model& model = models_[i];

    // Setup model vertex data.
    GL(BindBuffer(GL_ARRAY_BUFFER, model.vbo));

    // Bind shader
    model.shader->Bind(_transform, camera_->view_proj(),
                       sizeof(VertexPNC), 0,
                       sizeof(VertexPNC), 12,
                       sizeof(VertexPNC), 24);

    GL(BindBuffer(GL_ARRAY_BUFFER, 0));

    // Draw loop.
    const GLint joint_uniform = model.shader->joint_uniform();
    for (int i = 0; i < _instance_count; ++i) {
      GL(UniformMatrix4fv(joint_uniform, 1, false, _uniforms + 16 * i));
      GL(DrawArrays(model.mode, 0, model.count));
    }

    model.shader->Unbind();
  }
}

// "Draw posture" internal instanced rendering implementation.
void RendererImpl::DrawPosture_InstancedImpl(const ozz::math::Float4x4& _transform,
                                             const float* _uniforms,
                                             int _instance_count,
                                             bool _draw_joints) {
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
    model.shader->Bind(_transform, camera_->view_proj(),
                       sizeof(VertexPNC), 0,
                       sizeof(VertexPNC), 12,
                       sizeof(VertexPNC), 24);

    GL(BindBuffer(GL_ARRAY_BUFFER, 0));

    // Setup instanced GL context.
    const GLint joint_attrib = model.shader->joint_instanced_attrib();
    GL(BindBuffer(GL_ARRAY_BUFFER, dynamic_array_bo_));
    GL(EnableVertexAttribArray(joint_attrib + 0));
    GL(EnableVertexAttribArray(joint_attrib + 1));
    GL(EnableVertexAttribArray(joint_attrib + 2));
    GL(EnableVertexAttribArray(joint_attrib + 3));
    GL(VertexAttribDivisorARB(joint_attrib + 0, 1));
    GL(VertexAttribDivisorARB(joint_attrib + 1, 1));
    GL(VertexAttribDivisorARB(joint_attrib + 2, 1));
    GL(VertexAttribDivisorARB(joint_attrib + 3, 1));
    GL(VertexAttribPointer(joint_attrib + 0, 4, GL_FLOAT, GL_FALSE,
                           sizeof(math::Float4x4), GL_PTR_OFFSET(0)));
    GL(VertexAttribPointer(joint_attrib + 1, 4, GL_FLOAT, GL_FALSE,
                           sizeof(math::Float4x4), GL_PTR_OFFSET(16)));
    GL(VertexAttribPointer(joint_attrib + 2, 4, GL_FLOAT, GL_FALSE,
                           sizeof(math::Float4x4), GL_PTR_OFFSET(32)));
    GL(VertexAttribPointer(joint_attrib + 3, 4, GL_FLOAT, GL_FALSE,
                           sizeof(math::Float4x4), GL_PTR_OFFSET(48)));
    GL(BindBuffer(GL_ARRAY_BUFFER, 0));

    GL(DrawArraysInstancedARB(model.mode, 0, model.count, _instance_count));

    GL(DisableVertexAttribArray(joint_attrib + 0));
    GL(DisableVertexAttribArray(joint_attrib + 1));
    GL(DisableVertexAttribArray(joint_attrib + 2));
    GL(DisableVertexAttribArray(joint_attrib + 3));
    GL(VertexAttribDivisorARB(joint_attrib + 0, 0));
    GL(VertexAttribDivisorARB(joint_attrib + 1, 0));
    GL(VertexAttribDivisorARB(joint_attrib + 2, 0));
    GL(VertexAttribDivisorARB(joint_attrib + 3, 0));

    model.shader->Unbind();
  }
}

// Uses GL_ARB_instanced_arrays as a first choice to render the whole skeleton
// in a single draw call.
// Does a draw call per joint if no extension can help.
bool RendererImpl::DrawPosture(const ozz::animation::Skeleton& _skeleton,
                               ozz::Range<const ozz::math::Float4x4> _matrices,
                               const ozz::math::Float4x4& _transform,
                               bool _draw_joints) {
  if (!_matrices.begin || !_matrices.end) {
    return false;
  }
  if (_matrices.end - _matrices.begin < _skeleton.num_joints()) {
    return false;
  }

  // Convert matrices to uniforms.
  const int max_skeleton_pieces = animation::Skeleton::kMaxJoints * 2;
  const size_t max_uniforms_size = max_skeleton_pieces * 2 * 16 * sizeof(float);
  float* uniforms = static_cast<float*>(
    scratch_buffer_.Resize(max_uniforms_size));

  const int instance_count = DrawPosture_FillUniforms(
    _skeleton, _matrices, uniforms, max_skeleton_pieces);
  assert(instance_count <= max_skeleton_pieces);

  if (GL_ARB_instanced_arrays) {
    DrawPosture_InstancedImpl(_transform, uniforms, instance_count, _draw_joints);
  } else {
    DrawPosture_Impl(_transform, uniforms, instance_count, _draw_joints);
  }

  return true;
}

bool RendererImpl::DrawBox(const ozz::math::Box& _box,
                           const ozz::math::Float4x4& _transform,
                           const Color _colors[2]) {

  { // Filled boxed
    GlImmediatePC im(immediate_renderer(), GL_TRIANGLE_STRIP, _transform);
    GlImmediatePC::Vertex v = {
      {0, 0, 0},
      {_colors[0].r, _colors[0].g, _colors[0].b, _colors[0].a}
    };
    // First 3 cube faces
    v.pos[0] = _box.max.x; v.pos[1] = _box.min.y; v.pos[2] = _box.min.z;
    im.PushVertex(v);
    v.pos[0] = _box.min.x; im.PushVertex(v);
    v.pos[0] = _box.max.x; v.pos[1] = _box.max.y; im.PushVertex(v);
    v.pos[0] = _box.min.x; im.PushVertex(v);
    v.pos[0] = _box.max.x; v.pos[2] = _box.max.z; im.PushVertex(v);
    v.pos[0] = _box.min.x; im.PushVertex(v);
    v.pos[0] = _box.max.x; v.pos[1] = _box.min.y; im.PushVertex(v);
    v.pos[0] = _box.min.x; im.PushVertex(v);
    // Link next 3 cube faces with degenerated triangles.
    im.PushVertex(v);
    v.pos[0] = _box.min.x; v.pos[1] = _box.max.y; im.PushVertex(v);
    im.PushVertex(v);
    // Last 3 cube faces.
    v.pos[2] = _box.min.z; im.PushVertex(v);
    v.pos[1] = _box.min.y; v.pos[2] = _box.max.z; im.PushVertex(v);
    v.pos[2] = _box.min.z; im.PushVertex(v);
    v.pos[0] = _box.max.x; v.pos[2] = _box.max.z; im.PushVertex(v);
    v.pos[2] = _box.min.z; im.PushVertex(v);
    v.pos[1] = _box.max.y; v.pos[2] = _box.max.z; im.PushVertex(v);
    v.pos[2] = _box.min.z; im.PushVertex(v);
  }

  { // Wireframe boxed
    GlImmediatePC im(immediate_renderer(), GL_LINES, _transform);
    GlImmediatePC::Vertex v = {
      {0, 0, 0},
      {_colors[1].r, _colors[1].g, _colors[1].b, _colors[1].a}
    };
    // First face.
    v.pos[0] = _box.min.x; v.pos[1] = _box.min.y; v.pos[2] = _box.min.z;
    im.PushVertex(v);
    v.pos[1] = _box.max.y; im.PushVertex(v);
    im.PushVertex(v); v.pos[0] = _box.max.x; im.PushVertex(v);
    im.PushVertex(v); v.pos[1] = _box.min.y; im.PushVertex(v);
    im.PushVertex(v); v.pos[0] = _box.min.x; im.PushVertex(v);
    // Second face.
    v.pos[2] = _box.max.z; im.PushVertex(v);
    v.pos[1] = _box.max.y; im.PushVertex(v);
    im.PushVertex(v); v.pos[0] = _box.max.x; im.PushVertex(v);
    im.PushVertex(v); v.pos[1] = _box.min.y; im.PushVertex(v);
    im.PushVertex(v); v.pos[0] = _box.min.x; im.PushVertex(v);
    // Link faces.
    im.PushVertex(v); v.pos[2] = _box.min.z; im.PushVertex(v);
    v.pos[1] = _box.max.y; im.PushVertex(v);
    v.pos[2] = _box.max.z; im.PushVertex(v);
    v.pos[0] = _box.max.x; im.PushVertex(v);
    v.pos[2] = _box.min.z; im.PushVertex(v);
    v.pos[1] = _box.min.y; im.PushVertex(v);
    v.pos[2] = _box.max.z; im.PushVertex(v);
  }

  return true;
}

namespace {
const uint8_t kDefaultColorArray[][4] = {
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

const float kDefaultNormalArray[][3] = {
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
}  // namespace

bool RendererImpl::DrawMesh(const Mesh& _mesh,
                            const ozz::math::Float4x4& _transform) {

  const int vertex_count = _mesh.vertex_count();
  const GLsizei positions_offset = 0;
  const GLsizei positions_stride = sizeof(float) * 3;
  const GLsizei positions_size = vertex_count * positions_stride;
  const GLsizei normals_offset = positions_offset + positions_size;
  const GLsizei normals_stride = sizeof(float) * 3;
  const GLsizei normals_size = vertex_count * normals_stride;
  const GLsizei colors_offset = normals_offset + normals_size;
  const GLsizei colors_stride = sizeof(uint8_t) * 4;
  const GLsizei colors_size = vertex_count * colors_stride;

  // Reallocate vertex buffer.
  const GLsizei vbo_size = positions_size + normals_size + colors_size;
  GL(BindBuffer(GL_ARRAY_BUFFER, dynamic_array_bo_));
  GL(BufferData(GL_ARRAY_BUFFER, vbo_size, NULL, GL_STREAM_DRAW));

  // Iterate mesh parts and fills vbo.
  size_t vertex_offset = 0;
  for (size_t i = 0; i < _mesh.parts.size(); ++i) {
    const Mesh::Part& part = _mesh.parts[i];
    const size_t part_vertex_count = part.positions.size() / 3;

    // Handles positions.
    GL(BufferSubData(GL_ARRAY_BUFFER,
                     positions_offset + vertex_offset * positions_stride,
                     part_vertex_count * positions_stride,
                     array_begin(part.positions)));

    // Handles normals.
    const size_t part_normal_count = part.normals.size() / 3;
    if (part_vertex_count == part_normal_count) {
      // Optimal path used when the right number of normals is provided.
      GL(BufferSubData(GL_ARRAY_BUFFER,
                       normals_offset + vertex_offset * normals_stride,
                       part_normal_count * normals_stride,
                       array_begin(part.normals)));
    } else {
      // Un-optimal path used when the right number of normals is not provided.
      OZZ_STATIC_ASSERT(sizeof(kDefaultNormalArray[0]) == normals_stride);
      for (size_t j = 0;
           j < part_vertex_count;
           j += OZZ_ARRAY_SIZE(kDefaultNormalArray)) {
        const size_t this_loop_count =
          math::Min(OZZ_ARRAY_SIZE(kDefaultNormalArray), part_vertex_count - j);
        GL(BufferSubData(GL_ARRAY_BUFFER,
                         normals_offset + (vertex_offset + j) * normals_stride,
                         normals_stride * this_loop_count,
                         kDefaultNormalArray));
      }
    }

    // Handles colors.
    const size_t part_color_count = part.colors.size() / 4;
    if (part_vertex_count == part_color_count) {
      // Optimal path used when the right number of colors is provided.
      GL(BufferSubData(GL_ARRAY_BUFFER,
                       colors_offset + vertex_offset * colors_stride,
                       part_color_count * colors_stride,
                       array_begin(part.colors)));
    } else {
      // Un-optimal path used when the right number of colors is not provided.
      OZZ_STATIC_ASSERT(sizeof(kDefaultColorArray[0]) == colors_stride);
      for (size_t j = 0;
           j < part_vertex_count;
           j += OZZ_ARRAY_SIZE(kDefaultColorArray)) {
        const size_t this_loop_count =
          math::Min(OZZ_ARRAY_SIZE(kDefaultColorArray), part_vertex_count - j);
        GL(BufferSubData(GL_ARRAY_BUFFER,
                         colors_offset + (vertex_offset + j) * colors_stride,
                         colors_stride * this_loop_count,
                         kDefaultColorArray));
      }
    }

    // Computes next loop offset.
    vertex_offset += part_vertex_count;
  }
  
  // Binds shader with this array buffer.
  mesh_shader_->Bind(_transform,
                     camera()->view_proj(),
                     positions_stride, positions_offset,
                     normals_stride, normals_offset,
                     colors_stride, colors_offset);

  GL(BindBuffer(GL_ARRAY_BUFFER, 0));

  // Maps the index dynamic buffer and update it.
  GL(BindBuffer(GL_ELEMENT_ARRAY_BUFFER, dynamic_index_bo_));
  const Mesh::TriangleIndices& indices = _mesh.triangle_indices;
  GL(BufferData(GL_ELEMENT_ARRAY_BUFFER,
                indices.size() * sizeof(Mesh::TriangleIndices::value_type),
                array_begin(indices),
                GL_STREAM_DRAW));

  // Draws the mesh.
  OZZ_STATIC_ASSERT(sizeof(Mesh::TriangleIndices::value_type) == 2);
  GL(DrawElements(GL_TRIANGLES,
                  static_cast<GLsizei>(indices.size()),
                  GL_UNSIGNED_SHORT,
                  0));

  // Unbinds.
  GL(BindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0));
  mesh_shader_->Unbind();

  return true;
}

bool RendererImpl::DrawSkinnedMesh(const Mesh& _mesh,
                                   const Range<math::Float4x4> _skinning_matrices,
                                   const ozz::math::Float4x4& _transform) {

  const int vertex_count = _mesh.vertex_count();

  // Positions and normals are interleaved to improve caching while executing
  // skinning job.
  const GLsizei positions_offset = 0;
  const GLsizei normals_offset = sizeof(float) * 3;
  const GLsizei positions_stride = sizeof(float) * 6;
  const GLsizei normals_stride = positions_stride;
  const GLsizei skinned_data_size = vertex_count * positions_stride;

  // Colors are contiguous. They aren't transformed, so they can be directly
  // copied from source mesh which is non-interleaved as-well.
  const GLsizei colors_offset = skinned_data_size;
  const GLsizei colors_stride = sizeof(uint8_t) * 4;
  const GLsizei colors_size = vertex_count * colors_stride;

  // Reallocate vertex buffer.
  const GLsizei vbo_size = skinned_data_size + colors_size;
  GL(BindBuffer(GL_ARRAY_BUFFER, dynamic_array_bo_));
  GL(BufferData(GL_ARRAY_BUFFER, vbo_size, NULL, GL_STREAM_DRAW));
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
    skinning_job.joint_indices = make_range(part.joint_indices);
    skinning_job.joint_indices_stride = sizeof(uint16_t) * part_influences_count;

    // Setup joint's weights.
    if (part_influences_count > 1) {
      skinning_job.joint_weights = make_range(part.joint_weights);
      skinning_job.joint_weights_stride = sizeof(float) * (part_influences_count - 1);
    }

    // Setup input positions, coming from the loaded mesh.
    skinning_job.in_positions = make_range(part.positions);
    skinning_job.in_positions_stride = sizeof(float) * 3;

    // Setup output positions, coming from the rendering output mesh buffers.
    // We need to offset the buffer every loop.
    skinning_job.out_positions.begin = reinterpret_cast<float*>(
      ozz::PointerStride(
        vbo_map, positions_offset + processed_vertex_count * positions_stride));
    skinning_job.out_positions.end = ozz::PointerStride(
      skinning_job.out_positions.begin, part_vertex_count * positions_stride);
    skinning_job.out_positions_stride = positions_stride;

    // Setup normals if input are provided.
    float* out_normal_begin = reinterpret_cast<float*>(ozz::PointerStride(
      vbo_map, normals_offset + processed_vertex_count * normals_stride));
    const float* out_normal_end = ozz::PointerStride(
      out_normal_begin, part_vertex_count * normals_stride);

    if (part.normals.size() == part.positions.size()) {
      // Setup input normals, coming from the loaded mesh.
      skinning_job.in_normals = make_range(part.normals);
      skinning_job.in_normals_stride = sizeof(float) * 3;

      // Setup output normals, coming from the rendering output mesh buffers.
      // We need to offset the buffer every loop.
      skinning_job.out_normals.begin = out_normal_begin;
      skinning_job.out_normals.end = out_normal_end;
      skinning_job.out_normals_stride = normals_stride;
    } else {
      // Fills output with default normals.
      for (float* normal = out_normal_begin;
           normal < out_normal_end;
           normal = ozz::PointerStride(normal, normals_stride)) {
        normal[0] = 0.f;
        normal[1] = 1.f;
        normal[2] = 0.f;
      }
    }

    // Execute the job, which should succeed unless a parameter is invalid.
    if (!skinning_job.Run()) {
      return false;
    }

    // Handles colors which aren't affected by skinning.
    if (part_vertex_count == part.colors.size() / 4) {
      // Optimal path used when the right number of colors is provided.
      memcpy(ozz::PointerStride(vbo_map, colors_offset + processed_vertex_count * colors_stride),
             ozz::PointerStride(array_begin(part.colors), colors_offset + processed_vertex_count * colors_stride),
             part_vertex_count * colors_stride);
    } else {
      // Un-optimal path used when the right number of colors is not provided.
      OZZ_STATIC_ASSERT(sizeof(kDefaultColorArray[0]) == colors_stride);
      for (size_t j = 0;
           j < part_vertex_count;
           j += OZZ_ARRAY_SIZE(kDefaultColorArray)) {
        const size_t this_loop_count =
          math::Min(OZZ_ARRAY_SIZE(kDefaultColorArray), part_vertex_count - j);
        memcpy(ozz::PointerStride(vbo_map, colors_offset + (processed_vertex_count + j) * colors_stride),
               kDefaultColorArray,
               colors_stride * this_loop_count);
      }
    }

    // Some more vertices were processed.
    processed_vertex_count += part_vertex_count;
  }

  // Updates dynamic vertex buffer with skinned data.
  GL(BufferSubData(GL_ARRAY_BUFFER, 0, vbo_size, vbo_map));

  // Binds shader with this array buffer.
  mesh_shader_->Bind(_transform,
                     camera()->view_proj(),
                     positions_stride, positions_offset,
                     normals_stride, normals_offset,
                     colors_stride, colors_offset);

  GL(BindBuffer(GL_ARRAY_BUFFER, 0));

  // Maps the index dynamic buffer and update it.
  GL(BindBuffer(GL_ELEMENT_ARRAY_BUFFER, dynamic_index_bo_));
  const Mesh::TriangleIndices& indices = _mesh.triangle_indices;
  GL(BufferData(GL_ELEMENT_ARRAY_BUFFER,
                indices.size() * sizeof(Mesh::TriangleIndices::value_type),
                array_begin(indices),
                GL_STREAM_DRAW));

  // Draws the mesh.
  OZZ_STATIC_ASSERT(sizeof(Mesh::TriangleIndices::value_type) == 2);
  GL(DrawElements(GL_TRIANGLES,
                  static_cast<GLsizei>(indices.size()),
                  GL_UNSIGNED_SHORT,
                  0));

  // Unbinds.
  GL(BindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0));
  mesh_shader_->Unbind();

  return true;
}

// Helper macro used to initialize extension function pointer.
#define OZZ_INIT_GL_EXT(_fct, _fct_type, _success) \
do {\
  _fct = reinterpret_cast<_fct_type>(glfwGetProcAddress(#_fct)); \
  if (_fct == NULL) {\
    log::Err() << "Unable to install " #_fct " function." << std::endl;\
    _success &= false;\
  }\
} while (void(0), 0)

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
  OZZ_INIT_GL_EXT(
    glGetBufferParameteriv, PFNGLGETBUFFERPARAMETERIVPROC, success);
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
  OZZ_INIT_GL_EXT(
    glDisableVertexAttribArray, PFNGLDISABLEVERTEXATTRIBARRAYPROC, success);
  OZZ_INIT_GL_EXT(
    glEnableVertexAttribArray, PFNGLENABLEVERTEXATTRIBARRAYPROC, success);
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
  OZZ_INIT_GL_EXT(
    glGetVertexAttribPointerv, PFNGLGETVERTEXATTRIBPOINTERVPROC, success);
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
    log::Err() << "Failed to initialize all mandatory GL extensions." <<
      std::endl;
    return false;
  }
  if (!optional_success) {
    log::Err() << "Failed to initialize some optional GL extensions." <<
      std::endl;
  }

  GL_ARB_instanced_arrays =
    glfwExtensionSupported("GL_ARB_instanced_arrays") != 0;
  if (GL_ARB_instanced_arrays) {
    log::Log() << "Optional GL_ARB_instanced_arrays extensions found." <<
      std::endl;
    success = true;
    OZZ_INIT_GL_EXT(
      glVertexAttribDivisorARB, PFNGLVERTEXATTRIBDIVISORARBPROC, success);
    OZZ_INIT_GL_EXT(
      glDrawArraysInstancedARB, PFNGLDRAWARRAYSINSTANCEDARBPROC, success);
    OZZ_INIT_GL_EXT(
      glDrawElementsInstancedARB, PFNGLDRAWELEMENTSINSTANCEDARBPROC, success);
    if (!success) {
      log::Err() <<
        "Failed to setup GL_ARB_instanced_arrays, feature is disabled."
        << std::endl;
      GL_ARB_instanced_arrays = false;
    }
  } else {
    log::Log() << "Optional GL_ARB_instanced_arrays extensions not found." <<
      std::endl;
  }
  return true;
}


RendererImpl::ScratchBuffer::ScratchBuffer()
    : buffer_(NULL),
      size_(0) {
}

RendererImpl::ScratchBuffer::~ScratchBuffer() {
  memory::default_allocator()->Deallocate(buffer_);
}

void* RendererImpl::ScratchBuffer::Resize(size_t _size) {
  if (_size > size_) {
    size_ = _size;
    buffer_ = memory::default_allocator()->Reallocate(buffer_, _size, 16);
  }
  return buffer_;
}
}  // internal
}  // sample
}  // ozz

// Helper macro used to declare extension function pointer.
#define OZZ_DECL_GL_EXT(_fct, _fct_type) _fct_type _fct = NULL

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

bool GL_ARB_instanced_arrays = false;
OZZ_DECL_GL_EXT(glVertexAttribDivisorARB, PFNGLVERTEXATTRIBDIVISORARBPROC);
OZZ_DECL_GL_EXT(glDrawArraysInstancedARB, PFNGLDRAWARRAYSINSTANCEDARBPROC);
OZZ_DECL_GL_EXT(glDrawElementsInstancedARB, PFNGLDRAWELEMENTSINSTANCEDARBPROC);
