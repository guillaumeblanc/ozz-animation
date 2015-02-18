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

#define OZZ_INCLUDE_PRIVATE_HEADER  // Allows to include private headers.

#include "shader.h"

#include <cassert>
#include <cstdio>

#include "ozz/base/log.h"
#include "ozz/base/maths/simd_math.h"

namespace ozz {
namespace sample {
namespace internal {

#ifdef EMSCRIPTEN
  // WebGL requires to specify floating point precision
  static const char* kPlatformSpecivicVSHeader = "precision mediump float;\n";
  static const char* kPlatformSpecivicFSHeader = "precision mediump float;\n";
#else  // EMSCRIPTEN
  static const char* kPlatformSpecivicVSHeader = "";
  static const char* kPlatformSpecivicFSHeader = "";
#endif  // EMSCRIPTEN

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

namespace {
GLuint CompileShader(GLenum _type, int _count, const char** _src) {
  GLuint shader = glCreateShader(_type);
  GL(ShaderSource(shader, _count, _src, NULL));
  GL(CompileShader(shader));

  int infolog_length = 0;
  GL(GetShaderiv(shader, GL_INFO_LOG_LENGTH, &infolog_length));
  if (infolog_length > 1) {
    char* info_log = memory::default_allocator()->Allocate<char>(infolog_length);
    int chars_written  = 0;
    glGetShaderInfoLog(shader, infolog_length, &chars_written, info_log);
    log::Err() << info_log << std::endl;
    memory::default_allocator()->Deallocate(info_log);
  }

  int status;
  GL(GetShaderiv(shader, GL_COMPILE_STATUS, &status));
  if (status) {
    return shader;
  }

  GL(DeleteShader(shader));
  return 0;
}
} // namespace

bool Shader::BuildFromSource(int _vertex_count, const char** _vertex,
                             int _fragment_count, const char** _fragment) {
  // Tries to compile shaders.
  GLuint vertex_shader = 0;
  if (_vertex) {
    vertex_shader =
      CompileShader(GL_VERTEX_SHADER, _vertex_count, _vertex);
    if (!vertex_shader) {
      return false;
    }
  }
  GLuint fragment_shader = 0;
  if (_fragment) {
    fragment_shader =
      CompileShader(GL_FRAGMENT_SHADER, _fragment_count, _fragment);
    if (!fragment_shader) {
      if (vertex_shader) {
        GL(DeleteShader(vertex_shader));
      }
      return false;
    }
  }

  // Shaders are compiled, builds program.
  program_ = glCreateProgram();
  vertex_ = vertex_shader;
  fragment_ = fragment_shader;
  GL(AttachShader(program_, vertex_shader));
  GL(AttachShader(program_, fragment_shader));
  GL(LinkProgram(program_));

  int infolog_length = 0;
  GL(GetProgramiv(program_, GL_INFO_LOG_LENGTH, &infolog_length));
  if (infolog_length > 1) {
    char* info_log =
      memory::default_allocator()->Allocate<char>(infolog_length);
    int chars_written  = 0;
    glGetProgramInfoLog(program_, infolog_length, &chars_written, info_log);
    log::Err() << info_log << std::endl;
    memory::default_allocator()->Deallocate(info_log);
  }

  return true;
}

bool Shader::BindUniform(const char* _semantic) {
  GLint location = glGetUniformLocation(program_, _semantic);
  if (location == -1) {  // _semantic not found.
    return false;
  }
  uniforms_.push_back(location);
  return true;
}

bool Shader::FindAttrib(const char* _semantic) {
  GLint location = glGetAttribLocation(program_, _semantic);
  if (location == -1) {  // _semantic not found.
    return false;
  }
  attribs_.push_back(location);
  return true;
}

void Shader::UnbindAttribs() {
  for (size_t i = 0; i < attribs_.size(); ++i) {
    GL(DisableVertexAttribArray(attribs_[i]));
  }
}

ImmediatePCShader* ImmediatePCShader::Build() {
  bool success = true;

  const char* simple_pc_vs =
    "uniform mat4 u_mvp;\n"
    "attribute vec3 a_position;\n"
    "attribute vec4 a_color;\n"
    "varying vec4 v_vertex_color;\n"
    "void main() {\n"
    "  vec4 vertex = vec4(a_position.xyz, 1.);\n"
    "  gl_Position = u_mvp * vertex;\n"
    "  v_vertex_color = a_color;\n"
    "}\n";
  const char* simple_pc_ps =
    "varying vec4 v_vertex_color;\n"
    "void main() {\n"
    "  gl_FragColor = v_vertex_color;\n"
    "}\n";

  const char* vs[] = {
    kPlatformSpecivicVSHeader,
    simple_pc_vs};
  const char* fs[] = {
    kPlatformSpecivicFSHeader,
    simple_pc_ps};

  ImmediatePCShader* shader =
    memory::default_allocator()->New<ImmediatePCShader>();
  success &= shader->BuildFromSource(OZZ_ARRAY_SIZE(vs), vs,
                                     OZZ_ARRAY_SIZE(fs), fs);

  // Binds default attributes
  success &= shader->FindAttrib("a_position");
  success &= shader->FindAttrib("a_color");

  // Binds default uniforms
  success &= shader->BindUniform("u_mvp");

  if (!success) {
    memory::default_allocator()->Delete(shader);
    shader = NULL;
  }

  return shader;
}

void ImmediatePCShader::Bind(const math::Float4x4& _model,
                             const math::Float4x4& _view_proj,
                             GLsizei _pos_stride, GLsizei _pos_offset,
                             GLsizei _color_stride, GLsizei _color_offset) {

  GL(UseProgram(program()));

  const GLint position_attrib = attrib(0);
  GL(EnableVertexAttribArray(position_attrib));
  GL(VertexAttribPointer(position_attrib, 3, GL_FLOAT, GL_FALSE,
                         _pos_stride, GL_PTR_OFFSET(_pos_offset)));

  const GLint color_attrib = attrib(1);
  GL(EnableVertexAttribArray(color_attrib));
  GL(VertexAttribPointer(color_attrib, 4, GL_UNSIGNED_BYTE, GL_TRUE,
                         _color_stride, GL_PTR_OFFSET(_color_offset)));

  // Binds mvp uniform
  const GLint mvp_uniform = uniform(0);
  const ozz::math::Float4x4 mvp = _view_proj * _model;
  float values[16];
  math::StorePtrU(mvp.cols[0], values + 0);
  math::StorePtrU(mvp.cols[1], values + 4);
  math::StorePtrU(mvp.cols[2], values + 8);
  math::StorePtrU(mvp.cols[3], values + 12);
  GL(UniformMatrix4fv(mvp_uniform, 1, false, values));
}

void ImmediatePCShader::Unbind() {
  UnbindAttribs();
  GL(UseProgram(0));
}

ImmediatePTCShader* ImmediatePTCShader::Build() {
  bool success = true;

  const char* simple_pc_vs =
    "uniform mat4 u_mvp;\n"
    "attribute vec3 a_position;\n"
    "attribute vec2 a_tex_coord;\n"
    "attribute vec4 a_color;\n"
    "varying vec4 v_vertex_color;\n"
    "varying vec2 v_texture_coord;\n"
    "void main() {\n"
    "  vec4 vertex = vec4(a_position.xyz, 1.);\n"
    "  gl_Position = u_mvp * vertex;\n"
    "  v_vertex_color = a_color;\n"
    "  v_texture_coord = a_tex_coord;\n"
    "}\n";
  const char* simple_pc_ps =
    "uniform sampler2D u_texture;\n"
    "varying vec4 v_vertex_color;\n"
    "varying vec2 v_texture_coord;\n"
    "void main() {\n"
    "  vec4 tex_color = texture2D(u_texture, v_texture_coord);\n"
    "  gl_FragColor = v_vertex_color * tex_color;\n"
    "  if(gl_FragColor.a < .01) discard;\n"  // Implements alpha testing.
    "}\n";

  const char* vs[] = {
    kPlatformSpecivicVSHeader,
    simple_pc_vs};
  const char* fs[] = {
    kPlatformSpecivicFSHeader,
    simple_pc_ps};

  ImmediatePTCShader* shader =
    memory::default_allocator()->New<ImmediatePTCShader>();
  success &= shader->BuildFromSource(OZZ_ARRAY_SIZE(vs), vs,
                                     OZZ_ARRAY_SIZE(fs), fs);

  // Binds default attributes
  success &= shader->FindAttrib("a_position");
  success &= shader->FindAttrib("a_tex_coord");
  success &= shader->FindAttrib("a_color");

  // Binds default uniforms
  success &= shader->BindUniform("u_mvp");
  success &= shader->BindUniform("u_texture");

  if (!success) {
    memory::default_allocator()->Delete(shader);
    shader = NULL;
  }

  return shader;
}

void ImmediatePTCShader::Bind(const math::Float4x4& _model,
                              const math::Float4x4& _view_proj,
                              GLsizei _pos_stride, GLsizei _pos_offset,
                              GLsizei _tex_stride, GLsizei _tex_offset,
                              GLsizei _color_stride, GLsizei _color_offset) {

  GL(UseProgram(program()));

  const GLint position_attrib = attrib(0);
  GL(EnableVertexAttribArray(position_attrib));
  GL(VertexAttribPointer(position_attrib, 3, GL_FLOAT, GL_FALSE,
                         _pos_stride, GL_PTR_OFFSET(_pos_offset)));

  const GLint tex_attrib = attrib(1);
  GL(EnableVertexAttribArray(tex_attrib));
  GL(VertexAttribPointer(tex_attrib, 2, GL_FLOAT, GL_FALSE,
                         _tex_stride, GL_PTR_OFFSET(_tex_offset)));

  const GLint color_attrib = attrib(2);
  GL(EnableVertexAttribArray(color_attrib));
  GL(VertexAttribPointer(color_attrib, 4, GL_UNSIGNED_BYTE, GL_TRUE,
                         _color_stride, GL_PTR_OFFSET(_color_offset)));

  // Binds mvp uniform
  const GLint mvp_uniform = uniform(0);
  const ozz::math::Float4x4 mvp = _view_proj * _model;
  float values[16];
  math::StorePtrU(mvp.cols[0], values + 0);
  math::StorePtrU(mvp.cols[1], values + 4);
  math::StorePtrU(mvp.cols[2], values + 8);
  math::StorePtrU(mvp.cols[3], values + 12);
  GL(UniformMatrix4fv(mvp_uniform, 1, false, values));

  // Binds texture
  const GLint texture = uniform(1);
  GL(Uniform1i(texture, 0));
}

void ImmediatePTCShader::Unbind() {
  UnbindAttribs();
  GL(UseProgram(0));
}

void SkeletonShader::Bind(const math::Float4x4& _model,
                          const math::Float4x4& _view_proj,
                          GLsizei _pos_stride, GLsizei _pos_offset,
                          GLsizei _normal_stride, GLsizei _normal_offset,
                          GLsizei _color_stride, GLsizei _color_offset) {
  GL(UseProgram(program()));

  const GLint position_attrib = attrib(0);
  GL(EnableVertexAttribArray(position_attrib));
  GL(VertexAttribPointer(position_attrib, 3, GL_FLOAT, GL_FALSE,
    _pos_stride, GL_PTR_OFFSET(_pos_offset)));

  const GLint normal_attrib = attrib(1);
  GL(EnableVertexAttribArray(normal_attrib));
  GL(VertexAttribPointer(normal_attrib, 3, GL_FLOAT, GL_FALSE,
    _normal_stride, GL_PTR_OFFSET(_normal_offset)));

  const GLint color_attrib = attrib(2);
  GL(EnableVertexAttribArray(color_attrib));
  GL(VertexAttribPointer(color_attrib, 4, GL_UNSIGNED_BYTE, GL_TRUE,
    _color_stride, GL_PTR_OFFSET(_color_offset)));

  // Binds mvp uniform
  const GLint mvp_uniform = uniform(0);
  const ozz::math::Float4x4 mvp = _view_proj * _model;
  float values[16];
  math::StorePtrU(mvp.cols[0], values + 0);
  math::StorePtrU(mvp.cols[1], values + 4);
  math::StorePtrU(mvp.cols[2], values + 8);
  math::StorePtrU(mvp.cols[3], values + 12);
  GL(UniformMatrix4fv(mvp_uniform, 1, false, values));
}

void SkeletonShader::Unbind() {
  UnbindAttribs();
  GL(UseProgram(0));
}

namespace {

