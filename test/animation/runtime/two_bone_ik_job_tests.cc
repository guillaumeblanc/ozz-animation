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

#include "ozz/animation/runtime/two_bone_ik_job.h"

#include "ozz/base/maths/quaternion.h"
#include "ozz/base/maths/simd_math.h"
#include "ozz/base/maths/simd_quaternion.h"

#include "gtest/gtest.h"
#include "ozz/base/maths/gtest_math_helper.h"

TEST(JobValidity, TwoBoneIKJob) {
  // Setup initial pose
  const ozz::math::Float4x4 start = ozz::math::Float4x4::identity();
  const ozz::math::Float4x4 mid = ozz::math::Float4x4::FromAffine(
      ozz::math::simd_float4::y_axis(),
      ozz::math::SimdQuaternion::FromAxisAngle(
          ozz::math::simd_float4::z_axis(),
          ozz::math::simd_float4::Load1(ozz::math::kPi_2))
          .xyzw,
      ozz::math::simd_float4::one());
  const ozz::math::Float4x4 end = ozz::math::Float4x4::Translation(
      ozz::math::simd_float4::x_axis() + ozz::math::simd_float4::y_axis());

  {  // Default is invalid
    ozz::animation::TwoBoneIKJob job;
    EXPECT_FALSE(job.Validate());
  }

  {  // Missing start joint matrix
    ozz::animation::TwoBoneIKJob job;
    job.mid_joint = &mid;
    job.end_joint = &end;
    ozz::math::SimdQuaternion quat;
    job.start_joint_correction = &quat;
    job.mid_joint_correction = &quat;
    EXPECT_FALSE(job.Validate());
  }

  {  // Missing mid joint matrix
    ozz::animation::TwoBoneIKJob job;
    job.start_joint = &start;
    job.end_joint = &end;
    ozz::math::SimdQuaternion quat;
    job.start_joint_correction = &quat;
    job.mid_joint_correction = &quat;
    EXPECT_FALSE(job.Validate());
  }

  {  // Missing end joint matrix
    ozz::animation::TwoBoneIKJob job;
    job.start_joint = &start;
    job.mid_joint = &mid;
    ozz::math::SimdQuaternion quat;
    job.start_joint_correction = &quat;
    job.mid_joint_correction = &quat;
    EXPECT_FALSE(job.Validate());
  }

  {  // Missing start joint output quaternion
    ozz::animation::TwoBoneIKJob job;
    job.start_joint = &start;
    job.mid_joint = &mid;
    job.end_joint = &end;
    ozz::math::SimdQuaternion quat;
    job.mid_joint_correction = &quat;
    EXPECT_FALSE(job.Validate());
  }

  {  // Missing mid joint output quaternion
    ozz::animation::TwoBoneIKJob job;
    job.start_joint = &start;
    job.mid_joint = &mid;
    job.end_joint = &end;
    ozz::math::SimdQuaternion quat;
    job.start_joint_correction = &quat;
    EXPECT_FALSE(job.Validate());
  }

  {  // Unnormalized mid axis
    ozz::animation::TwoBoneIKJob job;
    job.mid_axis_fallback =
        ozz::math::simd_float4::Load(0.f, .70710678f, 0.f, .70710678f);
    job.start_joint = &start;
    job.mid_joint = &mid;
    job.end_joint = &end;
    ozz::math::SimdQuaternion quat;
    job.start_joint_correction = &quat;
    job.mid_joint_correction = &quat;
    EXPECT_FALSE(job.Validate());
  }

  {  // Valid
    ozz::animation::TwoBoneIKJob job;
    job.start_joint = &start;
    job.mid_joint = &mid;
    job.end_joint = &end;
    ozz::math::SimdQuaternion quat;
    job.start_joint_correction = &quat;
    job.mid_joint_correction = &quat;
    EXPECT_TRUE(job.Validate());
  }
}

