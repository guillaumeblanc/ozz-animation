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

// Implement helper macro that verify that handle was reached once ik job is
// executed.
#define EXPECT_REACHED(_job)    \
  \
do {                            \
    SCOPED_TRACE("");           \
    _ExpectReached(_job, true); \
  }                             \
  while (0)

#define EXPECT_NOT_REACHED(_job) \
  \
do {                             \
    SCOPED_TRACE("");            \
    _ExpectReached(_job, false); \
  }                              \
  while (0)

void _ExpectReached(const ozz::animation::TwoBoneIKJob& _job, bool _reachable) {
  // Computes local transforms
  const ozz::math::Float4x4 mid_local =
      Invert(*_job.start_joint) * *_job.mid_joint;
  const ozz::math::Float4x4 end_local =
      Invert(*_job.mid_joint) * *_job.end_joint;

  // Rebuild corrected model transforms
  const ozz::math::Float4x4 start_correction =
      ozz::math::Float4x4::FromQuaternion(_job.start_joint_correction->xyzw);
  const ozz::math::Float4x4 start_corrected =
      *_job.start_joint * start_correction;
  const ozz::math::Float4x4 mid_correction =
      ozz::math::Float4x4::FromQuaternion(_job.mid_joint_correction->xyzw);
  const ozz::math::Float4x4 mid_corrected =
      start_corrected * mid_local * mid_correction;
  const ozz::math::Float4x4 end_corrected = mid_corrected * end_local;

  const ozz::math::SimdFloat4 diff =
      ozz::math::Length3(end_corrected.cols[3] - _job.handle);
  EXPECT_EQ(ozz::math::GetX(diff) < 1e-2f, _reachable);
}

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
      ozz::math::Float4x4::FromEuler(ozz::math::simd_float4::Load(
          ozz::math::kPi / 3.f, 0.f, 0.f, 0.f)),  // Rotated
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

      EXPECT_REACHED(job);

      EXPECT_SIMDQUATERNION_EQ_TOL(qstart, 0.f, 0.f, 0.f, 1.f, 2e-3f);
      EXPECT_SIMDQUATERNION_EQ_TOL(qmid, 0.f, 0.f, 0.f, 1.f, 2e-3f);
    }

    {  // 90
      job.handle = TransformPoint(
          parent, ozz::math::simd_float4::Load(0.f, 1.f, 1.f, 0.f));
      ASSERT_TRUE(job.Run());

      EXPECT_REACHED(job);

      const ozz::math::Quaternion y_mPi_2 =
          ozz::math::Quaternion::FromAxisAngle(ozz::math::Float3::y_axis(),
                                               -ozz::math::kPi_2);
      EXPECT_SIMDQUATERNION_EQ_TOL(qstart, y_mPi_2.x, y_mPi_2.y, y_mPi_2.z,
                                   y_mPi_2.w, 2e-3f);
      EXPECT_SIMDQUATERNION_EQ_TOL(qmid, 0.f, 0.f, 0.f, 1.f, 2e-3f);
    }

    {  // 180 behind
      job.handle = TransformPoint(
          parent, ozz::math::simd_float4::Load(-1.f, 1.f, 0.f, 0.f));
      ASSERT_TRUE(job.Run());

      EXPECT_REACHED(job);

      const ozz::math::Quaternion y_kPi = ozz::math::Quaternion::FromAxisAngle(
          ozz::math::Float3::y_axis(), ozz::math::kPi);
      EXPECT_SIMDQUATERNION_EQ_TOL(qstart, y_kPi.x, y_kPi.y, y_kPi.z, y_kPi.w,
                                   2e-3f);
      EXPECT_SIMDQUATERNION_EQ_TOL(qmid, 0.f, 0.f, 0.f, 1.f, 2e-3f);
    }

    {  // 270
      job.handle = TransformPoint(
          parent, ozz::math::simd_float4::Load(0.f, 1.f, -1.f, 0.f));
      ASSERT_TRUE(job.Run());

      EXPECT_REACHED(job);

      const ozz::math::Quaternion y_kPi_2 =
          ozz::math::Quaternion::FromAxisAngle(ozz::math::Float3::y_axis(),
                                               ozz::math::kPi_2);
      EXPECT_SIMDQUATERNION_EQ_TOL(qstart, y_kPi_2.x, y_kPi_2.y, y_kPi_2.z,
                                   y_kPi_2.w, 2e-3f);
      EXPECT_SIMDQUATERNION_EQ_TOL(qmid, 0.f, 0.f, 0.f, 1.f, 2e-3f);
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

    EXPECT_REACHED(job);

    EXPECT_SIMDQUATERNION_EQ_TOL(qstart, 0.f, 0.f, 0.f, 1.f, 2e-3f);
    EXPECT_SIMDQUATERNION_EQ_TOL(qmid, 0.f, 0.f, 0.f, 1.f, 2e-3f);
  }

  // Pole Z
  {
    job.pole_vector = ozz::math::simd_float4::z_axis();
    job.handle = ozz::math::simd_float4::Load(1.f, 0.f, 1.f, 0.f);
    ASSERT_TRUE(job.Run());

    EXPECT_REACHED(job);

    const ozz::math::Quaternion x_kPi_2 = ozz::math::Quaternion::FromAxisAngle(
        ozz::math::Float3::x_axis(), ozz::math::kPi_2);
    EXPECT_SIMDQUATERNION_EQ_TOL(qstart, x_kPi_2.x, x_kPi_2.y, x_kPi_2.z,
                                 x_kPi_2.w, 2e-3f);
    EXPECT_SIMDQUATERNION_EQ_TOL(qmid, 0.f, 0.f, 0.f, 1.f, 2e-3f);
  }

  // Pole -Z
  {
    job.pole_vector = -ozz::math::simd_float4::z_axis();
    job.handle = ozz::math::simd_float4::Load(1.f, 0.f, -1.f, 0.f);
    ASSERT_TRUE(job.Run());

    EXPECT_REACHED(job);

    const ozz::math::Quaternion x_mPi_2 = ozz::math::Quaternion::FromAxisAngle(
        ozz::math::Float3::x_axis(), -ozz::math::kPi_2);
    EXPECT_SIMDQUATERNION_EQ_TOL(qstart, x_mPi_2.x, x_mPi_2.y, x_mPi_2.z,
                                 x_mPi_2.w, 2e-3f);
    EXPECT_SIMDQUATERNION_EQ_TOL(qmid, 0.f, 0.f, 0.f, 1.f, 2e-3f);
  }

  // Pole X
  {
    job.pole_vector = ozz::math::simd_float4::x_axis();
    job.handle = ozz::math::simd_float4::Load(1.f, -1.f, 0.f, 0.f);
    ASSERT_TRUE(job.Run());

    EXPECT_REACHED(job);

    const ozz::math::Quaternion z_mPi_2 = ozz::math::Quaternion::FromAxisAngle(
        ozz::math::Float3::z_axis(), -ozz::math::kPi_2);
    EXPECT_SIMDQUATERNION_EQ_TOL(qstart, z_mPi_2.x, z_mPi_2.y, z_mPi_2.z,
                                 z_mPi_2.w, 2e-3f);
    EXPECT_SIMDQUATERNION_EQ_TOL(qmid, 0.f, 0.f, 0.f, 1.f, 2e-3f);
  }

  // Pole -X
  {
    job.pole_vector = -ozz::math::simd_float4::x_axis();
    job.handle = ozz::math::simd_float4::Load(-1.f, 1.f, 0.f, 0.f);
    ASSERT_TRUE(job.Run());

    EXPECT_REACHED(job);

    const ozz::math::Quaternion z_Pi_2 = ozz::math::Quaternion::FromAxisAngle(
        ozz::math::Float3::z_axis(), ozz::math::kPi_2);
    EXPECT_SIMDQUATERNION_EQ_TOL(qstart, z_Pi_2.x, z_Pi_2.y, z_Pi_2.z, z_Pi_2.w,
                                 2e-3f);
    EXPECT_SIMDQUATERNION_EQ_TOL(qmid, 0.f, 0.f, 0.f, 1.f, 2e-3f);
  }
}

