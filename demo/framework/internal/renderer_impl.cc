//============================================================================//
// Copyright (c) <2012> <Guillaume Blanc>                                     //
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
//============================================================================//

// This file is allowed to include private headers.
#define OZZ_INCLUDE_PRIVATE_HEADER

#include "framework/internal/renderer_impl.h"

#include "ozz/animation/local_to_model_job.h"
#include "ozz/animation/skeleton.h"

#include "ozz/base/log.h"

#include "ozz/base/platform.h"

#include "ozz/base/maths/math_ex.h"
#include "ozz/base/maths/simd_math.h"
#include "ozz/base/maths/vec_float.h"

#include "ozz/base/memory/allocator.h"

namespace ozz {
namespace demo {
namespace internal {

namespace {
// A vertex made of positions and normals.
struct VertexPN {
  math::Float3 pos;
  math::Float3 normal;
};
}  // namespace

RendererImpl::RendererImpl()
    : ambient_shader_(NULL),
      joint_model_vbo_(0),
      joint_instance_vbo_(0) {
  prealloc_models_.end = prealloc_models_.begin = NULL;
}

RendererImpl::~RendererImpl() {
  memory::default_allocator().Deallocate(prealloc_models_.begin);
  prealloc_models_.end = prealloc_models_.begin = NULL;

  GL(DeleteBuffers(1, &joint_instance_vbo_));
  joint_instance_vbo_ = 0;

  DeinitPostureRendering();
  DeinitShading();
}

bool RendererImpl::Initialize() {
  if (!InitOpenGLExtensions()) {
    return false;
  }
  if (!InitShading()) {
    return false;
  }
  if (!InitPostureRendering()) {
    return false;
  }

  // Builds the dynamic vbo
  GL(GenBuffers(1, &joint_instance_vbo_));

  return true;
}

bool RendererImpl::InitShading() {
  // Builds a world matrix from joint's direction, binormal and position.
  const char* vs_joint_to_world_matrix =
    "mat4 GetWorldMatrix() {\n"
    "  // Builds the world matrix from joint specifications\n"
    "  mat4 matrix;\n"
    "  float len = length(joint[3].xyz);\n"
    "  float yz_len = /*sqrt*/(len) * .1;\n"
    "  matrix[0] = vec4(len * normalize(joint[0].xyz), 0.);\n"
    "  matrix[1] = vec4(\n"
    "    yz_len * normalize(cross(joint[1].xyz, matrix[0].xyz)), 0.);\n"
    "  matrix[2] = vec4(\n"
    "    yz_len * normalize(cross(matrix[0].xyz, matrix[1].xyz)), 0.);\n"
    "  matrix[3] = vec4(joint[2].xyz, 1.);\n"
    "  return matrix;\n"
    "}\n";
  /*const char* vs_world_matrix =
    "uniform mat4 world_matrix;\n"
    "mat4 GetWorldMatrix() {\n"
    "  return world_matrix;\n"
    "}\n";*/
  const char* lerp =
    "vec3 lerp(in vec3 alpha, in vec3 a, in vec3 b) {\n"
    "  return a + alpha * (b - a);\n"
    "}\n"
    "vec4 lerp(in vec4 alpha, in vec4 a, in vec4 b) {\n"
    "  return a + alpha * (b - a);\n"
    "}\n";
  const char* vs_code =
    "varying vec3 world_normal;\n"
    "void main() {\n"
    "  mat4 world_matrix = GetWorldMatrix();\n"
    "  gl_Position = gl_ModelViewProjectionMatrix * world_matrix * gl_Vertex;\n"
    "  mat3 cross_matrix = mat3(\n"
    "    cross(world_matrix[1].xyz, world_matrix[2].xyz),\n"
    "    cross(world_matrix[2].xyz, world_matrix[0].xyz),\n"
    "    cross(world_matrix[0].xyz, world_matrix[1].xyz));\n"
    "  float invdet = 1.0 / dot(cross_matrix[2], world_matrix[2].xyz);\n"
    "  mat3 normal_matrix = cross_matrix * invdet;\n"
    "  world_normal = normal_matrix * gl_Normal;\n"
    "}\n";
  const char* fs_code =
    "varying vec3 world_normal;\n"
    "void main() {\n"
    "  vec3 normal = normalize(world_normal);\n"
    "  vec3 alpha = (normal + 1.) * .5;\n"
    "  vec4 bt = lerp(\n"
    "    alpha.xzxz, vec4(.3, .3, .7, .7), vec4(.4, .4, .8, .8));\n"
    "  gl_FragColor = vec4(\n"
    "     lerp(alpha.yyy, vec3(bt.x, .3, bt.y), vec3(bt.z, .8, bt.w)), 1.);\n"
    "}\n";
  const char* vs[] = {
    "#version 110\n",
    GL_ARB_instanced_arrays ?
      "attribute mat4 joint;\n" :
      "uniform mat4 joint;\n",  // Simplest fallback.
    vs_joint_to_world_matrix,
    vs_code};
  const char* fs[] = {
    "#version 110\n",
    lerp,
    fs_code};

  ambient_shader_ = Shader::Build(OZZ_ARRAY_SIZE(vs), vs,
                                  OZZ_ARRAY_SIZE(fs), fs);
  if (!ambient_shader_) {
    return false;
  }
  if (GL_ARB_instanced_arrays &&
      !ambient_shader_->BindAttrib("joint")) {
    return false;
  } else if (!GL_ARB_instanced_arrays &&
             !ambient_shader_->BindUniform("joint")) {
    return false;
  }
  return true;
}

void RendererImpl::DeinitShading() {
  memory::default_allocator().Delete(ambient_shader_);
  ambient_shader_ = NULL;
}

void RendererImpl::DrawAxes(float _scale) {
  glBegin(GL_LINES);
  glColor4ub(255, 0, 0, 255);  // X axis (red).
  glVertex3f(0.f, 0.f, 0.f);
  glVertex3f(_scale, 0.f, 0.f);
  glColor4ub(0, 255, 0, 255);  // Y axis (green).
  glVertex3f(0.f, 0.f, 0.f);
  glVertex3f(0.f, _scale, 0.f);
  glColor4ub(0, 0, 255, 255);  // Z axis (blue).
  glVertex3f(0.f, 0.f, 0.f);
  glVertex3f(0.f, 0.f, _scale);
  GL(End());
}

void RendererImpl::DrawGrid(int _cell_count, float _cell_size) {
  const float extent = _cell_count * _cell_size;
  const float half_extent = extent * 0.5f;
  const ozz::math::Float3 corner(-half_extent, 0, -half_extent);

  GL(Enable(GL_BLEND));
  GL(BlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));
  GL(Disable(GL_CULL_FACE));
  glBegin(GL_QUADS);
  glColor4ub(0x80, 0xc0, 0xd0, 0xb0);
  glVertex3f(corner.x, corner.y, corner.z);
  glVertex3f(corner.x, corner.y, corner.z + extent);
  glVertex3f(corner.x + extent, corner.y, corner.z + extent);
  glVertex3f(corner.x + extent, corner.y, corner.z);
  GL(End());
  GL(Disable(GL_BLEND));
  GL(Enable(GL_CULL_FACE));