TEST(StartJointCorrection, TwoBoneIKJob) {
  // Setup initial pose
  const ozz::math::Float4x4 base_start = ozz::math::Float4x4::identity();
  const ozz::math::Float4x4 base_mid = ozz::math::Float4x4::FromAffine(
      ozz::math::simd_float4::y_axis(),
      ozz::math::SimdQuaternion::FromAxisAngle(
          ozz::math::simd_float4::z_axis(),
          ozz::math::simd_float4::Load1(ozz::math::kPi_2))
          .xyzw,
      ozz::math::simd_float4::one());
  const ozz::math::Float4x4 base_end = ozz::math::Float4x4::Translation(
      ozz::math::simd_float4::x_axis() + ozz::math::simd_float4::y_axis());

  // Test will be executed with different root transformations
  const ozz::math::Float4x4 parents[] = {
      ozz::math::Float4x4::identity(),  // No root transformation
      ozz::math::Float4x4::Translation(ozz::math::simd_float4::y_axis()),  // Up
      ozz::math::Float4x4::FromEuler(
          ozz::math::simd_float4::Load(.78f, 0.f, 0.f, 0.f)),  // Rotated
      ozz::math::Float4x4::Scaling(ozz::math::simd_float4::Load(
          2.f, 2.f, 2.f, 0.f)),  // Uniformly scaled
      ozz::math::Float4x4::Scaling(ozz::math::simd_float4::Load(
          1.f, 2.f, 1.f, 0.f)),  // Non-uniformly scaled
      ozz::math::Float4x4::Scaling(
          ozz::math::simd_float4::Load(-3.f, -3.f, -3.f, 0.f))  // Mirrored
  };

  for (size_t i = 0; i < OZZ_ARRAY_SIZE(parents); ++i) {
    const ozz::math::Float4x4& parent = parents[i];
    const ozz::math::Float4x4 start = parent * base_start;
    const ozz::math::Float4x4 mid = parent * base_mid;
    const ozz::math::Float4x4 end = parent * base_end;

    // Prepares job.
    ozz::animation::TwoBoneIKJob job;
    job.pole_vector = TransformVector(parent, ozz::math::simd_float4::y_axis());
    job.start_joint = &start;
    job.mid_joint = &mid;
    job.end_joint = &end;
    ozz::math::SimdQuaternion qstart;
    job.start_joint_correction = &qstart;
    ozz::math::SimdQuaternion qmid;
    job.mid_joint_correction = &qmid;
    ASSERT_TRUE(job.Validate());

    {  // No correction expected
      job.handle = TransformPoint(
          parent, ozz::math::simd_float4::Load(1.f, 1.f, 0.f, 0.f));
      ASSERT_TRUE(job.Run());

      EXPECT_SIMDQUATERNION_EQ_EST(qstart, 0.f, 0.f, 0.f, 1.f);
      EXPECT_SIMDQUATERNION_EQ_EST(qmid, 0.f, 0.f, 0.f, 1.f);
    }

    {  // 90
      job.handle = TransformPoint(
          parent, ozz::math::simd_float4::Load(0.f, 1.f, 1.f, 0.f));
      ASSERT_TRUE(job.Run());

      const ozz::math::Quaternion y_mPi_2 =
          ozz::math::Quaternion::FromAxisAngle(ozz::math::Float3::y_axis(),
                                               -ozz::math::kPi_2);
      EXPECT_SIMDQUATERNION_EQ_EST(qstart, y_mPi_2.x, y_mPi_2.y, y_mPi_2.z,
                                   y_mPi_2.w);
      EXPECT_SIMDQUATERNION_EQ_EST(qmid, 0.f, 0.f, 0.f, 1.f);
    }

    {  // 180 behind
      job.handle = TransformPoint(
          parent, ozz::math::simd_float4::Load(-1.f, 1.f, 0.f, 0.f));
      ASSERT_TRUE(job.Run());

      const ozz::math::Quaternion y_kPi = ozz::math::Quaternion::FromAxisAngle(
          ozz::math::Float3::y_axis(), ozz::math::kPi);
      EXPECT_SIMDQUATERNION_EQ_EST(qstart, y_kPi.x, y_kPi.y, y_kPi.z, y_kPi.w);
      EXPECT_SIMDQUATERNION_EQ_EST(qmid, 0.f, 0.f, 0.f, 1.f);
    }

    {  // 270
      job.handle = TransformPoint(
          parent, ozz::math::simd_float4::Load(0.f, 1.f, -1.f, 0.f));
      ASSERT_TRUE(job.Run());

      const ozz::math::Quaternion y_kPi_2 =
          ozz::math::Quaternion::FromAxisAngle(ozz::math::Float3::y_axis(),
                                               ozz::math::kPi_2);
      EXPECT_SIMDQUATERNION_EQ_EST(qstart, y_kPi_2.x, y_kPi_2.y, y_kPi_2.z,
                                   y_kPi_2.w);
      EXPECT_SIMDQUATERNION_EQ_EST(qmid, 0.f, 0.f, 0.f, 1.f);
    }
  }
}

