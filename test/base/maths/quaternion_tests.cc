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

#include "ozz/base/maths/quaternion.h"

#include "gtest/gtest.h"

#include "ozz/base/gtest_helper.h"
#include "ozz/base/maths/gtest_math_helper.h"

using ozz::math::Float3;
using ozz::math::Float4;
using ozz::math::Quaternion;

TEST(QuaternionConstant, ozz_math) {
  EXPECT_QUATERNION_EQ(Quaternion::identity(), 0.f, 0.f, 0.f, 1.f);
}

TEST(QuaternionAxisAngle, ozz_math) {
  // Expect assertions from invalid inputs
  EXPECT_ASSERTION(Quaternion::FromAxisAngle(Float3::zero(), 0.f),
                   "axis is not normalized");
  EXPECT_ASSERTION(ToAxisAngle(Quaternion(0.f, 0.f, 0.f, 2.f)), "IsNormalized");

  // Identity
  EXPECT_QUATERNION_EQ(Quaternion::FromAxisAngle(Float3::y_axis(), 0.f), 0.f,
                       0.f, 0.f, 1.f);
  EXPECT_FLOAT4_EQ(ToAxisAngle(Quaternion::identity()), 1.f, 0.f, 0.f, 0.f);

  // Other axis angles
  EXPECT_QUATERNION_EQ(
      Quaternion::FromAxisAngle(Float3::y_axis(), ozz::math::kPi_2), 0.f,
      .70710677f, 0.f, .70710677f);
  EXPECT_FLOAT4_EQ(ToAxisAngle(Quaternion(0.f, .70710677f, 0.f, .70710677f)),
                   0.f, 1.f, 0.f, ozz::math::kPi_2);

  EXPECT_QUATERNION_EQ(
      Quaternion::FromAxisAngle(Float3::y_axis(), -ozz::math::kPi_2), 0.f,
      -.70710677f, 0.f, .70710677f);
  EXPECT_QUATERNION_EQ(
      Quaternion::FromAxisAngle(-Float3::y_axis(), ozz::math::kPi_2), 0.f,
      -.70710677f, 0.f, .70710677f);
  EXPECT_FLOAT4_EQ(ToAxisAngle(Quaternion(0.f, -.70710677f, 0.f, .70710677f)),
                   0.f, -1.f, 0.f, ozz::math::kPi_2);

  EXPECT_QUATERNION_EQ(
      Quaternion::FromAxisAngle(Float3::y_axis(), 3.f * ozz::math::kPi_4), 0.f,
      0.923879504f, 0.f, 0.382683426f);
  EXPECT_FLOAT4_EQ(
      ToAxisAngle(Quaternion(0.f, 0.923879504f, 0.f, 0.382683426f)), 0.f, 1.f,
      0.f, 3.f * ozz::math::kPi_4);

  EXPECT_QUATERNION_EQ(
      Quaternion::FromAxisAngle(Float3(.819865f, .033034f, -.571604f), 1.123f),
      .4365425f, .017589169f, -.30435428f, .84645736f);
  EXPECT_FLOAT4_EQ(
      ToAxisAngle(Quaternion(.4365425f, .017589169f, -.30435428f, .84645736f)),
      .819865f, .033034f, -.571604f, 1.123f);
}

TEST(QuaternionAxisCosAngle, ozz_math) {
  // Expect assertions from invalid inputs
  EXPECT_ASSERTION(Quaternion::FromAxisCosAngle(Float3::zero(), 0.f),
                   "axis is not normalized");
  EXPECT_ASSERTION(Quaternion::FromAxisCosAngle(Float3::y_axis(), -1.0000001f),
                   "cos is not in \\[-1,1\\] range.");
  EXPECT_ASSERTION(Quaternion::FromAxisCosAngle(Float3::y_axis(), 1.0000001f),
                   "cos is not in \\[-1,1\\] range.");

  // Identity
  EXPECT_QUATERNION_EQ(Quaternion::FromAxisCosAngle(Float3::y_axis(), 1.f), 0.f,
                       0.f, 0.f, 1.f);

  // Other axis angles
  EXPECT_QUATERNION_EQ(Quaternion::FromAxisCosAngle(Float3::y_axis(),
                                                    std::cos(ozz::math::kPi_2)),
                       0.f, .70710677f, 0.f, .70710677f);
  EXPECT_QUATERNION_EQ(Quaternion::FromAxisCosAngle(-Float3::y_axis(),
                                                    std::cos(ozz::math::kPi_2)),
                       0.f, -.70710677f, 0.f, .70710677f);

  EXPECT_QUATERNION_EQ(Quaternion::FromAxisCosAngle(
                           Float3::y_axis(), std::cos(3.f * ozz::math::kPi_4)),
                       0.f, 0.923879504f, 0.f, 0.382683426f);

  EXPECT_QUATERNION_EQ(
      Quaternion::FromAxisCosAngle(Float3(.819865f, .033034f, -.571604f),
                                   std::cos(1.123f)),
      .4365425f, .017589169f, -.30435428f, .84645736f);
}

