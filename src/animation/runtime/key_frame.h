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

// Defines runtime key frame types.
// This is not declared as part of the animation class to avoid including
// mathematical stuff.

#include "ozz/base/maths/vec_float.h"
#include "ozz/base/maths/quaternion.h"

namespace ozz {
namespace animation {

// Defines the translation key frame type.
struct TranslationKey {
  int track;
  float time;
  ozz::math::Float3 value;
};

// Defines the rotation key frame type.
struct RotationKey {
  int track;
  float time;
  ozz::math::Quaternion value;
};

// Defines the scale key frame type.
struct ScaleKey {
  int track;
  float time;
  ozz::math::Float3 value;
};
}  // animation
}  // ozz
#endif  // OZZ_ANIMATION_RUNTIME_KEY_FRAME_H_
