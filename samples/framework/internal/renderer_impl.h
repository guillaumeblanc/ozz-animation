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

#ifndef OZZ_SAMPLES_FRAMEWORK_INTERNAL_RENDERER_IMPL_H_
#define OZZ_SAMPLES_FRAMEWORK_INTERNAL_RENDERER_IMPL_H_

#ifndef OZZ_INCLUDE_PRIVATE_HEADER
#error "This header is private, it cannot be included from public headers."
#endif  // OZZ_INCLUDE_PRIVATE_HEADER

// GL and GL ext requires that ptrdif_t is defined on APPLE platforms.
#include <cstddef>

// Don't allow gl.h to automatically include glext.h
#define GL_GLEXT_LEGACY

// Including glfw includes gl.h
#include "GL/glfw.h"

// Detects already defined GL_VERSION and deduces required extensions.
#ifndef GL_VERSION_1_5
#define OZZ_GL_VERSION_1_5_EXT
#endif  // GL_VERSION_1_5
#ifndef GL_VERSION_2_0
#define OZZ_GL_VERSION_2_0_EXT
#endif  // GL_VERSION_2_0

#include "GL/glext.h"

#include "framework/renderer.h"
#include "ozz/base/containers/vector.h"

// Provides helper macro to test for glGetError on a gl call.
#ifndef NDEBUG
#define GL(_f) do{\
  gl##_f;\
  GLenum error = glGetError();\
  assert(error == GL_NO_ERROR);\
} while(void(0), 0)
#else  // NDEBUG
#define GL(_f) gl##_f
#endif // NDEBUG

// Convenient macro definition for specifying buffer offsets.
#define GL_PTR_OFFSET(i) reinterpret_cast<void*>(i)

namespace ozz {
namespace animation { class Skeleton; }
namespace math { struct Float4x4; }
namespace sample {
namespace internal {

// Declares a shader program.
class Shader {
 public:
  // Constructs a shader from _vertex and _fragment glsl sources.
  // Mutliple source files can be specified using the *count argument.
  // Returns NULL if shader compilation failed or a valid Shader pointer on
  // success. The shader must then be deleted using default allocator Delete
  // function.
  static Shader* Build(int _vertex_count, const char** _vertex,
                       int _fragment_count, const char** _fragment);

  // Construct a fixed function pipeline shader. Use Shader::Build to specify
  // shader sources.
  Shader();

  // Destruct a shader.
  ~Shader();

  // Returns the shader program that can be bound to the OpenGL context.
  GLuint program() const {
    return program_;
  }

  // Request an uniform location and pushes it to the uniform stack.
  // The uniform location is then accessible thought uniform().
  bool BindUniform(const char* _semantic);

  // Get an uniform location from the stack at index _index.
  GLint uniform(int _index) const {
    return uniforms_[_index];
  }

  // Request an attribute location and pushes it to the uniform stack.
  // The varying location is then accessible thought uniform().
  bool BindAttrib(const char* _semantic);

  // Get an varying location from the stack at index _index.
  GLint attrib(int _index) const {
    return attribs_[_index];
  }

 private:

  // Compiles a shader from string.
  // Returns a valid shader handle on success. Dumps compiler output on error
  // and return 0.
  static GLuint CompileShader(GLenum _type, int _count, const char** _src);

  // Shader program
  GLuint program_;

  // Vertex and fragment shaders
  GLuint vertex_;
  GLuint fragment_;

  // Uniform locations, in the order they were requested.
  ozz::Vector<GLint>::Std uniforms_;

  // Varying locations, in the order they were requested.
  ozz::Vector<GLint>::Std attribs_;
};

// Implements Renderer interface.
class RendererImpl : public Renderer {
 public:

  RendererImpl();
  virtual ~RendererImpl();

  // See Renderer for all the details about the API.
  virtual bool Initialize();

  virtual void DrawAxes(const ozz::math::Float4x4& _transform);

