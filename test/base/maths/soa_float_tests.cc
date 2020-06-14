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

#include "ozz/base/maths/soa_float.h"

#include "gtest/gtest.h"

#include "ozz/base/gtest_helper.h"
#include "ozz/base/maths/gtest_math_helper.h"

using ozz::math::SimdFloat4;
using ozz::math::SoaFloat2;
using ozz::math::SoaFloat3;
using ozz::math::SoaFloat4;

TEST(SoaFloatLoad4, ozz_soa_math) {
  EXPECT_SOAFLOAT4_EQ(
      SoaFloat4::Load(ozz::math::simd_float4::Load(0.f, 1.f, 2.f, 3.f),
                      ozz::math::simd_float4::Load(4.f, 5.f, 6.f, 7.f),
                      ozz::math::simd_float4::Load(8.f, 9.f, 10.f, 11.f),
                      ozz::math::simd_float4::Load(12.f, 13.f, 14.f, 15.f)),
      0.f, 1.f, 2.f, 3.f, 4.f, 5.f, 6.f, 7.f, 8.f, 9.f, 10.f, 11.f, 12.f, 13.f,
      14.f, 15.f);
  EXPECT_SOAFLOAT4_EQ(
      SoaFloat4::Load(
          SoaFloat3::Load(ozz::math::simd_float4::Load(0.f, 1.f, 2.f, 3.f),
                          ozz::math::simd_float4::Load(4.f, 5.f, 6.f, 7.f),
                          ozz::math::simd_float4::Load(8.f, 9.f, 10.f, 11.f)),
          ozz::math::simd_float4::Load(12.f, 13.f, 14.f, 15.f)),
      0.f, 1.f, 2.f, 3.f, 4.f, 5.f, 6.f, 7.f, 8.f, 9.f, 10.f, 11.f, 12.f, 13.f,
      14.f, 15.f);
  EXPECT_SOAFLOAT4_EQ(
      SoaFloat4::Load(
          SoaFloat2::Load(ozz::math::simd_float4::Load(0.f, 1.f, 2.f, 3.f),
                          ozz::math::simd_float4::Load(4.f, 5.f, 6.f, 7.f)),
          ozz::math::simd_float4::Load(8.f, 9.f, 10.f, 11.f),
          ozz::math::simd_float4::Load(12.f, 13.f, 14.f, 15.f)),
      0.f, 1.f, 2.f, 3.f, 4.f, 5.f, 6.f, 7.f, 8.f, 9.f, 10.f, 11.f, 12.f, 13.f,
      14.f, 15.f);
}

TEST(SoaFloatLoad3, ozz_soa_math) {
  EXPECT_SOAFLOAT3_EQ(
      SoaFloat3::Load(ozz::math::simd_float4::Load(0.f, 1.f, 2.f, 3.f),
                      ozz::math::simd_float4::Load(4.f, 5.f, 6.f, 7.f),
                      ozz::math::simd_float4::Load(8.f, 9.f, 10.f, 11.f)),
      0.f, 1.f, 2.f, 3.f, 4.f, 5.f, 6.f, 7.f, 8.f, 9.f, 10.f, 11.f);
  EXPECT_SOAFLOAT3_EQ(
      SoaFloat3::Load(
          SoaFloat2::Load(ozz::math::simd_float4::Load(0.f, 1.f, 2.f, 3.f),
                          ozz::math::simd_float4::Load(4.f, 5.f, 6.f, 7.f)),
          ozz::math::simd_float4::Load(8.f, 9.f, 10.f, 11.f)),
      0.f, 1.f, 2.f, 3.f, 4.f, 5.f, 6.f, 7.f, 8.f, 9.f, 10.f, 11.f);
}

TEST(SoaFloatLoad2, ozz_soa_math) {
  EXPECT_SOAFLOAT2_EQ(
      SoaFloat2::Load(ozz::math::simd_float4::Load(0.f, 1.f, 2.f, 3.f),
                      ozz::math::simd_float4::Load(4.f, 5.f, 6.f, 7.f)),
      0.f, 1.f, 2.f, 3.f, 4.f, 5.f, 6.f, 7.f);
}

