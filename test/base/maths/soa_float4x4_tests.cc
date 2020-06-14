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

#include "ozz/base/maths/soa_float4x4.h"

#include "gtest/gtest.h"

#include "ozz/base/gtest_helper.h"
#include "ozz/base/maths/gtest_math_helper.h"

using ozz::math::SoaFloat3;
using ozz::math::SoaFloat4;
using ozz::math::SoaFloat4x4;
using ozz::math::SoaQuaternion;

TEST(SoaFloat4x4Constant, ozz_soa_math) {
  const SoaFloat4x4 identity = SoaFloat4x4::identity();
  EXPECT_SOAFLOAT4x4_EQ(identity, 1.f, 1.f, 1.f, 1.f, 0.f, 0.f, 0.f, 0.f, 0.f,
                        0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f,
                        1.f, 1.f, 1.f, 1.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f,
                        0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 1.f, 1.f,
                        1.f, 1.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f,
                        0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 1.f, 1.f, 1.f, 1.f);
}

TEST(SoaFloat4x4Arithmetic, ozz_soa_math) {
  const SoaFloat4x4 m0 = {
      {{ozz::math::simd_float4::Load(0.f, 1.f, 0.f, 0.f),
        ozz::math::simd_float4::Load(1.f, 0.f, 0.f, 0.f),
        ozz::math::simd_float4::Load(2.f, 0.f, 0.f, -1.f),
        ozz::math::simd_float4::Load(3.f, 0.f, 0.f, 0.f)},
       {ozz::math::simd_float4::Load(4.f, 0.f, 0.f, 0.f),
        ozz::math::simd_float4::Load(5.f, 1.f, 0.f, 1.f),
        ozz::math::simd_float4::Load(6.f, 0.f, 0.f, 0.f),
        ozz::math::simd_float4::Load(7.f, 0.f, 0.f, 0.f)},
       {ozz::math::simd_float4::Load(8.f, 0.f, 0.f, 1.f),
        ozz::math::simd_float4::Load(9.f, 0.f, 0.f, 0.f),
        ozz::math::simd_float4::Load(10.f, 1.f, 0.f, 0.f),
        ozz::math::simd_float4::Load(11.f, 0.f, 0.f, 0.f)},
       {ozz::math::simd_float4::Load(12.f, 0.f, 0.f, 0.f),
        ozz::math::simd_float4::Load(13.f, 0.f, 0.f, 0.f),
        ozz::math::simd_float4::Load(14.f, 0.f, 0.f, 0.f),
        ozz::math::simd_float4::Load(15.f, 1.f, 0.f, 1.f)}}};
  const SoaFloat4x4 m1 = {
      {{ozz::math::simd_float4::Load(-0.f, 0.f, 0.f, 1.f),
        ozz::math::simd_float4::Load(-1.f, -1.f, 0.f, 0.f),
        ozz::math::simd_float4::Load(-2.f, 2.f, -1.f, 0.f),
        ozz::math::simd_float4::Load(-3.f, 3.f, 0.f, 0.f)},
       {ozz::math::simd_float4::Load(-4.f, -4.f, 0.f, 0.f),
        ozz::math::simd_float4::Load(-5.f, 5.f, 1.f, 1.f),
        ozz::math::simd_float4::Load(-6.f, 6.f, 0.f, 0.f),
        ozz::math::simd_float4::Load(-7.f, -7.f, 0.f, 0.f)},
       {ozz::math::simd_float4::Load(-8.f, 8.f, 1.f, 0.f),
        ozz::math::simd_float4::Load(-9.f, -9.f, 0.f, 0.f),
        ozz::math::simd_float4::Load(-10.f, -10.f, 0.f, 1.f),
        ozz::math::simd_float4::Load(-11.f, 11.f, 0.f, 0.f)},
       {ozz::math::simd_float4::Load(-12.f, -12.f, 0.f, 0.f),
        ozz::math::simd_float4::Load(-13.f, 13.f, 0.f, 0.f),
        ozz::math::simd_float4::Load(-14.f, -14.f, 0.f, 0.f),
        ozz::math::simd_float4::Load(-15.f, 15.f, 1.f, 1.f)}}};
  const SoaFloat4x4 m2 = {
      {{ozz::math::simd_float4::Load(2.f, 0.f, 0.f, 1.f),
        ozz::math::simd_float4::Load(0.f, -1.f, 0.f, 0.f),
        ozz::math::simd_float4::Load(0.f, 2.f, -1.f, 0.f),
        ozz::math::simd_float4::Load(0.f, 3.f, 0.f, 0.f)},
       {ozz::math::simd_float4::Load(0.f, -4.f, 0.f, 0.f),
        ozz::math::simd_float4::Load(0.f, 5.f, 1.f, 1.f),
        ozz::math::simd_float4::Load(-2.f, 6.f, 0.f, 0.f),
        ozz::math::simd_float4::Load(0.f, -7.f, 0.f, 0.f)},
       {ozz::math::simd_float4::Load(0.f, 8.f, 1.f, 0.f),
        ozz::math::simd_float4::Load(3.f, -9.f, 0.f, 0.f),
        ozz::math::simd_float4::Load(0.f, -10.f, 0.f, 1.f),
        ozz::math::simd_float4::Load(0.f, 11.f, 0.f, 0.f)},
       {ozz::math::simd_float4::Load(0.f, -12.f, 0.f, 0.f),
        ozz::math::simd_float4::Load(46.f, 13.f, 0.f, 0.f),
        ozz::math::simd_float4::Load(12.f, -14.f, 0.f, 0.f),
        ozz::math::simd_float4::Load(1.f, 15.f, 1.f, 1.f)}}};
  const SoaFloat4 v = {ozz::math::simd_float4::Load(0.f, 1.f, -2.f, 3.f),
                       ozz::math::simd_float4::Load(-1.f, 2.f, 5.f, 46.f),
                       ozz::math::simd_float4::Load(-2.f, 3.f, 7.f, -1.f),
                       ozz::math::simd_float4::Load(-3.f, 4.f, 0.f, 1.f)};
  const SoaFloat4 mul_vector = m0 * v;
  EXPECT_SOAFLOAT4_EQ(mul_vector, -56.f, 1.f, 0.f, -1.f, -62.f, 2.f, 0.f, 46,
                      -68.f, 3.f, 0.f, -3.f, -74.f, 4.f, 0.f, 1.f);

  const SoaFloat4x4 mul_mat = m0 * m1;
  EXPECT_SOAFLOAT4x4_EQ(
      mul_mat, -56.f, 0.f, 0.f, 0.f, -62.f, -1.f, 0.f, 0.f, -68.f, 2.f, 0.f,
      -1.f, -74.f, 3.f, 0.f, 0.f, -152.f, -4.f, 0.f, 0.f, -174.f, 5.f, 0.f, 1.f,
      -196.f, 6.f, 0.f, 0.f, -218.f, -7.f, 0.f, 0.f, -248.f, 8.f, 0.f, 1.f,
      -286.f, -9.f, 0.f, 0.f, -324.f, -10.f, 0.f, 0.f, -362.f, 11.f, 0.f, 0.f,
      -344.f, -12.f, 0.f, 0.f, -398.f, 13.f, 0.f, 0.f, -452.f, -14.f, 0.f, 0.f,
      -506.f, 15.f, 0.f, 1.f);

  const SoaFloat4x4 add_mat = m0 + m1;
  EXPECT_SOAFLOAT4x4_EQ(
      add_mat, 0.f, 1.f, 0.f, 1.f, 0.f, -1.f, 0.f, 0.f, 0.f, 2.f, -1.f, -1.f,
      0.f, 3.f, 0.f, 0.f, 0.f, -4.f, 0.f, 0.f, 0.f, 6.f, 1.f, 2.f, 0.f, 6.f,
      0.f, 0.f, 0.f, -7.f, 0.f, 0.f, 0.f, 8.f, 1.f, 1.f, 0.f, -9.f, 0.f, 0.f,
      0.f, -9.f, 0.f, 1.f, 0.f, 11.f, 0.f, 0.f, 0.f, -12.f, 0.f, 0.f, 0.f, 13.f,
      0.f, 0.f, 0.f, -14.f, 0.f, 0.f, 0.f, 16.f, 1.f, 2.f);

  const SoaFloat4x4 sub_mat = m0 - m1;
  EXPECT_SOAFLOAT4x4_EQ(
      sub_mat, 0.f, 1.f, 0.f, -1.f, 2.f, 1.f, 0.f, 0.f, 4.f, -2.f, 1.f, -1.f,
      6.f, -3.f, 0.f, 0.f, 8.f, 4.f, 0.f, 0.f, 10.f, -4.f, -1.f, 0.f, 12.f,
      -6.f, 0.f, 0.f, 14.f, 7.f, 0.f, 0.f, 16.f, -8.f, -1.f, 1.f, 18.f, 9.f,
      0.f, 0.f, 20.f, 11.f, 0.f, -1.f, 22.f, -11.f, 0.f, 0.f, 24.f, 12.f, 0.f,
      0.f, 26.f, -13.f, 0.f, 0.f, 28.f, 14.f, 0.f, 0.f, 30.f, -14.f, -1.f, 0.f);

  const SoaFloat4x4 transpose = Transpose(m0);
  EXPECT_SOAFLOAT4x4_EQ(
      transpose, 0.f, 1.f, 0.f, 0.f, 4.f, 0.f, 0.f, 0.f, 8.f, 0.f, 0.f, 1.f,
      12.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f, 5.f, 1.f, 0.f, 1.f, 9.f, 0.f,
      0.f, 0.f, 13.f, 0.f, 0.f, 0.f, 2.f, 0.f, 0.f, -1.f, 6.f, 0.f, 0.f, 0.f,
      10.f, 1.f, 0.f, 0.f, 14.f, 0.f, 0.f, 0.f, 3.f, 0.f, 0.f, 0.f, 7.f, 0.f,
      0.f, 0.f, 11.f, 0.f, 0.f, 0.f, 15.f, 1.f, 0.f, 1.f);

  const SoaFloat4x4 invert_ident = Invert(SoaFloat4x4::identity());
  EXPECT_SOAFLOAT4x4_EQ(
      invert_ident, 1.f, 1.f, 1.f, 1.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f,
      0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 1.f, 1.f, 1.f, 1.f, 0.f, 0.f, 0.f,
      0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 1.f, 1.f,
      1.f, 1.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f,
      0.f, 0.f, 0.f, 1.f, 1.f, 1.f, 1.f);

  const SoaFloat4x4 invert = Invert(m2);
  EXPECT_SOAFLOAT4x4_EQ(invert, .5f, .216667f, 0.f, 1.f, 0.f, 2.75f, 0.f, 0.f,
                        0.f, 1.6f, 1.f, 0.f, 0.f, .066666f, 0.f, 0.f, 0.f, .2f,
                        0.f, 0.f, 0.f, 2.5f, 1.f, 1.f, .333333f, 1.4f, 0.f, 0.f,
                        0.f, .1f, 0.f, 0.f, 0.f, .25f, -1.f, 0.f, -.5f, .5f,
                        0.f, 0.f, 0.f, .25f, 0.f, 1.f, 0.f, 0.f, 0.f, 0.f, 0.f,
                        .233333f, 0.f, 0.f, 6.f, .5f, 0.f, 0.f, -15.33333f, .3f,
                        0.f, 0.f, 1.f, .03333f, 1.f, 1.f);

  const SoaFloat4x4 invert_mul = m2 * invert;
  EXPECT_SOAFLOAT4x4_EQ(invert_mul, 1.f, 1.f, 1.f, 1.f, 0.f, 0.f, 0.f, 0.f, 0.f,
                        0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f,
                        1.f, 1.f, 1.f, 1.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f,
                        0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 1.f, 1.f,
                        1.f, 1.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f,
                        0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 1.f, 1.f, 1.f, 1.f);

  // EXPECT_ASSERTION(Invert(m0), "Matrix is not invertible");

  // Invertible
  ozz::math::SimdInt4 invertible;
  EXPECT_SOAFLOAT4x4_EQ(
      Invert(m2, &invertible), .5f, .216667f, 0.f, 1.f, 0.f, 2.75f, 0.f, 0.f,
      0.f, 1.6f, 1.f, 0.f, 0.f, .066666f, 0.f, 0.f, 0.f, .2f, 0.f, 0.f, 0.f,
      2.5f, 1.f, 1.f, .333333f, 1.4f, 0.f, 0.f, 0.f, .1f, 0.f, 0.f, 0.f, .25f,
      -1.f, 0.f, -.5f, .5f, 0.f, 0.f, 0.f, .25f, 0.f, 1.f, 0.f, 0.f, 0.f, 0.f,
      0.f, .233333f, 0.f, 0.f, 6.f, .5f, 0.f, 0.f, -15.33333f, .3f, 0.f, 0.f,
      1.f, .03333f, 1.f, 1.f);
  EXPECT_SIMDINT_EQ(invertible, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff);

  // Non invertible
  EXPECT_ASSERTION(Invert(m0), "Matrix is not invertible");

  ozz::math::SimdInt4 not_invertible;
  EXPECT_SOAFLOAT4x4_EQ(
      Invert(m0, &not_invertible), 0.f, 1.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f,
      0.f, 0.f, 1.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 1.f,
      0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, -1.f, 0.f, 0.f,
      0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f,
      0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 1.f);
  EXPECT_SIMDINT_EQ(not_invertible, 0, 0xffffffff, 0, 0xffffffff);
}