TEST(QuaternionQuaternionEuler, ozz_math) {
  // Identity
  EXPECT_QUATERNION_EQ(Quaternion::FromEuler(0.f, 0.f, 0.f), 0.f, 0.f, 0.f,
                       1.f);
  EXPECT_FLOAT3_EQ(ToEuler(Quaternion::identity()), 0.f, 0.f, 0.f);

  // Heading
  EXPECT_QUATERNION_EQ(Quaternion::FromEuler(ozz::math::kPi_2, 0.f, 0.f), 0.f,
                       .70710677f, 0.f, .70710677f);
  EXPECT_FLOAT3_EQ(ToEuler(Quaternion(0.f, .70710677f, 0.f, .70710677f)),
                   ozz::math::kPi_2, 0.f, 0.f);

  // Elevation
  EXPECT_QUATERNION_EQ(Quaternion::FromEuler(0.f, ozz::math::kPi_2, 0.f), 0.f,
                       0.f, .70710677f, .70710677f);
  EXPECT_FLOAT3_EQ(ToEuler(Quaternion(0.f, 0.f, .70710677f, .70710677f)), 0.f,
                   ozz::math::kPi_2, 0.f);

  // Bank
  EXPECT_QUATERNION_EQ(Quaternion::FromEuler(0.f, 0.f, ozz::math::kPi_2),
                       .70710677f, 0.f, 0.f, .70710677f);
  EXPECT_FLOAT3_EQ(ToEuler(Quaternion(.70710677f, 0.f, 0.f, .70710677f)), 0.f,
                   0.f, ozz::math::kPi_2);

  // Any rotation
  EXPECT_QUATERNION_EQ(
      Quaternion::FromEuler(ozz::math::kPi / 4.f, -ozz::math::kPi / 6.f,
                            ozz::math::kPi_2),
      .56098551f, .092295974f, -0.43045932f, .70105737f);
  EXPECT_FLOAT3_EQ(
      ToEuler(Quaternion(.56098551f, .092295974f, -0.43045932f, .70105737f)),
      ozz::math::kPi / 4.f, -ozz::math::kPi / 6.f, ozz::math::kPi_2);
}

