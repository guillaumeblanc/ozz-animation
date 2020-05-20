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

#include "ozz/base/maths/vec_float.h"

#include "gtest/gtest.h"

#include "ozz/base/gtest_helper.h"
#include "ozz/base/maths/gtest_math_helper.h"

using ozz::math::Float2;
using ozz::math::Float3;
using ozz::math::Float4;

TEST(VectorLoad4, ozz_math) {
  EXPECT_FLOAT4_EQ(Float4(46.f), 46.f, 46.f, 46.f, 46.f);
  EXPECT_FLOAT4_EQ(Float4(-1.f, 0.f, 1.f, 2.f), -1.f, 0.f, 1.f, 2.f);
  const Float3 f3(-1.f, 0.f, 1.f);
  EXPECT_FLOAT4_EQ(Float4(f3, 2.f), -1.f, 0.f, 1.f, 2.f);
  const Float2 f2(-1.f, 0.f);
  EXPECT_FLOAT4_EQ(Float4(f2, 1.f, 2.f), -1.f, 0.f, 1.f, 2.f);
}

TEST(VectorLoad3, ozz_math) {
  EXPECT_FLOAT3_EQ(Float3(46.f), 46.f, 46.f, 46.f);
  EXPECT_FLOAT3_EQ(Float3(-1.f, 0.f, 1.f), -1.f, 0.f, 1.f);
  const Float2 f2(-1.f, 0.f);
  EXPECT_FLOAT3_EQ(Float3(f2, 1.f), -1.f, 0.f, 1.f);
}

TEST(VectorLoad2, ozz_math) {
  EXPECT_FLOAT2_EQ(Float2(46.f), 46.f, 46.f);
  EXPECT_FLOAT2_EQ(Float2(-1.f, 0.f), -1.f, 0.f);
}

TEST(VectorConstant4, ozz_math) {
  EXPECT_FLOAT4_EQ(Float4::zero(), 0.f, 0.f, 0.f, 0.f);
  EXPECT_FLOAT4_EQ(Float4::one(), 1.f, 1.f, 1.f, 1.f);
  EXPECT_FLOAT4_EQ(Float4::x_axis(), 1.f, 0.f, 0.f, 0.f);
  EXPECT_FLOAT4_EQ(Float4::y_axis(), 0.f, 1.f, 0.f, 0.f);
  EXPECT_FLOAT4_EQ(Float4::z_axis(), 0.f, 0.f, 1.f, 0.f);
  EXPECT_FLOAT4_EQ(Float4::w_axis(), 0.f, 0.f, 0.f, 1.f);
}

TEST(VectorConstant3, ozz_math) {
  EXPECT_FLOAT3_EQ(Float3::zero(), 0.f, 0.f, 0.f);
  EXPECT_FLOAT3_EQ(Float3::one(), 1.f, 1.f, 1.f);
  EXPECT_FLOAT3_EQ(Float3::x_axis(), 1.f, 0.f, 0.f);
  EXPECT_FLOAT3_EQ(Float3::y_axis(), 0.f, 1.f, 0.f);
  EXPECT_FLOAT3_EQ(Float3::z_axis(), 0.f, 0.f, 1.f);
}

TEST(VectorConstant2, ozz_math) {
  EXPECT_FLOAT2_EQ(Float2::zero(), 0.f, 0.f);
  EXPECT_FLOAT2_EQ(Float2::one(), 1.f, 1.f);
  EXPECT_FLOAT2_EQ(Float2::x_axis(), 1.f, 0.f);
  EXPECT_FLOAT2_EQ(Float2::y_axis(), 0.f, 1.f);
}

