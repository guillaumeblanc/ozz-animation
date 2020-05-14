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

#include "ozz/base/maths/simd_math.h"

#include <stdint.h>
#include <cmath>
#include <limits>

#include "gtest/gtest.h"

#include "ozz/base/gtest_helper.h"
#include "ozz/base/maths/gtest_math_helper.h"
#include "ozz/base/maths/math_constant.h"
#include "ozz/base/maths/math_ex.h"

using ozz::math::SimdFloat4;
using ozz::math::SimdInt4;

static_assert(sizeof(SimdFloat4) == 4 * sizeof(float),
              "Expects SimdFloat4 to be the size of 4 floats.");
static_assert(alignof(SimdFloat4) == 16,
              "Expects SimdFloat4 to be the size of 16 bytes.");

TEST(Name, ozz_simd_math) {
  EXPECT_TRUE(ozz::math::SimdImplementationName() != nullptr);
}

TEST(LoadFloat, ozz_simd_math) {
  const SimdFloat4 fX = ozz::math::simd_float4::LoadX(15.f);
  EXPECT_SIMDFLOAT_EQ(fX, 15.f, 0.f, 0.f, 0.f);

  const SimdFloat4 f1 = ozz::math::simd_float4::Load1(15.f);
  EXPECT_SIMDFLOAT_EQ(f1, 15.f, 15.f, 15.f, 15.f);

  const SimdFloat4 f4 = ozz::math::simd_float4::Load(1.f, -1.f, 2.f, -3.f);
  EXPECT_SIMDFLOAT_EQ(f4, 1.f, -1.f, 2.f, -3.f);
}

TEST(LoadFloatPtr, ozz_simd_math) {
  union Data {
    float f[4 + 5];
    char c[(4 + 5) * sizeof(float)];  // The 2nd char isn't aligned to a float.
  };
  const Data d_in = {{-1.f, 1.f, 2.f, 3.f, 4.f, 5.f, 6.f, 7.f, 8.f}};
  const int aligned_offset =
      (16 - (reinterpret_cast<intptr_t>(d_in.f) & 0xf)) / sizeof(float);
  assert(aligned_offset > 0 && aligned_offset <= 4);

  EXPECT_SIMDFLOAT_EQ(ozz::math::simd_float4::LoadPtr(d_in.f + aligned_offset),
                      d_in.f[aligned_offset + 0], d_in.f[aligned_offset + 1],
                      d_in.f[aligned_offset + 2], d_in.f[aligned_offset + 3]);
  EXPECT_ASSERTION(ozz::math::simd_float4::LoadPtr(d_in.f + aligned_offset - 1),
                   "alignment");

  EXPECT_SIMDFLOAT_EQ(
      ozz::math::simd_float4::LoadPtrU(d_in.f + aligned_offset + 1),
      d_in.f[aligned_offset + 1], d_in.f[aligned_offset + 2],
      d_in.f[aligned_offset + 3], d_in.f[aligned_offset + 4]);
  EXPECT_ASSERTION(ozz::math::simd_float4::LoadPtrU(
                       reinterpret_cast<const float*>(d_in.c + 1)),
                   "alignment");
  EXPECT_SIMDFLOAT_EQ(
      ozz::math::simd_float4::LoadXPtrU(d_in.f + aligned_offset + 1),
      d_in.f[aligned_offset + 1], 0.f, 0.f, 0.f);
  EXPECT_ASSERTION(ozz::math::simd_float4::LoadXPtrU(
                       reinterpret_cast<const float*>(d_in.c + 1)),
                   "alignment");
  EXPECT_SIMDFLOAT_EQ(
      ozz::math::simd_float4::Load1PtrU(d_in.f + aligned_offset + 1),
      d_in.f[aligned_offset + 1], d_in.f[aligned_offset + 1],
      d_in.f[aligned_offset + 1], d_in.f[aligned_offset + 1]);
  EXPECT_ASSERTION(ozz::math::simd_float4::Load1PtrU(
                       reinterpret_cast<const float*>(d_in.c + 1)),
                   "alignment");
  EXPECT_SIMDFLOAT_EQ(
      ozz::math::simd_float4::Load2PtrU(d_in.f + aligned_offset + 1),
      d_in.f[aligned_offset + 1], d_in.f[aligned_offset + 2], 0.f, 0.f);
  EXPECT_ASSERTION(ozz::math::simd_float4::Load2PtrU(
                       reinterpret_cast<const float*>(d_in.c + 1)),
                   "alignment");
  EXPECT_SIMDFLOAT_EQ(
      ozz::math::simd_float4::Load3PtrU(d_in.f + aligned_offset + 1),
      d_in.f[aligned_offset + 1], d_in.f[aligned_offset + 2],
      d_in.f[aligned_offset + 3], 0.f);
  EXPECT_ASSERTION(ozz::math::simd_float4::Load3PtrU(
                       reinterpret_cast<const float*>(d_in.c + 1)),
                   "alignment");
}

TEST(GetFloat, ozz_simd_math) {
  const SimdFloat4 f = ozz::math::simd_float4::Load(1.f, 2.f, 3.f, 4.f);

  EXPECT_FLOAT_EQ(ozz::math::GetX(f), 1.f);
  EXPECT_FLOAT_EQ(ozz::math::GetY(f), 2.f);
  EXPECT_FLOAT_EQ(ozz::math::GetZ(f), 3.f);
  EXPECT_FLOAT_EQ(ozz::math::GetW(f), 4.f);
}

TEST(SetFloat, ozz_simd_math) {
  const SimdFloat4 a = ozz::math::simd_float4::Load(1.f, 2.f, 3.f, 4.f);
  const SimdFloat4 b = ozz::math::simd_float4::Load(5.f, 6.f, 7.f, 8.f);

  EXPECT_SIMDFLOAT_EQ(ozz::math::SetX(a, b), 5.f, 2.f, 3.f, 4.f);
  EXPECT_SIMDFLOAT_EQ(ozz::math::SetY(a, b), 1.f, 5.f, 3.f, 4.f);
  EXPECT_SIMDFLOAT_EQ(ozz::math::SetZ(a, b), 1.f, 2.f, 5.f, 4.f);
  EXPECT_SIMDFLOAT_EQ(ozz::math::SetW(a, b), 1.f, 2.f, 3.f, 5.f);

  EXPECT_ASSERTION(ozz::math::SetI(a, b, 4), "Invalid index, out of range.");
  EXPECT_SIMDFLOAT_EQ(ozz::math::SetI(a, b, 0), 5.f, 2.f, 3.f, 4.f);
  EXPECT_SIMDFLOAT_EQ(ozz::math::SetI(a, b, 1), 1.f, 5.f, 3.f, 4.f);
  EXPECT_SIMDFLOAT_EQ(ozz::math::SetI(a, b, 2), 1.f, 2.f, 5.f, 4.f);
  EXPECT_SIMDFLOAT_EQ(ozz::math::SetI(a, b, 3), 1.f, 2.f, 3.f, 5.f);
}