TEST(SoaFloatConstant4, ozz_soa_math) {
  EXPECT_SOAFLOAT4_EQ(SoaFloat4::zero(), 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f,
                      0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f);
  EXPECT_SOAFLOAT4_EQ(SoaFloat4::one(), 1.f, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f,
                      1.f, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f);
  EXPECT_SOAFLOAT4_EQ(SoaFloat4::x_axis(), 1.f, 1.f, 1.f, 1.f, 0.f, 0.f, 0.f,
                      0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f);
  EXPECT_SOAFLOAT4_EQ(SoaFloat4::y_axis(), 0.f, 0.f, 0.f, 0.f, 1.f, 1.f, 1.f,
                      1.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f);
  EXPECT_SOAFLOAT4_EQ(SoaFloat4::z_axis(), 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f,
                      0.f, 1.f, 1.f, 1.f, 1.f, 0.f, 0.f, 0.f, 0.f);
  EXPECT_SOAFLOAT4_EQ(SoaFloat4::w_axis(), 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f,
                      0.f, 0.f, 0.f, 0.f, 0.f, 1.f, 1.f, 1.f, 1.f);
}

TEST(SoaFloatConstant3, ozz_soa_math) {
  EXPECT_SOAFLOAT3_EQ(SoaFloat3::zero(), 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f,
                      0.f, 0.f, 0.f, 0.f);
  EXPECT_SOAFLOAT3_EQ(SoaFloat3::one(), 1.f, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f,
                      1.f, 1.f, 1.f, 1.f);
  EXPECT_SOAFLOAT3_EQ(SoaFloat3::x_axis(), 1.f, 1.f, 1.f, 1.f, 0.f, 0.f, 0.f,
                      0.f, 0.f, 0.f, 0.f, 0.f);
  EXPECT_SOAFLOAT3_EQ(SoaFloat3::y_axis(), 0.f, 0.f, 0.f, 0.f, 1.f, 1.f, 1.f,
                      1.f, 0.f, 0.f, 0.f, 0.f);
  EXPECT_SOAFLOAT3_EQ(SoaFloat3::z_axis(), 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f,
                      0.f, 1.f, 1.f, 1.f, 1.f);
}

TEST(SoaFloatConstant2, ozz_soa_math) {
  EXPECT_SOAFLOAT2_EQ(SoaFloat2::zero(), 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f,
                      0.f);
  EXPECT_SOAFLOAT2_EQ(SoaFloat2::one(), 1.f, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f);
  EXPECT_SOAFLOAT2_EQ(SoaFloat2::x_axis(), 1.f, 1.f, 1.f, 1.f, 0.f, 0.f, 0.f,
                      0.f);
  EXPECT_SOAFLOAT2_EQ(SoaFloat2::y_axis(), 0.f, 0.f, 0.f, 0.f, 1.f, 1.f, 1.f,
                      1.f);
}

