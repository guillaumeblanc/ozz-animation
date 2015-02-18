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
  supported_ = glMapBuffer != NULL && glUnmapBuffer != NULL;

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
  for (int i = 0; i < kNumShots; ++i) {
    Shot& shot = shots_[i];
    GL(BindBuffer(GL_PIXEL_PACK_BUFFER, shot.pbo));
    GL(BufferData(GL_PIXEL_PACK_BUFFER, _width * _height * 4, 0, GL_STREAM_READ));
    shot.width = _width;
    shot.height = _height;
    assert(shot.cooldown == 0);  // Must have been processed.
  }
  GL(BindBuffer(GL_PIXEL_PACK_BUFFER, 0));
}

bool Shooter::Update() {
  return Process();
}

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
    GL(BindBuffer(GL_PIXEL_PACK_BUFFER, shot.pbo));
    const void* pixels = glMapBuffer(GL_PIXEL_PACK_BUFFER, GL_READ_ONLY);
    if(pixels) {
      char name[16];
      sprintf(name, "%06d.tga", shot_number_++);

      ozz::sample::image::WriteTGA(name,
                                   shot.width,
                                   shot.height,
                                   image_format_,
                                   reinterpret_cast<const uint8_t*>(pixels),
                                   false);
      GL(UnmapBuffer(GL_PIXEL_PACK_BUFFER));
    }
    GL(BindBuffer(GL_PIXEL_PACK_BUFFER, 0));
  }
  return true;
}

bool Shooter::ProcessAll() {
  // Reset cooldown to 1 for all "unprocessed" shots, so they will be "processed".
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
  for (shot = shots_;
       shot < shots_ + kNumShots && shot->cooldown != 0;
       ++shot) {
  }
  assert(shot != shots_ + kNumShots);

  // Initializes cooldown.
  shot->cooldown = kInitialCountDown;

  // Copy pixels to shot's pbo.
  GL(ReadBuffer(_buffer));
  GL(BindBuffer(GL_PIXEL_PACK_BUFFER, shot->pbo));
  GL(PixelStorei(GL_PACK_ALIGNMENT, 4));
  GL(ReadPixels(0, 0,
                shot->width, shot->height,
                gl_shot_format_, GL_UNSIGNED_BYTE, 0));
  GL(BindBuffer(GL_PIXEL_PACK_BUFFER, 0));

  return true;
}
}  // internal
}  // sample
}  // ozz
