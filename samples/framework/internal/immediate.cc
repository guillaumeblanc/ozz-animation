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

#define OZZ_INCLUDE_PRIVATE_HEADER  // Allows to include private headers.

#include "immediate.h"

#include <cassert>

#include "ozz/base/memory/allocator.h"

#include "shader.h"
#include "renderer_impl.h"
#include "camera.h"

namespace ozz {
namespace sample {
namespace internal {

GlImmediateRenderer::GlImmediateRenderer(RendererImpl* _renderer)
    : vbo_(0),
      buffer_(NULL),
      max_size_(1024),
      size_(0),
      immediate_pc_shader(NULL),
      immediate_ptc_shader(NULL),
      renderer_(_renderer){
}

GlImmediateRenderer::~GlImmediateRenderer() {
  assert(size_ == 0 && "Immediate rendering still in use.");
  
  if (vbo_) {
    GL(DeleteBuffers(1, &vbo_));
    vbo_ = 0;
  }
  ozz::memory::default_allocator()->Deallocate(buffer_);
  buffer_ = NULL;

  ozz::memory::default_allocator()->Delete(immediate_pc_shader);
  immediate_pc_shader = NULL;
  ozz::memory::default_allocator()->Delete(immediate_ptc_shader);
  immediate_ptc_shader = NULL;

  renderer_ = NULL;
}

bool GlImmediateRenderer::Initialize() {
  GL(GenBuffers(1, &vbo_));
  buffer_ = ozz::memory::default_allocator()->Allocate<char>(max_size_);

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

template<>
void GlImmediateRenderer::End<VertexPC>(GLenum _mode,
                                        const ozz::math::Float4x4& _transform) {
  GL(BindBuffer(GL_ARRAY_BUFFER, vbo_));
  GL(BufferData(GL_ARRAY_BUFFER, size_, buffer_, GL_STREAM_DRAW));

  immediate_pc_shader->Bind(_transform,
                            renderer_->camera()->view_proj(),
                            sizeof(VertexPC), 0,
                            sizeof(VertexPC), 12);

  const int count = static_cast<int>(size_ / sizeof(VertexPC));
  GL(DrawArrays(_mode, 0, count));

  immediate_pc_shader->Unbind();

  GL(BindBuffer(GL_ARRAY_BUFFER, 0));

  // Reset vertex count for the next call
  size_ = 0;
}

template<>
void GlImmediateRenderer::End<VertexPTC>(GLenum _mode,
                                         const ozz::math::Float4x4& _transform) {
  GL(BindBuffer(GL_ARRAY_BUFFER, vbo_));
  GL(BufferData(GL_ARRAY_BUFFER, size_, buffer_, GL_STREAM_DRAW));

  immediate_ptc_shader->Bind(_transform,
                             renderer_->camera()->view_proj(),
                             sizeof(VertexPTC), 0,
                             sizeof(VertexPTC), 12,
                             sizeof(VertexPTC), 20);

  const int count = static_cast<int>(size_ / sizeof(VertexPTC));
  GL(DrawArrays(_mode, 0, count));

  immediate_ptc_shader->Unbind();

  GL(BindBuffer(GL_ARRAY_BUFFER, 0));

  // Reset vertex count for the next call
  size_ = 0;
}
}  // internal
}  // sample
}  // ozz