TEST(SoaFloatArithmetic4, ozz_soa_math) {
  const SoaFloat4 a = {ozz::math::simd_float4::Load(.5f, 1.f, 2.f, 3.f),
                       ozz::math::simd_float4::Load(4.f, 5.f, 6.f, 7.f),
                       ozz::math::simd_float4::Load(8.f, 9.f, 10.f, 11.f),
                       ozz::math::simd_float4::Load(12.f, 13.f, 14.f, 15.f)};
  const SoaFloat4 b = {
      ozz::math::simd_float4::Load(-.5f, -1.f, -2.f, -3.f),
      ozz::math::simd_float4::Load(-4.f, -5.f, -6.f, -7.f),
      ozz::math::simd_float4::Load(-8.f, -9.f, -10.f, -11.f),
      ozz::math::simd_float4::Load(-12.f, -13.f, -14.f, -15.f)};
  const SoaFloat4 c = {ozz::math::simd_float4::Load(.05f, .1f, .2f, .3f),
                       ozz::math::simd_float4::Load(.4f, .5f, .6f, .7f),
                       ozz::math::simd_float4::Load(.8f, .9f, 1.f, 1.1f),
                       ozz::math::simd_float4::Load(1.2f, 1.3f, 1.4f, 1.5f)};

  const SoaFloat4 add = a + b;
  EXPECT_SOAFLOAT4_EQ(add, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f,
                      0.f, 0.f, 0.f, 0.f, 0.f, 0.f);

  const SoaFloat4 sub = a - b;
  EXPECT_SOAFLOAT4_EQ(sub, 1.f, 2.f, 4.f, 6.f, 8.f, 10.f, 12.f, 14.f, 16.f,
                      18.f, 20.f, 22.f, 24.f, 26.f, 28.f, 30.f);

  const SoaFloat4 neg = -a;
  EXPECT_SOAFLOAT4_EQ(neg, -.5f, -1.f, -2.f, -3.f, -4.f, -5.f, -6.f, -7.f, -8.f,
                      -9.f, -10.f, -11.f, -12.f, -13.f, -14.f, -15.f);

  const SoaFloat4 mul = a * b;
  EXPECT_SOAFLOAT4_EQ(mul, -0.25f, -1.f, -4.f, -9.f, -16.f, -25.f, -36.f, -49.f,
                      -64.f, -81.f, -100.f, -121.f, -144.f, -169.f, -196.f,
                      -225.f);

  const SoaFloat4 mul_add = MAdd(a, b, c);
  EXPECT_SOAFLOAT4_EQ(mul_add, -0.2f, -.9f, -3.8f, -8.7f, -15.6f, -24.5f,
                      -35.4f, -48.3f, -63.2f, -80.1f, -99.f, -119.9f, -142.8f,
                      -167.7f, -194.6f, -223.5f);

  const SoaFloat4 mul_scal = a * ozz::math::simd_float4::Load1(2.f);
  EXPECT_SOAFLOAT4_EQ(mul_scal, 1.f, 2.f, 4.f, 6.f, 8.f, 10.f, 12.f, 14.f, 16.f,
                      18.f, 20.f, 22.f, 24.f, 26.f, 28.f, 30.f);

  const SoaFloat4 div = a / b;
  EXPECT_SOAFLOAT4_EQ(div, -1.f, -1.f, -1.f, -1.f, -1.f, -1.f, -1.f, -1.f, -1.f,
                      -1.f, -1.f, -1.f, -1.f, -1.f, -1.f, -1.f);

  const SoaFloat4 div_scal = a / ozz::math::simd_float4::Load1(2.f);
  EXPECT_SOAFLOAT4_EQ(div_scal, 0.25f, .5f, 1.f, 1.5f, 2.f, 2.5f, 3.f, 3.5f,
                      4.f, 4.5, 5.f, 5.5f, 6.f, 6.5f, 7.f, 7.5f);

  const SimdFloat4 hadd4 = HAdd(a);
  EXPECT_SOAFLOAT1_EQ(hadd4, 24.5f, 28.f, 32.f, 36.f);

  const SimdFloat4 dot = Dot(a, b);
  EXPECT_SOAFLOAT1_EQ(dot, -224.25f, -276.f, -336.f, -404.f);

  const SimdFloat4 length = Length(a);
  EXPECT_SOAFLOAT1_EQ(length, 14.974979f, 16.613247f, 18.3303f, 20.09975f);

  const SimdFloat4 length2 = LengthSqr(a);
  EXPECT_SOAFLOAT1_EQ(length2, 224.25f, 276.f, 336.f, 404.f);

  EXPECT_ASSERTION(Normalize(SoaFloat4::zero()), "_v is not normalizable");
  EXPECT_TRUE(ozz::math::AreAllFalse(IsNormalized(a)));
  EXPECT_TRUE(ozz::math::AreAllFalse(IsNormalizedEst(a)));
  const SoaFloat4 normalize = Normalize(a);
  EXPECT_TRUE(ozz::math::AreAllTrue(IsNormalized(normalize)));
  EXPECT_TRUE(ozz::math::AreAllTrue(IsNormalizedEst(normalize)));
  EXPECT_SOAFLOAT4_EQ(normalize, .033389f, .0601929f, .1091089f, .1492555f,
                      .267112f, .300964f, .3273268f, .348263f, .53422445f,
                      .541736f, .545544f, .547270f, .80133667f, .782508f,
                      .763762f, .74627789f);

  EXPECT_ASSERTION(NormalizeSafe(a, a), "_safer is not normalized");
  const SoaFloat4 safe = ozz::math::SoaFloat4::x_axis();
  const SoaFloat4 normalize_safe = NormalizeSafe(a, safe);
  EXPECT_TRUE(ozz::math::AreAllTrue(IsNormalized(normalize_safe)));
  EXPECT_TRUE(ozz::math::AreAllTrue(IsNormalizedEst(normalize_safe)));
  EXPECT_SOAFLOAT4_EQ(normalize_safe, .033389f, .0601929f, .1091089f, .1492555f,
                      .267112f, .300964f, .3273268f, .348263f, .53422445f,
                      .541736f, .545544f, .547270f, .80133667f, .782508f,
                      .763762f, .74627789f);

  const SoaFloat4 normalize_safer = NormalizeSafe(SoaFloat4::zero(), safe);
  EXPECT_TRUE(ozz::math::AreAllTrue(IsNormalized(normalize_safer)));
  EXPECT_TRUE(ozz::math::AreAllTrue(IsNormalizedEst(normalize_safer)));
  EXPECT_SOAFLOAT4_EQ(normalize_safer, 1.f, 1.f, 1.f, 1.f, 0.f, 0.f, 0.f, 0.f,
                      0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f);

  const SoaFloat4 lerp_0 = Lerp(a, b, ozz::math::simd_float4::zero());
  EXPECT_SOAFLOAT4_EQ(lerp_0, .5f, 1.f, 2.f, 3.f, 4.f, 5.f, 6.f, 7.f, 8.f, 9.f,
                      10.f, 11.f, 12.f, 13.f, 14.f, 15.f);

  const SoaFloat4 lerp_1 = Lerp(a, b, ozz::math::simd_float4::one());
  EXPECT_SOAFLOAT4_EQ(lerp_1, -.5f, -1.f, -2.f, -3.f, -4.f, -5.f, -6.f, -7.f,
                      -8.f, -9.f, -10.f, -11.f, -12.f, -13.f, -14.f, -15.f);

  const SoaFloat4 lerp_0_5 = Lerp(a, b, ozz::math::simd_float4::Load1(.5f));
  EXPECT_SOAFLOAT4_EQ(lerp_0_5, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f,
                      0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f);
}

