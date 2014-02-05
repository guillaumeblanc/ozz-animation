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

#ifndef OZZ_SAMPLES_FRAMEWORK_IMAGE_H_
#define OZZ_SAMPLES_FRAMEWORK_IMAGE_H_

// Implements image files read/write.

#include "ozz/base/platform.h"

namespace ozz {
namespace sample {
namespace image {

// Pixel format definition.
struct Format {
 enum Value {
   kRGB,
   kBGR,
   kRGBA,
   kBGRA,
  };
};

// Tests if format specification contains alpha channel.
bool HasAlpha(Format::Value _format);

// Get stride from format specification.
int Stride(Format::Value _format);

// Writes as TARGA image to file _filename.
bool WriteTGA(const char* _filename,
              int _width, int _height,
              Format::Value _src_format,
              const ozz::uint8* _src_buffer,
              bool _write_alpha);

}  // image
}  // sample
}  // ozz
#endif  // OZZ_SAMPLES_FRAMEWORK_IMAGE_H_
