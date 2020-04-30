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

#include "shader.h"

#include <cassert>
#include <cstdio>
#include <limits>

#include "ozz/base/log.h"
#include "ozz/base/maths/simd_math.h"

namespace ozz {
namespace sample {
namespace internal {

#ifdef EMSCRIPTEN
// WebGL requires to specify floating point precision
static const char* kPlatformSpecivicVSHeader = "precision mediump float;\n";
static const char* kPlatformSpecivicFSHeader = "precision mediump float;\n";
#else   // EMSCRIPTEN
static const char* kPlatformSpecivicVSHeader = "";
static const char* kPlatformSpecivicFSHeader = "";
#endif  // EMSCRIPTEN

Shader::Shader() : program_(0), vertex_(0), fragment_(0) {}

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
  GL(ShaderSource(shader, _count, _src, nullptr));
  GL(CompileShader(shader));

  int infolog_length = 0;
  GL(GetShaderiv(shader, GL_INFO_LOG_LENGTH, &infolog_length));
  if (infolog_length > 1) {
    char* info_log = reinterpret_cast<char*>(
        memory::default_allocator()->Allocate(infolog_length, alignof(char)));
    int chars_written = 0;
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
}  // namespace

bool Shader::BuildFromSource(int _vertex_count, const char** _vertex,
                             int _fragment_count, const char** _fragment) {
  // Tries to compile shaders.
  GLuint vertex_shader = 0;
  if (_vertex) {
    vertex_shader = CompileShader(GL_VERTEX_SHADER, _vertex_count, _vertex);
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
    char* info_log = reinterpret_cast<char*>(
        memory::default_allocator()->Allocate(infolog_length, alignof(char)));
    int chars_written = 0;
    glGetProgramInfoLog(program_, infolog_length, &chars_written, info_log);
    log::Err() << info_log << std::endl;
    memory::default_allocator()->Deallocate(info_log);
  }

  return true;
}

bool Shader::BindUniform(const char* _semantic) {
  if (!program_) {
    return false;
  }
  GLint location = glGetUniformLocation(program_, _semantic);
  if (glGetError() != GL_NO_ERROR || location == -1) {  // _semantic not found.
    return false;
  }
  uniforms_.push_back(location);
  return true;
}

bool Shader::FindAttrib(const char* _semantic) {
  if (!program_) {
    return false;
  }
  GLint location = glGetAttribLocation(program_, _semantic);
  if (glGetError() != GL_NO_ERROR || location == -1) {  // _semantic not found.
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

void Shader::Unbind() {
  UnbindAttribs();
  GL(UseProgram(0));
}

ozz::unique_ptr<ImmediatePCShader> ImmediatePCShader::Build() {
  bool success = true;

  const char* kSimplePCVS =
      "uniform mat4 u_mvp;\n"
      "attribute vec3 a_position;\n"
      "attribute vec4 a_color;\n"
      "varying vec4 v_vertex_color;\n"
      "void main() {\n"
      "  vec4 vertex = vec4(a_position.xyz, 1.);\n"
      "  gl_Position = u_mvp * vertex;\n"
      "  v_vertex_color = a_color;\n"
      "}\n";
  const char* kSimplePCPS =
      "varying vec4 v_vertex_color;\n"
      "void main() {\n"
      "  gl_FragColor = v_vertex_color;\n"
      "}\n";

  const char* vs[] = {kPlatformSpecivicVSHeader, kSimplePCVS};
  const char* fs[] = {kPlatformSpecivicFSHeader, kSimplePCPS};

  ozz::unique_ptr<ImmediatePCShader> shader = make_unique<ImmediatePCShader>();
  success &=
      shader->BuildFromSource(OZZ_ARRAY_SIZE(vs), vs, OZZ_ARRAY_SIZE(fs), fs);

  // Binds default attributes
  success &= shader->FindAttrib("a_position");
  success &= shader->FindAttrib("a_color");

  // Binds default uniforms
  success &= shader->BindUniform("u_mvp");

  if (!success) {
    shader.reset();
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
  GL(VertexAttribPointer(position_attrib, 3, GL_FLOAT, GL_FALSE, _pos_stride,
                         GL_PTR_OFFSET(_pos_offset)));

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

ozz::unique_ptr<ImmediatePTCShader> ImmediatePTCShader::Build() {
  bool success = true;

  const char* kSimplePCVS =
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
  const char* kSimplePCPS =
      "uniform sampler2D u_texture;\n"
      "varying vec4 v_vertex_color;\n"
      "varying vec2 v_texture_coord;\n"
      "void main() {\n"
      "  vec4 tex_color = texture2D(u_texture, v_texture_coord);\n"
      "  gl_FragColor = v_vertex_color * tex_color;\n"
      "  if(gl_FragColor.a < .01) discard;\n"  // Implements alpha testing.
      "}\n";

  const char* vs[] = {kPlatformSpecivicVSHeader, kSimplePCVS};
  const char* fs[] = {kPlatformSpecivicFSHeader, kSimplePCPS};

  ozz::unique_ptr<ImmediatePTCShader> shader =
      make_unique<ImmediatePTCShader>();
  success &=
      shader->BuildFromSource(OZZ_ARRAY_SIZE(vs), vs, OZZ_ARRAY_SIZE(fs), fs);

  // Binds default attributes
  success &= shader->FindAttrib("a_position");
  success &= shader->FindAttrib("a_tex_coord");
  success &= shader->FindAttrib("a_color");

  // Binds default uniforms
  success &= shader->BindUniform("u_mvp");
  success &= shader->BindUniform("u_texture");

  if (!success) {
    shader.reset();
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
  GL(VertexAttribPointer(position_attrib, 3, GL_FLOAT, GL_FALSE, _pos_stride,
                         GL_PTR_OFFSET(_pos_offset)));

  const GLint tex_attrib = attrib(1);
  GL(EnableVertexAttribArray(tex_attrib));
  GL(VertexAttribPointer(tex_attrib, 2, GL_FLOAT, GL_FALSE, _tex_stride,
                         GL_PTR_OFFSET(_tex_offset)));

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

namespace {
const char* kPassUv =
    "attribute vec2 a_uv;\n"
    "varying vec2 v_vertex_uv;\n"
    "void PassUv() {\n"
    "  v_vertex_uv = a_uv;\n"
    "}\n";
const char* kPassNoUv =
    "void PassUv() {\n"
    "}\n";
const char* kShaderUberVS =
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
    "  PassUv();\n"
    "}\n";
const char* kShaderAmbientFct =
    "vec4 GetAmbient(vec3 _world_normal) {\n"
    "  vec3 normal = normalize(_world_normal);\n"
    "  vec3 alpha = (normal + 1.) * .5;\n"
    "  vec2 bt = mix(vec2(.3, .7), vec2(.4, .8), alpha.xz);\n"
    "  vec3 ambient = mix(vec3(bt.x, .3, bt.x), vec3(bt.y, .8, bt.y), "
    "alpha.y);\n"
    "  return vec4(ambient, 1.);\n"
    "}\n";
const char* kShaderAmbientFS =
    "varying vec3 v_world_normal;\n"
    "varying vec4 v_vertex_color;\n"
    "void main() {\n"
    "  vec4 ambient = GetAmbient(v_world_normal);\n"
    "  gl_FragColor = ambient *\n"
    "                 v_vertex_color;\n"
    "}\n";
const char* kShaderAmbientTexturedFS =
    "uniform sampler2D u_texture;\n"
    "varying vec3 v_world_normal;\n"
    "varying vec4 v_vertex_color;\n"
    "varying vec2 v_vertex_uv;\n"
    "void main() {\n"
    "  vec4 ambient = GetAmbient(v_world_normal);\n"
    "  gl_FragColor = ambient *\n"
    "                 v_vertex_color *\n"
    "                 texture2D(u_texture, v_vertex_uv);\n"
    "}\n";
}  // namespace

void SkeletonShader::Bind(const math::Float4x4& _model,
                          const math::Float4x4& _view_proj, GLsizei _pos_stride,
                          GLsizei _pos_offset, GLsizei _normal_stride,
                          GLsizei _normal_offset, GLsizei _color_stride,
                          GLsizei _color_offset) {
  GL(UseProgram(program()));

  const GLint position_attrib = attrib(0);
  GL(EnableVertexAttribArray(position_attrib));
  GL(VertexAttribPointer(position_attrib, 3, GL_FLOAT, GL_FALSE, _pos_stride,
                         GL_PTR_OFFSET(_pos_offset)));

  const GLint normal_attrib = attrib(1);
  GL(EnableVertexAttribArray(normal_attrib));
  GL(VertexAttribPointer(normal_attrib, 3, GL_FLOAT, GL_FALSE, _normal_stride,
                         GL_PTR_OFFSET(_normal_offset)));

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

ozz::unique_ptr<JointShader> JointShader::Build() {
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
  const char* vs[] = {kPlatformSpecivicVSHeader, kPassNoUv,
                      GL_ARB_instanced_arrays_supported
                          ? "attribute mat4 joint;\n"
                          : "uniform mat4 joint;\n",
                      vs_joint_to_world_matrix, kShaderUberVS};
  const char* fs[] = {kPlatformSpecivicFSHeader, kShaderAmbientFct,
                      kShaderAmbientFS};

  ozz::unique_ptr<JointShader> shader = make_unique<JointShader>();
  success &=
      shader->BuildFromSource(OZZ_ARRAY_SIZE(vs), vs, OZZ_ARRAY_SIZE(fs), fs);

  // Binds default attributes
  success &= shader->FindAttrib("a_position");
  success &= shader->FindAttrib("a_normal");
  success &= shader->FindAttrib("a_color");

  // Binds default uniforms
  success &= shader->BindUniform("u_mvp");

  if (GL_ARB_instanced_arrays_supported) {
    success &= shader->FindAttrib("joint");
  } else {
    success &= shader->BindUniform("joint");
  }

  if (!success) {
    shader.reset();
  }

  return shader;
}

ozz::unique_ptr<BoneShader>
BoneShader::Build() {  // Builds a world matrix from joint uniforms,
                       // sticking bone model between
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
      "  float dot1 = dot(joint[2].xyz, bone_dir);\n"
      "  float dot2 = dot(joint[0].xyz, bone_dir);\n"
      "  vec3 binormal = abs(dot1) < abs(dot2) ? joint[2].xyz : joint[0].xyz;\n"

      "  mat4 world_matrix;\n"
      "  world_matrix[0] = vec4(bone_dir, 0.);\n"
      "  world_matrix[1] = \n"
      "    vec4(bone_len * normalize(cross(binormal, bone_dir)), 0.);\n"
      "  world_matrix[2] =\n"
      "    vec4(bone_len * normalize(cross(bone_dir, world_matrix[1].xyz)), "
      "0.);\n"
      "  world_matrix[3] = vec4(joint[3].xyz, 1.);\n"
      "  return world_matrix;\n"
      "}\n";
  const char* vs[] = {kPlatformSpecivicVSHeader, kPassNoUv,
                      GL_ARB_instanced_arrays_supported
                          ? "attribute mat4 joint;\n"
                          : "uniform mat4 joint;\n",
                      vs_joint_to_world_matrix, kShaderUberVS};
  const char* fs[] = {kPlatformSpecivicFSHeader, kShaderAmbientFct,
                      kShaderAmbientFS};

  ozz::unique_ptr<BoneShader> shader = make_unique<BoneShader>();
  success &=
      shader->BuildFromSource(OZZ_ARRAY_SIZE(vs), vs, OZZ_ARRAY_SIZE(fs), fs);

  // Binds default attributes
  success &= shader->FindAttrib("a_position");
  success &= shader->FindAttrib("a_normal");
  success &= shader->FindAttrib("a_color");

  // Binds default uniforms
  success &= shader->BindUniform("u_mvp");

  if (GL_ARB_instanced_arrays_supported) {
    success &= shader->FindAttrib("joint");
  } else {
    success &= shader->BindUniform("joint");
  }

  if (!success) {
    shader.reset();
  }

  return shader;
}

ozz::unique_ptr<AmbientShader> AmbientShader::Build() {
  const char* vs[] = {
      kPlatformSpecivicVSHeader, kPassNoUv,
      "uniform mat4 u_mw;\n mat4 GetWorldMatrix() {return u_mw;}\n",
      kShaderUberVS};
  const char* fs[] = {kPlatformSpecivicFSHeader, kShaderAmbientFct,
                      kShaderAmbientFS};

  ozz::unique_ptr<AmbientShader> shader = make_unique<AmbientShader>();
  bool success =
      shader->InternalBuild(OZZ_ARRAY_SIZE(vs), vs, OZZ_ARRAY_SIZE(fs), fs);

  if (!success) {
    shader.reset();
  }

  return shader;
}

bool AmbientShader::InternalBuild(int _vertex_count, const char** _vertex,
                                  int _fragment_count, const char** _fragment) {
  bool success = true;

  success &=
      BuildFromSource(_vertex_count, _vertex, _fragment_count, _fragment);

  // Binds default attributes
  success &= FindAttrib("a_position");
  success &= FindAttrib("a_normal");
  success &= FindAttrib("a_color");

  // Binds default uniforms
  success &= BindUniform("u_mw");
  success &= BindUniform("u_mvp");

  return success;
}

void AmbientShader::Bind(const math::Float4x4& _model,
                         const math::Float4x4& _view_proj, GLsizei _pos_stride,
                         GLsizei _pos_offset, GLsizei _normal_stride,
                         GLsizei _normal_offset, GLsizei _color_stride,
                         GLsizei _color_offset) {
  GL(UseProgram(program()));

  const GLint position_attrib = attrib(0);
  GL(EnableVertexAttribArray(position_attrib));
  GL(VertexAttribPointer(position_attrib, 3, GL_FLOAT, GL_FALSE, _pos_stride,
                         GL_PTR_OFFSET(_pos_offset)));

  const GLint normal_attrib = attrib(1);
  GL(EnableVertexAttribArray(normal_attrib));
  GL(VertexAttribPointer(normal_attrib, 3, GL_FLOAT, GL_TRUE, _normal_stride,
                         GL_PTR_OFFSET(_normal_offset)));

  const GLint color_attrib = attrib(2);
  GL(EnableVertexAttribArray(color_attrib));
  GL(VertexAttribPointer(color_attrib, 4, GL_UNSIGNED_BYTE, GL_TRUE,
                         _color_stride, GL_PTR_OFFSET(_color_offset)));

  // Binds mw uniform
  float values[16];
  const GLint mw_uniform = uniform(0);
  math::StorePtrU(_model.cols[0], values + 0);
  math::StorePtrU(_model.cols[1], values + 4);
  math::StorePtrU(_model.cols[2], values + 8);
  math::StorePtrU(_model.cols[3], values + 12);
  GL(UniformMatrix4fv(mw_uniform, 1, false, values));

  // Binds mvp uniform
  const GLint mvp_uniform = uniform(1);
  math::StorePtrU(_view_proj.cols[0], values + 0);
  math::StorePtrU(_view_proj.cols[1], values + 4);
  math::StorePtrU(_view_proj.cols[2], values + 8);
  math::StorePtrU(_view_proj.cols[3], values + 12);
  GL(UniformMatrix4fv(mvp_uniform, 1, false, values));
}

ozz::unique_ptr<AmbientShaderInstanced> AmbientShaderInstanced::Build() {
  bool success = true;

  const char* vs[] = {
      kPlatformSpecivicVSHeader, kPassNoUv,
      "attribute mat4 a_mw;\n mat4 GetWorldMatrix() {return a_mw;}\n",
      kShaderUberVS};
  const char* fs[] = {kPlatformSpecivicFSHeader, kShaderAmbientFct,
                      kShaderAmbientFS};

  ozz::unique_ptr<AmbientShaderInstanced> shader =
      make_unique<AmbientShaderInstanced>();
  success &=
      shader->BuildFromSource(OZZ_ARRAY_SIZE(vs), vs, OZZ_ARRAY_SIZE(fs), fs);

  // Binds default attributes
  success &= shader->FindAttrib("a_position");
  success &= shader->FindAttrib("a_normal");
  success &= shader->FindAttrib("a_color");
  success &= shader->FindAttrib("a_mw");

  // Binds default uniforms
  success &= shader->BindUniform("u_mvp");

  if (!success) {
    shader.reset();
  }

  return shader;
}

void AmbientShaderInstanced::Bind(GLsizei _models_offset,
                                  const math::Float4x4& _view_proj,
                                  GLsizei _pos_stride, GLsizei _pos_offset,
                                  GLsizei _normal_stride,
                                  GLsizei _normal_offset, GLsizei _color_stride,
                                  GLsizei _color_offset) {
  GL(UseProgram(program()));

  const GLint position_attrib = attrib(0);
  GL(EnableVertexAttribArray(position_attrib));
  GL(VertexAttribPointer(position_attrib, 3, GL_FLOAT, GL_FALSE, _pos_stride,
                         GL_PTR_OFFSET(_pos_offset)));

  const GLint normal_attrib = attrib(1);
  GL(EnableVertexAttribArray(normal_attrib));
  GL(VertexAttribPointer(normal_attrib, 3, GL_FLOAT, GL_TRUE, _normal_stride,
                         GL_PTR_OFFSET(_normal_offset)));

  const GLint color_attrib = attrib(2);
  GL(EnableVertexAttribArray(color_attrib));
  GL(VertexAttribPointer(color_attrib, 4, GL_UNSIGNED_BYTE, GL_TRUE,
                         _color_stride, GL_PTR_OFFSET(_color_offset)));
  if (_color_stride == 0) {
    GL(VertexAttribDivisor_(color_attrib,
                            std::numeric_limits<unsigned int>::max()));
  }

  // Binds mw uniform
  const GLint models_attrib = attrib(3);
  GL(EnableVertexAttribArray(models_attrib + 0));
  GL(EnableVertexAttribArray(models_attrib + 1));
  GL(EnableVertexAttribArray(models_attrib + 2));
  GL(EnableVertexAttribArray(models_attrib + 3));
  GL(VertexAttribDivisor_(models_attrib + 0, 1));
  GL(VertexAttribDivisor_(models_attrib + 1, 1));
  GL(VertexAttribDivisor_(models_attrib + 2, 1));
  GL(VertexAttribDivisor_(models_attrib + 3, 1));
  GL(VertexAttribPointer(models_attrib + 0, 4, GL_FLOAT, GL_FALSE,
                         sizeof(math::Float4x4),
                         GL_PTR_OFFSET(0 + _models_offset)));
  GL(VertexAttribPointer(models_attrib + 1, 4, GL_FLOAT, GL_FALSE,
                         sizeof(math::Float4x4),
                         GL_PTR_OFFSET(16 + _models_offset)));
  GL(VertexAttribPointer(models_attrib + 2, 4, GL_FLOAT, GL_FALSE,
                         sizeof(math::Float4x4),
                         GL_PTR_OFFSET(32 + _models_offset)));
  GL(VertexAttribPointer(models_attrib + 3, 4, GL_FLOAT, GL_FALSE,
                         sizeof(math::Float4x4),
                         GL_PTR_OFFSET(48 + _models_offset)));

  // Binds mvp uniform
  const GLint mvp_uniform = uniform(0);
  float values[16];
  math::StorePtrU(_view_proj.cols[0], values + 0);
  math::StorePtrU(_view_proj.cols[1], values + 4);
  math::StorePtrU(_view_proj.cols[2], values + 8);
  math::StorePtrU(_view_proj.cols[3], values + 12);
  GL(UniformMatrix4fv(mvp_uniform, 1, false, values));
}

void AmbientShaderInstanced::Unbind() {
  const GLint color_attrib = attrib(2);
  GL(VertexAttribDivisor_(color_attrib, 0));

  const GLint models_attrib = attrib(3);
  GL(DisableVertexAttribArray(models_attrib + 0));
  GL(DisableVertexAttribArray(models_attrib + 1));
  GL(DisableVertexAttribArray(models_attrib + 2));
  GL(DisableVertexAttribArray(models_attrib + 3));
  GL(VertexAttribDivisor_(models_attrib + 0, 0));
  GL(VertexAttribDivisor_(models_attrib + 1, 0));
  GL(VertexAttribDivisor_(models_attrib + 2, 0));
  GL(VertexAttribDivisor_(models_attrib + 3, 0));
  Shader::Unbind();
}

ozz::unique_ptr<AmbientTexturedShader> AmbientTexturedShader::Build() {
  const char* vs[] = {
      kPlatformSpecivicVSHeader, kPassUv,
      "uniform mat4 u_mw;\n mat4 GetWorldMatrix() {return u_mw;}\n",
      kShaderUberVS};
  const char* fs[] = {kPlatformSpecivicFSHeader, kShaderAmbientFct,
                      kShaderAmbientTexturedFS};

  ozz::unique_ptr<AmbientTexturedShader> shader =
      make_unique<AmbientTexturedShader>();
  bool success =
      shader->InternalBuild(OZZ_ARRAY_SIZE(vs), vs, OZZ_ARRAY_SIZE(fs), fs);

  success &= shader->FindAttrib("a_uv");

  if (!success) {
    shader.reset();
  }

  return shader;
}

void AmbientTexturedShader::Bind(const math::Float4x4& _model,
                                 const math::Float4x4& _view_proj,
                                 GLsizei _pos_stride, GLsizei _pos_offset,
                                 GLsizei _normal_stride, GLsizei _normal_offset,
                                 GLsizei _color_stride, GLsizei _color_offset,
                                 GLsizei _uv_stride, GLsizei _uv_offset) {
  AmbientShader::Bind(_model, _view_proj, _pos_stride, _pos_offset,
                      _normal_stride, _normal_offset, _color_stride,
                      _color_offset);

  const GLint uv_attrib = attrib(3);
  GL(EnableVertexAttribArray(uv_attrib));
  GL(VertexAttribPointer(uv_attrib, 2, GL_FLOAT, GL_FALSE, _uv_stride,
                         GL_PTR_OFFSET(_uv_offset)));
}
}  // namespace internal
}  // namespace sample
}  // namespace ozz