TEST(Soften, TwoBoneIKJob) {
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

  // Reachable
  {
    job.pole_vector = ozz::math::simd_float4::y_axis();
    job.handle = ozz::math::simd_float4::Load(2.f, 0.f, 0.f, 0.f);
    job.soften = 1.f;
    ASSERT_TRUE(job.Run());

    EXPECT_REACHED(job);

    const ozz::math::Quaternion z_mPi_2 = ozz::math::Quaternion::FromAxisAngle(
        ozz::math::Float3::z_axis(), -ozz::math::kPi_2);
    EXPECT_SIMDQUATERNION_EQ_TOL(qstart, z_mPi_2.x, z_mPi_2.y, z_mPi_2.z,
                                 z_mPi_2.w, 2e-3f);
    const ozz::math::Quaternion z_Pi_2 = ozz::math::Quaternion::FromAxisAngle(
        ozz::math::Float3::z_axis(), ozz::math::kPi_2);
    EXPECT_SIMDQUATERNION_EQ_TOL(qmid, z_Pi_2.x, z_Pi_2.y, z_Pi_2.z, z_Pi_2.w,
                                 2e-3f);
  }

  // Reachable, softened
  {
    job.pole_vector = ozz::math::simd_float4::y_axis();
    job.handle = ozz::math::simd_float4::Load(2.f * .5f, 0.f, 0.f, 0.f);
    job.soften = .5f;
    ASSERT_TRUE(job.Run());

    EXPECT_REACHED(job);
  }

  // Reachable, softened
  {
    job.pole_vector = ozz::math::simd_float4::y_axis();
    job.handle = ozz::math::simd_float4::Load(2.f * .4f, 0.f, 0.f, 0.f);
    job.soften = .5f;
    ASSERT_TRUE(job.Run());

    EXPECT_REACHED(job);
  }

  // Not reachable, softened
  {
    job.pole_vector = ozz::math::simd_float4::y_axis();
    job.handle = ozz::math::simd_float4::Load(2.f * .6f, 0.f, 0.f, 0.f);
    job.soften = .5f;
    ASSERT_TRUE(job.Run());

    EXPECT_NOT_REACHED(job);
  }

  // Not reachable, softened at max
  {
    job.pole_vector = ozz::math::simd_float4::y_axis();
    job.handle = ozz::math::simd_float4::Load(2.f * .6f, 0.f, 0.f, 0.f);
    job.soften = 0.f;
    ASSERT_TRUE(job.Run());

    EXPECT_NOT_REACHED(job);
  }

  // Not reachable, softened
  {
    job.pole_vector = ozz::math::simd_float4::y_axis();
    job.handle = ozz::math::simd_float4::Load(2.f, 0.f, 0.f, 0.f);
    job.soften = .5f;
    ASSERT_TRUE(job.Run());

    EXPECT_NOT_REACHED(job);
  }

  // Not reachable, a bit too far
  {
    job.pole_vector = ozz::math::simd_float4::y_axis();
    job.handle = ozz::math::simd_float4::Load(3.f, 0.f, 0.f, 0.f);
    job.soften = 1.f;
    ASSERT_TRUE(job.Run());

    EXPECT_NOT_REACHED(job);
  }
}