TEST(Pole, TwoBoneIKJob) {
  // Setup initial pose
  const ozz::math::Float4x4 start = ozz::math::Float4x4::identity();
  const ozz::math::Float4x4 mid = ozz::math::Float4x4::FromAffine(
      ozz::math::simd_float4::y_axis(),
      ozz::math::SimdQuaternion::FromAxisAngle(
          ozz::math::simd_float4::z_axis(),
          ozz::math::simd_float4::Load1(ozz::math::kPi_2))
          .xyzw,
      ozz::math::simd_float4::one());
  const ozz::math::Float4x4 end = ozz::math::Float4x4::Translation(
      ozz::math::simd_float4::x_axis() + ozz::math::simd_float4::y_axis());

  // Prepares job.
  ozz::animation::TwoBoneIKJob job;
  job.start_joint = &start;
  job.mid_joint = &mid;
  job.end_joint = &end;
  ozz::math::SimdQuaternion qstart;
  job.start_joint_correction = &qstart;
  ozz::math::SimdQuaternion qmid;
  job.mid_joint_correction = &qmid;
  ASSERT_TRUE(job.Validate());

  // Pole Y
  {
    job.pole_vector = ozz::math::simd_float4::y_axis();
    job.handle = ozz::math::simd_float4::Load(1.f, 1.f, 0.f, 0.f);
    ASSERT_TRUE(job.Run());

    EXPECT_SIMDQUATERNION_EQ_EST(qstart, 0.f, 0.f, 0.f, 1.f);
    EXPECT_SIMDQUATERNION_EQ_EST(qmid, 0.f, 0.f, 0.f, 1.f);
  }

  // Pole Z
  {
    job.pole_vector = ozz::math::simd_float4::z_axis();
    job.handle = ozz::math::simd_float4::Load(1.f, 0.f, 1.f, 0.f);
    ASSERT_TRUE(job.Run());

    const ozz::math::Quaternion x_kPi_2 = ozz::math::Quaternion::FromAxisAngle(
        ozz::math::Float3::x_axis(), ozz::math::kPi_2);
    EXPECT_SIMDQUATERNION_EQ_EST(qstart, x_kPi_2.x, x_kPi_2.y, x_kPi_2.z,
                                 x_kPi_2.w);
    EXPECT_SIMDQUATERNION_EQ_EST(qmid, 0.f, 0.f, 0.f, 1.f);
  }

  // Pole -Z
  {
    job.pole_vector = -ozz::math::simd_float4::z_axis();
    job.handle = ozz::math::simd_float4::Load(1.f, 0.f, -1.f, 0.f);
    ASSERT_TRUE(job.Run());

    const ozz::math::Quaternion x_mPi_2 = ozz::math::Quaternion::FromAxisAngle(
        ozz::math::Float3::x_axis(), -ozz::math::kPi_2);
    EXPECT_SIMDQUATERNION_EQ_EST(qstart, x_mPi_2.x, x_mPi_2.y, x_mPi_2.z,
                                 x_mPi_2.w);
    EXPECT_SIMDQUATERNION_EQ_EST(qmid, 0.f, 0.f, 0.f, 1.f);
  }

  // Pole X
  {
    job.pole_vector = ozz::math::simd_float4::x_axis();
    job.handle = ozz::math::simd_float4::Load(1.f, -1.f, 0.f, 0.f);
    ASSERT_TRUE(job.Run());

    const ozz::math::Quaternion z_mPi_2 = ozz::math::Quaternion::FromAxisAngle(
        ozz::math::Float3::z_axis(), -ozz::math::kPi_2);
    EXPECT_SIMDQUATERNION_EQ_EST(qstart, z_mPi_2.x, z_mPi_2.y, z_mPi_2.z,
                                 z_mPi_2.w);
    EXPECT_SIMDQUATERNION_EQ_EST(qmid, 0.f, 0.f, 0.f, 1.f);
  }

  // Pole -X
  {
    job.pole_vector = -ozz::math::simd_float4::x_axis();
    job.handle = ozz::math::simd_float4::Load(-1.f, 1.f, 0.f, 0.f);
    ASSERT_TRUE(job.Run());

    const ozz::math::Quaternion z_Pi_2 = ozz::math::Quaternion::FromAxisAngle(
        ozz::math::Float3::z_axis(), ozz::math::kPi_2);
    EXPECT_SIMDQUATERNION_EQ_EST(qstart, z_Pi_2.x, z_Pi_2.y, z_Pi_2.z,
                                 z_Pi_2.w);
    EXPECT_SIMDQUATERNION_EQ_EST(qmid, 0.f, 0.f, 0.f, 1.f);
  }
}