TEST(VectorArithmetic4, ozz_math) {
  const Float4 a(.5f, 1.f, 2.f, 3.f);
  const Float4 b(4.f, 5.f, -6.f, 7.f);

  const Float4 add = a + b;
  EXPECT_FLOAT4_EQ(add, 4.5f, 6.f, -4.f, 10.f);

  const Float4 sub = a - b;
  EXPECT_FLOAT4_EQ(sub, -3.5f, -4.f, 8.f, -4.f);

  const Float4 neg = -b;
  EXPECT_FLOAT4_EQ(neg, -4.f, -5.f, 6.f, -7.f);

  const Float4 mul = a * b;
  EXPECT_FLOAT4_EQ(mul, 2.f, 5.f, -12.f, 21.f);

  const Float4 mul_scal = a * 2.f;
  EXPECT_FLOAT4_EQ(mul_scal, 1.f, 2.f, 4.f, 6.f);

  const Float4 div = a / b;
  EXPECT_FLOAT4_EQ(div, .5f / 4.f, 1.f / 5.f, -2.f / 6.f, 3.f / 7.f);

  const Float4 div_scal = a / 2.f;
  EXPECT_FLOAT4_EQ(div_scal, .5f / 2.f, 1.f / 2.f, 2.f / 2.f, 3.f / 2.f);

  const float hadd4 = HAdd(a);
  EXPECT_FLOAT_EQ(hadd4, 6.5f);

  const float dot = Dot(a, b);
  EXPECT_FLOAT_EQ(dot, 16.f);

  const float length = Length(a);
  EXPECT_FLOAT_EQ(length, std::sqrt(14.25f));

  const float length2 = LengthSqr(a);
  EXPECT_FLOAT_EQ(length2, 14.25f);

  EXPECT_ASSERTION(Normalize(Float4::zero()), "is not normalizable");
  EXPECT_FALSE(IsNormalized(a));
  const Float4 normalize = Normalize(a);
  EXPECT_TRUE(IsNormalized(normalize));
  EXPECT_FLOAT4_EQ(normalize, .13245323f, .26490647f, .52981293f, .79471946f);

  EXPECT_ASSERTION(NormalizeSafe(a, a), "_safer is not normalized");
  const Float4 safe(1.f, 0.f, 0.f, 0.f);
  const Float4 normalize_safe = NormalizeSafe(a, safe);
  EXPECT_TRUE(IsNormalized(normalize_safe));
  EXPECT_FLOAT4_EQ(normalize_safe, .13245323f, .26490647f, .52981293f,
                   .79471946f);

  const Float4 normalize_safer = NormalizeSafe(Float4::zero(), safe);
  EXPECT_TRUE(IsNormalized(normalize_safer));
  EXPECT_FLOAT4_EQ(normalize_safer, safe.x, safe.y, safe.z, safe.w);

  const Float4 lerp_0 = Lerp(a, b, 0.f);
  EXPECT_FLOAT4_EQ(lerp_0, a.x, a.y, a.z, a.w);

  const Float4 lerp_1 = Lerp(a, b, 1.f);
  EXPECT_FLOAT4_EQ(lerp_1, b.x, b.y, b.z, b.w);

  const Float4 lerp_0_5 = Lerp(a, b, .5f);
  EXPECT_FLOAT4_EQ(lerp_0_5, (a.x + b.x) * .5f, (a.y + b.y) * .5f,
                   (a.z + b.z) * .5f, (a.w + b.w) * .5f);

  const Float4 lerp_2 = Lerp(a, b, 2.f);
  EXPECT_FLOAT4_EQ(lerp_2, 2.f * b.x - a.x, 2.f * b.y - a.y, 2.f * b.z - a.z,
                   2.f * b.w - a.w);
}

