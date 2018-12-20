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

#include "ozz/animation/runtime/ik_aim_job.h"

#include "ozz/base/maths/quaternion.h"
#include "ozz/base/maths/simd_math.h"
#include "ozz/base/maths/simd_quaternion.h"

#include "gtest/gtest.h"
#include "ozz/base/maths/gtest_math_helper.h"

// Implement helper macro that verify that target was reached once ik job is
// executed.
#define EXPECT_REACHED(_job)    \
                                \
  do {                          \
    SCOPED_TRACE("");           \
    _ExpectReached(_job, true); \
  } while (void(0), 0)

#define EXPECT_NOT_REACHED(_job) \
                                 \
  do {                           \
    SCOPED_TRACE("");            \
    _ExpectReached(_job, false); \
  } while (void(0), 0)

void _ExpectReached(const ozz::animation::IKAimJob& /*_job*/,
                    bool /*_reachable*/) {
  /*
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
      ozz::math::Length3(end_corrected.cols[3] - _job.target);
  EXPECT_EQ(ozz::math::GetX(diff) < 1e-2f, _reachable);
  */
}

TEST(JobValidity, IKAimJob) {
  const ozz::math::Float4x4 joint = ozz::math::Float4x4::identity();
  ozz::math::SimdQuaternion quat;

  {  // Default is invalid
    ozz::animation::IKAimJob job;
    EXPECT_FALSE(job.Validate());
  }

  {  // Invalid joint matrix
    ozz::animation::IKAimJob job;
    job.joint = &joint;
    EXPECT_FALSE(job.Validate());
  }

  {  // Invalid output
    ozz::animation::IKAimJob job;
    job.joint_correction = &quat;
    EXPECT_FALSE(job.Validate());
  }

  {  // Valid
    ozz::animation::IKAimJob job;
    job.joint = &joint;
    job.joint_correction = &quat;
    EXPECT_TRUE(job.Validate());
    EXPECT_TRUE(job.Run());
  }
}

TEST(Correction, IKAimJob) {
  ozz::math::SimdQuaternion quat;

  ozz::animation::IKAimJob job;
  job.joint_correction = &quat;

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
    job.joint = &parent;

    // These are in joint local-space
    job.aim = ozz::math::simd_float4::x_axis();
    job.up = ozz::math::simd_float4::y_axis();

    // Pole vector is in model space
    job.pole_vector = TransformVector(parent, ozz::math::simd_float4::y_axis());

    {  // x
      job.target = TransformPoint(parent, ozz::math::simd_float4::x_axis());
      EXPECT_TRUE(job.Run());
      EXPECT_SIMDQUATERNION_EQ_TOL(quat, 0.f, 0.f, 0.f, 1.f, 2e-3f);
    }

    {  // -x
      job.target = TransformPoint(parent, -ozz::math::simd_float4::x_axis());
      EXPECT_TRUE(job.Run());
      const ozz::math::Quaternion y_Pi = ozz::math::Quaternion::FromAxisAngle(
          ozz::math::Float3::y_axis(), ozz::math::kPi);
      EXPECT_SIMDQUATERNION_EQ_TOL(quat, y_Pi.x, y_Pi.y, y_Pi.z, y_Pi.w, 2e-3f);
    }

    {  // z
      job.target = TransformPoint(parent, ozz::math::simd_float4::z_axis());
      EXPECT_TRUE(job.Run());
      const ozz::math::Quaternion y_mPi_2 =
          ozz::math::Quaternion::FromAxisAngle(ozz::math::Float3::y_axis(),
                                               -ozz::math::kPi_2);
      EXPECT_SIMDQUATERNION_EQ_TOL(quat, y_mPi_2.x, y_mPi_2.y, y_mPi_2.z,
                                   y_mPi_2.w, 2e-3f);
    }

    {  // -z
      job.target = TransformPoint(parent, -ozz::math::simd_float4::z_axis());
      EXPECT_TRUE(job.Run());
      const ozz::math::Quaternion y_Pi_2 = ozz::math::Quaternion::FromAxisAngle(
          ozz::math::Float3::y_axis(), ozz::math::kPi_2);
      EXPECT_SIMDQUATERNION_EQ_TOL(quat, y_Pi_2.x, y_Pi_2.y, y_Pi_2.z, y_Pi_2.w,
                                   2e-3f);
    }

    {  // 45 up y
      job.target = TransformPoint(
          parent, ozz::math::simd_float4::Load(1.f, 1.f, 0.f, 0.f));
      EXPECT_TRUE(job.Run());
      const ozz::math::Quaternion z_Pi_4 = ozz::math::Quaternion::FromAxisAngle(
          ozz::math::Float3::z_axis(), ozz::math::kPi_4);
      EXPECT_SIMDQUATERNION_EQ_TOL(quat, z_Pi_4.x, z_Pi_4.y, z_Pi_4.z, z_Pi_4.w,
                                   2e-3f);
    }

    {  // 45 up y, further
      job.target = TransformPoint(
          parent, ozz::math::simd_float4::Load(2.f, 2.f, 0.f, 0.f));
      EXPECT_TRUE(job.Run());
      const ozz::math::Quaternion z_Pi_4 = ozz::math::Quaternion::FromAxisAngle(
          ozz::math::Float3::z_axis(), ozz::math::kPi_4);
      EXPECT_SIMDQUATERNION_EQ_TOL(quat, z_Pi_4.x, z_Pi_4.y, z_Pi_4.z, z_Pi_4.w,
                                   2e-3f);
    }
  }
}