TEST(Twist, TwoBoneIKJob) {
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

  // Twist angle 0
  {
    job.twist_angle = 0.f;
    ASSERT_TRUE(job.Run());

    EXPECT_REACHED(job);

    EXPECT_SIMDQUATERNION_EQ_TOL(qstart, 0.f, 0.f, 0.f, 1.f, 2e-3f);
    EXPECT_SIMDQUATERNION_EQ_TOL(qmid, 0.f, 0.f, 0.f, 1.f, 2e-3f);
  }

  // Twist angle pi / 2
  {
    job.twist_angle = ozz::math::kPi_2;
    ASSERT_TRUE(job.Run());

    EXPECT_REACHED(job);

    const ozz::math::Quaternion h_Pi_2 = ozz::math::Quaternion::FromAxisAngle(
        ozz::math::Float3(.70710678f, .70710678f, 0.f), ozz::math::kPi_2);
    EXPECT_SIMDQUATERNION_EQ_TOL(qstart, h_Pi_2.x, h_Pi_2.y, h_Pi_2.z, h_Pi_2.w,
                                 2e-3f);
    EXPECT_SIMDQUATERNION_EQ_TOL(qmid, 0.f, 0.f, 0.f, 1.f, 2e-3f);
  }

  // Twist angle pi
  {
    job.twist_angle = ozz::math::kPi;
    ASSERT_TRUE(job.Run());

    EXPECT_REACHED(job);

    const ozz::math::Quaternion h_Pi = ozz::math::Quaternion::FromAxisAngle(
        ozz::math::Float3(.70710678f, .70710678f, 0.f), ozz::math::kPi);
    EXPECT_SIMDQUATERNION_EQ_TOL(qstart, h_Pi.x, h_Pi.y, h_Pi.z, h_Pi.w, 2e-3f);
    EXPECT_SIMDQUATERNION_EQ_TOL(qmid, 0.f, 0.f, 0.f, 1.f, 2e-3f);
  }

  // Twist angle 2pi
  {
    job.twist_angle = ozz::math::kPi * 2.f;
    ASSERT_TRUE(job.Run());

    EXPECT_REACHED(job);

    EXPECT_SIMDQUATERNION_EQ_TOL(qstart, 0.f, 0.f, 0.f, -1.f, 2e-3f);
    EXPECT_SIMDQUATERNION_EQ_TOL(qmid, 0.f, 0.f, 0.f, 1.f, 2e-3f);
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

    EXPECT_REACHED(job);

    // qstart is undefined, many solutions in this case
    EXPECT_SIMDQUATERNION_EQ_TOL(qmid, 0.f, 0.f, 0.f, 1.f, 2e-3f);
  }

  {  // Reachable, defined qstart
    job.pole_vector = ozz::math::simd_float4::y_axis();
    job.handle =
        ozz::math::simd_float4::Load(.001f, ozz::math::kSqrt2, 0.f, 0.f);
    ASSERT_TRUE(job.Run());

    EXPECT_REACHED(job);

    const ozz::math::Quaternion z_Pi_4 = ozz::math::Quaternion::FromAxisAngle(
        ozz::math::Float3::z_axis(), ozz::math::kPi_4);
    EXPECT_SIMDQUATERNION_EQ_TOL(qstart, z_Pi_4.x, z_Pi_4.y, z_Pi_4.z, z_Pi_4.w,
                                 2e-3f);
    EXPECT_SIMDQUATERNION_EQ_TOL(qmid, 0.f, 0.f, 0.f, 1.f, 2e-3f);
  }

  {  // Full extent, undefined qstart, end not reached
    job.pole_vector = ozz::math::simd_float4::y_axis();
    job.handle = ozz::math::simd_float4::Load(0.f, 3.f, 0.f, 0.f);
    ASSERT_TRUE(job.Run());

    // qstart is undefined, many solutions in this case
    const ozz::math::Quaternion z_Pi_2 = ozz::math::Quaternion::FromAxisAngle(
        ozz::math::Float3::z_axis(), ozz::math::kPi_2);
    EXPECT_SIMDQUATERNION_EQ_TOL(qmid, z_Pi_2.x, z_Pi_2.y, z_Pi_2.z, z_Pi_2.w,
                                 2e-3f);
  }
}