TEST(StoreFloatPtr, ozz_simd_math) {
  const SimdFloat4 f4 = ozz::math::simd_float4::Load(-1.f, 1.f, 2.f, 3.f);

  union Data {
    float f[4 + 4];    // The 2nd float isn't aligned to a SimdFloat4.
    SimdFloat4 f4[2];  // Forces alignment.
    char c[(4 + 4) * sizeof(float)];  // The 2nd char isn't aligned to a float.
  };

  {
    Data d_out = {};
    ozz::math::StorePtrU(f4, d_out.f + 1);
    EXPECT_FLOAT_EQ(d_out.f[0], 0.f);
    EXPECT_FLOAT_EQ(d_out.f[1], -1.f);
    EXPECT_FLOAT_EQ(d_out.f[2], 1.f);
    EXPECT_FLOAT_EQ(d_out.f[3], 2.f);
    EXPECT_FLOAT_EQ(d_out.f[4], 3.f);
    EXPECT_ASSERTION(
        ozz::math::StorePtrU(f4, reinterpret_cast<float*>(d_out.c + 1)),
        "alignment");
  }
  {
    Data d_out = {};
    ozz::math::Store1PtrU(f4, d_out.f + 1);
    EXPECT_FLOAT_EQ(d_out.f[0], 0.f);
    EXPECT_FLOAT_EQ(d_out.f[1], -1.f);
    EXPECT_FLOAT_EQ(d_out.f[2], 0.f);
    EXPECT_FLOAT_EQ(d_out.f[3], 0.f);
    EXPECT_FLOAT_EQ(d_out.f[4], 0.f);
    EXPECT_ASSERTION(
        ozz::math::Store1PtrU(f4, reinterpret_cast<float*>(d_out.c + 1)),
        "alignment");
  }
  {
    Data d_out = {};
    ozz::math::Store2PtrU(f4, d_out.f + 1);
    EXPECT_FLOAT_EQ(d_out.f[0], 0.f);
    EXPECT_FLOAT_EQ(d_out.f[1], -1.f);
    EXPECT_FLOAT_EQ(d_out.f[2], 1.f);
    EXPECT_FLOAT_EQ(d_out.f[3], 0.f);
    EXPECT_FLOAT_EQ(d_out.f[4], 0.f);
    EXPECT_ASSERTION(
        ozz::math::Store2Ptr(f4, reinterpret_cast<float*>(d_out.c + 1)),
        "alignment");
  }
  {
    Data d_out = {};
    ozz::math::Store3PtrU(f4, d_out.f + 1);
    EXPECT_FLOAT_EQ(d_out.f[0], 0.f);
    EXPECT_FLOAT_EQ(d_out.f[1], -1.f);
    EXPECT_FLOAT_EQ(d_out.f[2], 1.f);
    EXPECT_FLOAT_EQ(d_out.f[3], 2.f);
    EXPECT_FLOAT_EQ(d_out.f[4], 0.f);
    EXPECT_ASSERTION(
        ozz::math::Store3Ptr(f4, reinterpret_cast<float*>(d_out.c + 1)),
        "alignment");
  }
  {
    Data d_out = {};
    ozz::math::StorePtr(f4, d_out.f);
    EXPECT_FLOAT_EQ(d_out.f[0], -1.f);
    EXPECT_FLOAT_EQ(d_out.f[1], 1.f);
    EXPECT_FLOAT_EQ(d_out.f[2], 2.f);
    EXPECT_FLOAT_EQ(d_out.f[3], 3.f);
    EXPECT_FLOAT_EQ(d_out.f[4], 0.f);
    EXPECT_ASSERTION(ozz::math::StorePtr(f4, d_out.f + 1), "alignment");
  }
  {
    Data d_out = {};
    ozz::math::Store1Ptr(f4, d_out.f);
    EXPECT_FLOAT_EQ(d_out.f[0], -1.f);
    EXPECT_FLOAT_EQ(d_out.f[1], 0.f);
    EXPECT_FLOAT_EQ(d_out.f[2], 0.f);
    EXPECT_FLOAT_EQ(d_out.f[3], 0.f);
    EXPECT_FLOAT_EQ(d_out.f[4], 0.f);
    EXPECT_ASSERTION(ozz::math::Store1Ptr(f4, d_out.f + 1), "alignment");
  }
  {
    Data d_out = {};
    ozz::math::Store2Ptr(f4, d_out.f);
    EXPECT_FLOAT_EQ(d_out.f[0], -1.f);
    EXPECT_FLOAT_EQ(d_out.f[1], 1.f);
    EXPECT_FLOAT_EQ(d_out.f[2], 0.f);
    EXPECT_FLOAT_EQ(d_out.f[3], 0.f);
    EXPECT_FLOAT_EQ(d_out.f[4], 0.f);
    EXPECT_ASSERTION(ozz::math::Store2Ptr(f4, d_out.f + 1), "alignment");
  }
  {
    Data d_out = {};
    ozz::math::Store3Ptr(f4, d_out.f);
    EXPECT_FLOAT_EQ(d_out.f[0], -1.f);
    EXPECT_FLOAT_EQ(d_out.f[1], 1.f);
    EXPECT_FLOAT_EQ(d_out.f[2], 2.f);
    EXPECT_FLOAT_EQ(d_out.f[3], 0.f);
    EXPECT_FLOAT_EQ(d_out.f[4], 0.f);
    EXPECT_ASSERTION(ozz::math::Store3Ptr(f4, d_out.f + 1), "alignment");
  }
}