TEST(QuaternionFromVectors, ozz_math) {
  // Returns identity for a 0 length vector
  EXPECT_QUATERNION_EQ(
      Quaternion::FromVectors(Float3::zero(), Float3::x_axis()), 0.f, 0.f, 0.f,
      1.f);

  // pi/2 around y
  EXPECT_QUATERNION_EQ(
      Quaternion::FromVectors(Float3::z_axis(), Float3::x_axis()), 0.f,
      0.707106769f, 0.f, 0.707106769f);

  // Non unit pi/2 around y
  EXPECT_QUATERNION_EQ(
      Quaternion::FromVectors(Float3::z_axis() * 7.f, Float3::x_axis()), 0.f,
      0.707106769f, 0.f, 0.707106769f);

  // Minus pi/2 around y
  EXPECT_QUATERNION_EQ(
      Quaternion::FromVectors(Float3::x_axis(), Float3::z_axis()), 0.f,
      -0.707106769f, 0.f, 0.707106769f);

  // pi/2 around x
  EXPECT_QUATERNION_EQ(
      Quaternion::FromVectors(Float3::y_axis(), Float3::z_axis()), 0.707106769f,
      0.f, 0.f, 0.707106769f);

  // Non unit pi/2 around x
  EXPECT_QUATERNION_EQ(
      Quaternion::FromVectors(Float3::y_axis() * 9.f, Float3::z_axis() * 13.f),
      0.707106769f, 0.f, 0.f, 0.707106769f);

  // pi/2 around z
  EXPECT_QUATERNION_EQ(
      Quaternion::FromVectors(Float3::x_axis(), Float3::y_axis()), 0.f, 0.f,
      0.707106769f, 0.707106769f);

  // pi/2 around z also
  EXPECT_QUATERNION_EQ(
      Quaternion::FromVectors(Float3(0.707106769f, 0.707106769f, 0.f),
                              Float3(-0.707106769f, 0.707106769f, 0.f)),
      0.f, 0.f, 0.707106769f, 0.707106769f);

  // Aligned vectors
  EXPECT_QUATERNION_EQ(
      Quaternion::FromVectors(Float3::x_axis(), Float3::x_axis()), 0.f, 0.f,
      0.f, 1.f);

  // Non-unit aligned vectors
  EXPECT_QUATERNION_EQ(
      Quaternion::FromVectors(Float3::x_axis(), Float3::x_axis() * 2.f), 0.f,
      0.f, 0.f, 1.f);

  // Opposed vectors
  EXPECT_QUATERNION_EQ(
      Quaternion::FromVectors(Float3::x_axis(), -Float3::x_axis()), 0.f, 1.f,
      0.f, 0);
  EXPECT_QUATERNION_EQ(
      Quaternion::FromVectors(-Float3::x_axis(), Float3::x_axis()), 0.f, -1.f,
      0.f, 0);
  EXPECT_QUATERNION_EQ(
      Quaternion::FromVectors(Float3::y_axis(), -Float3::y_axis()), 0.f, 0.f,
      1.f, 0);
  EXPECT_QUATERNION_EQ(
      Quaternion::FromVectors(-Float3::y_axis(), Float3::y_axis()), 0.f, 0.f,
      -1.f, 0);
  EXPECT_QUATERNION_EQ(
      Quaternion::FromVectors(Float3::z_axis(), -Float3::z_axis()), 0.f, -1.f,
      0.f, 0);
  EXPECT_QUATERNION_EQ(
      Quaternion::FromVectors(-Float3::z_axis(), Float3::z_axis()), 0.f, 1.f,
      0.f, 0);
  EXPECT_QUATERNION_EQ(
      Quaternion::FromVectors(Float3(0.707106769f, 0.707106769f, 0.f),
                              -Float3(0.707106769f, 0.707106769f, 0.f)),
      -0.707106769f, 0.707106769f, 0.f, 0);
  EXPECT_QUATERNION_EQ(
      Quaternion::FromVectors(Float3(0.f, 0.707106769f, 0.707106769f),
                              -Float3(0.f, 0.707106769f, 0.707106769f)),
      0.f, -0.707106769f, 0.707106769f, 0);

  // Non-unit opposed vectors
  EXPECT_QUATERNION_EQ(
      Quaternion::FromVectors(Float3(2.f, 2.f, 2.f), -Float3(2.f, 2.f, 2.f)),
      0.f, -0.707106769f, 0.707106769f, 0);
}