  virtual void DrawGrid(int _cell_count, float _cell_size);

  virtual bool DrawSkeleton(const animation::Skeleton& _skeleton,
                            const ozz::math::Float4x4& _transform,
                            bool _draw_joints);

  virtual bool DrawPosture(const animation::Skeleton& _skeleton,
                           ozz::Range<const ozz::math::Float4x4> _matrices,
                           const ozz::math::Float4x4& _transform,
                           bool _draw_joints);

  virtual bool DrawBox(const ozz::math::Box& _box,
                       const ozz::math::Float4x4& _transform,
                       const Color _colors[2]);

 private:

  // Defines the internal structure used to define a model.
  struct Model {
    Model();
    ~Model();

    GLuint vbo;
    GLenum mode;
    GLsizei count;
    Shader* shader;
  };

  // Detects and initializes all OpenGL extension.
  // Return true if all mandatory extensions were found.
  bool InitOpenGLExtensions();

  // Initializes posture rendering.
  // Return true if initialization succeeded.
  bool InitPostureRendering();

  // Draw posture internal non-instanced rendering fall back implementation.
  void DrawPosture_Impl(int _instance_count, bool _draw_joints);

  // Draw posture internal instanced rendering implementation.
  void DrawPosture_InstancedImpl(int _instance_count, bool _draw_joints);

  // Array of matrices used to store model space matrices during DrawSkeleton
  // execution.
  ozz::Range<ozz::math::Float4x4> prealloc_models_;

  // The maximum number of pieces needed to render a skeleton.
  int max_skeleton_pieces_;

  // Array of floats used to store shader uniforms while rendering a skeleton
  // without gl instancing extension.
  float* prealloc_uniforms_;

  // Bone and joint model objects.
  Model models_[2];

  // Dynamic vbo used for joint's pre-instance data.
  GLuint joint_instance_vbo_;
};
}  // internal
}  // sample
}  // ozz

// OpenGL 1.5 buffer object management functions, mandatory.
#ifdef OZZ_GL_VERSION_1_5_EXT
extern PFNGLBINDBUFFERPROC glBindBuffer;
extern PFNGLDELETEBUFFERSPROC glDeleteBuffers;
extern PFNGLGENBUFFERSPROC glGenBuffers;
extern PFNGLISBUFFERPROC glIsBuffer;
extern PFNGLBUFFERDATAPROC glBufferData;
extern PFNGLBUFFERSUBDATAPROC glBufferSubData;
extern PFNGLGETBUFFERSUBDATAPROC glGetBufferSubData;
extern PFNGLMAPBUFFERPROC glMapBuffer;
extern PFNGLUNMAPBUFFERPROC glUnmapBuffer;
extern PFNGLGETBUFFERPARAMETERIVPROC glGetBufferParameteriv;
extern PFNGLGETBUFFERPOINTERVPROC glGetBufferPointerv;
#endif  // OZZ_GL_VERSION_1_5_EXT