TEST(ConstantFloat, ozz_simd_math) {
  const SimdFloat4 zero = ozz::math::simd_float4::zero();
  EXPECT_SIMDFLOAT_EQ(zero, 0.f, 0.f, 0.f, 0.f);

  const SimdFloat4 one = ozz::math::simd_float4::one();
  EXPECT_SIMDFLOAT_EQ(one, 1.f, 1.f, 1.f, 1.f);

  const SimdFloat4 x_axis = ozz::math::simd_float4::x_axis();
  EXPECT_SIMDFLOAT_EQ(x_axis, 1.f, 0.f, 0.f, 0.f);

  const SimdFloat4 y_axis = ozz::math::simd_float4::y_axis();
  EXPECT_SIMDFLOAT_EQ(y_axis, 0.f, 1.f, 0.f, 0.f);

  const SimdFloat4 z_axis = ozz::math::simd_float4::z_axis();
  EXPECT_SIMDFLOAT_EQ(z_axis, 0.f, 0.f, 1.f, 0.f);

  const SimdFloat4 w_axis = ozz::math::simd_float4::w_axis();
  EXPECT_SIMDFLOAT_EQ(w_axis, 0.f, 0.f, 0.f, 1.f);
}

TEST(SplatFloat, ozz_simd_math) {
  const SimdFloat4 f = ozz::math::simd_float4::Load(1.f, -1.f, 2.f, -3.f);

  const SimdFloat4 x = ozz::math::SplatX(f);
  EXPECT_SIMDFLOAT_EQ(x, 1.f, 1.f, 1.f, 1.f);

  const SimdFloat4 y = ozz::math::SplatY(f);
  EXPECT_SIMDFLOAT_EQ(y, -1.f, -1.f, -1.f, -1.f);

  const SimdFloat4 z = ozz::math::SplatZ(f);
  EXPECT_SIMDFLOAT_EQ(z, 2.f, 2.f, 2.f, 2.f);

  const SimdFloat4 w = ozz::math::SplatW(f);
  EXPECT_SIMDFLOAT_EQ(w, -3.f, -3.f, -3.f, -3.f);

  const SimdFloat4 s3210 = ozz::math::Swizzle<3, 2, 1, 0>(f);
  EXPECT_SIMDFLOAT_EQ(s3210, -3.f, 2.f, -1.f, 1.f);

  const SimdFloat4 s0123 = ozz::math::Swizzle<0, 1, 2, 3>(f);
  EXPECT_SIMDFLOAT_EQ(s0123, 1.f, -1.f, 2.f, -3.f);

  const SimdFloat4 s0011 = ozz::math::Swizzle<0, 0, 1, 1>(f);
  EXPECT_SIMDFLOAT_EQ(s0011, 1.f, 1.f, -1.f, -1.f);

  const SimdFloat4 s2233 = ozz::math::Swizzle<2, 2, 3, 3>(f);
  EXPECT_SIMDFLOAT_EQ(s2233, 2.f, 2.f, -3.f, -3.f);

  const SimdFloat4 s0101 = ozz::math::Swizzle<0, 1, 0, 1>(f);
  EXPECT_SIMDFLOAT_EQ(s0101, 1.f, -1.f, 1.f, -1.f);

  const SimdFloat4 s2323 = ozz::math::Swizzle<2, 3, 2, 3>(f);
  EXPECT_SIMDFLOAT_EQ(s2323, 2.f, -3.f, 2.f, -3.f);
}

TEST(FromInt, ozz_simd_math) {
  const ozz::math::SimdInt4 i = ozz::math::simd_int4::Load(0, 46, -93, 9926429);
  EXPECT_SIMDFLOAT_EQ(ozz::math::simd_float4::FromInt(i), 0.f, 46.f, -93.f,
                      9926429.f);
}