TEST(PoleHandleAlignment, TwoBoneIKJob) {
  // Setup initial pose
  const ozz::math::Float4x4 start = ozz::math::Float4x4::identity();
  const ozz::math::Float4x4 mid = ozz::math::Float4x4::FromAffine(
      ozz::math::simd_float4::y_axis(),
      ozz::math::SimdQuaternion::FromAxisAngle(
          ozz::math::simd_float4::z_axis(),
          ozz::math::simd_float4::Load1(ozz::math::kPi_2))
          .xyzw,
      ozz::math::simd_float4::one());
  const ozz::math::Float4x4 end = ozz::math::Float4x4::Translation(
      ozz::math::simd_float4::x_axis() + ozz::math::simd_float4::y_axis());

  // Prepares job.
  ozz::animation::TwoBoneIKJob job;
  job.start_joint = &start;
  job.mid_joint = &mid;
  job.end_joint = &end;
  ozz::math::SimdQuaternion qstart;
  job.start_joint_correction = &qstart;
  ozz::math::SimdQuaternion qmid;
  job.mid_joint_correction = &qmid;
  ASSERT_TRUE(job.Validate());

  {  // Reachable, undefined qstart
    job.pole_vector = ozz::math::simd_float4::y_axis();
    job.handle = ozz::math::simd_float4::Load(0.f, ozz::math::kSqrt2, 0.f, 0.f);
    ASSERT_TRUE(job.Run());

    // qstart is undefined, many solutions in this case
    EXPECT_SIMDQUATERNION_EQ_EST(qmid, 0.f, 0.f, 0.f, 1.f);
  }

  {  // Reachable, defined qstart
    job.pole_vector = ozz::math::simd_float4::y_axis();
    job.handle =
        ozz::math::simd_float4::Load(.001f, ozz::math::kSqrt2, 0.f, 0.f);
    ASSERT_TRUE(job.Run());

    const ozz::math::Quaternion z_Pi_4 = ozz::math::Quaternion::FromAxisAngle(
        ozz::math::Float3::z_axis(), ozz::math::kPi_4);
    EXPECT_SIMDQUATERNION_EQ_EST(qstart, z_Pi_4.x, z_Pi_4.y, z_Pi_4.z,
                                 z_Pi_4.w);
    EXPECT_SIMDQUATERNION_EQ_EST(qmid, 0.f, 0.f, 0.f, 1.f);
  }

  {  // Full extent, undefined qstart
    job.pole_vector = ozz::math::simd_float4::y_axis();
    job.handle = ozz::math::simd_float4::Load(0.f, 3.f, 0.f, 0.f);
    ASSERT_TRUE(job.Run());

    // qstart is undefined, many solutions in this case
    const ozz::math::Quaternion z_Pi_2 = ozz::math::Quaternion::FromAxisAngle(
        ozz::math::Float3::z_axis(), ozz::math::kPi_2);
    EXPECT_SIMDQUATERNION_EQ_EST(qmid, z_Pi_2.x, z_Pi_2.y, z_Pi_2.z, z_Pi_2.w);
  }
}

