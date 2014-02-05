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

#include "ozz/base/maths/soa_quaternion.h"

#include "gtest/gtest.h"

#include "ozz/base/gtest_helper.h"
#include "ozz/base/maths/gtest_math_helper.h"

using ozz::math::SoaQuaternion;
using ozz::math::SoaFloat4;
using ozz::math::SoaFloat3;

TEST(QuaternionConstant, ozz_math) {
  EXPECT_SOAQUATERNION_EQ(SoaQuaternion::identity(), 0.f, 0.f, 0.f, 0.f,
                                                     0.f, 0.f, 0.f, 0.f,
                                                     0.f, 0.f, 0.f, 0.f,
                                                     1.f, 1.f, 1.f, 1.f);
}

TEST(QuaternionArithmetic, ozz_math) {
  const SoaQuaternion a = SoaQuaternion::Load(
    ozz::math::simd_float4::Load(.70710677f, 0.f, 0.f, .382683432f),
    ozz::math::simd_float4::Load(0.f, 0.f, .70710677f, 0.f),
    ozz::math::simd_float4::Load(0.f, 0.f, 0.f, 0.f),
    ozz::math::simd_float4::Load(.70710677f, 1.f, .70710677f, .9238795f));
  const SoaQuaternion b = SoaQuaternion::Load(
    ozz::math::simd_float4::Load(0.f, .70710677f, 0.f, -.382683432f),
    ozz::math::simd_float4::Load(0.f, 0.f, .70710677f, 0.f),
    ozz::math::simd_float4::Load(0.f, 0.f, 0.f, 0.f),
    ozz::math::simd_float4::Load(1.f, .70710677f, .70710677f, .9238795f));
  const SoaQuaternion denorm = SoaQuaternion::Load(
    ozz::math::simd_float4::Load(.5f, 0.f, 2.f, 3.f),
    ozz::math::simd_float4::Load(4.f, 0.f, 6.f, 7.f),
    ozz::math::simd_float4::Load(8.f, 0.f, 10.f, 11.f),
    ozz::math::simd_float4::Load(12.f, 1.f, 14.f, 15.f));

  EXPECT_TRUE(ozz::math::AreAllTrue(IsNormalized(a)));
  EXPECT_TRUE(ozz::math::AreAllTrue(IsNormalized(b)));
  EXPECT_SIMDINT_EQ(ozz::math::IsNormalized(denorm), 0, 0xffffffff, 0, 0);

  const SoaQuaternion conjugate = Conjugate(a);
  EXPECT_SOAQUATERNION_EQ(conjugate, -.70710677f, -0.f, -0.f, -.382683432f,
                                     -0.f, -0.f, -.70710677f, -0.f,
                                     -0.f, -0.f, -0.f, -0.f,
                                     .70710677f, 1.f, .70710677f, .9238795f);
  EXPECT_TRUE(ozz::math::AreAllTrue(IsNormalized(conjugate)));

  const SoaQuaternion negate = -a;
  EXPECT_SOAQUATERNION_EQ(negate, -.70710677f, 0.f, 0.f, -.382683432f,
                                  0.f, 0.f, -.70710677f, 0.f,
                                  0.f, 0.f, 0.f, 0.f,
                                  -.70710677f, -1.f, -.70710677f, -.9238795f);

  const SoaQuaternion add = a + b;
  EXPECT_SOAQUATERNION_EQ(add, .70710677f, .70710677f, 0.f, 0.f,
                               0.f, 0.f, 1.41421354f, 0.f,
                               0.f, 0.f, 0.f, 0.f,
                               1.70710677f, 1.70710677f, 1.41421354f, 1.847759f);

  const SoaQuaternion muls = a * ozz::math::simd_float4::Load1(2.f);
  EXPECT_SOAQUATERNION_EQ(muls, 1.41421354f, 0.f, 0.f, .765366864f,
                                0.f, 0.f, 1.41421354f, 0.f,
                                0.f, 0.f, 0.f, 0.f,
                                1.41421354f, 2.f, 1.41421354f, 1.847759f);

  const SoaQuaternion mul0 = a * conjugate;
  EXPECT_SOAQUATERNION_EQ(mul0, 0.f, 0.f, 0.f, 0.f,
                                0.f, 0.f, 0.f, 0.f,
                                0.f, 0.f, 0.f, 0.f,
                                1.f, 1.f, 1.f, 1.f);
  EXPECT_TRUE(ozz::math::AreAllTrue(IsNormalized(mul0)));

  const SoaQuaternion mul1 = conjugate * a;
  EXPECT_SOAQUATERNION_EQ(mul1, 0.f, 0.f, 0.f, 0.f,
                                0.f, 0.f, 0.f, 0.f,
                                0.f, 0.f, 0.f, 0.f,
                                1.f, 1.f, 1.f, 1.f);
  EXPECT_TRUE(ozz::math::AreAllTrue(IsNormalized(mul1)));

  const SoaQuaternion normalize = Normalize(denorm);
  EXPECT_TRUE(ozz::math::AreAllTrue(IsNormalized(normalize)));
  EXPECT_SOAQUATERNION_EQ(normalize, .033389f, 0.f, .1091089f, .1492555f,
                                     .267112f, 0.f, .3273268f, .348263f,
                                     .53422445f, 0.f, .545544f, .547270f,
                                     .80133667f, 1.f, .763762f, .74627789f);

  const SoaQuaternion normalize_est = NormalizeEst(denorm);
  EXPECT_SOAQUATERNION_EQ_EST(normalize_est, .033389f, 0.f, .1091089f, .1492555f,
                                             .267112f, 0.f, .3273268f, .348263f,
                                             .53422445f, 0.f, .545544f, .547270f,
                                             .80133667f, 1.f, .763762f, .74627789f);
  EXPECT_TRUE(ozz::math::AreAllTrue(IsNormalizedEst(normalize_est)));

  const SoaQuaternion lerp_0 = Lerp(a, b, ozz::math::simd_float4::zero());
  EXPECT_SOAQUATERNION_EQ(lerp_0, .70710677f, 0.f, 0.f, .382683432f,
                                  0.f, 0.f, .70710677f, 0.f,
                                  0.f, 0.f, 0.f, 0.f,
                                  .70710677f, 1.f, .70710677f, .9238795f);

  const SoaQuaternion lerp_1 = Lerp(a, b, ozz::math::simd_float4::one());
  EXPECT_SOAQUATERNION_EQ(lerp_1, 0.f, .70710677f, 0.f, -.382683432f,
                                  0.f, 0.f, .70710677f, 0.f,
                                  0.f, 0.f, 0.f, 0.f,
                                  1.f, .70710677f, .70710677f, .9238795f);


  const SoaQuaternion lerp_0_2 = Lerp(a, b, ozz::math::simd_float4::Load1(.2f));
  EXPECT_SOAQUATERNION_EQ(lerp_0_2, .565685416f, .14142136f, 0.f, .22961006f,
                                    0.f, 0.f, .70710677f, 0.f,
                                    0.f, 0.f, 0.f, 0.f,
                                    .76568544f, .94142133f, .70710677f, .92387950f);
 
  const SoaQuaternion lerp_m = Lerp(a, b, ozz::math::simd_float4::Load(0.f, 1.f, 1.f, .2f));
  EXPECT_SOAQUATERNION_EQ(lerp_m, .70710677f, .70710677f, 0.f, .22961006f,
                                  0.f, 0.f, .70710677f, 0.f,
                                  0.f, 0.f, 0.f, 0.f,
                                  .70710677f, .70710677f, .70710677f, .92387950f);

  const SoaQuaternion nlerp_0 = NLerp(a, b, ozz::math::simd_float4::zero());
  EXPECT_SOAQUATERNION_EQ(nlerp_0, .70710677f, 0.f, 0.f, .382683432f,
                                   0.f, 0.f, .70710677f, 0.f,
                                   0.f, 0.f, 0.f, 0.f,
                                   .70710677f, 1.f, .70710677f, .9238795f);
  EXPECT_TRUE(ozz::math::AreAllTrue(IsNormalized(nlerp_0)));

  const SoaQuaternion nlerp_1 = NLerp(a, b, ozz::math::simd_float4::one());
  EXPECT_SOAQUATERNION_EQ(nlerp_1, 0.f, .70710677f, 0.f, -.382683432f,
                                   0.f, 0.f, .70710677f, 0.f,
                                   0.f, 0.f, 0.f, 0.f,
                                   1.f, .70710677f, .70710677f, .9238795f);
  EXPECT_TRUE(ozz::math::AreAllTrue(IsNormalized(nlerp_1)));

  const SoaQuaternion nlerp_0_2 = NLerp(a, b, ozz::math::simd_float4::Load1(.2f));
  EXPECT_TRUE(ozz::math::AreAllTrue(IsNormalized(nlerp_0_2)));
  EXPECT_SOAQUATERNION_EQ(nlerp_0_2, .59421712f, .14855431f, 0.f, .24119100f,
                                     0.f, 0.f, .70710683f, 0.f,
                                     0.f, 0.f, 0.f, 0.f,
                                     .80430466f, .98890430f, .70710683f, .97047764f);
  EXPECT_TRUE(ozz::math::AreAllTrue(IsNormalized(nlerp_0_2)));

  const SoaQuaternion nlerp_m = NLerp(a, b, ozz::math::simd_float4::Load(0.f, 1.f, 1.f, .2f));
  EXPECT_SOAQUATERNION_EQ(nlerp_m, .70710677f, .70710677f, 0.f, .24119100f,
                                   0.f, 0.f, .70710677f, 0.f,
                                   0.f, 0.f, 0.f, 0.f,
                                   .70710677f, .70710677f, .70710677f, .97047764f);
  EXPECT_TRUE(ozz::math::AreAllTrue(IsNormalized(nlerp_m)));

  const SoaQuaternion nlerp_est_m = NLerpEst(a, b, ozz::math::simd_float4::Load(0.f, 1.f, 1.f, .2f));
  EXPECT_SOAQUATERNION_EQ_EST(nlerp_est_m, .70710677f, .70710677f, 0.f, .24119100f,
                                           0.f, 0.f, .70710677f, 0.f,
                                           0.f, 0.f, 0.f, 0.f,
                                           .70710677f, .70710677f, .70710677f, .97047764f);
  EXPECT_TRUE(ozz::math::AreAllTrue(IsNormalizedEst(nlerp_est_m)));
}
