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

#ifndef OZZ_ANIMATION_RUNTIME_KEY_FRAME_H_
#define OZZ_ANIMATION_RUNTIME_KEY_FRAME_H_

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
#endif  // OZZ_ANIMATION_RUNTIME_KEY_FRAME_H_
