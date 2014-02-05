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

#include "ozz/base/maths/simd_math.h"

#include "gtest/gtest.h"

#include "ozz/base/gtest_helper.h"
#include "ozz/base/maths/gtest_math_helper.h"

using ozz::math::SimdFloat4;

TEST(TransposeFloat, ozz_simd_math) {
  const SimdFloat4 src[4] = {ozz::math::simd_float4::Load(0.f, 1.f, 2.f, 3.f),
                             ozz::math::simd_float4::Load(4.f, 5.f, 6.f, 7.f),
                             ozz::math::simd_float4::Load(8.f, 9.f, 10.f, 11.f),
                             ozz::math::simd_float4::Load(12.f, 13.f, 14.f, 15.f)};

  SimdFloat4 t4x1[1];
  ozz::math::Transpose4x1(src, t4x1);
  EXPECT_SIMDFLOAT_EQ(t4x1[0], 0.f, 4.f, 8.f, 12.f);

  SimdFloat4 t1x4[4];
  ozz::math::Transpose1x4(src, t1x4);
  EXPECT_SIMDFLOAT_EQ(t1x4[0], 0.f, 0.f, 0.f, 0.f);
  EXPECT_SIMDFLOAT_EQ(t1x4[1], 1.f, 0.f, 0.f, 0.f);
  EXPECT_SIMDFLOAT_EQ(t1x4[2], 2.f, 0.f, 0.f, 0.f);
  EXPECT_SIMDFLOAT_EQ(t1x4[3], 3.f, 0.f, 0.f, 0.f);

  SimdFloat4 t4x2[2];
  ozz::math::Transpose4x2(src, t4x2);
  EXPECT_SIMDFLOAT_EQ(t4x2[0], 0.f, 4.f, 8.f, 12.f);
  EXPECT_SIMDFLOAT_EQ(t4x2[1], 1.f, 5.f, 9.f, 13.f);

  SimdFloat4 t2x4[4];
  ozz::math::Transpose2x4(src, t2x4);
  EXPECT_SIMDFLOAT_EQ(t2x4[0], 0.f, 4.f, 0.f, 0.f);
  EXPECT_SIMDFLOAT_EQ(t2x4[1], 1.f, 5.f, 0.f, 0.f);
  EXPECT_SIMDFLOAT_EQ(t2x4[2], 2.f, 6.f, 0.f, 0.f);
  EXPECT_SIMDFLOAT_EQ(t2x4[3], 3.f, 7.f, 0.f, 0.f);

  SimdFloat4 t4x3[3];
  ozz::math::Transpose4x3(src, t4x3);
  EXPECT_SIMDFLOAT_EQ(t4x3[0], 0.f, 4.f, 8.f, 12.f);
  EXPECT_SIMDFLOAT_EQ(t4x3[1], 1.f, 5.f, 9.f, 13.f);
  EXPECT_SIMDFLOAT_EQ(t4x3[2], 2.f, 6.f, 10.f, 14.f);

  SimdFloat4 t3x4[4];
  ozz::math::Transpose3x4(src, t3x4);
  EXPECT_SIMDFLOAT_EQ(t3x4[0], 0.f, 4.f, 8.f, 0.f);
  EXPECT_SIMDFLOAT_EQ(t3x4[1], 1.f, 5.f, 9.f, 0.f);
  EXPECT_SIMDFLOAT_EQ(t3x4[2], 2.f, 6.f, 10.f, 0.f);
  EXPECT_SIMDFLOAT_EQ(t3x4[3], 3.f, 7.f, 11.f, 0.f);

  SimdFloat4 t4x4[4];
  ozz::math::Transpose4x4(src, t4x4);
  EXPECT_SIMDFLOAT_EQ(t4x4[0], 0.f, 4.f, 8.f, 12.f);
  EXPECT_SIMDFLOAT_EQ(t4x4[1], 1.f, 5.f, 9.f, 13.f);
  EXPECT_SIMDFLOAT_EQ(t4x4[2], 2.f, 6.f, 10.f, 14.f);
  EXPECT_SIMDFLOAT_EQ(t4x4[3], 3.f, 7.f, 11.f, 15.f);

  const SimdFloat4 src2[16] = {ozz::math::simd_float4::Load(0.f, 16.f, 32.f, 48.f),
                               ozz::math::simd_float4::Load(1.f, 17.f, 33.f, 49.f),
                               ozz::math::simd_float4::Load(2.f, 18.f, 34.f, 50.f),
                               ozz::math::simd_float4::Load(3.f, 19.f, 35.f, 51.f),
                               ozz::math::simd_float4::Load(4.f, 20.f, 36.f, 52.f),
                               ozz::math::simd_float4::Load(5.f, 21.f, 37.f, 53.f),
                               ozz::math::simd_float4::Load(6.f, 22.f, 38.f, 54.f),
                               ozz::math::simd_float4::Load(7.f, 23.f, 39.f, 55.f),
                               ozz::math::simd_float4::Load(8.f, 24.f, 40.f, 56.f),
                               ozz::math::simd_float4::Load(9.f, 25.f, 41.f, 57.f),
                               ozz::math::simd_float4::Load(10.f, 26.f, 42.f, 58.f),
                               ozz::math::simd_float4::Load(11.f, 27.f, 43.f, 59.f),
                               ozz::math::simd_float4::Load(12.f, 28.f, 44.f, 60.f),
                               ozz::math::simd_float4::Load(13.f, 29.f, 45.f, 61.f),
                               ozz::math::simd_float4::Load(14.f, 30.f, 46.f, 62.f),
                               ozz::math::simd_float4::Load(15.f, 31.f, 47.f, 63.f)};
  SimdFloat4 t16x16[16];
  ozz::math::Transpose16x16(src2, t16x16);
  EXPECT_SIMDFLOAT_EQ(t16x16[0], 0.f, 1.f, 2.f, 3.f);
  EXPECT_SIMDFLOAT_EQ(t16x16[1], 4.f, 5.f, 6.f, 7.f);
  EXPECT_SIMDFLOAT_EQ(t16x16[2], 8.f, 9.f, 10.f, 11.f);
  EXPECT_SIMDFLOAT_EQ(t16x16[3], 12.f, 13.f, 14.f, 15.f);
  EXPECT_SIMDFLOAT_EQ(t16x16[4], 16.f, 17.f, 18.f, 19.f);
  EXPECT_SIMDFLOAT_EQ(t16x16[5], 20.f, 21.f, 22.f, 23.f);
  EXPECT_SIMDFLOAT_EQ(t16x16[6], 24.f, 25.f, 26.f, 27.f);
  EXPECT_SIMDFLOAT_EQ(t16x16[7], 28.f, 29.f, 30.f, 31.f);
  EXPECT_SIMDFLOAT_EQ(t16x16[8], 32.f, 33.f, 34.f, 35.f);
  EXPECT_SIMDFLOAT_EQ(t16x16[9], 36.f, 37.f, 38.f, 39.f);
  EXPECT_SIMDFLOAT_EQ(t16x16[10], 40.f, 41.f, 42.f, 43.f);
  EXPECT_SIMDFLOAT_EQ(t16x16[11], 44.f, 45.f, 46.f, 47.f);
  EXPECT_SIMDFLOAT_EQ(t16x16[12], 48.f, 49.f, 50.f, 51.f);
  EXPECT_SIMDFLOAT_EQ(t16x16[13], 52.f, 53.f, 54.f, 55.f);
  EXPECT_SIMDFLOAT_EQ(t16x16[14], 56.f, 57.f, 58.f, 59.f);
  EXPECT_SIMDFLOAT_EQ(t16x16[15], 60.f, 61.f, 62.f, 63.f);
}
