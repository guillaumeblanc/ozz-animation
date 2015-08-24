//----------------------------------------------------------------------------//
//                                                                            //
// ozz-animation is hosted at http://github.com/guillaumeblanc/ozz-animation  //
// and distributed under the MIT License (MIT).                               //
//                                                                            //
// Copyright (c) 2015 Guillaume Blanc                                         //
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

#ifndef OZZ_ANIMATION_RUNTIME_ANIMATION_KEYFRAME_H_
#define OZZ_ANIMATION_RUNTIME_ANIMATION_KEYFRAME_H_

#ifndef OZZ_INCLUDE_PRIVATE_HEADER
#error "This header is private, it cannot be included from public headers."
#endif  // OZZ_INCLUDE_PRIVATE_HEADER

namespace ozz {
namespace animation {

// Define animation key frame types (translation, rotation, scale). Every type
// as the same base made of the key time and it's track. This is required as
// key frames are not sorted per track, but sorted by time to favor cache
// coherency.
// Key frame values are compressed, according on their type. Decompression is
// efficient because it's done on SoA data and cached during sampling.

// Defines the translation key frame type.
// Translation values are stored as half precision floats with 16 bits per
// component.
struct TranslationKey {
  float time;
  uint16_t track;
  uint16_t value[3];
};

// Defines the rotation key frame type.
// Rotation value is a quaternion. Quaternion are normalized, which means each
// component is in range [0:1]. This property allows to quantize the first 3
// components to 3 signed integer 16 bits values. The 4th component is restored
// at runtime, using the knowledge that |w| = √(1 - (x^2 + y^2 + z^2)). The sign
// of this 4th component is stored using 1 bit taken from the track member.
//
// It's often recommended to store the 3 smallest components (and restore the
// largest) as they can be pre-multiplied by √2 to gain some precision. As
// decompression is done in SoA format, restoring a non-fixed component requires
// too much code to reshuffle the SoaQuaternion. Furthermore 16 bits quantization
// is already very precise, not worth a 1.4 factor.
// Quantization could be reduced to 11-11-10 bits as often used for animation
// key frames, but in this case RotationKey structure would contain 16 bits of
// padding.
struct RotationKey {
  float time;
  uint16_t track:15;
  bool wsign:1;
  int16_t value[3];
};

// Defines the scale key frame type.
// Scale values are stored as half precision floats with 16 bits per
// component.
struct ScaleKey {
  float time;
  uint16_t track;
  uint16_t value[3];
};
}  // animation
}  // ozz
#endif  // OZZ_ANIMATION_RUNTIME_ANIMATION_KEYFRAME_H_