TEST(MidAxis, TwoBoneIKJob) {
  // Setup initial pose
  const ozz::math::Float4x4 start = ozz::math::Float4x4::identity();
  const ozz::math::Float4x4 mid = ozz::math::Float4x4::FromAffine(
      ozz::math::simd_float4::y_axis(),
      ozz::math::SimdQuaternion::FromAxisAngle(
          ozz::math::simd_float4::z_axis(),
          ozz::math::simd_float4::Load1(ozz::math::kPi_2))
          .xyzw,
      ozz::math::simd_float4::one());
  const ozz::math::Float4x4 end = ozz::math::Float4x4::Translation(
      ozz::math::simd_float4::x_axis() + ozz::math::simd_float4::y_axis());

  // Prepares job.
  ozz::animation::TwoBoneIKJob job;
  job.pole_vector = ozz::math::simd_float4::y_axis();
  job.handle = ozz::math::simd_float4::Load(1.f, 1.f, 0.f, 0.f);
  job.start_joint = &start;
  job.mid_joint = &mid;
  job.end_joint = &end;
  ozz::math::SimdQuaternion qstart;
  job.start_joint_correction = &qstart;
  ozz::math::SimdQuaternion qmid;
  job.mid_joint_correction = &qmid;
  ASSERT_TRUE(job.Validate());

  // Computed mid joint axis
  {
    ASSERT_TRUE(job.Run());

    EXPECT_SIMDQUATERNION_EQ_EST(qstart, 0.f, 0.f, 0.f, 1.f);
    EXPECT_SIMDQUATERNION_EQ_EST(qmid, 0.f, 0.f, 0.f, 1.f);
  }

  // Fall back mid joint axis
  {
    // Replaces "end" joint matrix to align the 3 joints.
    const ozz::math::Float4x4 aligned_end = ozz::math::Float4x4::Translation(
        ozz::math::simd_float4::Load(0.f, 2.f, 0.f, 0.f));
    job.end_joint = &aligned_end;

    // Fixes fall back axis
    job.mid_axis_fallback = ozz::math::simd_float4::z_axis();

    ASSERT_TRUE(job.Run());

    // Start rotates 180 on y, to allow Mid to turn positively on z axis.
    EXPECT_SIMDQUATERNION_EQ_EST(qstart, 0.f, 1.f, 0.f, 0.f);
    const ozz::math::Quaternion z_Pi_2 = ozz::math::Quaternion::FromAxisAngle(
        ozz::math::Float3::z_axis(), ozz::math::kPi_2);
    EXPECT_SIMDQUATERNION_EQ_EST(qmid, z_Pi_2.x, z_Pi_2.y, z_Pi_2.z, z_Pi_2.w);
  }

  // Fall back opposite mid joint axis
  {
    // Replaces "end" joint matrix to align the 3 joints.
    const ozz::math::Float4x4 aligned_end = ozz::math::Float4x4::Translation(
        ozz::math::simd_float4::Load(0.f, 2.f, 0.f, 0.f));
    job.end_joint = &aligned_end;

    // Fixes fall back axis
    job.mid_axis_fallback = -ozz::math::simd_float4::z_axis();

    ASSERT_TRUE(job.Run());

    EXPECT_SIMDQUATERNION_EQ_EST(qstart, 0.f, 0.f, 0.f, 1.f);
    const ozz::math::Quaternion z_mPi_2 = ozz::math::Quaternion::FromAxisAngle(
        ozz::math::Float3::z_axis(), -ozz::math::kPi_2);
    EXPECT_SIMDQUATERNION_EQ_EST(qmid, z_mPi_2.x, z_mPi_2.y, z_mPi_2.z,
                                 z_mPi_2.w);
  }
}

TEST(ZeroLengthStartHandle, TwoBoneIKJob) {
  // Setup initial pose
  const ozz::math::Float4x4 start = ozz::math::Float4x4::identity();
  const ozz::math::Float4x4 mid = ozz::math::Float4x4::FromAffine(
      ozz::math::simd_float4::y_axis(),
      ozz::math::SimdQuaternion::FromAxisAngle(
          ozz::math::simd_float4::z_axis(),
          ozz::math::simd_float4::Load1(ozz::math::kPi_2))
          .xyzw,
      ozz::math::simd_float4::one());
  const ozz::math::Float4x4 end = ozz::math::Float4x4::Translation(
      ozz::math::simd_float4::x_axis() + ozz::math::simd_float4::y_axis());

  // Prepares job.
  ozz::animation::TwoBoneIKJob job;
  job.pole_vector = ozz::math::simd_float4::y_axis();
  job.handle = start.cols[3];  // 0 length from start to handle
  job.start_joint = &start;
  job.mid_joint = &mid;
  job.end_joint = &end;
  ozz::math::SimdQuaternion qstart;
  job.start_joint_correction = &qstart;
  ozz::math::SimdQuaternion qmid;
  job.mid_joint_correction = &qmid;
  ASSERT_TRUE(job.Validate());

  ASSERT_TRUE(job.Run());

  EXPECT_SIMDQUATERNION_EQ_EST(qstart, 0.f, 0.f, 0.f, 1.f);
  // Mid joint is bent -90 degrees to reach start.
  const ozz::math::Quaternion z_mPi_2 = ozz::math::Quaternion::FromAxisAngle(
      ozz::math::Float3::z_axis(), -ozz::math::kPi_2);
  EXPECT_SIMDQUATERNION_EQ_EST(qmid, z_mPi_2.x, z_mPi_2.y, z_mPi_2.z,
                               z_mPi_2.w);
}