TEST(QuaternionFromUnitVectors, ozz_math) {
  // assert 0 length vectors
  EXPECT_ASSERTION(
      Quaternion::FromUnitVectors(Float3::zero(), Float3::x_axis()),
      "Input vectors must be normalized.");
  EXPECT_ASSERTION(
      Quaternion::FromUnitVectors(Float3::x_axis(), Float3::zero()),
      "Input vectors must be normalized.");
  // assert non unit vectors
  EXPECT_ASSERTION(
      Quaternion::FromUnitVectors(Float3::x_axis() * 2.f, Float3::x_axis()),
      "Input vectors must be normalized.");
  EXPECT_ASSERTION(
      Quaternion::FromUnitVectors(Float3::x_axis(), Float3::x_axis() * .5f),
      "Input vectors must be normalized.");

  // pi/2 around y
  EXPECT_QUATERNION_EQ(
      Quaternion::FromUnitVectors(Float3::z_axis(), Float3::x_axis()), 0.f,
      0.707106769f, 0.f, 0.707106769f);

  // Minus pi/2 around y
  EXPECT_QUATERNION_EQ(
      Quaternion::FromUnitVectors(Float3::x_axis(), Float3::z_axis()), 0.f,
      -0.707106769f, 0.f, 0.707106769f);

  // pi/2 around x
  EXPECT_QUATERNION_EQ(
      Quaternion::FromUnitVectors(Float3::y_axis(), Float3::z_axis()),
      0.707106769f, 0.f, 0.f, 0.707106769f);

  // pi/2 around z
  EXPECT_QUATERNION_EQ(
      Quaternion::FromUnitVectors(Float3::x_axis(), Float3::y_axis()), 0.f, 0.f,
      0.707106769f, 0.707106769f);

  // pi/2 around z also
  EXPECT_QUATERNION_EQ(
      Quaternion::FromUnitVectors(Float3(0.707106769f, 0.707106769f, 0.f),
                                  Float3(-0.707106769f, 0.707106769f, 0.f)),
      0.f, 0.f, 0.707106769f, 0.707106769f);

  // Aligned vectors
  EXPECT_QUATERNION_EQ(
      Quaternion::FromUnitVectors(Float3::x_axis(), Float3::x_axis()), 0.f, 0.f,
      0.f, 1.f);

  // Opposed vectors
  EXPECT_QUATERNION_EQ(
      Quaternion::FromUnitVectors(Float3::x_axis(), -Float3::x_axis()), 0.f,
      1.f, 0.f, 0);
  EXPECT_QUATERNION_EQ(
      Quaternion::FromUnitVectors(-Float3::x_axis(), Float3::x_axis()), 0.f,
      -1.f, 0.f, 0);
  EXPECT_QUATERNION_EQ(
      Quaternion::FromUnitVectors(Float3::y_axis(), -Float3::y_axis()), 0.f,
      0.f, 1.f, 0);
  EXPECT_QUATERNION_EQ(
      Quaternion::FromUnitVectors(-Float3::y_axis(), Float3::y_axis()), 0.f,
      0.f, -1.f, 0);
  EXPECT_QUATERNION_EQ(
      Quaternion::FromUnitVectors(Float3::z_axis(), -Float3::z_axis()), 0.f,
      -1.f, 0.f, 0);
  EXPECT_QUATERNION_EQ(
      Quaternion::FromUnitVectors(-Float3::z_axis(), Float3::z_axis()), 0.f,
      1.f, 0.f, 0);
  EXPECT_QUATERNION_EQ(
      Quaternion::FromUnitVectors(Float3(0.707106769f, 0.707106769f, 0.f),
                                  -Float3(0.707106769f, 0.707106769f, 0.f)),
      -0.707106769f, 0.707106769f, 0.f, 0);
  EXPECT_QUATERNION_EQ(
      Quaternion::FromUnitVectors(Float3(0.f, 0.707106769f, 0.707106769f),
                                  -Float3(0.f, 0.707106769f, 0.707106769f)),
      0.f, -0.707106769f, 0.707106769f, 0);
}

TEST(QuaternionCompare, ozz_math) {
  EXPECT_TRUE(Quaternion::identity() == Quaternion(0.f, 0.f, 0.f, 1.f));
  EXPECT_TRUE(Quaternion::identity() != Quaternion(1.f, 0.f, 0.f, 0.f));
  EXPECT_TRUE(Compare(Quaternion::identity(), Quaternion::identity(), std::cos(.5f * 0.f)));
  EXPECT_TRUE(Compare(Quaternion::identity(),
                      Quaternion::FromEuler(0.f, 0.f, ozz::math::kPi / 100.f),
                      std::cos(.5f * ozz::math::kPi / 50.f)));
  EXPECT_TRUE(Compare(Quaternion::identity(),
                      -Quaternion::FromEuler(0.f, 0.f, ozz::math::kPi / 100.f),
                      std::cos(.5f * ozz::math::kPi / 50.f)));
  EXPECT_FALSE(Compare(Quaternion::identity(),
                       Quaternion::FromEuler(0.f, 0.f, ozz::math::kPi / 100.f),
                       std::cos(.5f * ozz::math::kPi / 200.f)));
}