TEST(VectorArithmetic3, ozz_math) {
  const Float3 a(.5f, 1.f, 2.f);
  const Float3 b(4.f, 5.f, -6.f);

  const Float3 add = a + b;
  EXPECT_FLOAT3_EQ(add, 4.5f, 6.f, -4.f);

  const Float3 sub = a - b;
  EXPECT_FLOAT3_EQ(sub, -3.5f, -4.f, 8.f);

  const Float3 neg = -b;
  EXPECT_FLOAT3_EQ(neg, -4.f, -5.f, 6.f);

  const Float3 mul = a * b;
  EXPECT_FLOAT3_EQ(mul, 2.f, 5.f, -12.f);

  const Float3 mul_scal = a * 2.f;
  EXPECT_FLOAT3_EQ(mul_scal, 1.f, 2.f, 4.f);

  const Float3 div = a / b;
  EXPECT_FLOAT3_EQ(div, .5f / 4.f, 1.f / 5.f, -2.f / 6.f);

  const Float3 div_scal = a / 2.f;
  EXPECT_FLOAT3_EQ(div_scal, .5f / 2.f, 1.f / 2.f, 2.f / 2.f);

  const float hadd4 = HAdd(a);
  EXPECT_FLOAT_EQ(hadd4, 3.5f);

  const float dot = Dot(a, b);
  EXPECT_FLOAT_EQ(dot, -5.f);

  const Float3 cross = Cross(a, b);
  EXPECT_FLOAT3_EQ(cross, -16.f, 11.f, -1.5f);

  const float length = Length(a);
  EXPECT_FLOAT_EQ(length, std::sqrt(5.25f));

  const float length2 = LengthSqr(a);
  EXPECT_FLOAT_EQ(length2, 5.25f);

  EXPECT_ASSERTION(Normalize(Float3::zero()), "is not normalizable");
  EXPECT_FALSE(IsNormalized(a));
  const Float3 normalize = Normalize(a);
  EXPECT_TRUE(IsNormalized(normalize));
  EXPECT_FLOAT3_EQ(normalize, .21821788f, .43643576f, .87287152f);

  EXPECT_ASSERTION(NormalizeSafe(a, a), "_safer is not normalized");
  const Float3 safe(1.f, 0.f, 0.f);
  const Float3 normalize_safe = NormalizeSafe(a, safe);
  EXPECT_TRUE(IsNormalized(normalize_safe));
  EXPECT_FLOAT3_EQ(normalize_safe, .21821788f, .43643576f, .87287152f);

  const Float3 normalize_safer = NormalizeSafe(Float3::zero(), safe);
  EXPECT_TRUE(IsNormalized(normalize_safer));
  EXPECT_FLOAT3_EQ(normalize_safer, safe.x, safe.y, safe.z);

  const Float3 lerp_0 = Lerp(a, b, 0.f);
  EXPECT_FLOAT3_EQ(lerp_0, a.x, a.y, a.z);

  const Float3 lerp_1 = Lerp(a, b, 1.f);
  EXPECT_FLOAT3_EQ(lerp_1, b.x, b.y, b.z);

  const Float3 lerp_0_5 = Lerp(a, b, .5f);
  EXPECT_FLOAT3_EQ(lerp_0_5, (a.x + b.x) * .5f, (a.y + b.y) * .5f,
                   (a.z + b.z) * .5f);

  const Float3 lerp_2 = Lerp(a, b, 2.f);
  EXPECT_FLOAT3_EQ(lerp_2, 2.f * b.x - a.x, 2.f * b.y - a.y, 2.f * b.z - a.z);
}

TEST(VectorArithmetic2, ozz_math) {
  const Float2 a(.5f, 1.f);
  const Float2 b(4.f, 5.f);

  const Float2 add = a + b;
  EXPECT_FLOAT2_EQ(add, 4.5f, 6.f);

  const Float2 sub = a - b;
  EXPECT_FLOAT2_EQ(sub, -3.5f, -4.f);

  const Float2 neg = -b;
  EXPECT_FLOAT2_EQ(neg, -4.f, -5.f);

  const Float2 mul = a * b;
  EXPECT_FLOAT2_EQ(mul, 2.f, 5.f);

  const Float2 mul_scal = a * 2.f;
  EXPECT_FLOAT2_EQ(mul_scal, 1.f, 2.f);

  const Float2 div = a / b;
  EXPECT_FLOAT2_EQ(div, .5f / 4.f, 1.f / 5.f);
  const Float2 div_scal = a / 2.f;
  EXPECT_FLOAT2_EQ(div_scal, .5f / 2.f, 1.f / 2.f);

  const float hadd4 = HAdd(a);
  EXPECT_FLOAT_EQ(hadd4, 1.5f);

  const float dot = Dot(a, b);
  EXPECT_FLOAT_EQ(dot, 7.f);

  const float length = Length(a);
  EXPECT_FLOAT_EQ(length, std::sqrt(1.25f));

  const float length2 = LengthSqr(a);
  EXPECT_FLOAT_EQ(length2, 1.25f);

  EXPECT_ASSERTION(Normalize(Float2::zero()), "is not normalizable");
  EXPECT_FALSE(IsNormalized(a));
  const Float2 normalize = Normalize(a);
  EXPECT_TRUE(IsNormalized(normalize));
  EXPECT_FLOAT2_EQ(normalize, .44721359f, .89442718f);

  EXPECT_ASSERTION(NormalizeSafe(a, a), "_safer is not normalized");
  const Float2 safe(1.f, 0.f);
  const Float2 normalize_safe = NormalizeSafe(a, safe);
  EXPECT_TRUE(IsNormalized(normalize_safe));
  EXPECT_FLOAT2_EQ(normalize_safe, .44721359f, .89442718f);

  const Float2 normalize_safer = NormalizeSafe(Float2::zero(), safe);
  EXPECT_TRUE(IsNormalized(normalize_safer));
  EXPECT_FLOAT2_EQ(normalize_safer, safe.x, safe.y);

  const Float2 lerp_0 = Lerp(a, b, 0.f);
  EXPECT_FLOAT2_EQ(lerp_0, a.x, a.y);

  const Float2 lerp_1 = Lerp(a, b, 1.f);
  EXPECT_FLOAT2_EQ(lerp_1, b.x, b.y);

  const Float2 lerp_0_5 = Lerp(a, b, .5f);
  EXPECT_FLOAT2_EQ(lerp_0_5, (a.x + b.x) * .5f, (a.y + b.y) * .5f);

  const Float2 lerp_2 = Lerp(a, b, 2.f);
  EXPECT_FLOAT2_EQ(lerp_2, 2.f * b.x - a.x, 2.f * b.y - a.y);
}