  glBegin(GL_LINES);
  glColor4ub(0xb0, 0xb0, 0xb0, 0xff);
  // Renders lines along X axis.
  ozz::math::Float3 x_line_begin = corner;
  ozz::math::Float3 x_line_end(corner.x + extent, corner.y, corner.z);
  for (int i = 0; i < _cell_count + 1; i++) {
    glVertex3fv(&x_line_begin.x);
    glVertex3fv(&x_line_end.x);
    x_line_begin.z += _cell_size;
    x_line_end.z += _cell_size;
  }
  // Renders lines along Z axis.
  ozz::math::Float3 z_line_begin = corner;
  ozz::math::Float3 z_line_end(corner.x, corner.y, corner.z + extent);
  for (int i = 0; i < _cell_count + 1; i++) {
    glVertex3fv(&z_line_begin.x);
    glVertex3fv(&z_line_end.x);
    z_line_begin.x += _cell_size;
    z_line_end.x += _cell_size;
  }
  GL(End());
}

// Computes the model space bind pose and renders it.
bool RendererImpl::DrawSkeleton(const ozz::animation::Skeleton& _skeleton,
                                bool _render_leaf) {
  using ozz::math::Float4x4;

  const int num_joints = _skeleton.num_joints();
  if (!num_joints) {
    return true;
  }

  // Reallocate matrix array if necessary.
  if (prealloc_models_.end - prealloc_models_.begin < num_joints) {
    prealloc_models_ = ozz::animation::AllocateModels(
      &memory::default_allocator(), num_joints);
  }
  if (!prealloc_models_.begin) {
    return false;
  }

  // Compute model space bind pose.
  ozz::animation::LocalToModelJob job;
  job.input_begin = _skeleton.bind_pose();
  job.input_end = _skeleton.bind_pose_end();
  job.output_begin = prealloc_models_.begin;
  job.output_end = prealloc_models_.end;
  job.skeleton = &_skeleton;
  if (!job.Run()) {
    return false;
  }

  // Forwards to rendering.
  return DrawPosture(_skeleton,
                     prealloc_models_.begin, prealloc_models_.end,
                     _render_leaf);
}

bool RendererImpl::InitPostureRendering() {
  // Prepares joint mesh.
  const float inter = .15f;
  const math::Float3 pos[6] = {
    math::Float3(1.f, 0.f, 0.f),
    math::Float3(inter, 1.f, 1.f),
    math::Float3(inter, 1.f, -1.f),
    math::Float3(inter, -1.f, -1.f),
    math::Float3(inter, -1.f, 1.f),
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
  const VertexPN vertices[24] = {
    {pos[0], normals[0]}, {pos[2], normals[0]}, {pos[1], normals[0]},
    {pos[5], normals[1]}, {pos[1], normals[1]}, {pos[2], normals[1]},
    {pos[0], normals[2]}, {pos[3], normals[2]}, {pos[2], normals[2]},
    {pos[5], normals[3]}, {pos[2], normals[3]}, {pos[3], normals[3]},
    {pos[0], normals[4]}, {pos[4], normals[4]}, {pos[3], normals[4]},
    {pos[5], normals[5]}, {pos[3], normals[5]}, {pos[4], normals[5]},
    {pos[0], normals[6]}, {pos[1], normals[6]}, {pos[4], normals[6]},
    {pos[5], normals[7]}, {pos[4], normals[7]}, {pos[1], normals[7]}};

  // Builds and fills the vbo.
  GL(GenBuffers(1, &joint_model_vbo_));
  GL(BindBuffer(GL_ARRAY_BUFFER, joint_model_vbo_));
  GL(BufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW));
  GL(BindBuffer(GL_ARRAY_BUFFER, 0));  // Unbinds.

  return true;
}

void RendererImpl::DeinitPostureRendering() {
  GL(DeleteBuffers(1, &joint_model_vbo_));
  joint_model_vbo_ = 0;
}

// Draw posture internal non-instanced rendering fallback implementation.
void RendererImpl::DrawPosture_Impl(const ozz::animation::Skeleton& _skeleton,
                                    const ozz::math::Float4x4* _matrices,
                                    bool _render_leaf) {
  // Prepares computation constants.
  const math::SimdFloat4 comperand = math::simd_float4::Load1(1e-3f);
  const math::SimdFloat4 x_axis = math::simd_float4::x_axis();

  // Get the properties buffer.
  const ozz::animation::Skeleton::JointProperties* properties =
    _skeleton.joint_properties();

  OZZ_ALIGN(16) float uniforms[16];
  const GLint joint_uniform_slot = ambient_shader_->uniform(0);

  const int num_joints = _skeleton.num_joints();
  for (int i = 0; i < num_joints; i++) {
    const math::Float4x4& current = _matrices[i];

    // If there's a parent, renders it.
    const int parent_id = properties[i].parent;
    math::SimdFloat4 parent_right = x_axis;
    if (parent_id != ozz::animation::Skeleton::kRootIndex) {
      const math::Float4x4& parent = _matrices[parent_id];

      parent_right = current.cols[3] - parent.cols[3];
      const math::SimdInt4 choose_axis_z =
        math::CmpLt(
          math::Abs(math::SplatX(math::Dot3(parent.cols[2], parent_right))),
          comperand);
      math::StorePtr(parent_right, uniforms + 0);
      math::StorePtr(
        math::Select(choose_axis_z, parent.cols[2], parent.cols[1]),
        uniforms + 4);
      math::StorePtr(parent.cols[3], uniforms + 8);
      math::StorePtr(parent_right, uniforms + 12);  // Uses its own length.

      GL(UniformMatrix4fv(joint_uniform_slot, 1, false, uniforms));
      GL(DrawArrays(GL_TRIANGLES, 0, 24));
    }

    // Renders current joint's x vector if it is a leaf.
    if (_render_leaf && properties[i].is_leaf) {
      const math::SimdInt4 choose_axis_z =
        math::CmpLt(
          math::Abs(math::SplatX(math::Dot3(current.cols[2], current.cols[0]))),
          comperand);
      math::StorePtr(current.cols[0], uniforms + 0);
      math::StorePtr(
        math::Select(choose_axis_z, current.cols[2], current.cols[1]),
        uniforms + 4);
      math::StorePtr(current.cols[3], uniforms + 8);
      math::StorePtr(parent_right, uniforms + 12);  // Uses parent's length.

      GL(UniformMatrix4fv(joint_uniform_slot, 1, false, uniforms));
      GL(DrawArrays(GL_TRIANGLES, 0, 24));
    }
  }
}

// Draw posture internal instanced rendering implementation.
void RendererImpl::DrawPosture_InstancedImpl(
  const ozz::animation::Skeleton& _skeleton,
  const ozz::math::Float4x4* _matrices,
  bool _render_leaf) {
  // Prepares computation constants.
  const math::SimdFloat4 comperand = math::simd_float4::Load1(1e-3f);
  const math::SimdFloat4 x_axis = math::simd_float4::x_axis();

  const int num_joints = _skeleton.num_joints();

  // Get the properties buffer.
  const ozz::animation::Skeleton::JointProperties* properties =
    _skeleton.joint_properties();

  // Maps the dynamic buffer
  GL(BindBuffer(GL_ARRAY_BUFFER, joint_instance_vbo_));
  const int max_instances = num_joints * 2;  // If all joints are leaves.
  const std::size_t required_vbo_size_ =
    max_instances * sizeof(math::Float4x4) + AlignOf<math::Float4x4>::value - 1;
  GL(BufferData(GL_ARRAY_BUFFER, required_vbo_size_, NULL, GL_STREAM_DRAW));
  void* unaligned = glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);
  math::Float4x4* attribs = reinterpret_cast<math::Float4x4*>(
    math::Align(unaligned, AlignOf<math::Float4x4>::value));
  const std::ptrdiff_t alignment_offset =
    std::ptrdiff_t(attribs) - std::ptrdiff_t(unaligned);