TEST(ArithmeticFloat, ozz_simd_math) {
  const ozz::math::SimdFloat4 a =
      ozz::math::simd_float4::Load(0.5f, 1.f, 2.f, 3.f);
  const ozz::math::SimdFloat4 b =
      ozz::math::simd_float4::Load(4.f, 5.f, -6.f, 0.f);
  const ozz::math::SimdFloat4 c =
      ozz::math::simd_float4::Load(-8.f, 9.f, 10.f, 11.f);

  const ozz::math::SimdFloat4 add = a + b;
  EXPECT_SIMDFLOAT_EQ(add, 4.5f, 6.f, -4.f, 3.f);

  const ozz::math::SimdFloat4 sub = a - b;
  EXPECT_SIMDFLOAT_EQ(sub, -3.5f, -4.f, 8.f, 3.f);

  const ozz::math::SimdFloat4 neg = -b;
  EXPECT_SIMDFLOAT_EQ(neg, -4.f, -5.f, 6.f, -0.f);

  const ozz::math::SimdFloat4 mul = a * b;
  EXPECT_SIMDFLOAT_EQ(mul, 2.f, 5.f, -12.f, 0.f);

  const ozz::math::SimdFloat4 div = a / b;
  EXPECT_SIMDFLOAT3_EQ(div, 0.5f / 4.f, 1.f / 5.f, -2.f / 6.f);

  const ozz::math::SimdFloat4 madd = ozz::math::MAdd(a, b, c);
  EXPECT_SIMDFLOAT_EQ(madd, -6.f, 14.f, -2.f, 11.f);

  const ozz::math::SimdFloat4 msub = ozz::math::MSub(a, b, c);
  EXPECT_SIMDFLOAT_EQ(msub, 10.f, -4.f, -22.f, -11.f);

  const ozz::math::SimdFloat4 nmadd = ozz::math::NMAdd(a, b, c);
  EXPECT_SIMDFLOAT_EQ(nmadd, -10.f, 4.f, 22.f, 11.f);

  const ozz::math::SimdFloat4 nmsub = ozz::math::NMSub(a, b, c);
  EXPECT_SIMDFLOAT_EQ(nmsub, 6.f, -14.f, 2.f, -11.f);

  const ozz::math::SimdFloat4 divx = ozz::math::DivX(a, b);
  EXPECT_SIMDFLOAT_EQ(divx, 0.5f / 4.f, 1.f, 2.f, 3.f);

  const ozz::math::SimdFloat4 hadd2 = ozz::math::HAdd2(a);
  EXPECT_SIMDFLOAT_EQ(hadd2, 1.5f, 1.f, 2.f, 3.f);

  const ozz::math::SimdFloat4 hadd3 = ozz::math::HAdd3(a);
  EXPECT_SIMDFLOAT_EQ(hadd3, 3.5f, 1.f, 2.f, 3.f);

  const ozz::math::SimdFloat4 hadd4 = ozz::math::HAdd4(a);
  EXPECT_FLOAT_EQ(ozz::math::GetX(hadd4), 6.5f);

  const ozz::math::SimdFloat4 dot2 = ozz::math::Dot2(a, b);
  EXPECT_FLOAT_EQ(ozz::math::GetX(dot2), 7.f);

  const ozz::math::SimdFloat4 dot3 = ozz::math::Dot3(a, b);
  EXPECT_FLOAT_EQ(ozz::math::GetX(dot3), -5.f);

  const ozz::math::SimdFloat4 dot4 = ozz::math::Dot4(a, b);
  EXPECT_FLOAT_EQ(ozz::math::GetX(dot4), -5.f);

  const ozz::math::SimdFloat4 cross =
      ozz::math::Cross3(ozz::math::simd_float4::Load(1.f, -2.f, 3.f, 46.f),
                        ozz::math::simd_float4::Load(4.f, 5.f, 6.f, 27.f));
  EXPECT_FLOAT_EQ(ozz::math::GetX(cross), -27.f);
  EXPECT_FLOAT_EQ(ozz::math::GetY(cross), 6.f);
  EXPECT_FLOAT_EQ(ozz::math::GetZ(cross), 13.f);

  const ozz::math::SimdFloat4 rcp = ozz::math::RcpEst(b);
  EXPECT_SIMDFLOAT3_EQ_EST(rcp, 1.f / 4.f, 1.f / 5.f, -1.f / 6.f);

  const ozz::math::SimdFloat4 rcpnr = ozz::math::RcpEstNR(b);
  EXPECT_SIMDFLOAT3_EQ(rcpnr, 1.f / 4.f, 1.f / 5.f, -1.f / 6.f);

  const ozz::math::SimdFloat4 rcpxnr = ozz::math::RcpEstXNR(b);
  EXPECT_FLOAT_EQ(ozz::math::GetX(rcpxnr), 1.f / 4.f);

  const ozz::math::SimdFloat4 rcpx = ozz::math::RcpEstX(b);
  EXPECT_SIMDFLOAT_EQ_EST(rcpx, 1.f / 4.f, 5.f, -6.f, 0.f);

  const ozz::math::SimdFloat4 sqrt = ozz::math::Sqrt(a);
  EXPECT_SIMDFLOAT_EQ(sqrt, .7071068f, 1.f, 1.4142135f, 1.7320508f);

  const ozz::math::SimdFloat4 sqrtx = ozz::math::SqrtX(b);
  EXPECT_SIMDFLOAT_EQ(sqrtx, 2.f, 5.f, -6.f, 0.f);

  const ozz::math::SimdFloat4 rsqrt = ozz::math::RSqrtEst(b);
  EXPECT_SIMDFLOAT2_EQ_EST(rsqrt, 1.f / 2.f, 1.f / 2.23606798f);

  const ozz::math::SimdFloat4 rsqrtnr = ozz::math::RSqrtEstNR(b);
  EXPECT_SIMDFLOAT2_EQ_EST(rsqrtnr, 1.f / 2.f, 1.f / 2.23606798f);

  const ozz::math::SimdFloat4 rsqrtx = ozz::math::RSqrtEstX(a);
  EXPECT_SIMDFLOAT_EQ_EST(rsqrtx, 1.f / .7071068f, 1.f, 2.f, 3.f);

  const ozz::math::SimdFloat4 rsqrtxnr = ozz::math::RSqrtEstXNR(a);
  EXPECT_FLOAT_EQ(ozz::math::GetX(rsqrtxnr), 1.f / .7071068f);

  const ozz::math::SimdFloat4 abs = ozz::math::Abs(b);
  EXPECT_SIMDFLOAT_EQ(abs, 4.f, 5.f, 6.f, 0.f);

  const SimdInt4 sign = ozz::math::Sign(b);
  EXPECT_SIMDINT_EQ(sign, 0, 0, 0x80000000, 0);
}

TEST(LengthFloat, ozz_simd_math) {
  const SimdFloat4 f = ozz::math::simd_float4::Load(1.f, 2.f, 4.f, 8.f);

  const SimdFloat4 len2 = ozz::math::Length2(f);
  EXPECT_FLOAT_EQ(ozz::math::GetX(len2), 2.236068f);

  const SimdFloat4 len3 = ozz::math::Length3(f);
  EXPECT_FLOAT_EQ(ozz::math::GetX(len3), 4.5825758f);

  const SimdFloat4 len4 = ozz::math::Length4(f);
  EXPECT_FLOAT_EQ(ozz::math::GetX(len4), 9.2195444f);

  const SimdFloat4 len2sqr = ozz::math::Length2Sqr(f);
  EXPECT_FLOAT_EQ(ozz::math::GetX(len2sqr), 5.f);

  const SimdFloat4 len3sqr = ozz::math::Length3Sqr(f);
  EXPECT_FLOAT_EQ(ozz::math::GetX(len3sqr), 21.f);

  const SimdFloat4 len4sqr = ozz::math::Length4Sqr(f);
  EXPECT_FLOAT_EQ(ozz::math::GetX(len4sqr), 85.f);
}