TEST(SoaFloatArithmetic3, ozz_soa_math) {
  const SoaFloat3 a = {ozz::math::simd_float4::Load(.5f, 1.f, 2.f, 3.f),
                       ozz::math::simd_float4::Load(4.f, 5.f, 6.f, 7.f),
                       ozz::math::simd_float4::Load(8.f, 9.f, 10.f, 11.f)};
  const SoaFloat3 b = {ozz::math::simd_float4::Load(-.5f, -1.f, -2.f, -3.f),
                       ozz::math::simd_float4::Load(-4.f, -5.f, -6.f, -7.f),
                       ozz::math::simd_float4::Load(-8.f, -9.f, -10.f, -11.f)};
  const SoaFloat3 c = {ozz::math::simd_float4::Load(.05f, .1f, .2f, .3f),
                       ozz::math::simd_float4::Load(.4f, .5f, .6f, .7f),
                       ozz::math::simd_float4::Load(.8f, .9f, 1.f, 1.1f)};

  const SoaFloat3 add = a + b;
  EXPECT_SOAFLOAT3_EQ(add, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f,
                      0.f, 0.f);

  const SoaFloat3 sub = a - b;
  EXPECT_SOAFLOAT3_EQ(sub, 1.f, 2.f, 4.f, 6.f, 8.f, 10.f, 12.f, 14.f, 16.f,
                      18.f, 20.f, 22.f);

  const SoaFloat3 neg = -a;
  EXPECT_SOAFLOAT3_EQ(neg, -.5f, -1.f, -2.f, -3.f, -4.f, -5.f, -6.f, -7.f, -8.f,
                      -9.f, -10.f, -11.f);

  const SoaFloat3 mul = a * b;
  EXPECT_SOAFLOAT3_EQ(mul, -0.25f, -1.f, -4.f, -9.f, -16.f, -25.f, -36.f, -49.f,
                      -64.f, -81.f, -100.f, -121.f);

  const SoaFloat3 mul_add = MAdd(a, b, c);
  EXPECT_SOAFLOAT3_EQ(mul_add, -0.2f, -.9f, -3.8f, -8.7f, -15.6f, -24.5f,
                      -35.4f, -48.3f, -63.2f, -80.1f, -99.f, -119.9f);

  const SoaFloat3 mul_scal = a * ozz::math::simd_float4::Load1(2.f);
  EXPECT_SOAFLOAT3_EQ(mul_scal, 1.f, 2.f, 4.f, 6.f, 8.f, 10.f, 12.f, 14.f, 16.f,
                      18.f, 20.f, 22.f);

  const SoaFloat3 div = a / b;
  EXPECT_SOAFLOAT3_EQ(div, -1.f, -1.f, -1.f, -1.f, -1.f, -1.f, -1.f, -1.f, -1.f,
                      -1.f, -1.f, -1.f);

  const SoaFloat3 div_scal = a / ozz::math::simd_float4::Load1(2.f);
  EXPECT_SOAFLOAT3_EQ(div_scal, 0.25f, .5f, 1.f, 1.5f, 2.f, 2.5f, 3.f, 3.5f,
                      4.f, 4.5, 5.f, 5.5f);

  const SimdFloat4 hadd4 = HAdd(a);
  EXPECT_SOAFLOAT1_EQ(hadd4, 12.5f, 15.f, 18.f, 21.f);

  const SimdFloat4 dot = Dot(a, b);
  EXPECT_SOAFLOAT1_EQ(dot, -80.25f, -107.f, -140.f, -179.f);

  const SimdFloat4 length = Length(a);
  EXPECT_SOAFLOAT1_EQ(length, 8.958236f, 10.34408f, 11.83216f, 13.37909f);

  const SimdFloat4 length2 = LengthSqr(a);
  EXPECT_SOAFLOAT1_EQ(length2, 80.25f, 107.f, 140.f, 179.f);

  const SoaFloat3 cross = Cross(a, b);
  EXPECT_SOAFLOAT3_EQ(cross, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f,
                      0.f, 0.f);

  EXPECT_ASSERTION(Normalize(SoaFloat3::zero()), "_v is not normalizable");
  EXPECT_TRUE(ozz::math::AreAllFalse(IsNormalized(a)));
  EXPECT_TRUE(ozz::math::AreAllFalse(IsNormalizedEst(a)));
  const SoaFloat3 normalize = Normalize(a);
  EXPECT_TRUE(ozz::math::AreAllTrue(IsNormalized(normalize)));
  EXPECT_TRUE(ozz::math::AreAllTrue(IsNormalizedEst(normalize)));
  EXPECT_SOAFLOAT3_EQ(normalize, .055814f, .096673f, .16903f, .22423f, .446516f,
                      .483368f, .50709f, .52320f, .893033f, .870063f, .84515f,
                      .822178f);

  EXPECT_ASSERTION(NormalizeSafe(a, a), "_safer is not normalized");
  const SoaFloat3 safe = ozz::math::SoaFloat3::x_axis();
  const SoaFloat3 normalize_safe = NormalizeSafe(a, safe);
  EXPECT_TRUE(ozz::math::AreAllTrue(IsNormalized(normalize_safe)));
  EXPECT_TRUE(ozz::math::AreAllTrue(IsNormalizedEst(normalize_safe)));
  EXPECT_SOAFLOAT3_EQ(normalize_safe, .055814f, .096673f, .16903f, .22423f,
                      .446516f, .483368f, .50709f, .52320f, .893033f, .870063f,
                      .84515f, .822178f);

  const SoaFloat3 normalize_safer = NormalizeSafe(SoaFloat3::zero(), safe);
  EXPECT_TRUE(ozz::math::AreAllTrue(IsNormalized(normalize_safer)));
  EXPECT_TRUE(ozz::math::AreAllTrue(IsNormalizedEst(normalize_safer)));
  EXPECT_SOAFLOAT3_EQ(normalize_safer, 1.f, 1.f, 1.f, 1.f, 0.f, 0.f, 0.f, 0.f,
                      0.f, 0.f, 0.f, 0.f);

  const SoaFloat3 lerp_0 = Lerp(a, b, ozz::math::simd_float4::zero());
  EXPECT_SOAFLOAT3_EQ(lerp_0, .5f, 1.f, 2.f, 3.f, 4.f, 5.f, 6.f, 7.f, 8.f, 9.f,
                      10.f, 11.f);

  const SoaFloat3 lerp_1 = Lerp(a, b, ozz::math::simd_float4::one());
  EXPECT_SOAFLOAT3_EQ(lerp_1, -.5f, -1.f, -2.f, -3.f, -4.f, -5.f, -6.f, -7.f,
                      -8.f, -9.f, -10.f, -11.f);

  const SoaFloat3 lerp_0_5 = Lerp(a, b, ozz::math::simd_float4::Load1(.5f));
  EXPECT_SOAFLOAT3_EQ(lerp_0_5, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f,
                      0.f, 0.f, 0.f);
}

