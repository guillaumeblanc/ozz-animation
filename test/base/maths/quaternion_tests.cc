//----------------------------------------------------------------------------//
//                                                                            //
// ozz-animation is hosted at http://github.com/guillaumeblanc/ozz-animation  //
// and distributed under the MIT License (MIT).                               //
//                                                                            //
// Copyright (c) 2017 Guillaume Blanc                                         //
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

using ozz::math::Quaternion;
using ozz::math::Float4;
using ozz::math::Float3;

TEST(Constant, Quaternion) {
  EXPECT_QUATERNION_EQ(Quaternion::identity(), 0.f, 0.f, 0.f, 1.f);
}

TEST(AxisAngle, Quaternion) {
  // Expect assertions from invalid inputs
  EXPECT_ASSERTION(Quaternion::FromAxisAngle(Float4(Float3::zero(), 0.f)),
                   "IsNormalized");
  EXPECT_ASSERTION(ToAxisAngle(Quaternion(0.f, 0.f, 0.f, 2.f)), "IsNormalized");

  // Identity
  EXPECT_QUATERNION_EQ(Quaternion::FromAxisAngle(Float4(Float3::y_axis(), 0.f)),
                       0.f, 0.f, 0.f, 1.f);
  EXPECT_FLOAT4_EQ(ToAxisAngle(Quaternion::identity()), 1.f, 0.f, 0.f, 0.f);

  // Other axis angles
  EXPECT_QUATERNION_EQ(
      Quaternion::FromAxisAngle(Float4(Float3::y_axis(), ozz::math::kPi_2)),
      0.f, .70710677f, 0.f, .70710677f);
  EXPECT_FLOAT4_EQ(ToAxisAngle(Quaternion(0.f, .70710677f, 0.f, .70710677f)),
                   0.f, 1.f, 0.f, ozz::math::kPi_2);

  EXPECT_QUATERNION_EQ(
      Quaternion::FromAxisAngle(Float4(Float3::y_axis(), -ozz::math::kPi_2)),
      0.f, -.70710677f, 0.f, .70710677f);
  EXPECT_FLOAT4_EQ(ToAxisAngle(Quaternion(0.f, -.70710677f, 0.f, .70710677f)),
                   0.f, -1.f, 0.f, ozz::math::kPi_2);

  EXPECT_QUATERNION_EQ(
      Quaternion::FromAxisAngle(Float4(Float3::y_axis(), -ozz::math::kPi_2)),
      0.f, -.70710677f, 0.f, .70710677f);
  EXPECT_FLOAT4_EQ(ToAxisAngle(Quaternion(0.f, -.70710677f, 0.f, .70710677f)),
                   0.f, -1.f, 0.f, ozz::math::kPi_2);

  EXPECT_QUATERNION_EQ(
      Quaternion::FromAxisAngle(Float4(.819865f, .033034f, -.571604f, 1.123f)),
      .4365425f, .017589169f, -.30435428f, .84645736f);
  EXPECT_FLOAT4_EQ(
      ToAxisAngle(Quaternion(.4365425f, .017589169f, -.30435428f, .84645736f)),
      .819865f, .033034f, -.571604f, 1.123f);
}

TEST(QuaternionEuler, Quaternion) {
  // Identity
  EXPECT_QUATERNION_EQ(Quaternion::FromEuler(Float3(0.f, 0.f, 0.f)), 0.f, 0.f,
                       0.f, 1.f);
  EXPECT_FLOAT3_EQ(ToEuler(Quaternion::identity()), 0.f, 0.f, 0.f);

  // Heading
  EXPECT_QUATERNION_EQ(
      Quaternion::FromEuler(Float3(ozz::math::kPi_2, 0.f, 0.f)), 0.f,
      .70710677f, 0.f, .70710677f);
  EXPECT_FLOAT3_EQ(ToEuler(Quaternion(0.f, .70710677f, 0.f, .70710677f)),
                   ozz::math::kPi_2, 0.f, 0.f);

  // Elevation
  EXPECT_QUATERNION_EQ(
      Quaternion::FromEuler(Float3(0.f, ozz::math::kPi_2, 0.f)), 0.f, 0.f,
      .70710677f, .70710677f);
  EXPECT_FLOAT3_EQ(ToEuler(Quaternion(0.f, 0.f, .70710677f, .70710677f)), 0.f,
                   ozz::math::kPi_2, 0.f);

  // Bank
  EXPECT_QUATERNION_EQ(
      Quaternion::FromEuler(Float3(0.f, 0.f, ozz::math::kPi_2)), .70710677f,
      0.f, 0.f, .70710677f);
  EXPECT_FLOAT3_EQ(ToEuler(Quaternion(.70710677f, 0.f, 0.f, .70710677f)), 0.f,
                   0.f, ozz::math::kPi_2);

  // Any rotation
  EXPECT_QUATERNION_EQ(
      Quaternion::FromEuler(Float3(ozz::math::kPi / 4.f, -ozz::math::kPi / 6.f,
                                   ozz::math::kPi_2)),
      .56098551f, .092295974f, -0.43045932f, .70105737f);
  EXPECT_FLOAT3_EQ(
      ToEuler(Quaternion(.56098551f, .092295974f, -0.43045932f, .70105737f)),
      ozz::math::kPi / 4.f, -ozz::math::kPi / 6.f, ozz::math::kPi_2);
}

TEST(Compare, Quaternion) {
  EXPECT_TRUE(Quaternion::identity() == Quaternion(0.f, 0.f, 0.f, 1.f));
  EXPECT_TRUE(Quaternion::identity() != Quaternion(1.f, 0.f, 0.f, 0.f));
  EXPECT_TRUE(Compare(Quaternion::identity(), Quaternion::identity(), 0.f));
  EXPECT_TRUE(
      Compare(Quaternion::identity(),
              Quaternion::FromEuler(Float3(0.f, 0.f, ozz::math::kPi / 100.f)),
              ozz::math::kPi / 50.f));
  EXPECT_TRUE(
      Compare(Quaternion::identity(),
              -Quaternion::FromEuler(Float3(0.f, 0.f, ozz::math::kPi / 100.f)),
              ozz::math::kPi / 50.f));
  EXPECT_FALSE(
      Compare(Quaternion::identity(),
              Quaternion::FromEuler(Float3(0.f, 0.f, ozz::math::kPi / 100.f)),
              ozz::math::kPi / 200.f));
}

TEST(Arithmetic, Quaternion) {
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
}