TEST(NormalizeFloat, ozz_simd_math) {
  const SimdFloat4 f = ozz::math::simd_float4::Load(1.f, 2.f, 4.f, 8.f);
  const SimdFloat4 unit = ozz::math::simd_float4::x_axis();
  const SimdFloat4 zero = ozz::math::simd_float4::zero();

  EXPECT_SIMDINT_EQ(ozz::math::IsNormalized2(f), 0, 0, 0, 0);
  const SimdFloat4 norm2 = ozz::math::Normalize2(f);
  EXPECT_SIMDFLOAT_EQ(norm2, .44721359f, .89442718f, 4.f, 8.f);
  EXPECT_SIMDINT_EQ(ozz::math::IsNormalized2(norm2), 0xffffffff, 0, 0, 0);

  const SimdFloat4 norm_est2 = ozz::math::NormalizeEst2(f);
  EXPECT_SIMDFLOAT_EQ_EST(norm_est2, .44721359f, .89442718f, 4.f, 8.f);
  EXPECT_SIMDINT_EQ(ozz::math::IsNormalizedEst2(norm_est2), 0xffffffff, 0, 0,
                    0);

  EXPECT_ASSERTION(ozz::math::Normalize2(zero), "_v is not normalizable");
  EXPECT_ASSERTION(ozz::math::NormalizeEst2(zero), "_v is not normalizable");

  EXPECT_SIMDINT_EQ(ozz::math::IsNormalized3(f), 0, 0, 0, 0);
  const SimdFloat4 norm3 = ozz::math::Normalize3(f);
  EXPECT_SIMDFLOAT_EQ(norm3, .21821788f, .43643576f, .87287152f, 8.f);
  EXPECT_SIMDINT_EQ(ozz::math::IsNormalized3(norm3), 0xffffffff, 0, 0, 0);

  const SimdFloat4 norm_est3 = ozz::math::NormalizeEst3(f);
  EXPECT_SIMDFLOAT_EQ_EST(norm_est3, .21821788f, .43643576f, .87287152f, 8.f);
  EXPECT_SIMDINT_EQ(ozz::math::IsNormalizedEst3(norm_est3), 0xffffffff, 0, 0,
                    0);

  EXPECT_ASSERTION(ozz::math::Normalize3(zero), "_v is not normalizable");
  EXPECT_ASSERTION(ozz::math::NormalizeEst3(zero), "_v is not normalizable");

  EXPECT_SIMDINT_EQ(ozz::math::IsNormalized4(f), 0, 0, 0, 0);
  const SimdFloat4 norm4 = ozz::math::Normalize4(f);
  EXPECT_SIMDFLOAT_EQ(norm4, .1084652f, .2169304f, .4338609f, .86772186f);
  EXPECT_SIMDINT_EQ(ozz::math::IsNormalized4(norm4), 0xffffffff, 0, 0, 0);

  const SimdFloat4 norm_est4 = ozz::math::NormalizeEst4(f);
  EXPECT_SIMDFLOAT_EQ_EST(norm_est4, .1084652f, .2169304f, .4338609f,
                          .86772186f);
  EXPECT_SIMDINT_EQ(ozz::math::IsNormalizedEst4(norm_est4), 0xffffffff, 0, 0,
                    0);

  EXPECT_ASSERTION(ozz::math::Normalize4(zero), "_v is not normalizable");
  EXPECT_ASSERTION(ozz::math::NormalizeEst4(zero), "_v is not normalizable");

  const SimdFloat4 safe2 = ozz::math::NormalizeSafe2(f, unit);
  EXPECT_SIMDFLOAT_EQ(safe2, .4472136f, .8944272f, 4.f, 8.f);
  EXPECT_SIMDINT_EQ(ozz::math::IsNormalized2(safe2), 0xffffffff, 0, 0, 0);
  const SimdFloat4 safer2 = ozz::math::NormalizeSafe2(zero, unit);
  EXPECT_SIMDFLOAT_EQ(safer2, 1.f, 0.f, 0.f, 0.f);
  const SimdFloat4 safe_est2 = ozz::math::NormalizeSafeEst2(f, unit);
  EXPECT_SIMDFLOAT_EQ_EST(safe_est2, .4472136f, .8944272f, 4.f, 8.f);
  EXPECT_SIMDINT_EQ(ozz::math::IsNormalizedEst2(safe_est2), 0xffffffff, 0, 0,
                    0);
  const SimdFloat4 safer_est2 = ozz::math::NormalizeSafeEst2(zero, unit);
  EXPECT_SIMDFLOAT_EQ_EST(safer_est2, 1.f, 0.f, 0.f, 0.f);

  const SimdFloat4 safe3 = ozz::math::NormalizeSafe3(f, unit);
  EXPECT_SIMDFLOAT_EQ(safe3, .21821788f, .43643576f, .87287152f, 8.f);
  EXPECT_SIMDINT_EQ(ozz::math::IsNormalized3(safe3), 0xffffffff, 0, 0, 0);
  const SimdFloat4 safer3 = ozz::math::NormalizeSafe3(zero, unit);
  EXPECT_SIMDFLOAT_EQ(safer3, 1.f, 0.f, 0.f, 0.f);
  const SimdFloat4 safe_est3 = ozz::math::NormalizeSafeEst3(f, unit);
  EXPECT_SIMDFLOAT_EQ_EST(safe_est3, .21821788f, .43643576f, .87287152f, 8.f);
  EXPECT_SIMDINT_EQ(ozz::math::IsNormalizedEst3(safe_est3), 0xffffffff, 0, 0,
                    0);
  const SimdFloat4 safer_est3 = ozz::math::NormalizeSafeEst3(zero, unit);
  EXPECT_SIMDFLOAT_EQ_EST(safer_est3, 1.f, 0.f, 0.f, 0.f);

  const SimdFloat4 safe4 = ozz::math::NormalizeSafe4(f, unit);
  EXPECT_SIMDFLOAT_EQ(safe4, .1084652f, .2169305f, .433861f, .8677219f);
  EXPECT_SIMDINT_EQ(ozz::math::IsNormalized4(safe4), 0xffffffff, 0, 0, 0);
  const SimdFloat4 safer4 = ozz::math::NormalizeSafe4(zero, unit);
  EXPECT_SIMDFLOAT_EQ(safer4, 1.f, 0.f, 0.f, 0.f);
  const SimdFloat4 safe_est4 = ozz::math::NormalizeSafeEst4(f, unit);
  EXPECT_SIMDFLOAT_EQ_EST(safe_est4, .1084652f, .2169305f, .433861f, .8677219f);
  EXPECT_SIMDINT_EQ(ozz::math::IsNormalizedEst4(safe_est4), 0xffffffff, 0, 0,
                    0);
  const SimdFloat4 safer_est4 = ozz::math::NormalizeSafeEst4(zero, unit);
  EXPECT_SIMDFLOAT_EQ_EST(safer_est4, 1.f, 0.f, 0.f, 0.f);
}