TEST(Aim, IKAimJob) {
  ozz::animation::IKAimJob job;
  ozz::math::SimdQuaternion quat;
  job.joint_correction = &quat;
  const ozz::math::Float4x4 joint = ozz::math::Float4x4::identity();
  job.joint = &joint;

  job.target = ozz::math::simd_float4::x_axis();
  job.up = ozz::math::simd_float4::y_axis();
  job.pole_vector = ozz::math::simd_float4::y_axis();

  {  // aim x
    job.aim = ozz::math::simd_float4::x_axis();
    EXPECT_TRUE(job.Run());
    EXPECT_SIMDQUATERNION_EQ_TOL(quat, 0.f, 0.f, 0.f, 1.f, 2e-3f);
  }

  {  // aim -x
    job.aim = -ozz::math::simd_float4::x_axis();
    EXPECT_TRUE(job.Run());
    const ozz::math::Quaternion y_Pi = ozz::math::Quaternion::FromAxisAngle(
        ozz::math::Float3::y_axis(), -ozz::math::kPi);
    EXPECT_SIMDQUATERNION_EQ_TOL(quat, y_Pi.x, y_Pi.y, y_Pi.z, y_Pi.w, 2e-3f);
  }

  {  // aim z
    job.aim = ozz::math::simd_float4::z_axis();
    EXPECT_TRUE(job.Run());
    const ozz::math::Quaternion y_Pi_2 = ozz::math::Quaternion::FromAxisAngle(
        ozz::math::Float3::y_axis(), ozz::math::kPi_2);
    EXPECT_SIMDQUATERNION_EQ_TOL(quat, y_Pi_2.x, y_Pi_2.y, y_Pi_2.z, y_Pi_2.w,
                                 2e-3f);
  }
}

TEST(Up, IKAimJob) {
  ozz::animation::IKAimJob job;
  ozz::math::SimdQuaternion quat;
  job.joint_correction = &quat;
  const ozz::math::Float4x4 joint = ozz::math::Float4x4::identity();
  job.joint = &joint;

  job.target = ozz::math::simd_float4::x_axis();
  job.aim = ozz::math::simd_float4::x_axis();
  job.up = ozz::math::simd_float4::y_axis();
  job.pole_vector = ozz::math::simd_float4::y_axis();

  {  // up y
    job.up = ozz::math::simd_float4::y_axis();
    EXPECT_TRUE(job.Run());
    EXPECT_SIMDQUATERNION_EQ_TOL(quat, 0.f, 0.f, 0.f, 1.f, 2e-3f);
  }

  {  // up -y
    job.up = -ozz::math::simd_float4::y_axis();
    EXPECT_TRUE(job.Run());
    const ozz::math::Quaternion x_Pi = ozz::math::Quaternion::FromAxisAngle(
        ozz::math::Float3::x_axis(), ozz::math::kPi);
    EXPECT_SIMDQUATERNION_EQ_TOL(quat, x_Pi.x, x_Pi.y, x_Pi.z, x_Pi.w, 2e-3f);
  }

  {  // up z
    job.up = ozz::math::simd_float4::z_axis();
    EXPECT_TRUE(job.Run());
    const ozz::math::Quaternion x_mPi_2 = ozz::math::Quaternion::FromAxisAngle(
        ozz::math::Float3::x_axis(), -ozz::math::kPi_2);
    EXPECT_SIMDQUATERNION_EQ_TOL(quat, x_mPi_2.x, x_mPi_2.y, x_mPi_2.z,
                                 x_mPi_2.w, 2e-3f);
  }
}

