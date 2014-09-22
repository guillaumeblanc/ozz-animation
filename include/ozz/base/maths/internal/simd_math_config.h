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

#ifndef OZZ_OZZ_BASE_MATHS_INTERNAL_SIMD_MATH_CONFIG_H_
#define OZZ_OZZ_BASE_MATHS_INTERNAL_SIMD_MATH_CONFIG_H_

#include "ozz/base/platform.h"

// Try to match a SSE version
#if defined(__AVX__)  || defined(OZZ_HAS_AVX)
#include <immintrin.h>
#ifndef OZZ_HAS_AVX
#define OZZ_HAS_AVX
#endif  // OZZ_HAS_AVX
#define OZZ_HAS_SSE4_2
#endif

#if defined(__SSE4_2__) || defined(OZZ_HAS_SSE4_2)
#include <nmmintrin.h>
#ifndef OZZ_HAS_SSE4_2
#define OZZ_HAS_SSE4_2
#endif  // OZZ_HAS_SSE4_2
#define OZZ_HAS_SSE4_1
#endif

#if defined(__SSE4_1__) || defined(OZZ_HAS_SSE4_1)
#include <smmintrin.h>
#ifndef OZZ_HAS_SSE4_1
#define OZZ_HAS_SSE4_1
#endif  // OZZ_HAS_SSE4_1
#define OZZ_HAS_SSSE3
#endif

#if defined(__SSSE3__) || defined(OZZ_HAS_SSSE3)
#include <tmmintrin.h>
#ifndef OZZ_HAS_SSSE3
#define OZZ_HAS_SSSE3
#endif  // OZZ_HAS_SSSE3
#define OZZ_HAS_SSE3
#endif

#if defined(__SSE3__) || defined(OZZ_HAS_SSE3)
#include <pmmintrin.h>
#ifndef OZZ_HAS_SSE3
#define OZZ_HAS_SSE3
#endif  // OZZ_HAS_SSE3
#define OZZ_HAS_SSE2
#endif

#if defined(__SSE2__) || defined(OZZ_HAS_SSE2)
#include <emmintrin.h>
#ifndef OZZ_HAS_SSE2
#define OZZ_HAS_SSE2
#endif  // OZZ_HAS_SSE2

// SSE detected
#define OZZ_HAS_SSEx

namespace ozz {
namespace math {

// Vector of four floating point values.
typedef __m128 SimdFloat4;

// Argument type for Float4.
typedef const __m128 _SimdFloat4;

// Vector of four integer values.
typedef __m128i SimdInt4;

// Argument type for Int4.
typedef const __m128i _SimdInt4;
}  // math
}  // ozz

#else  // OZZ_HAS_x

// No simd instruction set detected, switch back to reference implementation.
#define OZZ_HAS_REF

// Declares reference simd float and integer vectors outside of ozz::math, in
// order to match non-reference implementation details.

// Vector of four floating point values.
struct SimdFloat4Def {
  OZZ_ALIGN(16) float x;
  float y;
  float z;
  float w;
};

// Vector of four integer values.
struct SimdInt4Def {
  OZZ_ALIGN(16) int x;
  int y;
  int z;
  int w;
};

namespace ozz {
namespace math {

// Vector of four floating point values.
typedef SimdFloat4Def SimdFloat4;

// Argument type for SimdFloat4
typedef const SimdFloat4& _SimdFloat4;

// Vector of four integer values.
typedef SimdInt4Def SimdInt4;

// Argument type for SimdInt4.
typedef const SimdInt4& _SimdInt4;

}  // math
}  // ozz

#endif  // OZZ_HAS_x

#endif  // OZZ_OZZ_BASE_MATHS_INTERNAL_SIMD_MATH_CONFIG_H_