TEST(QuaternionArithmetic, ozz_math) {
  const Quaternion a(.70710677f, 0.f, 0.f, .70710677f);
  const Quaternion b(0.f, .70710677f, 0.f, .70710677f);
  const Quaternion c(0.f, .70710677f, 0.f, -.70710677f);
  const Quaternion denorm(1.414212f, 0.f, 0.f, 1.414212f);

  EXPECT_TRUE(IsNormalized(a));
  EXPECT_TRUE(IsNormalized(b));
  EXPECT_TRUE(IsNormalized(c));
  EXPECT_FALSE(IsNormalized(denorm));

  const Quaternion conjugate = Conjugate(a);
  EXPECT_QUATERNION_EQ(conjugate, -a.x, -a.y, -a.z, a.w);
  EXPECT_TRUE(IsNormalized(conjugate));

  const Quaternion negate = -a;
  EXPECT_QUATERNION_EQ(negate, -a.x, -a.y, -a.z, -a.w);
  EXPECT_TRUE(IsNormalized(negate));

  const Quaternion add = a + b;
  EXPECT_QUATERNION_EQ(add, .70710677f, .70710677f, 0.f, 1.41421354f);

  const Quaternion mul0 = a * conjugate;
  EXPECT_QUATERNION_EQ(mul0, 0.f, 0.f, 0.f, 1.f);
  EXPECT_TRUE(IsNormalized(mul0));

  const Quaternion muls = a * 2.f;
  EXPECT_QUATERNION_EQ(muls, 1.41421354f, 0.f, 0.f, 1.41421354f);

  const Quaternion mul1 = conjugate * a;
  EXPECT_QUATERNION_EQ(mul1, 0.f, 0.f, 0.f, 1.f);
  EXPECT_TRUE(IsNormalized(mul1));

  const Quaternion q1234(1.f, 2.f, 3.f, 4.f);
  const Quaternion q5678(5.f, 6.f, 7.f, 8.f);
  const Quaternion mul12345678 = q1234 * q5678;
  EXPECT_QUATERNION_EQ(mul12345678, 24.f, 48.f, 48.f, -6.f);

  EXPECT_ASSERTION(Normalize(Quaternion(0.f, 0.f, 0.f, 0.f)),
                   "is not normalizable");
  const Quaternion normalize = Normalize(denorm);
  EXPECT_TRUE(IsNormalized(normalize));
  EXPECT_QUATERNION_EQ(normalize, .7071068f, 0.f, 0.f, .7071068f);

  EXPECT_ASSERTION(NormalizeSafe(denorm, Quaternion(0.f, 0.f, 0.f, 0.f)),
                   "_safer is not normalized");
  const Quaternion normalize_safe =
      NormalizeSafe(denorm, Quaternion::identity());
  EXPECT_TRUE(IsNormalized(normalize_safe));
  EXPECT_QUATERNION_EQ(normalize_safe, .7071068f, 0.f, 0.f, .7071068f);

  const Quaternion normalize_safer =
      NormalizeSafe(Quaternion(0.f, 0.f, 0.f, 0.f), Quaternion::identity());
  EXPECT_TRUE(IsNormalized(normalize_safer));
  EXPECT_QUATERNION_EQ(normalize_safer, 0.f, 0.f, 0.f, 1.f);

  const Quaternion lerp_0 = Lerp(a, b, 0.f);
  EXPECT_QUATERNION_EQ(lerp_0, a.x, a.y, a.z, a.w);

  const Quaternion lerp_1 = Lerp(a, b, 1.f);
  EXPECT_QUATERNION_EQ(lerp_1, b.x, b.y, b.z, b.w);

  const Quaternion lerp_0_2 = Lerp(a, b, .2f);
  EXPECT_QUATERNION_EQ(lerp_0_2, .5656853f, .1414213f, 0.f, .7071068f);

  const Quaternion nlerp_0 = NLerp(a, b, 0.f);
  EXPECT_TRUE(IsNormalized(nlerp_0));
  EXPECT_QUATERNION_EQ(nlerp_0, a.x, a.y, a.z, a.w);

  const Quaternion nlerp_1 = NLerp(a, b, 1.f);
  EXPECT_TRUE(IsNormalized(nlerp_1));
  EXPECT_QUATERNION_EQ(nlerp_1, b.x, b.y, b.z, b.w);

  const Quaternion nlerp_0_2 = NLerp(a, b, .2f);
  EXPECT_TRUE(IsNormalized(nlerp_0_2));
  EXPECT_QUATERNION_EQ(nlerp_0_2, .6172133f, .1543033f, 0.f, .7715167f);

  EXPECT_ASSERTION(SLerp(denorm, b, 0.f), "IsNormalized\\(_a\\)");
  EXPECT_ASSERTION(SLerp(a, denorm, 0.f), "IsNormalized\\(_b\\)");

  const Quaternion slerp_0 = SLerp(a, b, 0.f);
  EXPECT_TRUE(IsNormalized(slerp_0));
  EXPECT_QUATERNION_EQ(slerp_0, a.x, a.y, a.z, a.w);

  const Quaternion slerp_c_0 = SLerp(a, c, 0.f);
  EXPECT_TRUE(IsNormalized(slerp_c_0));
  EXPECT_QUATERNION_EQ(slerp_c_0, a.x, a.y, a.z, a.w);

  const Quaternion slerp_c_1 = SLerp(a, c, 1.f);
  EXPECT_TRUE(IsNormalized(slerp_c_1));
  EXPECT_QUATERNION_EQ(slerp_c_1, c.x, c.y, c.z, c.w);

  const Quaternion slerp_c_0_6 = SLerp(a, c, .6f);
  EXPECT_TRUE(IsNormalized(slerp_c_0_6));
  EXPECT_QUATERNION_EQ(slerp_c_0_6, .6067752f, .7765344f, 0.f, -.1697592f);

  const Quaternion slerp_1 = SLerp(a, b, 1.f);
  EXPECT_TRUE(IsNormalized(slerp_1));
  EXPECT_QUATERNION_EQ(slerp_1, b.x, b.y, b.z, b.w);

  const Quaternion slerp_0_2 = SLerp(a, b, .2f);
  EXPECT_TRUE(IsNormalized(slerp_0_2));
  EXPECT_QUATERNION_EQ(slerp_0_2, .6067752f, .1697592f, 0.f, .7765344f);

  const Quaternion slerp_0_7 = SLerp(a, b, .7f);
  EXPECT_TRUE(IsNormalized(slerp_0_7));
  EXPECT_QUATERNION_EQ(slerp_0_7, .2523113f, .5463429f, 0.f, .798654f);

  const float dot = Dot(a, b);
  EXPECT_FLOAT_EQ(dot, .5f);
}

