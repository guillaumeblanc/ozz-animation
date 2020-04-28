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

#ifdef EMSCRIPTEN
// include features as core functions.
#include <GLES2/gl2.h>

#else  // EMSCRIPTEN

// Detects already defined GL_VERSION and deduces required extensions.
#ifndef GL_VERSION_1_5
#define OZZ_GL_VERSION_1_5_EXT
#endif  // GL_VERSION_1_5
#ifndef GL_VERSION_2_0
#define OZZ_GL_VERSION_2_0_EXT
#endif  // GL_VERSION_2_0

#endif  // EMSCRIPTEN

// Include features as extentions
#include "GL/glext.h"

#include "framework/renderer.h"
#include "ozz/base/containers/vector.h"
#include "ozz/base/memory/unique_ptr.h"

// Provides helper macro to test for glGetError on a gl call.
#ifndef NDEBUG
#define GL(_f)                    \
  do {                            \
    gl##_f;                       \
    GLenum error = glGetError();  \
    assert(error == GL_NO_ERROR); \
                                  \
  } while (void(0), 0)
#else  // NDEBUG
#define GL(_f) gl##_f
#endif  // NDEBUG

// Convenient macro definition for specifying buffer offsets.
#define GL_PTR_OFFSET(i) reinterpret_cast<void*>(static_cast<intptr_t>(i))

namespace ozz {
namespace animation {
class Skeleton;
}
namespace math {
struct Float4x4;
}
namespace sample {
namespace internal {
class Camera;
class Shader;
class SkeletonShader;
class AmbientShader;
class AmbientTexturedShader;
class AmbientShaderInstanced;
class GlImmediateRenderer;

// Implements Renderer interface.
class RendererImpl : public Renderer {
 public:
  RendererImpl(Camera* _camera);
  virtual ~RendererImpl();

  // See Renderer for all the details about the API.
  virtual bool Initialize();

  virtual bool DrawAxes(const ozz::math::Float4x4& _transform);

  virtual bool DrawGrid(int _cell_count, float _cell_size);

  virtual bool DrawSkeleton(const animation::Skeleton& _skeleton,
                            const ozz::math::Float4x4& _transform,
                            bool _draw_joints);

  virtual bool DrawPosture(const animation::Skeleton& _skeleton,
                           ozz::span<const ozz::math::Float4x4> _matrices,
                           const ozz::math::Float4x4& _transform,
                           bool _draw_joints);

  virtual bool DrawBoxIm(const ozz::math::Box& _box,
                         const ozz::math::Float4x4& _transform,
                         const Color _colors[2]);

  virtual bool DrawBoxShaded(const ozz::math::Box& _box,
                             ozz::span<const ozz::math::Float4x4> _transforms,
                             Color _color);

  virtual bool DrawSphereIm(float _radius,
                            const ozz::math::Float4x4& _transform,
                            const Color _color);

  virtual bool DrawSphereShaded(
      float _radius, ozz::span<const ozz::math::Float4x4> _transforms,
      Color _color);

  virtual bool DrawSkinnedMesh(const Mesh& _mesh,
                               const span<math::Float4x4> _skinning_matrices,
                               const ozz::math::Float4x4& _transform,
                               const Options& _options = Options());

  virtual bool DrawMesh(const Mesh& _mesh,
                        const ozz::math::Float4x4& _transform,
                        const Options& _options = Options());

  virtual bool DrawSegment(const math::Float3& _begin, const math::Float3& _end,
                           Color _color, const ozz::math::Float4x4& _transform);

  virtual bool DrawVectors(ozz::span<const float> _positions,
                           size_t _positions_stride,
                           ozz::span<const float> _directions,
                           size_t _directions_stride, int _num_vectors,
                           float _vector_length, Color _color,
                           const ozz::math::Float4x4& _transform);

  virtual bool DrawBinormals(
      ozz::span<const float> _positions, size_t _positions_stride,
      ozz::span<const float> _normals, size_t _normals_stride,
      ozz::span<const float> _tangents, size_t _tangents_stride,
      ozz::span<const float> _handenesses, size_t _handenesses_stride,
      int _num_vectors, float _vector_length, Color _color,
      const ozz::math::Float4x4& _transform);

  // Get GL immediate renderer implementation;
  GlImmediateRenderer* immediate_renderer() const { return immediate_.get(); }

  // Get application camera that provides rendering matrices.
  Camera* camera() const { return camera_; }

 private:
  // Defines the internal structure used to define a model.
  struct Model {
    Model();
    ~Model();

    GLuint vbo;
    GLenum mode;
    GLsizei count;
    ozz::unique_ptr<SkeletonShader> shader;
  };

  // Detects and initializes all OpenGL extension.
  // Return true if all mandatory extensions were found.
  bool InitOpenGLExtensions();

  // Initializes posture rendering.
  // Return true if initialization succeeded.
  bool InitPostureRendering();

  // Initializes the checkered texture.
  // Return true if initialization succeeded.
  bool InitCheckeredTexture();

  // Draw posture internal non-instanced rendering fall back implementation.
  void DrawPosture_Impl(const ozz::math::Float4x4& _transform,
                        const float* _uniforms, int _instance_count,
                        bool _draw_joints);

  // Draw posture internal instanced rendering implementation.
  void DrawPosture_InstancedImpl(const ozz::math::Float4x4& _transform,
                                 const float* _uniforms, int _instance_count,
                                 bool _draw_joints);

  // Array of matrices used to store model space matrices during DrawSkeleton
  // execution.
  ozz::vector<ozz::math::Float4x4> prealloc_models_;

  // Application camera that provides rendering matrices.
  Camera* camera_;

  // Bone and joint model objects.
  Model models_[2];

  // Dynamic vbo used for arrays.
  GLuint dynamic_array_bo_;

  // Dynamic vbo used for indices.
  GLuint dynamic_index_bo_;

  // Volatile memory buffer that can be used within function scope.
  // Minimum alignment is 16 bytes.
  class ScratchBuffer {
   public:
    ScratchBuffer();
    ~ScratchBuffer();

    // Resizes the buffer to the new size and return the memory address.
    void* Resize(size_t _size);

   private:
    void* buffer_;
    size_t size_;
  };
  ScratchBuffer scratch_buffer_;

  // Immediate renderer implementation.
  ozz::unique_ptr<GlImmediateRenderer> immediate_;

  // Ambient rendering shader.
  ozz::unique_ptr<AmbientShader> ambient_shader;
  ozz::unique_ptr<AmbientTexturedShader> ambient_textured_shader;
  ozz::unique_ptr<AmbientShaderInstanced> ambient_shader_instanced;

  // Checkered texture
  unsigned int checkered_texture_;
};
}  // namespace internal
}  // namespace sample
}  // namespace ozz

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
extern PFNGLUNIFORM1IPROC glUniform1i;
extern PFNGLUNIFORM2IPROC glUniform2i;
extern PFNGLUNIFORM3IPROC glUniform3i;
extern PFNGLUNIFORM4IPROC glUniform4i;
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
extern bool GL_ARB_instanced_arrays_supported;
extern PFNGLVERTEXATTRIBDIVISORARBPROC glVertexAttribDivisor_;
extern PFNGLDRAWARRAYSINSTANCEDARBPROC glDrawArraysInstanced_;
extern PFNGLDRAWELEMENTSINSTANCEDARBPROC glDrawElementsInstanced_;
#endif  // OZZ_SAMPLES_FRAMEWORK_INTERNAL_RENDERER_IMPL_H_