TEST(Pole, IKAimJob) {
  ozz::animation::IKAimJob job;
  ozz::math::SimdQuaternion quat;
  job.joint_correction = &quat;
  const ozz::math::Float4x4 joint = ozz::math::Float4x4::identity();
  job.joint = &joint;

  job.target = ozz::math::simd_float4::x_axis();
  job.aim = ozz::math::simd_float4::x_axis();
  job.up = ozz::math::simd_float4::y_axis();

  {  // Pole y
    job.pole_vector = ozz::math::simd_float4::y_axis();
    EXPECT_TRUE(job.Run());
    EXPECT_SIMDQUATERNION_EQ_TOL(quat, 0.f, 0.f, 0.f, 1.f, 2e-3f);
  }

  {  // Pole -y
    job.pole_vector = -ozz::math::simd_float4::y_axis();
    EXPECT_TRUE(job.Run());
    const ozz::math::Quaternion x_Pi = ozz::math::Quaternion::FromAxisAngle(
        ozz::math::Float3::x_axis(), ozz::math::kPi);
    EXPECT_SIMDQUATERNION_EQ_TOL(quat, x_Pi.x, x_Pi.y, x_Pi.z, x_Pi.w, 2e-3f);
  }

  {  // Pole z
    job.pole_vector = ozz::math::simd_float4::z_axis();
    EXPECT_TRUE(job.Run());
    const ozz::math::Quaternion x_Pi_2 = ozz::math::Quaternion::FromAxisAngle(
        ozz::math::Float3::x_axis(), ozz::math::kPi_2);
    EXPECT_SIMDQUATERNION_EQ_TOL(quat, x_Pi_2.x, x_Pi_2.y, x_Pi_2.z, x_Pi_2.w,
                                 2e-3f);
  }
}

TEST(Twist, IKAimJob) {
  ozz::animation::IKAimJob job;
  ozz::math::SimdQuaternion quat;
  job.joint_correction = &quat;
  const ozz::math::Float4x4 joint = ozz::math::Float4x4::identity();
  job.joint = &joint;

  job.target = ozz::math::simd_float4::x_axis();
  job.aim = ozz::math::simd_float4::x_axis();
  job.up = ozz::math::simd_float4::y_axis();

  {  // Pole y, twist 0
    job.pole_vector = ozz::math::simd_float4::y_axis();
    job.twist_angle = 0.f;
    EXPECT_TRUE(job.Run());
    EXPECT_SIMDQUATERNION_EQ_TOL(quat, 0.f, 0.f, 0.f, 1.f, 2e-3f);
  }

  {  // Pole y, twist pi
    job.pole_vector = ozz::math::simd_float4::y_axis();
    job.twist_angle = ozz::math::kPi;
    EXPECT_TRUE(job.Run());
    const ozz::math::Quaternion x_Pi = ozz::math::Quaternion::FromAxisAngle(
        ozz::math::Float3::x_axis(), -ozz::math::kPi);
    EXPECT_SIMDQUATERNION_EQ_TOL(quat, x_Pi.x, x_Pi.y, x_Pi.z, x_Pi.w, 2e-3f);
  }

  {  // Pole y, twist -pi
    job.pole_vector = ozz::math::simd_float4::y_axis();
    job.twist_angle = -ozz::math::kPi;
    EXPECT_TRUE(job.Run());
    const ozz::math::Quaternion x_mPi = ozz::math::Quaternion::FromAxisAngle(
        ozz::math::Float3::x_axis(), -ozz::math::kPi);
    EXPECT_SIMDQUATERNION_EQ_TOL(quat, x_mPi.x, x_mPi.y, x_mPi.z, x_mPi.w,
                                 2e-3f);
  }

  {  // Pole y, twist pi/2
    job.pole_vector = ozz::math::simd_float4::y_axis();
    job.twist_angle = ozz::math::kPi_2;
    EXPECT_TRUE(job.Run());
    const ozz::math::Quaternion x_Pi_2 = ozz::math::Quaternion::FromAxisAngle(
        ozz::math::Float3::x_axis(), ozz::math::kPi_2);
    EXPECT_SIMDQUATERNION_EQ_TOL(quat, x_Pi_2.x, x_Pi_2.y, x_Pi_2.z, x_Pi_2.w,
                                 2e-3f);
  }

  {  // Pole z, twist pi/2
    job.pole_vector = ozz::math::simd_float4::z_axis();
    job.twist_angle = ozz::math::kPi_2;
    EXPECT_TRUE(job.Run());
    const ozz::math::Quaternion x_Pi = ozz::math::Quaternion::FromAxisAngle(
        ozz::math::Float3::x_axis(), ozz::math::kPi);
    EXPECT_SIMDQUATERNION_EQ_TOL(quat, x_Pi.x, x_Pi.y, x_Pi.z, x_Pi.w, 2e-3f);
  }
}

