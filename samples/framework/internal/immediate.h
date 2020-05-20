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

#ifndef OZZ_SAMPLES_FRAMEWORK_INTERNAL_IMMEDIATE_H_
#define OZZ_SAMPLES_FRAMEWORK_INTERNAL_IMMEDIATE_H_

#ifndef OZZ_INCLUDE_PRIVATE_HEADER
#error "This header is private, it cannot be included from public headers."
#endif  // OZZ_INCLUDE_PRIVATE_HEADER

#include <cstring>

#include "ozz/base/containers/vector.h"
#include "ozz/base/maths/math_ex.h"
#include "ozz/base/maths/simd_math.h"
#include "ozz/base/memory/unique_ptr.h"
#include "ozz/base/platform.h"
#include "renderer_impl.h"

namespace ozz {
namespace sample {
namespace internal {

class RendererImpl;
class ImmediatePCShader;
class ImmediatePNShader;
class ImmediatePTCShader;

// Declares supported vertex formats.
// Position + Color.
struct VertexPC {
  float pos[3];
  GLubyte rgba[4];
};

// Declares supported vertex formats.
// Position + Normal.
struct VertexPN {
  float pos[3];
  float normal[3];
};

// Position + Texture coordinate + Color.
struct VertexPTC {
  float pos[3];
  float uv[2];
  GLubyte rgba[4];
};

// Declares Immediate mode types.
template <typename _Ty>
class GlImmediate;
typedef GlImmediate<VertexPC> GlImmediatePC;
typedef GlImmediate<VertexPN> GlImmediatePN;
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
  template <typename _Ty>
  friend class GlImmediate;

  // Begin stacking vertices.
  void Begin();

  // End stacking vertices. Call GL rendering.
  template <typename _Ty>
  void End(GLenum _mode, const ozz::math::Float4x4& _transform);

  // Push a new vertex to the buffer.
  template <typename _Ty>
  OZZ_INLINE void PushVertex(const _Ty& _vertex) {
    // Resize buffer if needed.
    const size_t new_size = size_ + sizeof(_Ty);
    if (new_size > buffer_.size()) {
      buffer_.resize(new_size);
    }

    // Copy this last vertex.
    std::memcpy(buffer_.data() + size_, &_vertex, sizeof(_Ty));
    size_ = new_size;
  }

  // The vertex object used by the renderer.
  GLuint vbo_;

  // Buffer of vertices.
  ozz::vector<char> buffer_;

  // Number of vertices.
  size_t size_;

  // Immediate mode shaders;
  ozz::unique_ptr<ImmediatePCShader> immediate_pc_shader;
  ozz::unique_ptr<ImmediatePTCShader> immediate_ptc_shader;

  // The renderer object.
  RendererImpl* renderer_;
};

// RAII object that allows to push vertices to the imrender stack.
template <typename _Ty>
class GlImmediate {
 public:
  // Immediate object vertex format.
  typedef _Ty Vertex;

  // Start a new immediate stack.
  GlImmediate(GlImmediateRenderer* _renderer, GLenum _mode,
              const ozz::math::Float4x4& _transform)
      : transform_(_transform), renderer_(_renderer), mode_(_mode) {
    renderer_->Begin();
  }

  // End immediate vertex stacking, and renders all vertices.
  ~GlImmediate() { renderer_->End<_Ty>(mode_, transform_); }

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
}  // namespace internal
}  // namespace sample
}  // namespace ozz
#endif  // OZZ_SAMPLES_FRAMEWORK_INTERNAL_IMMEDIATE_H_