  // Setup GL context.
  const GLint joint_attrib = ambient_shader_->attrib(0);
  GL(EnableVertexAttribArray(joint_attrib + 0));
  GL(EnableVertexAttribArray(joint_attrib + 1));
  GL(EnableVertexAttribArray(joint_attrib + 2));
  GL(EnableVertexAttribArray(joint_attrib + 3));
  GL(VertexAttribDivisorARB(joint_attrib + 0, 1));
  GL(VertexAttribDivisorARB(joint_attrib + 1, 1));
  GL(VertexAttribDivisorARB(joint_attrib + 2, 1));
  GL(VertexAttribDivisorARB(joint_attrib + 3, 1));
  GL(VertexAttribPointer(joint_attrib + 0, 4, GL_FLOAT, GL_FALSE,
                         sizeof(math::Float4x4),
                         GL_OFFSET(0 + alignment_offset)));
  GL(VertexAttribPointer(joint_attrib + 1, 4, GL_FLOAT, GL_FALSE,
                         sizeof(math::Float4x4),
                         GL_OFFSET(16 + alignment_offset)));
  GL(VertexAttribPointer(joint_attrib + 2, 4, GL_FLOAT, GL_FALSE,
                         sizeof(math::Float4x4),
                         GL_OFFSET(32 + alignment_offset)));
  GL(VertexAttribPointer(joint_attrib + 3, 4, GL_FLOAT, GL_FALSE,
                         sizeof(math::Float4x4),
                         GL_OFFSET(48 + alignment_offset)));