TEST(CompareFloat, ozz_simd_math) {
  const SimdFloat4 a = ozz::math::simd_float4::Load(0.5f, 1.f, 2.f, 3.f);
  const SimdFloat4 b = ozz::math::simd_float4::Load(4.f, 1.f, -6.f, 7.f);
  const SimdFloat4 c = ozz::math::simd_float4::Load(4.f, 5.f, 6.f, 7.f);

  const SimdFloat4 min = ozz::math::Min(a, b);
  EXPECT_SIMDFLOAT_EQ(min, 0.5f, 1.f, -6.f, 3.f);

  const SimdFloat4 max = ozz::math::Max(a, b);
  EXPECT_SIMDFLOAT_EQ(max, 4.f, 1.f, 2.f, 7.f);

  const SimdFloat4 min0 = ozz::math::Min0(b);
  EXPECT_SIMDFLOAT_EQ(min0, 0.f, 0.f, -6.f, 0.f);

  const SimdFloat4 max0 = ozz::math::Max0(b);
  EXPECT_SIMDFLOAT_EQ(max0, 4.f, 1.f, 0.f, 7.f);

  EXPECT_SIMDFLOAT_EQ(
      ozz::math::Clamp(a, ozz::math::simd_float4::Load(-12.f, 2.f, 9.f, 3.f),
                       c),
      .5f, 2.f, 6.f, 3.f);

  const SimdInt4 eq1 = ozz::math::CmpEq(a, b);
  EXPECT_SIMDINT_EQ(eq1, 0, 0xffffffff, 0, 0);

  const SimdInt4 eq2 = ozz::math::CmpEq(a, a);
  EXPECT_SIMDINT_EQ(eq2, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff);

  const SimdInt4 neq1 = ozz::math::CmpNe(a, b);
  EXPECT_SIMDINT_EQ(neq1, 0xffffffff, 0, 0xffffffff, 0xffffffff);

  const SimdInt4 neq2 = ozz::math::CmpNe(a, a);
  EXPECT_SIMDINT_EQ(neq2, 0, 0, 0, 0);

  const SimdInt4 lt = ozz::math::CmpLt(a, b);
  EXPECT_SIMDINT_EQ(lt, 0xffffffff, 0, 0, 0xffffffff);

  const SimdInt4 le = ozz::math::CmpLe(a, b);
  EXPECT_SIMDINT_EQ(le, 0xffffffff, 0xffffffff, 0u, 0xffffffff);

  const SimdInt4 gt = ozz::math::CmpGt(a, b);
  EXPECT_SIMDINT_EQ(gt, 0u, 0u, 0xffffffff, 0u);

  const SimdInt4 ge = ozz::math::CmpGe(a, b);
  EXPECT_SIMDINT_EQ(ge, 0u, 0xffffffff, 0xffffffff, 0u);
}

TEST(LerpFloat, ozz_simd_math) {
  const SimdFloat4 a = ozz::math::simd_float4::Load(0.f, 1.f, 2.f, 4.f);
  const SimdFloat4 b = ozz::math::simd_float4::Load(0.f, -1.f, -2.f, -4.f);
  const SimdFloat4 zero = ozz::math::simd_float4::Load1(0.f);
  const SimdFloat4 one = ozz::math::simd_float4::Load1(1.f);

  const SimdFloat4 lerp0 = ozz::math::Lerp(a, b, zero);
  EXPECT_SIMDFLOAT_EQ(lerp0, 0.f, 1.f, 2.f, 4.f);

  const SimdFloat4 lerp1 = ozz::math::Lerp(a, b, one);
  EXPECT_SIMDFLOAT_EQ(lerp1, 0.f, -1.f, -2.f, -4.f);

  const SimdFloat4 lhalf =
      ozz::math::Lerp(a, b, ozz::math::simd_float4::Load1(0.5f));
  EXPECT_SIMDFLOAT_EQ(lhalf, 0.f, 0.f, 0.f, 0.f);

  const SimdFloat4 lmixed =
      ozz::math::Lerp(a, b, ozz::math::simd_float4::Load(0.f, -1.f, 0.5f, 2.f));
  EXPECT_SIMDFLOAT_EQ(lmixed, 0.f, 3.f, 0.f, -12.f);
}

TEST(TrigonometryFloat, ozz_simd_math) {
  using ozz::math::k2Pi;
  using ozz::math::kPi;
  using ozz::math::kPi_2;

  const SimdFloat4 angle =
      ozz::math::simd_float4::Load(kPi, kPi / 6.f, -kPi_2, 5.f * kPi_2);
  const SimdFloat4 cos =
      ozz::math::simd_float4::Load(-1.f, .86602539f, 0.f, 0.f);
  const SimdFloat4 sin = ozz::math::simd_float4::Load(0.f, .5f, -1.f, 1.f);

  const SimdFloat4 angle_tan =
      ozz::math::simd_float4::Load(0.f, kPi / 6.f, -kPi / 3.f, 9.f * kPi / 4.f);
  const SimdFloat4 tan =
      ozz::math::simd_float4::Load(0.f, .57735f, -1.73205f, 1.f);

  EXPECT_SIMDFLOAT_EQ(ozz::math::Cos(angle), -1.f, .86602539f, 0.f, 0.f);
  EXPECT_SIMDFLOAT_EQ(
      ozz::math::Cos(angle + ozz::math::simd_float4::Load1(k2Pi)), -1.f,
      .86602539f, 0.f, 0.f);
  EXPECT_SIMDFLOAT_EQ(
      ozz::math::Cos(angle + ozz::math::simd_float4::Load1(k2Pi * 12.f)), -1.f,
      .86602539f, 0.f, 0.f);
  EXPECT_SIMDFLOAT_EQ(
      ozz::math::Cos(angle - ozz::math::simd_float4::Load1(k2Pi * 24.f)), -1.f,
      .86602539f, 0.f, 0.f);

  EXPECT_SIMDFLOAT_EQ(ozz::math::CosX(angle), -1.f, kPi / 6.f, -kPi_2,
                      5.f * kPi_2);
  EXPECT_SIMDFLOAT_EQ(
      ozz::math::CosX(angle + ozz::math::simd_float4::LoadX(k2Pi)), -1.f,
      kPi / 6.f, -kPi_2, 5.f * kPi_2);
  EXPECT_SIMDFLOAT_EQ(
      ozz::math::CosX(angle + ozz::math::simd_float4::LoadX(k2Pi * 12.f)), -1.f,
      kPi / 6.f, -kPi_2, 5.f * kPi_2);
  EXPECT_SIMDFLOAT_EQ(
      ozz::math::CosX(angle - ozz::math::simd_float4::LoadX(k2Pi * 24.f)), -1.f,
      kPi / 6.f, -kPi_2, 5.f * kPi_2);

  EXPECT_SIMDFLOAT_EQ(ozz::math::Sin(angle), 0.f, .5f, -1.f, 1.f);
  EXPECT_SIMDFLOAT_EQ(
      ozz::math::Sin(angle + ozz::math::simd_float4::Load1(k2Pi)), 0.f, 0.5f,
      -1.f, 1.f);
  EXPECT_SIMDFLOAT_EQ(
      ozz::math::Sin(angle + ozz::math::simd_float4::Load1(k2Pi * 12.f)), 0.f,
      .5f, -1.f, 1.f);
  EXPECT_SIMDFLOAT_EQ(
      ozz::math::Sin(angle - ozz::math::simd_float4::Load1(k2Pi * 24.f)), 0.f,
      .5f, -1.f, 1.f);

  EXPECT_SIMDFLOAT_EQ(ozz::math::SinX(angle), 0.f, kPi / 6.f, -kPi_2,
                      5.f * kPi_2);
  EXPECT_SIMDFLOAT_EQ(
      ozz::math::SinX(angle + ozz::math::simd_float4::LoadX(k2Pi)), 0.f,
      kPi / 6.f, -kPi_2, 5.f * kPi_2);
  EXPECT_SIMDFLOAT_EQ(
      ozz::math::SinX(angle + ozz::math::simd_float4::LoadX(k2Pi * 12.f)), 0.f,
      kPi / 6.f, -kPi_2, 5.f * kPi_2);
  EXPECT_SIMDFLOAT_EQ(
      ozz::math::SinX(angle - ozz::math::simd_float4::LoadX(k2Pi * 24.f)), 0.f,
      kPi / 6.f, -kPi_2, 5.f * kPi_2);

  EXPECT_SIMDFLOAT_EQ(ozz::math::ACos(cos), kPi, kPi / 6.f, kPi_2, kPi_2);
  EXPECT_SIMDFLOAT_EQ(ozz::math::ACosX(cos), kPi, .86602539f, 0.f, 0.f);

  EXPECT_SIMDFLOAT_EQ(ozz::math::ASin(sin), 0.f, kPi / 6.f, -kPi_2, kPi_2);
  EXPECT_SIMDFLOAT_EQ(ozz::math::ASinX(sin), 0.f, .5f, -1.f, 1.f);

  EXPECT_SIMDFLOAT_EQ(ozz::math::Tan(angle_tan), 0.f, 0.57735f, -1.73205f, 1.f);
  EXPECT_SIMDFLOAT_EQ(ozz::math::TanX(angle_tan), 0.f, kPi / 6.f, -kPi / 3.f,
                      9.f * kPi / 4.f);

  EXPECT_SIMDFLOAT_EQ(ozz::math::ATan(tan), 0.f, kPi / 6.f, -kPi / 3.f,
                      kPi / 4.f);
  EXPECT_SIMDFLOAT_EQ(ozz::math::ATanX(tan), 0.f, .57735f, -1.73205f, 1.f);
}