  const char* shader_uber_vs =
    "uniform mat4 u_mvp;\n"
    "attribute vec3 a_position;\n"
    "attribute vec3 a_normal;\n"
    "attribute vec4 a_color;\n"
    "varying vec3 v_world_normal;\n"
    "varying vec4 v_vertex_color;\n"
    "void main() {\n"
    "  mat4 world_matrix = GetWorldMatrix();\n"
    "  vec4 vertex = vec4(a_position.xyz, 1.);\n"
    "  gl_Position = u_mvp * world_matrix * vertex;\n"
    "  mat3 cross_matrix = mat3(\n"
    "    cross(world_matrix[1].xyz, world_matrix[2].xyz),\n"
    "    cross(world_matrix[2].xyz, world_matrix[0].xyz),\n"
    "    cross(world_matrix[0].xyz, world_matrix[1].xyz));\n"
    "  float invdet = 1.0 / dot(cross_matrix[2], world_matrix[2].xyz);\n"
    "  mat3 normal_matrix = cross_matrix * invdet;\n"
    "  v_world_normal = normal_matrix * a_normal;\n"
    "  v_vertex_color = a_color;\n"
    "}\n";
  const char* shader_ambient_fs =
    "vec3 lerp(in vec3 alpha, in vec3 a, in vec3 b) {\n"
    "  return a + alpha * (b - a);\n"
    "}\n"
    "vec4 lerp(in vec4 alpha, in vec4 a, in vec4 b) {\n"
    "  return a + alpha * (b - a);\n"
    "}\n"
    "varying vec3 v_world_normal;\n"
    "varying vec4 v_vertex_color;\n"
    "void main() {\n"
    "  vec3 normal = normalize(v_world_normal);\n"
    "  vec3 alpha = (normal + 1.) * .5;\n"
    "  vec4 bt = lerp(\n"
    "    alpha.xzxz, vec4(.3, .3, .7, .7), vec4(.4, .4, .8, .8));\n"
    "  gl_FragColor = vec4(\n"
    "     lerp(alpha.yyy, vec3(bt.x, .3, bt.y), vec3(bt.z, .8, bt.w)), 1.);\n"
    "  gl_FragColor *= v_vertex_color;\n"
    "}\n";
}

JointShader* JointShader::Build() {
  bool success = true;

  const char* vs_joint_to_world_matrix =
    "mat4 GetWorldMatrix() {\n"
    "  // Rebuilds joint matrix.\n"
    "  mat4 joint_matrix;\n"
    "  joint_matrix[0] = vec4(normalize(joint[0].xyz), 0.);\n"
    "  joint_matrix[1] = vec4(normalize(joint[1].xyz), 0.);\n"
    "  joint_matrix[2] = vec4(normalize(joint[2].xyz), 0.);\n"
    "  joint_matrix[3] = vec4(joint[3].xyz, 1.);\n"

    "  // Rebuilds bone properties.\n"
    "  vec3 bone_dir = vec3(joint[0].w, joint[1].w, joint[2].w);\n"
    "  float bone_len = length(bone_dir);\n"

    "  // Setup rendering world matrix.\n"
    "  mat4 world_matrix;\n"
    "  world_matrix[0] = joint_matrix[0] * bone_len;\n"
    "  world_matrix[1] = joint_matrix[1] * bone_len;\n"
    "  world_matrix[2] = joint_matrix[2] * bone_len;\n"
    "  world_matrix[3] = joint_matrix[3];\n"
    "  return world_matrix;\n"
    "}\n";
  const char* vs[] = {
    kPlatformSpecivicVSHeader,
    GL_ARB_instanced_arrays ?
    "attribute mat4 joint;\n" :
    "uniform mat4 joint;\n",  // Simplest fall back.
      vs_joint_to_world_matrix,
      shader_uber_vs};
  const char* fs[] = {
    kPlatformSpecivicFSHeader,
    shader_ambient_fs};

  JointShader* shader = memory::default_allocator()->New<JointShader>();
  success &= shader->BuildFromSource(OZZ_ARRAY_SIZE(vs), vs,
                                     OZZ_ARRAY_SIZE(fs), fs);

  // Binds default attributes
  success &= shader->FindAttrib("a_position");
  success &= shader->FindAttrib("a_normal");
  success &= shader->FindAttrib("a_color");

  // Binds default uniforms
  success &= shader->BindUniform("u_mvp");

  if (GL_ARB_instanced_arrays) {
    success &= shader->FindAttrib("joint");
  } else {
    success &= shader->BindUniform("joint");
  }

  if (!success) {
    memory::default_allocator()->Delete(shader);
    shader = NULL;
  }

  return shader;
}

BoneShader* BoneShader::Build() {  // Builds a world matrix from joint uniforms, sticking bone model between
  bool success = true;

  // parent and child joints.
  const char* vs_joint_to_world_matrix =
    "mat4 GetWorldMatrix() {\n"
    "  // Rebuilds bone properties.\n"
    "  // Bone length is set to zero to disable leaf rendering.\n"
    "  float is_bone = joint[3].w;\n"
    "  vec3 bone_dir = vec3(joint[0].w, joint[1].w, joint[2].w) * is_bone;\n"
    "  float bone_len = length(bone_dir);\n"

    "  // Setup rendering world matrix.\n"
    "  float dot = dot(joint[2].xyz, bone_dir);\n"
    "  vec3 binormal = abs(dot) < .01 ? joint[2].xyz : joint[1].xyz;\n"

    "  mat4 world_matrix;\n"
    "  world_matrix[0] = vec4(bone_dir, 0.);\n"
    "  world_matrix[1] = \n"
    "    vec4(bone_len * normalize(cross(binormal, bone_dir)), 0.);\n"
    "  world_matrix[2] =\n"
    "    vec4(bone_len * normalize(cross(bone_dir, world_matrix[1].xyz)), 0.);\n"
    "  world_matrix[3] = vec4(joint[3].xyz, 1.);\n"
    "  return world_matrix;\n"
    "}\n";
  const char* vs[] = {
    kPlatformSpecivicVSHeader,
    GL_ARB_instanced_arrays ?
    "attribute mat4 joint;\n" :
    "uniform mat4 joint;\n",  // Simplest fall back.
      vs_joint_to_world_matrix,
      shader_uber_vs};
  const char* fs[] = {
    kPlatformSpecivicFSHeader,
    shader_ambient_fs};

  BoneShader* shader = memory::default_allocator()->New<BoneShader>();
  success &= shader->BuildFromSource(OZZ_ARRAY_SIZE(vs), vs,
    OZZ_ARRAY_SIZE(fs), fs);

  // Binds default attributes
  success &= shader->FindAttrib("a_position");
  success &= shader->FindAttrib("a_normal");
  success &= shader->FindAttrib("a_color");

  // Binds default uniforms
  success &= shader->BindUniform("u_mvp");

  if (GL_ARB_instanced_arrays) {
    success &= shader->FindAttrib("joint");
  } else {
    success &= shader->BindUniform("joint");
  }

  if (!success) {
    memory::default_allocator()->Delete(shader);
    shader = NULL;
  }

  return shader;
}

AmbientShader* AmbientShader::Build() {
  bool success = true;

  const char* simple_pc_vs =
    "uniform mat4 u_mvp;\n"
    "attribute vec3 a_position;\n"
    "attribute vec3 a_normal;\n"
    "attribute vec4 a_color;\n"
    "varying vec3 v_world_normal;\n"
    "varying vec4 v_vertex_color;\n"
    "void main() {\n"
    "  vec4 vertex = vec4(a_position.xyz, 1.);\n"
    "  gl_Position = u_mvp * vertex;\n"
    "  v_world_normal = a_normal;\n"
    "  v_vertex_color = a_color;\n"
    "}\n";

  const char* vs[] = {
    kPlatformSpecivicVSHeader,
    simple_pc_vs};
  const char* fs[] = {
    kPlatformSpecivicFSHeader,
    shader_ambient_fs};

  AmbientShader* shader =
    memory::default_allocator()->New<AmbientShader>();
  success &= shader->BuildFromSource(OZZ_ARRAY_SIZE(vs), vs,
                                     OZZ_ARRAY_SIZE(fs), fs);

  // Binds default attributes
  success &= shader->FindAttrib("a_position");
  success &= shader->FindAttrib("a_normal");
  success &= shader->FindAttrib("a_color");

  // Binds default uniforms
  success &= shader->BindUniform("u_mvp");

  if (!success) {
    memory::default_allocator()->Delete(shader);
    shader = NULL;
  }

  return shader;
}

void AmbientShader::Bind(const math::Float4x4& _model,
                         const math::Float4x4& _view_proj,
                         GLsizei _pos_stride, GLsizei _pos_offset,
                         GLsizei _normal_stride, GLsizei _normal_offset,
                         GLsizei _color_stride, GLsizei _color_offset) {
  GL(UseProgram(program()));

  const GLint position_attrib = attrib(0);
  GL(EnableVertexAttribArray(position_attrib));
  GL(VertexAttribPointer(position_attrib, 3, GL_FLOAT, GL_FALSE,
    _pos_stride, GL_PTR_OFFSET(_pos_offset)));

  const GLint normal_attrib = attrib(1);
  GL(EnableVertexAttribArray(normal_attrib));
  GL(VertexAttribPointer(normal_attrib, 3, GL_FLOAT, GL_TRUE,
    _normal_stride, GL_PTR_OFFSET(_normal_offset)));

  const GLint color_attrib = attrib(2);
  GL(EnableVertexAttribArray(color_attrib));
  GL(VertexAttribPointer(color_attrib, 4, GL_UNSIGNED_BYTE, GL_TRUE,
    _color_stride, GL_PTR_OFFSET(_color_offset)));

  // Binds mvp uniform
  const GLint mvp_uniform = uniform(0);
  const ozz::math::Float4x4 mvp = _view_proj * _model;
  float values[16];
  math::StorePtrU(mvp.cols[0], values + 0);
  math::StorePtrU(mvp.cols[1], values + 4);
  math::StorePtrU(mvp.cols[2], values + 8);
  math::StorePtrU(mvp.cols[3], values + 12);
  GL(UniformMatrix4fv(mvp_uniform, 1, false, values));
}

void AmbientShader::Unbind() {
  UnbindAttribs();
  GL(UseProgram(0));
}
}  // internal
}  // sample
}  // ozz