  int instance_count = 0;
  for (int i = 0; i < num_joints; i++) {
    const math::Float4x4& current = _matrices[i];

    // If there's a parent, renders it.
    const int parent_id = properties[i].parent;
    math::SimdFloat4 parent_right = x_axis;
    if (parent_id != ozz::animation::Skeleton::kRootIndex) {
      const math::Float4x4& parent = _matrices[parent_id];
      parent_right = current.cols[3] - parent.cols[3];

      const math::SimdInt4 choose_axis_z =
        math::CmpLt(
          math::Abs(math::SplatX(math::Dot3(parent.cols[2], parent_right))),
          comperand);
      attribs[instance_count].cols[0] = parent_right;
      attribs[instance_count].cols[1] =
        math::Select(choose_axis_z, parent.cols[2], parent.cols[1]);
      attribs[instance_count].cols[2] = parent.cols[3];
      attribs[instance_count].cols[3] = parent_right;  // Uses its own length.

      instance_count++;
    }

    // Renders current joint's x vector if it is a leaf.
    if (_render_leaf && properties[i].is_leaf) {
      const math::SimdInt4 choose_axis_z =
        math::CmpLt(
          math::Abs(math::SplatX(math::Dot3(current.cols[2], current.cols[0]))),
          comperand);
      attribs[instance_count].cols[0] = current.cols[0];
      attribs[instance_count].cols[1] =
        math::Select(choose_axis_z, current.cols[2], current.cols[1]);
      attribs[instance_count].cols[2] = current.cols[3];
      attribs[instance_count].cols[3] = parent_right;  // Uses parent's length.

      instance_count++;
    }
  }