TEST(QuaternionTransformVector, ozz_math) {
  // 0 length
  EXPECT_FLOAT3_EQ(
      TransformVector(Quaternion::FromAxisAngle(Float3::y_axis(), 0.f),
                      Float3::zero()),
      0, 0, 0);

  // Unit length
  EXPECT_FLOAT3_EQ(
      TransformVector(Quaternion::FromAxisAngle(Float3::y_axis(), 0.f),
                      Float3::z_axis()),
      0, 0, 1);
  EXPECT_FLOAT3_EQ(TransformVector(Quaternion::FromAxisAngle(Float3::y_axis(),
                                                             ozz::math::kPi_2),
                                   Float3::y_axis()),
                   0, 1, 0);
  EXPECT_FLOAT3_EQ(TransformVector(Quaternion::FromAxisAngle(Float3::y_axis(),
                                                             ozz::math::kPi_2),
                                   Float3::x_axis()),
                   0, 0, -1);
  EXPECT_FLOAT3_EQ(TransformVector(Quaternion::FromAxisAngle(Float3::y_axis(),
                                                             ozz::math::kPi_2),
                                   Float3::z_axis()),
                   1, 0, 0);

  // Non unit
  EXPECT_FLOAT3_EQ(TransformVector(Quaternion::FromAxisAngle(Float3::z_axis(),
                                                             ozz::math::kPi_2),
                                   Float3::x_axis() * 2),
                   0, 2, 0);
}