// OpenGL 2.0 shader management functions, mandatory.
#ifdef OZZ_GL_VERSION_2_0_EXT
extern PFNGLATTACHSHADERPROC glAttachShader;
extern PFNGLBINDATTRIBLOCATIONPROC glBindAttribLocation;
extern PFNGLCOMPILESHADERPROC glCompileShader;
extern PFNGLCREATEPROGRAMPROC glCreateProgram;
extern PFNGLCREATESHADERPROC glCreateShader;
extern PFNGLDELETEPROGRAMPROC glDeleteProgram;
extern PFNGLDELETESHADERPROC glDeleteShader;
extern PFNGLDETACHSHADERPROC glDetachShader;
extern PFNGLDISABLEVERTEXATTRIBARRAYPROC glDisableVertexAttribArray;
extern PFNGLENABLEVERTEXATTRIBARRAYPROC glEnableVertexAttribArray;
extern PFNGLGETACTIVEATTRIBPROC glGetActiveAttrib;
extern PFNGLGETACTIVEUNIFORMPROC glGetActiveUniform;
extern PFNGLGETATTACHEDSHADERSPROC glGetAttachedShaders;
extern PFNGLGETATTRIBLOCATIONPROC glGetAttribLocation;
extern PFNGLGETPROGRAMIVPROC glGetProgramiv;
extern PFNGLGETPROGRAMINFOLOGPROC glGetProgramInfoLog;
extern PFNGLGETSHADERIVPROC glGetShaderiv;
extern PFNGLGETSHADERINFOLOGPROC glGetShaderInfoLog;
extern PFNGLGETSHADERSOURCEPROC glGetShaderSource;
extern PFNGLGETUNIFORMLOCATIONPROC glGetUniformLocation;
extern PFNGLGETUNIFORMFVPROC glGetUniformfv;
extern PFNGLGETUNIFORMIVPROC glGetUniformiv;
extern PFNGLGETVERTEXATTRIBDVPROC glGetVertexAttribdv;
extern PFNGLGETVERTEXATTRIBFVPROC glGetVertexAttribfv;
extern PFNGLGETVERTEXATTRIBIVPROC glGetVertexAttribiv;
extern PFNGLGETVERTEXATTRIBPOINTERVPROC glGetVertexAttribPointerv;
extern PFNGLISPROGRAMPROC glIsProgram;
extern PFNGLISSHADERPROC glIsShader;
extern PFNGLLINKPROGRAMPROC glLinkProgram;
extern PFNGLSHADERSOURCEPROC glShaderSource;
extern PFNGLUSEPROGRAMPROC glUseProgram;
extern PFNGLUNIFORM1FPROC glUniform1f;
extern PFNGLUNIFORM2FPROC glUniform2f;
extern PFNGLUNIFORM3FPROC glUniform3f;
extern PFNGLUNIFORM4FPROC glUniform4f;
extern PFNGLUNIFORM1FVPROC glUniform1fv;
extern PFNGLUNIFORM2FVPROC glUniform2fv;
extern PFNGLUNIFORM3FVPROC glUniform3fv;
extern PFNGLUNIFORM4FVPROC glUniform4fv;
extern PFNGLUNIFORMMATRIX2FVPROC glUniformMatrix2fv;
extern PFNGLUNIFORMMATRIX3FVPROC glUniformMatrix3fv;
extern PFNGLUNIFORMMATRIX4FVPROC glUniformMatrix4fv;
extern PFNGLVALIDATEPROGRAMPROC glValidateProgram;
extern PFNGLVERTEXATTRIB1FPROC glVertexAttrib1f;
extern PFNGLVERTEXATTRIB1FVPROC glVertexAttrib1fv;
extern PFNGLVERTEXATTRIB2FPROC glVertexAttrib2f;
extern PFNGLVERTEXATTRIB2FVPROC glVertexAttrib2fv;
extern PFNGLVERTEXATTRIB3FPROC glVertexAttrib3f;
extern PFNGLVERTEXATTRIB3FVPROC glVertexAttrib3fv;
extern PFNGLVERTEXATTRIB4FPROC glVertexAttrib4f;
extern PFNGLVERTEXATTRIB4FVPROC glVertexAttrib4fv;
extern PFNGLVERTEXATTRIBPOINTERPROC glVertexAttribPointer;
#endif  // OZZ_GL_VERSION_2_0_EXT

// OpenGL ARB_instanced_arrays extension, optional.
#undef GL_ARB_instanced_arrays
extern bool GL_ARB_instanced_arrays;
extern PFNGLVERTEXATTRIBDIVISORARBPROC glVertexAttribDivisorARB;
extern PFNGLDRAWARRAYSINSTANCEDARBPROC glDrawArraysInstancedARB;
extern PFNGLDRAWELEMENTSINSTANCEDARBPROC glDrawElementsInstancedARB;
#endif  // OZZ_SAMPLES_FRAMEWORK_INTERNAL_RENDERER_IMPL_H_