TEST(SoaFloat4x4Scale, ozz_soa_math) {
  const SoaFloat4x4 m0 = {
      {{ozz::math::simd_float4::Load(0.f, 1.f, 0.f, 0.f),
        ozz::math::simd_float4::Load(1.f, 0.f, -1.f, 0.f),
        ozz::math::simd_float4::Load(2.f, 0.f, 2.f, -1.f),
        ozz::math::simd_float4::Load(3.f, 0.f, 3.f, 0.f)},
       {ozz::math::simd_float4::Load(4.f, 0.f, -4.f, 0.f),
        ozz::math::simd_float4::Load(5.f, 1.f, 5.f, 1.f),
        ozz::math::simd_float4::Load(6.f, 0.f, 6.f, 0.f),
        ozz::math::simd_float4::Load(7.f, 0.f, -7.f, 0.f)},
       {ozz::math::simd_float4::Load(8.f, 0.f, 8.f, 1.f),
        ozz::math::simd_float4::Load(9.f, 0.f, -9.f, 0.f),
        ozz::math::simd_float4::Load(10.f, 1.f, -10.f, 0.f),
        ozz::math::simd_float4::Load(11.f, 0.f, 11.f, 0.f)},
       {ozz::math::simd_float4::Load(12.f, 0.f, -12.f, 0.f),
        ozz::math::simd_float4::Load(13.f, 0.f, 13.f, 0.f),
        ozz::math::simd_float4::Load(14.f, 0.f, -14.f, 0.f),
        ozz::math::simd_float4::Load(15.f, 1.f, 15.f, 1.f)}}};
  const SoaFloat4 v = {ozz::math::simd_float4::Load(0.f, 1.f, -2.f, 3.f),
                       ozz::math::simd_float4::Load(-1.f, 2.f, 5.f, 46.f),
                       ozz::math::simd_float4::Load(-2.f, 3.f, 7.f, -1.f),
                       ozz::math::simd_float4::Load(-3.f, 4.f, 0.f, 1.f)};

  const SoaFloat4x4 scaling = SoaFloat4x4::Scaling(v);
  EXPECT_SOAFLOAT4x4_EQ(scaling, 0.f, 1.f, -2.f, 3.f, 0.f, 0.f, 0.f, 0.f, 0.f,
                        0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f,
                        -1.f, 2.f, 5.f, 46.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f,
                        0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, -2.f, 3.f,
                        7.f, -1.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f,
                        0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 1.f, 1.f, 1.f, 1.f);

  const SoaFloat4x4 scale_mul = m0 * scaling;
  EXPECT_SOAFLOAT4x4_EQ(scale_mul, 0.f, 1.f, 0.f, 0.f, 0.f, 0.f, 2.f, 0.f, 0.f,
                        0.f, -4.f, -3.f, 0.f, 0.f, -6.f, 0.f, -4.f, 0.f, -20.f,
                        0.f, -5.f, 2.f, 25.f, 46.f, -6.f, 0.f, 30.f, 0.f, -7.f,
                        0.f, -35.f, 0.f, -16.f, 0.f, 56.f, -1.f, -18.f, 0.f,
                        -63.f, 0.f, -20.f, 3.f, -70.f, 0.f, -22.f, 0.f, 77.f,
                        0.f, 12.f, 0.f, -12.f, 0.f, 13.f, 0.f, 13.f, 0.f, 14.f,
                        0.f, -14.f, 0.f, 15.f, 1.f, 15.f, 1.f);

  const SoaFloat4x4 scale = Scale(m0, v);
  EXPECT_SOAFLOAT4x4_EQ(scale, 0.f, 1.f, 0.f, 0.f, 0.f, 0.f, 2.f, 0.f, 0.f, 0.f,
                        -4.f, -3.f, 0.f, 0.f, -6.f, 0.f, -4.f, 0.f, -20.f, 0.f,
                        -5.f, 2.f, 25.f, 46.f, -6.f, 0.f, 30.f, 0.f, -7.f, 0.f,
                        -35.f, 0.f, -16.f, 0.f, 56.f, -1.f, -18.f, 0.f, -63.f,
                        0.f, -20.f, 3.f, -70.f, 0.f, -22.f, 0.f, 77.f, 0.f,
                        12.f, 0.f, -12.f, 0.f, 13.f, 0.f, 13.f, 0.f, 14.f, 0.f,
                        -14.f, 0.f, 15.f, 1.f, 15.f, 1.f);
}