TEST(SoaFloatArithmetic2, ozz_soa_math) {
  const SoaFloat2 a = {ozz::math::simd_float4::Load(.5f, 1.f, 2.f, 3.f),
                       ozz::math::simd_float4::Load(4.f, 5.f, 6.f, 7.f)};
  const SoaFloat2 b = {ozz::math::simd_float4::Load(-.5f, -1.f, -2.f, -3.f),
                       ozz::math::simd_float4::Load(-4.f, -5.f, -6.f, -7.f)};
  const SoaFloat2 c = {ozz::math::simd_float4::Load(.05f, .1f, .2f, .3f),
                       ozz::math::simd_float4::Load(.4f, .5f, .6f, .7f)};

  const SoaFloat2 add = a + b;
  EXPECT_SOAFLOAT2_EQ(add, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f);

  const SoaFloat2 sub = a - b;
  EXPECT_SOAFLOAT2_EQ(sub, 1.f, 2.f, 4.f, 6.f, 8.f, 10.f, 12.f, 14.f);

  const SoaFloat2 neg = -a;
  EXPECT_SOAFLOAT2_EQ(neg, -.5f, -1.f, -2.f, -3.f, -4.f, -5.f, -6.f, -7.f);

  const SoaFloat2 mul = a * b;
  EXPECT_SOAFLOAT2_EQ(mul, -0.25f, -1.f, -4.f, -9.f, -16.f, -25.f, -36.f,
                      -49.f);

  const SoaFloat2 mul_add = MAdd(a, b, c);
  EXPECT_SOAFLOAT2_EQ(mul_add, -0.2f, -.9f, -3.8f, -8.7f, -15.6f, -24.5f,
                      -35.4f, -48.3f);

  const SoaFloat2 mul_scal = a * ozz::math::simd_float4::Load1(2.f);
  EXPECT_SOAFLOAT2_EQ(mul_scal, 1.f, 2.f, 4.f, 6.f, 8.f, 10.f, 12.f, 14.f);

  const SoaFloat2 div = a / b;
  EXPECT_SOAFLOAT2_EQ(div, -1.f, -1.f, -1.f, -1.f, -1.f, -1.f, -1.f, -1.f);

  const SoaFloat2 div_scal = a / ozz::math::simd_float4::Load1(2.f);
  EXPECT_SOAFLOAT2_EQ(div_scal, 0.25f, .5f, 1.f, 1.5f, 2.f, 2.5f, 3.f, 3.5f);

  const SimdFloat4 hadd4 = HAdd(a);
  EXPECT_SOAFLOAT1_EQ(hadd4, 4.5f, 6.f, 8.f, 10.f);

  const SimdFloat4 dot = Dot(a, b);
  EXPECT_SOAFLOAT1_EQ(dot, -16.25f, -26.f, -40.f, -58.f);

  const SimdFloat4 length = Length(a);
  EXPECT_SOAFLOAT1_EQ(length, 4.031129f, 5.09902f, 6.324555f, 7.615773f);

  const SimdFloat4 length2 = LengthSqr(a);
  EXPECT_SOAFLOAT1_EQ(length2, 16.25f, 26.f, 40.f, 58.f);

  EXPECT_ASSERTION(Normalize(SoaFloat2::zero()), "_v is not normalizable");
  EXPECT_TRUE(ozz::math::AreAllFalse(IsNormalized(a)));
  EXPECT_TRUE(ozz::math::AreAllFalse(IsNormalizedEst(a)));
  const SoaFloat2 normalize = Normalize(a);
  EXPECT_TRUE(ozz::math::AreAllTrue(IsNormalized(normalize)));
  EXPECT_TRUE(ozz::math::AreAllTrue(IsNormalizedEst(normalize)));
  EXPECT_SOAFLOAT2_EQ(normalize, .124034f, .196116f, .316227f, .393919f,
                      .992277f, .980580f, .9486832f, .919145f);

  EXPECT_ASSERTION(NormalizeSafe(a, a), "_safer is not normalized");
  const SoaFloat2 safe = ozz::math::SoaFloat2::x_axis();
  const SoaFloat2 normalize_safe = NormalizeSafe(a, safe);
  EXPECT_TRUE(ozz::math::AreAllTrue(IsNormalized(normalize_safe)));
  EXPECT_TRUE(ozz::math::AreAllTrue(IsNormalizedEst(normalize_safe)));
  EXPECT_SOAFLOAT2_EQ(normalize, .124034f, .196116f, .316227f, .393919f,
                      .992277f, .980580f, .9486832f, .919145f);

  const SoaFloat2 normalize_safer = NormalizeSafe(SoaFloat2::zero(), safe);
  EXPECT_TRUE(ozz::math::AreAllTrue(IsNormalized(normalize_safer)));
  EXPECT_TRUE(ozz::math::AreAllTrue(IsNormalizedEst(normalize_safer)));
  EXPECT_SOAFLOAT2_EQ(normalize_safer, 1.f, 1.f, 1.f, 1.f, 0.f, 0.f, 0.f, 0.f);

  const SoaFloat2 lerp_0 = Lerp(a, b, ozz::math::simd_float4::zero());
  EXPECT_SOAFLOAT2_EQ(lerp_0, .5f, 1.f, 2.f, 3.f, 4.f, 5.f, 6.f, 7.f);

  const SoaFloat2 lerp_1 = Lerp(a, b, ozz::math::simd_float4::one());
  EXPECT_SOAFLOAT2_EQ(lerp_1, -.5f, -1.f, -2.f, -3.f, -4.f, -5.f, -6.f, -7.f);

  const SoaFloat2 lerp_0_5 = Lerp(a, b, ozz::math::simd_float4::Load1(.5f));
  EXPECT_SOAFLOAT2_EQ(lerp_0_5, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f);
}