TEST(AlignedTargetUp, IKAimJob) {
  ozz::animation::IKAimJob job;
  ozz::math::SimdQuaternion quat;
  job.joint_correction = &quat;
  const ozz::math::Float4x4 joint = ozz::math::Float4x4::identity();
  job.joint = &joint;

  job.aim = ozz::math::simd_float4::x_axis();
  job.pole_vector = ozz::math::simd_float4::y_axis();

  {  // Not aligned
    job.target = ozz::math::simd_float4::x_axis();
    job.up = ozz::math::simd_float4::y_axis();
    EXPECT_TRUE(job.Run());
    EXPECT_SIMDQUATERNION_EQ_TOL(quat, 0.f, 0.f, 0.f, 1.f, 2e-3f);
  }

  {  // Aligned y
    job.target = ozz::math::simd_float4::y_axis();
    job.up = ozz::math::simd_float4::y_axis();
    EXPECT_TRUE(job.Run());
    const ozz::math::Quaternion z_Pi_2 = ozz::math::Quaternion::FromAxisAngle(
        ozz::math::Float3::z_axis(), ozz::math::kPi_2);
    EXPECT_SIMDQUATERNION_EQ_TOL(quat, z_Pi_2.x, z_Pi_2.y, z_Pi_2.z, z_Pi_2.w,
                                 2e-3f);
  }

  {  // Aligned 2*y
    job.target =
        ozz::math::simd_float4::y_axis() * ozz::math::simd_float4::Load1(2.f);
    job.up = ozz::math::simd_float4::y_axis();
    EXPECT_TRUE(job.Run());
    const ozz::math::Quaternion z_Pi_2 = ozz::math::Quaternion::FromAxisAngle(
        ozz::math::Float3::z_axis(), ozz::math::kPi_2);
    EXPECT_SIMDQUATERNION_EQ_TOL(quat, z_Pi_2.x, z_Pi_2.y, z_Pi_2.z, z_Pi_2.w,
                                 2e-3f);
  }

  {  // Aligned -2*y
    job.target =
        ozz::math::simd_float4::y_axis() * ozz::math::simd_float4::Load1(-2.f);
    job.up = ozz::math::simd_float4::y_axis();
    EXPECT_TRUE(job.Run());
    const ozz::math::Quaternion z_mPi_2 = ozz::math::Quaternion::FromAxisAngle(
        ozz::math::Float3::z_axis(), -ozz::math::kPi_2);
    EXPECT_SIMDQUATERNION_EQ_TOL(quat, z_mPi_2.x, z_mPi_2.y, z_mPi_2.z,
                                 z_mPi_2.w, 2e-3f);
  }
}

TEST(AlignedTargetPole, IKAimJob) {
  ozz::animation::IKAimJob job;
  ozz::math::SimdQuaternion quat;
  job.joint_correction = &quat;
  const ozz::math::Float4x4 joint = ozz::math::Float4x4::identity();
  job.joint = &joint;

  job.aim = ozz::math::simd_float4::x_axis();
  job.up = ozz::math::simd_float4::y_axis();

  {  // Not aligned
    job.target = ozz::math::simd_float4::x_axis();
    job.pole_vector = ozz::math::simd_float4::y_axis();
    EXPECT_TRUE(job.Run());
    EXPECT_SIMDQUATERNION_EQ_TOL(quat, 0.f, 0.f, 0.f, 1.f, 2e-3f);
  }

  {  // Aligned y
    job.target = ozz::math::simd_float4::y_axis();
    job.pole_vector = ozz::math::simd_float4::y_axis();
    EXPECT_TRUE(job.Run());
    const ozz::math::Quaternion z_Pi_2 = ozz::math::Quaternion::FromAxisAngle(
        ozz::math::Float3::z_axis(), ozz::math::kPi_2);
    EXPECT_SIMDQUATERNION_EQ_TOL(quat, z_Pi_2.x, z_Pi_2.y, z_Pi_2.z, z_Pi_2.w,
                                 2e-3f);
  }
}