TEST(SoaFloat4x4Rotate, ozz_soa_math) {
#ifndef NDEBUG
  const SoaQuaternion unormalized =
      SoaQuaternion::Load(ozz::math::simd_float4::Load(0.f, 0.f, 0.f, 0.f),
                          ozz::math::simd_float4::Load(0.f, 0.f, 1.f, 0.f),
                          ozz::math::simd_float4::Load(0.f, 0.f, 0.f, 0.f),
                          ozz::math::simd_float4::Load(1.f, 1.f, 1.f, 1.f));
#endif  // NDEBUG
  EXPECT_ASSERTION(SoaFloat4x4::FromQuaternion(unormalized), "IsNormalized");

  const SoaFloat4x4 identity =
      SoaFloat4x4::FromQuaternion(SoaQuaternion::identity());
  EXPECT_SOAFLOAT4x4_EQ(identity, 1.f, 1.f, 1.f, 1.f, 0.f, 0.f, 0.f, 0.f, 0.f,
                        0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f,
                        1.f, 1.f, 1.f, 1.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f,
                        0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 1.f, 1.f,
                        1.f, 1.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f,
                        0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 1.f, 1.f, 1.f, 1.f);
  const SoaQuaternion quaternion = SoaQuaternion::Load(
      ozz::math::simd_float4::Load(.70710677f, 0.f, 0.f, -.382683432f),
      ozz::math::simd_float4::Load(0.f, .70710677f, 0.f, 0.f),
      ozz::math::simd_float4::Load(.70710677f, 0.f, 0.f, 0.f),
      ozz::math::simd_float4::Load(0.f, .70710677f, 1.f, .9238795f));
  const SoaFloat4x4 matrix = SoaFloat4x4::FromQuaternion(quaternion);
  EXPECT_SOAFLOAT4x4_EQ(
      matrix, 0.f, 0.f, 1.f, 1.f, 0.f, 0.f, 0.f, 0.f, 1.f, -1.f, 0.f, 0.f, 0.f,
      0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, -1.f, 1.f, 1.f, .707106f, 0.f, 0.f,
      0.f, -.707106f, 0.f, 0.f, 0.f, 0.f, 1.f, 1.f, 0.f, 0.f, 0.f, 0.f, 0.f,
      .707106f, 0.f, 0.f, 1.f, .707106f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f,
      0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 1.f, 1.f, 1.f, 1.f);
}