TEST(SoaFloatComparison4, ozz_soa_math) {
  const SoaFloat4 a = {ozz::math::simd_float4::Load(.5f, 1.f, 2.f, 3.f),
                       ozz::math::simd_float4::Load(1.f, 5.f, 6.f, 7.f),
                       ozz::math::simd_float4::Load(2.f, 9.f, 10.f, 11.f),
                       ozz::math::simd_float4::Load(3.f, 13.f, 14.f, 15.f)};
  const SoaFloat4 b = {ozz::math::simd_float4::Load(4.f, 3.f, 7.f, 3.f),
                       ozz::math::simd_float4::Load(2.f, -5.f, 6.f, 5.f),
                       ozz::math::simd_float4::Load(-6.f, 9.f, -10.f, 2.f),
                       ozz::math::simd_float4::Load(7.f, -8.f, 1.f, 5.f)};
  const SoaFloat4 c = {ozz::math::simd_float4::Load(7.5f, 12.f, 46.f, 31.f),
                       ozz::math::simd_float4::Load(1.f, 58.f, 16.f, 78.f),
                       ozz::math::simd_float4::Load(2.5f, 9.f, 111.f, 22.f),
                       ozz::math::simd_float4::Load(8.f, 23.f, 41.f, 18.f)};
  const SoaFloat4 min = Min(a, b);
  EXPECT_SOAFLOAT4_EQ(min, .5f, 1.f, 2.f, 3.f, 1.f, -5.f, 6.f, 5.f, -6.f, 9.f,
                      -10.f, 2.f, 3.f, -8.f, 1.f, 5.f);

  const SoaFloat4 max = Max(a, b);
  EXPECT_SOAFLOAT4_EQ(max, 4.f, 3.f, 7.f, 3.f, 2.f, 5.f, 6.f, 7.f, 2.f, 9.f,
                      10.f, 11.f, 7.f, 13.f, 14.f, 15.f);

  EXPECT_SOAFLOAT4_EQ(
      Clamp(a, SoaFloat4::Load(
                   ozz::math::simd_float4::Load(1.5f, 5.f, -2.f, 24.f),
                   ozz::math::simd_float4::Load(2.f, -5.f, 7.f, 1.f),
                   ozz::math::simd_float4::Load(-3.f, 1.f, 200.f, 0.f),
                   ozz::math::simd_float4::Load(-9.f, 15.f, 46.f, -1.f)),
            c),
      1.5f, 5.f, 2.f, 24.f, 1.f, 5.f, 7.f, 7.f, 2.f, 9.f, 111.f, 11.f, 3.f,
      15.f, 41.f, 15.f);

  EXPECT_SIMDINT_EQ(a < c, 0, 0, 0xffffffff, 0xffffffff);
  EXPECT_SIMDINT_EQ(a <= c, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff);
  EXPECT_SIMDINT_EQ(c <= c, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff);

  EXPECT_SIMDINT_EQ(c > a, 0, 0, 0xffffffff, 0xffffffff);
  EXPECT_SIMDINT_EQ(c >= a, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff);
  EXPECT_SIMDINT_EQ(a >= a, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff);

  EXPECT_SIMDINT_EQ(a == a, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff);
  EXPECT_SIMDINT_EQ(a == c, 0, 0, 0, 0);
  EXPECT_SIMDINT_EQ(a != b, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff);
}