TEST(LogicalFloat, ozz_simd_math) {
  const SimdFloat4 a = ozz::math::simd_float4::Load(0.f, 1.f, 2.f, 3.f);
  const SimdFloat4 b = ozz::math::simd_float4::Load(1.f, -1.f, -3.f, -4.f);
  const SimdInt4 mbool =
      ozz::math::simd_int4::Load(0xffffffff, 0, 0, 0xffffffff);
  const SimdInt4 mbit =
      ozz::math::simd_int4::Load(0xffffffff, 0, 0x80000000, 0x7fffffff);
  const SimdFloat4 mfloat = ozz::math::simd_float4::Load(1.f, 0.f, -0.f, 3.f);

  const SimdFloat4 select = ozz::math::Select(mbool, a, b);
  EXPECT_SIMDFLOAT_EQ(select, 0.f, -1.f, -3.f, 3.f);

  const SimdFloat4 andm = ozz::math::And(b, mbit);
  EXPECT_SIMDFLOAT_EQ(andm, 1.f, 0.f, 0.f, 4.f);

  const SimdFloat4 andnm = ozz::math::AndNot(b, mbit);
  EXPECT_SIMDFLOAT_EQ(andnm, 0.f, -1.f, 3.f, -0.f);

  const SimdFloat4 andf = ozz::math::And(b, mfloat);
  EXPECT_SIMDFLOAT_EQ(andf, 1.f, 0.f, -0.f, 2.f);

  const SimdFloat4 orm = ozz::math::Or(a, mbit);
  union {
    float f;
    unsigned int i;
  } orx = {ozz::math::GetX(orm)};
  EXPECT_EQ(orx.i, 0xffffffff);
  EXPECT_FLOAT_EQ(ozz::math::GetY(orm), 1.f);
  EXPECT_FLOAT_EQ(ozz::math::GetZ(orm), -2.f);
  union {
    float f;
    int i;
  } orw = {ozz::math::GetW(orm)};
  EXPECT_TRUE(orw.i == 0x7fffffff);

  const SimdFloat4 ormf = ozz::math::Or(a, mfloat);
  EXPECT_SIMDFLOAT_EQ(ormf, 1.f, 1.f, -2.f, 3.f);

  const SimdFloat4 xorm = ozz::math::Xor(a, mbit);
  union {
    float f;
    unsigned int i;
  } xorx = {ozz::math::GetX(xorm)};
  EXPECT_TRUE(xorx.i == 0xffffffff);
  EXPECT_FLOAT_EQ(ozz::math::GetY(xorm), 1.f);
  EXPECT_FLOAT_EQ(ozz::math::GetZ(xorm), -2.f);
  union {
    float f;
    unsigned int i;
  } xorw = {ozz::math::GetW(xorm)};
  EXPECT_TRUE(xorw.i == 0x3fbfffff);

  const SimdFloat4 xormf = ozz::math::Xor(a, mfloat);
  EXPECT_SIMDFLOAT_EQ(xormf, 1.f, 1.f, -2.f, 0.f);
}