TEST(TargetTooClose, IKAimJob) {
  ozz::animation::IKAimJob job;
  ozz::math::SimdQuaternion quat;
  job.joint_correction = &quat;
  const ozz::math::Float4x4 joint = ozz::math::Float4x4::identity();
  job.joint = &joint;

  job.target = ozz::math::simd_float4::zero();
  job.aim = ozz::math::simd_float4::x_axis();
  job.up = ozz::math::simd_float4::y_axis();
  job.pole_vector = ozz::math::simd_float4::y_axis();

  EXPECT_TRUE(job.Run());
  EXPECT_SIMDQUATERNION_EQ_TOL(quat, 0.f, 0.f, 0.f, 1.f, 2e-3f);
}

TEST(Weight, IKAimJob) {
  ozz::animation::IKAimJob job;
  ozz::math::SimdQuaternion quat;
  job.joint_correction = &quat;
  const ozz::math::Float4x4 joint = ozz::math::Float4x4::identity();
  job.joint = &joint;

  job.target = ozz::math::simd_float4::z_axis();
  job.aim = ozz::math::simd_float4::x_axis();
  job.up = ozz::math::simd_float4::y_axis();
  job.pole_vector = ozz::math::simd_float4::y_axis();

  {  // Full weight
    job.weight = 1.f;
    EXPECT_TRUE(job.Run());
    const ozz::math::Quaternion y_mPi2 = ozz::math::Quaternion::FromAxisAngle(
        ozz::math::Float3::y_axis(), -ozz::math::kPi_2);
    EXPECT_SIMDQUATERNION_EQ_TOL(quat, y_mPi2.x, y_mPi2.y, y_mPi2.z, y_mPi2.w,
                                 2e-3f);
  }

  {  // > 1
    job.weight = 2.f;
    EXPECT_TRUE(job.Run());
    const ozz::math::Quaternion y_mPi2 = ozz::math::Quaternion::FromAxisAngle(
        ozz::math::Float3::y_axis(), -ozz::math::kPi_2);
    EXPECT_SIMDQUATERNION_EQ_TOL(quat, y_mPi2.x, y_mPi2.y, y_mPi2.z, y_mPi2.w,
                                 2e-3f);
  }

  {  // Half weight
    job.weight = .5f;
    EXPECT_TRUE(job.Run());
    const ozz::math::Quaternion y_mPi4 = ozz::math::Quaternion::FromAxisAngle(
        ozz::math::Float3::y_axis(), -ozz::math::kPi_4);
    EXPECT_SIMDQUATERNION_EQ_TOL(quat, y_mPi4.x, y_mPi4.y, y_mPi4.z, y_mPi4.w,
                                 2e-3f);
  }

  {  // Zero weight
    job.weight = 0.f;
    EXPECT_TRUE(job.Run());
    EXPECT_SIMDQUATERNION_EQ_TOL(quat, 0.f, 0.f, 0.f, 1.f, 2e-3f);
  }

  {  // < 0
    job.weight = -.5f;
    EXPECT_TRUE(job.Run());
    EXPECT_SIMDQUATERNION_EQ_TOL(quat, 0.f, 0.f, 0.f, 1.f, 2e-3f);
  }
}

TEST(ZeroScale, IKAimJob) {
  ozz::animation::IKAimJob job;
  ozz::math::SimdQuaternion quat;
  job.joint_correction = &quat;
  const ozz::math::Float4x4 joint =
      ozz::math::Float4x4::Scaling(ozz::math::simd_float4::zero());
  job.joint = &joint;

  EXPECT_TRUE(job.Run());
  EXPECT_SIMDQUATERNION_EQ_TOL(quat, 0.f, 0.f, 0.f, 1.f, 2e-3f);
}
