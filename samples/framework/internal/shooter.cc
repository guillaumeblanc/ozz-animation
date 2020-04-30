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

#include "shooter.h"

#include <cassert>
#include <cstdio>

#include "renderer_impl.h"

namespace ozz {
namespace sample {
namespace internal {

Shooter::Shooter()
    : gl_shot_format_(GL_RGBA),  // Default fail safe format and types.
      image_format_(image::Format::kRGBA),
      shot_number_(0) {
  // Test required extension (optional for the framework).
  // If GL_VERSION_1_5 is defined (aka OZZ_GL_VERSION_1_5_EXT not defined), then
  // these functions are part of the library (don't need extensions).
  supported_ = true;
#ifdef OZZ_GL_VERSION_1_5_EXT
  supported_ &= glMapBuffer != nullptr && glUnmapBuffer != nullptr;
#endif  // OZZ_GL_VERSION_1_5_EXT

  // Initializes shots
  GLuint pbos[kNumShots];
  GL(GenBuffers(kNumShots, pbos));
  for (int i = 0; i < kNumShots; ++i) {
    Shot& shot = shots_[i];
    shot.pbo = pbos[i];
  }

  // OpenGL ES2 compatibility extension allows to query for implementation best
  // format and type.
  if (glfwExtensionSupported("GL_ARB_ES2_compatibility") != 0) {
    GLint format;
    GL(GetIntegerv(GL_IMPLEMENTATION_COLOR_READ_FORMAT_OES, &format));
    GLint type;
    GL(GetIntegerv(GL_IMPLEMENTATION_COLOR_READ_TYPE_OES, &type));

    // Only support GL_UNSIGNED_BYTE.
    if (type == GL_UNSIGNED_BYTE) {
      switch (format) {
        case GL_RGBA:
          gl_shot_format_ = format;
          image_format_ = image::Format::kRGBA;
          break;
        case GL_BGRA:
          gl_shot_format_ = format;
          image_format_ = image::Format::kBGRA;
          break;
        case GL_RGB:
          gl_shot_format_ = format;
          image_format_ = image::Format::kRGB;
          break;
        case GL_BGR:
          gl_shot_format_ = format;
          image_format_ = image::Format::kBGR;
          break;
      }
    }
  } else {
    // Default fail safe format and types.
    gl_shot_format_ = GL_RGBA;
    image_format_ = image::Format::kRGBA;
  }
}

Shooter::~Shooter() {
  // Process all remaining shots.
  ProcessAll();

  // Clean shot pbos.
  for (int i = 0; i < kNumShots; ++i) {
    Shot& shot = shots_[i];
    GL(DeleteBuffers(1, &shot.pbo));

    assert(shot.cooldown == 0);  // Must have been processed.
  }
}

void Shooter::Resize(int _width, int _height) {
  // Early out if not supported.
  if (!supported_) {
    return;
  }

  // Process all remaining shots.
  ProcessAll();

  // Resizes all pbos.
#ifndef EMSCRIPTEN
  for (int i = 0; i < kNumShots; ++i) {
    Shot& shot = shots_[i];
    GL(BindBuffer(GL_PIXEL_PACK_BUFFER, shot.pbo));
    GL(BufferData(GL_PIXEL_PACK_BUFFER, _width * _height * 4, 0,
                  GL_STREAM_READ));
    shot.width = _width;
    shot.height = _height;
    assert(shot.cooldown == 0);  // Must have been processed.
  }
  GL(BindBuffer(GL_PIXEL_PACK_BUFFER, 0));
#endif  // EMSCRIPTEN
}

bool Shooter::Update() { return Process(); }

bool Shooter::Process() {
  // Early out if not supported.
  if (!supported_) {
    return true;
  }

  // Early out if process stack is empty.
  for (int i = 0; i < kNumShots; ++i) {
    Shot& shot = shots_[i];

    // Early out for already processed, or empty shots.
    if (shot.cooldown == 0) {
      continue;
    }

    // Skip shots that didn't reached the cooldown.
    if (--shot.cooldown != 0) {
      continue;
    }

    // Processes this shot.
#ifdef EMSCRIPTEN
    (void)shot_number_;
#else   // EMSCRIPTEN
    GL(BindBuffer(GL_PIXEL_PACK_BUFFER, shot.pbo));
    const void* pixels = glMapBuffer(GL_PIXEL_PACK_BUFFER, GL_READ_ONLY);
    if (pixels) {
      char name[16];
      sprintf(name, "%06d.tga", shot_number_++);

      ozz::sample::image::WriteTGA(name, shot.width, shot.height, image_format_,
                                   reinterpret_cast<const uint8_t*>(pixels),
                                   false);
      GL(UnmapBuffer(GL_PIXEL_PACK_BUFFER));
    }
    GL(BindBuffer(GL_PIXEL_PACK_BUFFER, 0));
#endif  // EMSCRIPTEN
  }
  return true;
}

bool Shooter::ProcessAll() {
  // Reset cooldown to 1 for all "unprocessed" shots, so they will be
  // "processed".
  for (int i = 0; i < kNumShots; ++i) {
    Shot& shot = shots_[i];
    shot.cooldown = shot.cooldown > 0 ? 1 : 0;
  }

  return Process();
}

bool Shooter::Capture(int _buffer) {
  assert(_buffer == GL_FRONT || _buffer == GL_BACK);

  // Early out if not supported.
  if (!supported_) {
    return true;
  }

  // Finds the shot to use for this capture.
  Shot* shot;
  for (shot = shots_; shot < shots_ + kNumShots && shot->cooldown != 0;
       ++shot) {
  }
  assert(shot != shots_ + kNumShots);

  // Initializes cooldown.
  shot->cooldown = kInitialCountDown;

#ifndef EMSCRIPTEN
  // Copy pixels to shot's pbo.
  GL(ReadBuffer(_buffer));
  GL(BindBuffer(GL_PIXEL_PACK_BUFFER, shot->pbo));
  GL(PixelStorei(GL_PACK_ALIGNMENT, 4));
  GL(ReadPixels(0, 0, shot->width, shot->height, gl_shot_format_,
                GL_UNSIGNED_BYTE, 0));
  GL(BindBuffer(GL_PIXEL_PACK_BUFFER, 0));
#endif  // EMSCRIPTEN

  return true;
}
}  // namespace internal
}  // namespace sample
}  // namespace ozz