TEST(SoaFloat4x4Affine, ozz_soa_math) {
  const SoaFloat4x4 identity = SoaFloat4x4::FromAffine(
      SoaFloat3::zero(), SoaQuaternion::identity(), SoaFloat3::one());
  EXPECT_SOAFLOAT4x4_EQ(identity, 1.f, 1.f, 1.f, 1.f, 0.f, 0.f, 0.f, 0.f, 0.f,
                        0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f,
                        1.f, 1.f, 1.f, 1.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f,
                        0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 1.f, 1.f,
                        1.f, 1.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f,
                        0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 1.f, 1.f, 1.f, 1.f);
  const SoaFloat3 translation =
      SoaFloat3::Load(ozz::math::simd_float4::Load(0.f, 46.f, 7.f, -12.f),
                      ozz::math::simd_float4::Load(0.f, 12.f, 7.f, -46.f),
                      ozz::math::simd_float4::Load(0.f, 0.f, 7.f, 46.f));
  const SoaFloat3 scale =
      SoaFloat3::Load(ozz::math::simd_float4::Load(1.f, 1.f, -1.f, 0.1f),
                      ozz::math::simd_float4::Load(1.f, 2.f, -1.f, 0.1f),
                      ozz::math::simd_float4::Load(1.f, 3.f, -1.f, 0.1f));
  const SoaQuaternion quaternion = SoaQuaternion::Load(
      ozz::math::simd_float4::Load(.70710677f, 0.f, 0.f, -.382683432f),
      ozz::math::simd_float4::Load(0.f, .70710677f, 0.f, 0.f),
      ozz::math::simd_float4::Load(.70710677f, 0.f, 0.f, 0.f),
      ozz::math::simd_float4::Load(0.f, .70710677f, 1.f, .9238795f));
  const SoaFloat4x4 matrix =
      SoaFloat4x4::FromAffine(translation, quaternion, scale);
  EXPECT_SOAFLOAT4x4_EQ(
      matrix, 0.f, 0.f, -1.f, .1f, 0.f, 0.f, 0.f, 0.f, 1.f, -1.f, 0.f, 0.f, 0.f,
      0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, -1.f, 2.f, -1.f, .0707106f, 0.f, 0.f,
      0.f, -.0707106f, 0.f, 0.f, 0.f, 0.f, 1.f, 3.f, 0.f, 0.f, 0.f, 0.f, 0.f,
      .0707106f, 0.f, 0.f, -1.f, .0707106f, 0.f, 0.f, 0.f, 0.f, 0.f, 46.f, 7.f,
      -12.f, 0.f, 12.f, 7.f, -46.f, 0.f, 0.f, 7.f, 46.f, 1.f, 1.f, 1.f, 1.f);
}
