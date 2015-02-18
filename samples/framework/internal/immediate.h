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

#ifndef OZZ_SAMPLES_FRAMEWORK_INTERNAL_IMMEDIATE_H_
#define OZZ_SAMPLES_FRAMEWORK_INTERNAL_IMMEDIATE_H_

#ifndef OZZ_INCLUDE_PRIVATE_HEADER
#error "This header is private, it cannot be included from public headers."
#endif  // OZZ_INCLUDE_PRIVATE_HEADER

#include "renderer_impl.h"

#include "ozz/base/platform.h"

#include "ozz/base/maths/math_ex.h"
#include "ozz/base/maths/simd_math.h"

namespace ozz {
namespace sample {
namespace internal {

class RendererImpl;
class ImmediatePCShader;
class ImmediatePTCShader;

// Declares supported vertex formats.
// Position + Color.
struct VertexPC {
    float pos[3];
    GLubyte rgba[4];
};

// Position + Texture coordinate + Color.
struct VertexPTC {
    float pos[3];
    float uv[2];
    GLubyte rgba[4];
};

// Declares Immediate mode types.
template<typename _Ty> class GlImmediate;
typedef GlImmediate<VertexPC> GlImmediatePC;
typedef GlImmediate<VertexPTC> GlImmediatePTC;

// GL immedialte mode renderer.
// Should be used with a GlImmediate object to push vertices to the renderer.
class GlImmediateRenderer {
 public:
  GlImmediateRenderer(RendererImpl* _renderer);
  ~GlImmediateRenderer();

  // Initializes immediate  mode renderer. Can fail.
  bool Initialize();

 private:
  // GlImmediate is used to work with the renderer.
  template<typename _Ty>
  friend class GlImmediate;

  // Begin stacking vertices.
  void Begin();

  // End stacking vertices. Call GL rendering.
  template<typename _Ty>
  void End(GLenum _mode, const ozz::math::Float4x4& _transform);

  // Push a new vertex to the buffer.
  template<typename _Ty>
  OZZ_INLINE void PushVertex(const _Ty& _vertex) {
    // Resize buffer if needed.
    const size_t new_size = size_ + sizeof(_Ty);
    if (new_size > max_size_) {
      ResizeVbo(new_size);
    }
    // Copy this last vertex.
    *reinterpret_cast<_Ty*>(&buffer_[size_]) = _vertex;
    size_ = new_size;
  }

  // Resize vertex buffer to at least _new_size. This function can only grow
  // vbo size.
  void ResizeVbo(size_t _new_size);

  // The vertex object used by the renderer.
  GLuint vbo_;

  // Buffer of vertices.
  char* buffer_;
  size_t max_size_;

  // Number of vertices.
  size_t size_;

  // Immediate mode shaders;
  ImmediatePCShader* immediate_pc_shader;
  ImmediatePTCShader* immediate_ptc_shader;

  // The renderer object.
  RendererImpl* renderer_;
};

// RAII object that allows to push vertices to the imrender stack.
template<typename _Ty>
class GlImmediate {
 public:

  // Immediate object vertex format.
  typedef _Ty Vertex;

  // Start a new immediate stack.
  GlImmediate(GlImmediateRenderer* _renderer,
              GLenum _mode,
              const ozz::math::Float4x4& _transform)
      : transform_(_transform),
        renderer_(_renderer),
        mode_(_mode) {
    renderer_->Begin();
  }

  // End immediate vertex stacking, and renders all vertices.
  ~GlImmediate() {
    renderer_->End<_Ty>(mode_, transform_);
  }

  // Pushes a new vertex to the stack.
  OZZ_INLINE void PushVertex(const _Ty& _vertex) {
    renderer_->PushVertex(_vertex);    
  }

 private:
  // Non copyable.
  GlImmediate(const GlImmediate&);
  void operator=(const GlImmediate&);

  // Transformation matrix.
  const ozz::math::Float4x4 transform_;

  // Shared renderer.
  GlImmediateRenderer* renderer_;

  // Draw array mode GL_POINTS, GL_LINE_STRIP, ...
  GLenum mode_;
};
}  // internal
}  // sample
}  // ozz
#endif  // OZZ_SAMPLES_FRAMEWORK_INTERNAL_IMMEDIATE_H_
