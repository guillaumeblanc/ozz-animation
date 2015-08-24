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