TEST(MidAxisFallback, TwoBoneIKJob) {
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

    EXPECT_REACHED(job);

    EXPECT_SIMDQUATERNION_EQ_TOL(qstart, 0.f, 0.f, 0.f, 1.f, 2e-3f);
    EXPECT_SIMDQUATERNION_EQ_TOL(qmid, 0.f, 0.f, 0.f, 1.f, 2e-3f);
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

    EXPECT_REACHED(job);

    // Restore end joint matrix
    job.end_joint = &end;

    // Start rotates 180 on y, to allow Mid to turn positively on z axis.
    EXPECT_SIMDQUATERNION_EQ_TOL(qstart, 0.f, 1.f, 0.f, 0.f, 2e-3f);
    const ozz::math::Quaternion z_Pi_2 = ozz::math::Quaternion::FromAxisAngle(
        ozz::math::Float3::z_axis(), ozz::math::kPi_2);
    EXPECT_SIMDQUATERNION_EQ_TOL(qmid, z_Pi_2.x, z_Pi_2.y, z_Pi_2.z, z_Pi_2.w,
                                 2e-3f);
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

    EXPECT_REACHED(job);

    // Restore end joint matrix
    job.end_joint = &end;

    EXPECT_SIMDQUATERNION_EQ_TOL(qstart, 0.f, 0.f, 0.f, 1.f, 2e-3f);
    const ozz::math::Quaternion z_mPi_2 = ozz::math::Quaternion::FromAxisAngle(
        ozz::math::Float3::z_axis(), -ozz::math::kPi_2);
    EXPECT_SIMDQUATERNION_EQ_TOL(qmid, z_mPi_2.x, z_mPi_2.y, z_mPi_2.z,
                                 z_mPi_2.w, 2e-3f);
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

  EXPECT_SIMDQUATERNION_EQ_TOL(qstart, 0.f, 0.f, 0.f, 1.f, 2e-3f);
  // Mid joint is bent -90 degrees to reach start.
  const ozz::math::Quaternion z_mPi_2 = ozz::math::Quaternion::FromAxisAngle(
      ozz::math::Float3::z_axis(), -ozz::math::kPi_2);
  EXPECT_SIMDQUATERNION_EQ_TOL(qmid, z_mPi_2.x, z_mPi_2.y, z_mPi_2.z, z_mPi_2.w,
                               2e-3f);
}

TEST(ZeroLengthBoneChain, TwoBoneIKJob) {
  // Setup initial pose
  const ozz::math::Float4x4 start = ozz::math::Float4x4::identity();
  const ozz::math::Float4x4 mid = ozz::math::Float4x4::identity();
  const ozz::math::Float4x4 end = ozz::math::Float4x4::identity();

  // Prepares job.
  ozz::animation::TwoBoneIKJob job;
  job.pole_vector = ozz::math::simd_float4::y_axis();
  job.handle = ozz::math::simd_float4::x_axis();
  job.start_joint = &start;
  job.mid_joint = &mid;
  job.end_joint = &end;
  ozz::math::SimdQuaternion qstart;
  job.start_joint_correction = &qstart;
  ozz::math::SimdQuaternion qmid;
  job.mid_joint_correction = &qmid;
  ASSERT_TRUE(job.Validate());

  // Just expecting it's not crashing
  ASSERT_TRUE(job.Run());

  EXPECT_NOT_REACHED(job);
}