TEST(SoaFloatComparison3, ozz_soa_math) {
  const SoaFloat3 a = {ozz::math::simd_float4::Load(.5f, 1.f, 2.f, 3.f),
                       ozz::math::simd_float4::Load(1.f, 5.f, 6.f, 7.f),
                       ozz::math::simd_float4::Load(2.f, 9.f, 10.f, 11.f)};
  const SoaFloat3 b = {ozz::math::simd_float4::Load(4.f, 3.f, 7.f, 3.f),
                       ozz::math::simd_float4::Load(2.f, -5.f, 6.f, 5.f),
                       ozz::math::simd_float4::Load(-6.f, 9.f, -10.f, 2.f)};
  const SoaFloat3 c = {ozz::math::simd_float4::Load(7.5f, 12.f, 46.f, 31.f),
                       ozz::math::simd_float4::Load(1.f, 58.f, 16.f, 78.f),
                       ozz::math::simd_float4::Load(2.5f, 9.f, 111.f, 22.f)};
  const SoaFloat3 min = Min(a, b);
  EXPECT_SOAFLOAT3_EQ(min, .5f, 1.f, 2.f, 3.f, 1.f, -5.f, 6.f, 5.f, -6.f, 9.f,
                      -10.f, 2.f);

  const SoaFloat3 max = Max(a, b);
  EXPECT_SOAFLOAT3_EQ(max, 4.f, 3.f, 7.f, 3.f, 2.f, 5.f, 6.f, 7.f, 2.f, 9.f,
                      10.f, 11.f);

  EXPECT_SOAFLOAT3_EQ(
      Clamp(a, SoaFloat3::Load(
                   ozz::math::simd_float4::Load(1.5f, 5.f, -2.f, 24.f),
                   ozz::math::simd_float4::Load(2.f, -5.f, 7.f, 1.f),
                   ozz::math::simd_float4::Load(-3.f, 1.f, 200.f, 0.f)),
            c),
      1.5f, 5.f, 2.f, 24.f, 1.f, 5.f, 7.f, 7.f, 2.f, 9.f, 111.f, 11.f);

  EXPECT_SIMDINT_EQ(a < c, 0, 0, 0xffffffff, 0xffffffff);
  EXPECT_SIMDINT_EQ(a <= c, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff);
  EXPECT_SIMDINT_EQ(c <= c, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff);

  EXPECT_SIMDINT_EQ(c > a, 0, 0, 0xffffffff, 0xffffffff);
  EXPECT_SIMDINT_EQ(c >= a, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff);
  EXPECT_SIMDINT_EQ(a >= a, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff);

  EXPECT_SIMDINT_EQ(a == a, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff);
  EXPECT_SIMDINT_EQ(a == c, 0, 0, 0, 0);
  EXPECT_SIMDINT_EQ(a != b, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff);
}