  assert(instance_count <= max_instances);
  if (glUnmapBuffer(GL_ARRAY_BUFFER)) {  // Skips rendering if unmaping failed.
    if (instance_count) {
      GL(DrawArraysInstancedARB(GL_TRIANGLES, 0, 24, instance_count));
    }

    GL(DisableVertexAttribArray(joint_attrib + 0));
    GL(DisableVertexAttribArray(joint_attrib + 1));
    GL(DisableVertexAttribArray(joint_attrib + 2));
    GL(DisableVertexAttribArray(joint_attrib + 3));
    GL(VertexAttribDivisorARB(joint_attrib + 0, 0));
    GL(VertexAttribDivisorARB(joint_attrib + 1, 0));
    GL(VertexAttribDivisorARB(joint_attrib + 2, 0));
    GL(VertexAttribDivisorARB(joint_attrib + 3, 0));
  }
  GL(BindBuffer(GL_ARRAY_BUFFER, 0));
}

// Uses GL_ARB_instanced_arrays as a first choice to render the whole skeleton
// in a single draw call.
// Does a draw call per joint if no extension can help.
bool RendererImpl::DrawPosture(const ozz::animation::Skeleton& _skeleton,
                               const ozz::math::Float4x4* _begin,
                               const ozz::math::Float4x4* _end,
                               bool _render_leaf) {
  if (!_begin || !_end) {
    return false;
  }
  if (_end - _begin < _skeleton.num_joints()) {
    return false;
  }

  // Switches to ambient shader.
  GL(UseProgram(ambient_shader_->program()));
  GL(BindBuffer(GL_ARRAY_BUFFER, joint_model_vbo_));
  GL(EnableClientState(GL_VERTEX_ARRAY));
  GL(VertexPointer(3, GL_FLOAT, sizeof(VertexPN), 0));
  GL(EnableClientState(GL_NORMAL_ARRAY));
  GL(NormalPointer(GL_FLOAT, sizeof(VertexPN), GL_OFFSET(12)));
  GL(BindBuffer(GL_ARRAY_BUFFER, 0));

  if (GL_ARB_instanced_arrays) {
    DrawPosture_InstancedImpl(_skeleton, _begin, _render_leaf);
  } else {
    DrawPosture_Impl(_skeleton, _begin, _render_leaf);
  }

  // Restores fixed pipeline.
  GL(UseProgram(0));
  GL(DisableClientState(GL_VERTEX_ARRAY));
  GL(DisableClientState(GL_NORMAL_ARRAY));

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
  bool success = true;
  OZZ_INIT_GL_EXT(glBindBuffer, PFNGLBINDBUFFERPROC, success);
  OZZ_INIT_GL_EXT(glDeleteBuffers, PFNGLDELETEBUFFERSPROC, success);
  OZZ_INIT_GL_EXT(glGenBuffers, PFNGLGENBUFFERSPROC, success);
  OZZ_INIT_GL_EXT(glIsBuffer, PFNGLISBUFFERPROC, success);
  OZZ_INIT_GL_EXT(glBufferData, PFNGLBUFFERDATAPROC, success);
  OZZ_INIT_GL_EXT(glBufferSubData, PFNGLBUFFERSUBDATAPROC, success);
  OZZ_INIT_GL_EXT(glGetBufferSubData, PFNGLGETBUFFERSUBDATAPROC, success);
  OZZ_INIT_GL_EXT(glMapBuffer, PFNGLMAPBUFFERPROC, success);
  OZZ_INIT_GL_EXT(glUnmapBuffer, PFNGLUNMAPBUFFERPROC, success);
  OZZ_INIT_GL_EXT(
    glGetBufferParameteriv, PFNGLGETBUFFERPARAMETERIVPROC, success);
  OZZ_INIT_GL_EXT(glGetBufferPointerv, PFNGLGETBUFFERPOINTERVPROC, success);

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
  OZZ_INIT_GL_EXT(glGetVertexAttribdv, PFNGLGETVERTEXATTRIBDVPROC, success);
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
  if (!success) {
    log::Err() << "Failed to initialize mandatory GL extensions." << std::endl;
    return false;
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

Shader::Shader()
    : program_(0),
      vertex_(0),
      fragment_(0) {
}

Shader::~Shader() {
  if (vertex_) {
    GL(DetachShader(program_, vertex_));
    GL(DeleteShader(vertex_));
  }
  if (fragment_) {
    GL(DetachShader(program_, fragment_));
    GL(DeleteShader(fragment_));
  }
  if (program_) {
    GL(DeleteProgram(program_));
  }
}

Shader* Shader::Build(int _vertex_count, const char** _vertex,
                      int _fragment_count, const char** _fragment) {
  // Tries to compile shaders.
  GLuint vertex_shader = 0;
  if (_vertex) {
    vertex_shader =
      CompileShader(GL_VERTEX_SHADER, _vertex_count, _vertex);
    if (!vertex_shader) {
      return NULL;
    }
  }
  GLuint fragment_shader = 0;
  if (_fragment) {
    fragment_shader =
      CompileShader(GL_FRAGMENT_SHADER, _fragment_count, _fragment);
    if (!fragment_shader) {
      return NULL;
    }
  }

  // Shaders are compiled, builds program.
  Shader* shader = memory::default_allocator().New<Shader>();
  shader->program_ = glCreateProgram();
  shader->vertex_ = vertex_shader;
  shader->fragment_ = fragment_shader;
  GL(AttachShader(shader->program_, vertex_shader));
  GL(AttachShader(shader->program_, fragment_shader));
  GL(LinkProgram(shader->program_));

  int infolog_length = 0;
  GL(GetProgramiv(shader->program_, GL_INFO_LOG_LENGTH, &infolog_length));
  if (infolog_length > 0) {
    char* info_log = memory::default_allocator().Allocate<char>(infolog_length);
    int chars_written  = 0;
    glGetProgramInfoLog(
      shader->program_, infolog_length, &chars_written, info_log);
    log::Err() << info_log << std::endl;
    memory::default_allocator().Deallocate(info_log);
  }

  return shader;
}

GLuint Shader::CompileShader(GLenum _type, int _count, const char** _src) {
  GLuint shader = glCreateShader(_type);
  GL(ShaderSource(shader, _count, _src, NULL));
  GL(CompileShader(shader));

  int infolog_length = 0;
  GL(GetShaderiv(shader, GL_INFO_LOG_LENGTH, &infolog_length));
  if (infolog_length > 0) {
    char* info_log = memory::default_allocator().Allocate<char>(infolog_length);
    int chars_written  = 0;
    glGetShaderInfoLog(shader, infolog_length, &chars_written, info_log);
    log::Err() << info_log << std::endl;
    memory::default_allocator().Deallocate(info_log);
  }

  int status;
  GL(GetShaderiv(shader, GL_COMPILE_STATUS, &status));
  if (status) {
    return shader;
  }

  GL(DeleteShader(shader));
  return 0;
}

bool Shader::BindUniform(const char* _semantic) {
  GLint location = glGetUniformLocation(program_, _semantic);
  if (location == -1) {  // _semantic not found.
    return false;
  }
  uniforms_.push_back(location);
  return true;
}

bool Shader::BindAttrib(const char* _semantic) {
  GLint location = glGetAttribLocation(program_, _semantic);
  if (location == -1) {  // _semantic not found.
    return false;
  }
  attribs_.push_back(location);
  return true;
}
}  // internal
}  // demo
}  // ozz

// Helper macro used to declare extension function pointer.
#define OZZ_DECL_GL_EXT(_fct, _fct_type) _fct_type _fct = NULL

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

bool GL_ARB_instanced_arrays = false;
OZZ_DECL_GL_EXT(glVertexAttribDivisorARB, PFNGLVERTEXATTRIBDIVISORARBPROC);
OZZ_DECL_GL_EXT(glDrawArraysInstancedARB, PFNGLDRAWARRAYSINSTANCEDARBPROC);
OZZ_DECL_GL_EXT(glDrawElementsInstancedARB, PFNGLDRAWELEMENTSINSTANCEDARBPROC);