TEST(Half, ozz_simd_math) {
  // 0
  EXPECT_EQ(ozz::math::FloatToHalf(0.f), 0);
  EXPECT_FLOAT_EQ(ozz::math::HalfToFloat(0), 0.f);
  EXPECT_EQ(ozz::math::FloatToHalf(-0.f), 0x8000);
  EXPECT_FLOAT_EQ(ozz::math::HalfToFloat(0x8000), -0.f);
  EXPECT_EQ(ozz::math::FloatToHalf(std::numeric_limits<float>::min()), 0);
  EXPECT_EQ(ozz::math::FloatToHalf(std::numeric_limits<float>::denorm_min()),
            0);
  EXPECT_EQ(
      ozz::math::FloatToHalf(std::numeric_limits<float>::denorm_min() / 10.f),
      0);

  // 1
  EXPECT_EQ(ozz::math::FloatToHalf(1.f), 0x3c00);
  EXPECT_FLOAT_EQ(ozz::math::HalfToFloat(0x3c00), 1.f);
  EXPECT_EQ(ozz::math::FloatToHalf(-1.f), 0xbc00);
  EXPECT_FLOAT_EQ(ozz::math::HalfToFloat(0xbc00), -1.f);

  // Bounds
  EXPECT_EQ(ozz::math::FloatToHalf(65504.f), 0x7bff);
  EXPECT_EQ(ozz::math::FloatToHalf(-65504.f), 0xfbff);

  // Min, Max, Infinity
  EXPECT_EQ(ozz::math::FloatToHalf(10e-16f), 0);
  EXPECT_EQ(ozz::math::FloatToHalf(10e+16f), 0x7c00);
  EXPECT_FLOAT_EQ(ozz::math::HalfToFloat(0x7c00),
                  std::numeric_limits<float>::infinity());
  EXPECT_EQ(ozz::math::FloatToHalf(std::numeric_limits<float>::max()), 0x7c00);
  EXPECT_EQ(ozz::math::FloatToHalf(std::numeric_limits<float>::infinity()),
            0x7c00);
  EXPECT_EQ(ozz::math::FloatToHalf(-10e+16f), 0xfc00);
  EXPECT_EQ(ozz::math::FloatToHalf(-std::numeric_limits<float>::infinity()),
            0xfc00);
  EXPECT_EQ(ozz::math::FloatToHalf(-std::numeric_limits<float>::max()), 0xfc00);
  EXPECT_FLOAT_EQ(ozz::math::HalfToFloat(0xfc00),
                  -std::numeric_limits<float>::infinity());

  // Nan
  EXPECT_EQ(ozz::math::FloatToHalf(std::numeric_limits<float>::quiet_NaN()),
            0x7e00);
  EXPECT_EQ(ozz::math::FloatToHalf(std::numeric_limits<float>::signaling_NaN()),
            0x7e00);
  // According to the IEEE standard, NaN values have the odd property that
  // comparisons involving them are always false
  EXPECT_FALSE(ozz::math::HalfToFloat(0x7e00) ==
               ozz::math::HalfToFloat(0x7e00));

  // Random tries in range [10e-4,10e4].
  for (float pow = -4.f; pow <= 4.f; pow += 1.f) {
    const float max = powf(10.f, pow);
    // Expect a 1/1000 precision over floats.
    const float precision = max / 1000.f;

    const int n = 1000;
    for (int i = 0; i < n; ++i) {
      const float frand = max * (2.f * i / n - 1.f);

      const uint16_t h = ozz::math::FloatToHalf(frand);
      const float f = ozz::math::HalfToFloat(h);
      EXPECT_NEAR(frand, f, precision);
    }
  }
}

TEST(SimdHalf, ozz_simd_math) {
  // 0
  EXPECT_SIMDINT_EQ(ozz::math::FloatToHalf(ozz::math::simd_float4::Load(
                        0.f, -0.f, std::numeric_limits<float>::min(),
                        std::numeric_limits<float>::denorm_min())),
                    0, 0x00008000, 0, 0);
  EXPECT_SIMDFLOAT_EQ(
      ozz::math::HalfToFloat(ozz::math::simd_int4::Load(0, 0x00008000, 0, 0)),
      0.f, -0.f, 0.f, 0.f);

  // 1
  EXPECT_SIMDINT_EQ(ozz::math::FloatToHalf(
                        ozz::math::simd_float4::Load(1.f, -1.f, 0.f, -0.f)),
                    0x00003c00, 0x0000bc00, 0, 0x00008000);
  EXPECT_SIMDFLOAT_EQ(ozz::math::HalfToFloat(ozz::math::simd_int4::Load(
                          0x3c00, 0xbc00, 0, 0x00008000)),
                      1.f, -1.f, 0.f, -0.f);

  // Bounds
  EXPECT_SIMDINT_EQ(ozz::math::FloatToHalf(ozz::math::simd_float4::Load(
                        65504.f, -65504.f, 65604.f, -65604.f)),
                    0x00007bff, 0x0000fbff, 0x00007c00, 0x0000fc00);

  // Min, Max, Infinity
  EXPECT_SIMDINT_EQ(ozz::math::FloatToHalf(ozz::math::simd_float4::Load(
                        10e-16f, 10e+16f, std::numeric_limits<float>::max(),
                        std::numeric_limits<float>::infinity())),
                    0, 0x00007c00, 0x00007c00, 0x00007c00);
  EXPECT_SIMDINT_EQ(ozz::math::FloatToHalf(ozz::math::simd_float4::Load(
                        -10e-16f, -10e+16f, -std::numeric_limits<float>::max(),
                        -std::numeric_limits<float>::max())),
                    0x00008000, 0x0000fc00, 0x0000fc00, 0x0000fc00);

  // Nan
  EXPECT_SIMDINT_EQ(ozz::math::FloatToHalf(ozz::math::simd_float4::Load(
                        std::numeric_limits<float>::quiet_NaN(),
                        std::numeric_limits<float>::signaling_NaN(), 0, 0)),
                    0x00007e00, 0x00007e00, 0, 0);

  // Inf and NAN
  const SimdFloat4 infnan = ozz::math::HalfToFloat(
      ozz::math::simd_int4::Load(0x00007c00, 0x0000fc00, 0x00007e00, 0));
  EXPECT_FLOAT_EQ(ozz::math::GetX(infnan),
                  std::numeric_limits<float>::infinity());
  EXPECT_FLOAT_EQ(ozz::math::GetY(infnan),
                  -std::numeric_limits<float>::infinity());
  EXPECT_FALSE(ozz::math::GetZ(infnan) == ozz::math::GetZ(infnan));

  // Random tries in range [10e-4,10e4].
  srand(0);
  for (float pow = -4.f; pow <= 4.f; pow += 1.f) {
    const float max = powf(10.f, pow);
    // Expect a 1/1000 precision over floats.
    const float precision = max / 1000.f;

    const int n = 1000;
    for (int i = 0; i < n; ++i) {
      const SimdFloat4 frand = ozz::math::simd_float4::Load(
          max * (.5f * i / n - .25f), max * (1.f * i / n - .5f),
          max * (1.5f * i / n - .75f), max * (2.f * i / n - 1.f));

      const SimdInt4 h = ozz::math::FloatToHalf(frand);
      const SimdFloat4 f = ozz::math::HalfToFloat(h);

      EXPECT_NEAR(ozz::math::GetX(frand), ozz::math::GetX(f), precision);
      EXPECT_NEAR(ozz::math::GetY(frand), ozz::math::GetY(f), precision);
      EXPECT_NEAR(ozz::math::GetZ(frand), ozz::math::GetZ(f), precision);
      EXPECT_NEAR(ozz::math::GetW(frand), ozz::math::GetW(f), precision);
    }
  }
}