TEST(SoaFloatComparison2, ozz_soa_math) {
  const SoaFloat2 a = {ozz::math::simd_float4::Load(.5f, 1.f, 2.f, 3.f),
                       ozz::math::simd_float4::Load(1.f, 5.f, 6.f, 7.f)};
  const SoaFloat2 b = {ozz::math::simd_float4::Load(4.f, 3.f, 7.f, 3.f),
                       ozz::math::simd_float4::Load(2.f, -5.f, 6.f, 5.f)};
  const SoaFloat2 c = {ozz::math::simd_float4::Load(7.5f, 12.f, 46.f, 31.f),
                       ozz::math::simd_float4::Load(1.f, 58.f, 16.f, 78.f)};
  const SoaFloat2 min = Min(a, b);
  EXPECT_SOAFLOAT2_EQ(min, .5f, 1.f, 2.f, 3.f, 1.f, -5.f, 6.f, 5.f);

  const SoaFloat2 max = Max(a, b);
  EXPECT_SOAFLOAT2_EQ(max, 4.f, 3.f, 7.f, 3.f, 2.f, 5.f, 6.f, 7.f);

  EXPECT_SOAFLOAT2_EQ(
      Clamp(a,
            SoaFloat2::Load(ozz::math::simd_float4::Load(1.5f, 5.f, -2.f, 24.f),
                            ozz::math::simd_float4::Load(2.f, -5.f, 7.f, 1.f)),
            c),
      1.5f, 5.f, 2.f, 24.f, 1.f, 5.f, 7.f, 7.f);

  EXPECT_SIMDINT_EQ(a < c, 0, 0xffffffff, 0xffffffff, 0xffffffff);
  EXPECT_SIMDINT_EQ(a <= c, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff);
  EXPECT_SIMDINT_EQ(c <= c, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff);

  EXPECT_SIMDINT_EQ(c > a, 0, 0xffffffff, 0xffffffff, 0xffffffff);
  EXPECT_SIMDINT_EQ(c >= a, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff);
  EXPECT_SIMDINT_EQ(a >= a, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff);

  EXPECT_SIMDINT_EQ(a == a, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff);
  EXPECT_SIMDINT_EQ(a == c, 0, 0, 0, 0);
  EXPECT_SIMDINT_EQ(a != b, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff);
}
