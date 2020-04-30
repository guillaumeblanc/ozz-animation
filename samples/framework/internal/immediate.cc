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

#include "immediate.h"

#include <cassert>

#include "camera.h"
#include "ozz/base/memory/allocator.h"
#include "renderer_impl.h"
#include "shader.h"

namespace ozz {
namespace sample {
namespace internal {

GlImmediateRenderer::GlImmediateRenderer(RendererImpl* _renderer)
    : vbo_(0), size_(0), renderer_(_renderer) {}

GlImmediateRenderer::~GlImmediateRenderer() {
  assert(size_ == 0 && "Immediate rendering still in use.");

  if (vbo_) {
    GL(DeleteBuffers(1, &vbo_));
    vbo_ = 0;
  }
}

bool GlImmediateRenderer::Initialize() {
  GL(GenBuffers(1, &vbo_));

  immediate_pc_shader = ImmediatePCShader::Build();
  if (!immediate_pc_shader) {
    return false;
  }
  immediate_ptc_shader = ImmediatePTCShader::Build();
  if (!immediate_ptc_shader) {
    return false;
  }

  return true;
}

void GlImmediateRenderer::Begin() {
  assert(size_ == 0 && "Immediate rendering already in use.");
}

template <>
void GlImmediateRenderer::End<VertexPC>(GLenum _mode,
                                        const ozz::math::Float4x4& _transform) {
  GL(BindBuffer(GL_ARRAY_BUFFER, vbo_));
  GL(BufferData(GL_ARRAY_BUFFER, size_, buffer_.data(), GL_STREAM_DRAW));

  immediate_pc_shader->Bind(_transform, renderer_->camera()->view_proj(),
                            sizeof(VertexPC), 0, sizeof(VertexPC), 12);

  const int count = static_cast<int>(size_ / sizeof(VertexPC));
  GL(DrawArrays(_mode, 0, count));

  immediate_pc_shader->Unbind();

  GL(BindBuffer(GL_ARRAY_BUFFER, 0));

  // Reset vertex count for the next call
  size_ = 0;
}

template <>
void GlImmediateRenderer::End<VertexPTC>(
    GLenum _mode, const ozz::math::Float4x4& _transform) {
  GL(BindBuffer(GL_ARRAY_BUFFER, vbo_));
  GL(BufferData(GL_ARRAY_BUFFER, size_, buffer_.data(), GL_STREAM_DRAW));

  immediate_ptc_shader->Bind(_transform, renderer_->camera()->view_proj(),
                             sizeof(VertexPTC), 0, sizeof(VertexPTC), 12,
                             sizeof(VertexPTC), 20);

  const int count = static_cast<int>(size_ / sizeof(VertexPTC));
  GL(DrawArrays(_mode, 0, count));

  immediate_ptc_shader->Unbind();

  GL(BindBuffer(GL_ARRAY_BUFFER, 0));

  // Reset vertex count for the next call
  size_ = 0;
}
}  // namespace internal
}  // namespace sample
}  // namespace ozz