TEST(VectorComparison4, ozz_math) {
  const Float4 a(.5f, 1.f, 2.f, 3.f);
  const Float4 b(4.f, 5.f, -6.f, 7.f);
  const Float4 c(4.f, 5.f, 6.f, 7.f);
  const Float4 d(4.f, 5.f, 6.f, 7.1f);

  const Float4 min = Min(a, b);
  EXPECT_FLOAT4_EQ(min, .5f, 1.f, -6.f, 3.f);

  const Float4 max = Max(a, b);
  EXPECT_FLOAT4_EQ(max, 4.f, 5.f, 2.f, 7.f);

  EXPECT_FLOAT4_EQ(Clamp(a, Float4(-12.f, 2.f, 9.f, 3.f), c), .5f, 2.f, 6.f,
                   3.f);

  EXPECT_TRUE(a < c);
  EXPECT_TRUE(a <= c);
  EXPECT_TRUE(c <= c);

  EXPECT_TRUE(c > a);
  EXPECT_TRUE(c >= a);
  EXPECT_TRUE(a >= a);

  EXPECT_TRUE(a == a);
  EXPECT_TRUE(a != b);

  EXPECT_TRUE(Compare(a, a, 0.f));
  EXPECT_TRUE(Compare(c, d, .2f));
  EXPECT_FALSE(Compare(c, d, .05f));
}

TEST(VectorComparison3, ozz_math) {
  const Float3 a(.5f, -1.f, 2.f);
  const Float3 b(4.f, 5.f, -6.f);
  const Float3 c(4.f, 5.f, 6.f);
  const Float3 d(4.f, 5.f, 6.1f);

  const Float3 min = Min(a, b);
  EXPECT_FLOAT3_EQ(min, .5f, -1.f, -6.f);

  const Float3 max = Max(a, b);
  EXPECT_FLOAT3_EQ(max, 4.f, 5.f, 2.f);

  EXPECT_FLOAT3_EQ(Clamp(a, Float3(-12.f, 2.f, 9.f), c), .5f, 2.f, 6.f);

  EXPECT_TRUE(a < c);
  EXPECT_TRUE(a <= c);
  EXPECT_TRUE(c <= c);

  EXPECT_TRUE(c > a);
  EXPECT_TRUE(c >= a);
  EXPECT_TRUE(a >= a);

  EXPECT_TRUE(a == a);
  EXPECT_TRUE(a != b);

  EXPECT_TRUE(Compare(a, a, 1e-3f));
  EXPECT_TRUE(Compare(c, d, .2f));
  EXPECT_FALSE(Compare(c, d, .05f));
}

TEST(VectorComparison2, ozz_math) {
  const Float2 a(.5f, 1.f);
  const Float2 b(4.f, -5.f);
  const Float2 c(4.f, 5.f);
  const Float2 d(4.f, 5.1f);

  const Float2 min = Min(a, b);
  EXPECT_FLOAT2_EQ(min, .5f, -5.f);

  const Float2 max = Max(a, b);
  EXPECT_FLOAT2_EQ(max, 4.f, 1.f);

  EXPECT_FLOAT2_EQ(Clamp(a, Float2(-12.f, 2.f), c), .5f, 2.f);

  EXPECT_TRUE(a < c);
  EXPECT_TRUE(a <= c);
  EXPECT_TRUE(c <= c);

  EXPECT_TRUE(c > a);
  EXPECT_TRUE(c >= a);
  EXPECT_TRUE(a >= a);

  EXPECT_TRUE(a == a);
  EXPECT_TRUE(a != b);

  EXPECT_TRUE(Compare(a, a, 1e-3f));
  EXPECT_TRUE(Compare(c, d, .2f));
  EXPECT_FALSE(Compare(c, d, .05f));
}
