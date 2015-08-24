//----------------------------------------------------------------------------//
//                                                                            //
// ozz-animation is hosted at http://github.com/guillaumeblanc/ozz-animation  //
// and distributed under the MIT License (MIT).                               //
//                                                                            //
// Copyright (c) 2015 Guillaume Blanc                                         //
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

#include "ozz/animation/runtime/blending_job.h"

#include "gtest/gtest.h"
#include "ozz/base/maths/gtest_math_helper.h"

#include "ozz/base/maths/soa_transform.h"

using ozz::animation::BlendingJob;

TEST(JobValidity, BlendingJob) {
  const ozz::math::SoaTransform identity = ozz::math::SoaTransform::identity();
  const ozz::math::SimdFloat4 zero = ozz::math::simd_float4::zero();
  BlendingJob::Layer layers[2];
  ozz::math::SoaTransform bind_poses[3] = {
    identity, identity, identity};
  ozz::math::SoaTransform input_transforms[3] = {
    identity, identity, identity};
  ozz::math::SoaTransform output_transforms[3] = {
    identity, identity, identity};
  ozz::math::SimdFloat4 joint_weights[3] = {
    zero, zero, zero};

  layers[0].transform.begin = input_transforms;
  layers[0].transform.end = input_transforms + 3;
  layers[1].transform.begin = input_transforms;
  layers[1].transform.end = input_transforms + 2;

  { // Empty/default job.
    BlendingJob job;
    EXPECT_FALSE(job.Validate());
    EXPECT_FALSE(job.Run());
  }

  {  // Invalid NULL output.
    BlendingJob job;
    job.layers.begin = layers;
    job.layers.end = layers + 2;
    job.bind_pose.begin = bind_poses;
    job.bind_pose.end = bind_poses + 2;
    EXPECT_FALSE(job.Validate());
    EXPECT_FALSE(job.Run());
  }
  {  // Invalid NULL input.
    BlendingJob job;
    job.bind_pose.begin = bind_poses;
    job.bind_pose.end = bind_poses + 2;
    job.output.begin = output_transforms;
    job.output.end = output_transforms + 2;
    EXPECT_FALSE(job.Validate());
    EXPECT_FALSE(job.Run());
  }
  {  // Invalid NULL input begin.
    BlendingJob job;
    job.layers.end = layers + 2;
    job.bind_pose.begin = bind_poses;
    job.bind_pose.end = bind_poses + 2;
    job.output.begin = output_transforms;
    job.output.end = output_transforms + 2;
    EXPECT_FALSE(job.Validate());
    EXPECT_FALSE(job.Run());
  }
  {  // Invalid NULL bind_pose.
    BlendingJob job;
    job.layers.begin = layers;
    job.layers.end = layers + 2;
    job.bind_pose.begin = bind_poses;
    job.bind_pose.end = NULL;
    job.output.begin = output_transforms;
    job.output.end = output_transforms + 2;
    EXPECT_FALSE(job.Validate());
    EXPECT_FALSE(job.Run());
  }
  {  // Invalid NULL bind_pose.
    BlendingJob job;
    job.layers.begin = layers;
    job.layers.end = layers + 2;
    job.bind_pose.begin = NULL;
    job.bind_pose.end = bind_poses + 2;
    job.output.begin = output_transforms;
    job.output.end = output_transforms + 2;
    EXPECT_FALSE(job.Validate());
    EXPECT_FALSE(job.Run());
  }

  {  // Invalid input range.
    BlendingJob job;
    job.layers.begin = layers + 1;
    job.layers.end = layers;
    job.bind_pose.begin = bind_poses;
    job.bind_pose.end = bind_poses + 2;
    job.output.begin = output_transforms;
    job.output.end = output_transforms + 2;
    EXPECT_FALSE(job.Validate());
    EXPECT_FALSE(job.Run());
  }
  {  // Invalid output range.
    BlendingJob job;
    job.layers.begin = layers;
    job.layers.end = layers + 2;
    job.bind_pose.begin = bind_poses;
    job.bind_pose.end = bind_poses + 2;
    job.output.begin = output_transforms + 1;
    job.output.end = output_transforms;
    EXPECT_FALSE(job.Validate());
    EXPECT_FALSE(job.Run());
  }
  {  // Invalid output range, smaller output.
    BlendingJob job;
    job.layers.begin = layers;
    job.layers.end = layers + 2;
    job.bind_pose.begin = bind_poses;
    job.bind_pose.end = bind_poses + 2;
    job.output.begin = output_transforms;
    job.output.end = output_transforms + 1;
    EXPECT_FALSE(job.Validate());
    EXPECT_FALSE(job.Run());
  }
  {  // Invalid output range, smaller output.
    BlendingJob job;
    job.layers.begin = layers;
    job.layers.end = layers + 2;
    job.bind_pose.begin = bind_poses;
    job.bind_pose.end = bind_poses + 2;
    job.output.begin = output_transforms;
    job.output.end = output_transforms + 1;
    EXPECT_FALSE(job.Validate());
    EXPECT_FALSE(job.Run());
  }

  {  // Invalid smaller input.
    BlendingJob job;
    job.layers.begin = layers;
    job.layers.end = layers + 2;
    job.bind_pose.begin = bind_poses;
    job.bind_pose.end = bind_poses + 3;
    job.output.begin = output_transforms;
    job.output.end = output_transforms + 3;
    EXPECT_FALSE(job.Validate());
    EXPECT_FALSE(job.Run());
  }

  {  // Invalid threasold.
    BlendingJob job;
    job.threshold = 0.f;
    job.layers.begin = layers;
    job.layers.end = layers + 2;
    job.bind_pose.begin = bind_poses;
    job.bind_pose.end = bind_poses + 2;
    job.output.begin = output_transforms;
    job.output.end = output_transforms + 2;
    EXPECT_FALSE(job.Validate());
    EXPECT_FALSE(job.Run());
  }

  {  // Invalid joint weights range.
    layers[0].joint_weights.begin = joint_weights;

    BlendingJob job;
    job.layers.begin = layers;
    job.layers.end = layers + 2;
    job.bind_pose.begin = bind_poses;
    job.bind_pose.end = bind_poses + 2;
    job.output.begin = output_transforms;
    job.output.end = output_transforms + 2;
    EXPECT_FALSE(job.Validate());
    EXPECT_FALSE(job.Run());
  }

  {  // Invalid joint weights range.
    layers[0].joint_weights.begin = joint_weights;
    layers[0].joint_weights.end = joint_weights + 1;

    BlendingJob job;
    job.layers.begin = layers;
    job.layers.end = layers + 2;
    job.bind_pose.begin = bind_poses;
    job.bind_pose.end = bind_poses + 2;
    job.output.begin = output_transforms;
    job.output.end = output_transforms + 2;
    EXPECT_FALSE(job.Validate());
    EXPECT_FALSE(job.Run());
  }

  {  // Invalid joint weights range.
    layers[0].joint_weights.begin = NULL;
    layers[0].joint_weights.end = joint_weights + 2;

    BlendingJob job;
    job.layers.begin = layers;
    job.layers.end = layers + 2;
    job.bind_pose.begin = bind_poses;
    job.bind_pose.end = bind_poses + 2;
    job.output.begin = output_transforms;
    job.output.end = output_transforms + 2;
    EXPECT_FALSE(job.Validate());
    EXPECT_FALSE(job.Run());
  }

  {  // Valid job.
    layers[0].joint_weights.begin = NULL;
    layers[0].joint_weights.end = NULL;

    BlendingJob job;
    job.layers.begin = layers;
    job.layers.end = layers + 2;
    job.bind_pose.begin = bind_poses;
    job.bind_pose.end = bind_poses + 2;
    job.output.begin = output_transforms;
    job.output.end = output_transforms + 2;
    EXPECT_TRUE(job.Validate());
    EXPECT_TRUE(job.Run());
  }

  {  // Valid joint weights range.
    layers[0].joint_weights.begin = joint_weights;
    layers[0].joint_weights.end = joint_weights + 2;

    BlendingJob job;
    job.layers.begin = layers;
    job.layers.end = layers + 2;
    job.bind_pose.begin = bind_poses;
    job.bind_pose.end = bind_poses + 2;
    job.output.begin = output_transforms;
    job.output.end = output_transforms + 2;
    EXPECT_TRUE(job.Validate());
    EXPECT_TRUE(job.Run());
  }

  {  // Valid job, bigger output.
    layers[0].joint_weights.begin = joint_weights;
    layers[0].joint_weights.end = joint_weights + 2;

    BlendingJob job;
    job.layers.begin = layers;
    job.layers.end = layers + 2;
    job.bind_pose.begin = bind_poses;
    job.bind_pose.end = bind_poses + 2;
    job.output.begin = output_transforms;
    job.output.end = output_transforms + 3;
    EXPECT_TRUE(job.Validate());
    EXPECT_TRUE(job.Run());
  }
}

TEST(Weight, BlendingJob) {
  const ozz::math::SoaTransform identity = ozz::math::SoaTransform::identity();

  // Initialize inputs.
  ozz::math::SoaTransform input_transforms[2][3] = {
    {identity, identity, identity},
    {identity, identity, identity}};
  input_transforms[0][0].translation = ozz::math::SoaFloat3::Load(
    ozz::math::simd_float4::Load(0.f, 1.f, 2.f, 3.f),
    ozz::math::simd_float4::Load(4.f, 5.f, 6.f, 7.f),
    ozz::math::simd_float4::Load(8.f, 9.f, 10.f, 11.f));
  input_transforms[0][1].translation = ozz::math::SoaFloat3::Load(
    ozz::math::simd_float4::Load(12.f, 13.f, 14.f, 15.f),
    ozz::math::simd_float4::Load(16.f, 17.f, 18.f, 19.f),
    ozz::math::simd_float4::Load(20.f, 21.f, 22.f, 23.f));
  input_transforms[1][0].translation = -input_transforms[0][0].translation;
  input_transforms[1][1].translation = -input_transforms[0][1].translation;

  // Initialize bind pose.
  ozz::math::SoaTransform bind_poses[3] = {
    identity, identity, identity};
  bind_poses[0].scale = ozz::math::SoaFloat3::Load(
    ozz::math::simd_float4::Load(0.f, 1.f, 2.f, 3.f),
    ozz::math::simd_float4::Load(4.f, 5.f, 6.f, 7.f),
    ozz::math::simd_float4::Load(8.f, 9.f, 10.f, 11.f));
  bind_poses[1].scale =
    bind_poses[0].scale * ozz::math::simd_float4::Load(2.f, 2.f, 2.f, 2.f);

  {
    BlendingJob::Layer layers[2];
    layers[0].transform.begin = input_transforms[0];
    layers[0].transform.end = input_transforms[0] + 2;
    layers[1].transform.begin = input_transforms[1];
    layers[1].transform.end = input_transforms[1] + 2;

    BlendingJob job;
    job.layers.begin = layers;
    job.layers.end = layers + 2;
    job.bind_pose.begin = bind_poses;
    job.bind_pose.end = bind_poses + 2;
    ozz::math::SoaTransform output_transforms[3];
    job.output.begin = output_transforms;
    job.output.end = output_transforms + 2;

    // Weight 0 (a bit less must give the same result) for the first layer,
    // 1 for the second.
    layers[0].weight = -.07f;
    layers[1].weight = 1.f;

    EXPECT_TRUE(job.Run());

    EXPECT_SOAFLOAT3_EQ(output_transforms[0].translation,
                        -0.f, -1.f, -2.f, -3.f,
                        -4.f, -5.f, -6.f, -7.f,
                        -8.f, -9.f, -10.f, -11.f);
    EXPECT_SOAFLOAT3_EQ(output_transforms[0].scale,
                        1.f, 1.f, 1.f, 1.f,
                        1.f, 1.f, 1.f, 1.f,
                        1.f, 1.f, 1.f, 1.f);
    EXPECT_SOAFLOAT3_EQ(output_transforms[1].translation,
                        -12.f, -13.f, -14.f, -15.f,
                        -16.f, -17.f, -18.f, -19.f,
                        -20.f, -21.f, -22.f, -23.f);
    EXPECT_SOAFLOAT3_EQ(output_transforms[1].scale,
                        1.f, 1.f, 1.f, 1.f,
                        1.f, 1.f, 1.f, 1.f,
                        1.f, 1.f, 1.f, 1.f);

    // Weight 1 for the first layer, 0 for the second.
    layers[0].weight = 1.f;
    layers[1].weight = 1e-27f; // Very low weight value.

    EXPECT_TRUE(job.Run());

    EXPECT_SOAFLOAT3_EQ(output_transforms[0].translation,
                        0.f, 1.f, 2.f, 3.f,
                        4.f, 5.f, 6.f, 7.f,
                        8.f, 9.f, 10.f, 11.f);
    EXPECT_SOAFLOAT3_EQ(output_transforms[0].scale,
                        1.f, 1.f, 1.f, 1.f,
                        1.f, 1.f, 1.f, 1.f,
                        1.f, 1.f, 1.f, 1.f);
    EXPECT_SOAFLOAT3_EQ(output_transforms[1].translation,
                        12.f, 13.f, 14.f, 15.f,
                        16.f, 17.f, 18.f, 19.f,
                        20.f, 21.f, 22.f, 23.f);
    EXPECT_SOAFLOAT3_EQ(output_transforms[1].scale,
                        1.f, 1.f, 1.f, 1.f,
                        1.f, 1.f, 1.f, 1.f,
                        1.f, 1.f, 1.f, 1.f);

    // Weight .5 for both layers.
    layers[0].weight = .5f;
    layers[1].weight = .5f;

    EXPECT_TRUE(job.Run());

    EXPECT_SOAFLOAT3_EQ(output_transforms[0].translation,
                        0.f, 0.f, 0.f, 0.f,
                        0.f, 0.f, 0.f, 0.f,
                        0.f, 0.f, 0.f, 0.f);
    EXPECT_SOAFLOAT3_EQ(output_transforms[0].scale,
                        1.f, 1.f, 1.f, 1.f,
                        1.f, 1.f, 1.f, 1.f,
                        1.f, 1.f, 1.f, 1.f);
    EXPECT_SOAFLOAT3_EQ(output_transforms[1].translation,
                        0.f, 0.f, 0.f, 0.f,
                        0.f, 0.f, 0.f, 0.f,
                        0.f, 0.f, 0.f, 0.f);
    EXPECT_SOAFLOAT3_EQ(output_transforms[1].scale,
                        1.f, 1.f, 1.f, 1.f,
                        1.f, 1.f, 1.f, 1.f,
                        1.f, 1.f, 1.f, 1.f);
  }
}

TEST(JointWeights, BlendingJob) {
  const ozz::math::SoaTransform identity = ozz::math::SoaTransform::identity();

  // Initialize inputs.
  ozz::math::SoaTransform input_transforms[2][3] = {
    {identity, identity, identity},
    {identity, identity, identity}};
  input_transforms[0][0].translation = ozz::math::SoaFloat3::Load(
    ozz::math::simd_float4::Load(0.f, 1.f, 2.f, 3.f),
    ozz::math::simd_float4::Load(4.f, 5.f, 6.f, 7.f),
    ozz::math::simd_float4::Load(8.f, 9.f, 10.f, 11.f));
  input_transforms[0][1].translation = ozz::math::SoaFloat3::Load(
    ozz::math::simd_float4::Load(12.f, 13.f, 14.f, 15.f),
    ozz::math::simd_float4::Load(16.f, 17.f, 18.f, 19.f),
    ozz::math::simd_float4::Load(20.f, 21.f, 22.f, 23.f));
  input_transforms[1][0].translation = -input_transforms[0][0].translation;
  input_transforms[1][1].translation = -input_transforms[0][1].translation;
  ozz::math::SimdFloat4 joint_weights[2][3] = {
    {ozz::math::simd_float4::Load(1.f, 1.f, 0.f, 0.f),
     ozz::math::simd_float4::Load(1.f, 0.f, 1.f, 1.f),
     ozz::math::simd_float4::one()},
    {ozz::math::simd_float4::Load(1.f, 1.f, 1.f, 0.f),
     ozz::math::simd_float4::Load(0.f, 1.f, 1.f, 1.f),
     ozz::math::simd_float4::one()}};
  // Initialize bind pose.
  ozz::math::SoaTransform bind_poses[3] = {
    identity, identity, identity};
  bind_poses[0].translation = ozz::math::SoaFloat3::Load(
    ozz::math::simd_float4::Load(10.f, 11.f, 12.f, 13.f),
    ozz::math::simd_float4::Load(14.f, 15.f, 16.f, 17.f),
    ozz::math::simd_float4::Load(18.f, 19.f, 20.f, 21.f));
  bind_poses[0].scale = ozz::math::SoaFloat3::Load(
    ozz::math::simd_float4::Load(0.f, 1.f, 2.f, 3.f),
    ozz::math::simd_float4::Load(4.f, 5.f, 6.f, 7.f),
    ozz::math::simd_float4::Load(8.f, 9.f, 10.f, 11.f));
  bind_poses[1].scale =
    bind_poses[0].scale * ozz::math::simd_float4::Load(2.f, 2.f, 2.f, 2.f);

  BlendingJob::Layer layers[2];
  layers[0].transform.begin = input_transforms[0];
  layers[0].transform.end = input_transforms[0] + 2;
  layers[0].joint_weights.begin = joint_weights[0];
  layers[0].joint_weights.end = joint_weights[0] + 2;
  layers[1].transform.begin = input_transforms[1];
  layers[1].transform.end = input_transforms[1] + 2;
  layers[1].joint_weights.begin = joint_weights[1];
  layers[1].joint_weights.end = joint_weights[1] + 2;

  { // Weight .5 for both layers.
    BlendingJob job;
    job.layers.begin = layers;
    job.layers.end = layers + 2;
    job.bind_pose.begin = bind_poses;
    job.bind_pose.end = bind_poses + 2;
    ozz::math::SoaTransform output_transforms[3];
    job.output.begin = output_transforms;
    job.output.end = output_transforms + 2;

    layers[0].weight = .5f;
    layers[1].weight = .5f;

    EXPECT_TRUE(job.Run());

    EXPECT_SOAFLOAT3_EQ(output_transforms[0].translation,
                        0.f, 0.f, -2.f, 13.f,
                        0.f, 0.f, -6.f, 17.f,
                        0.f, 0.f, -10.f, 21.f);
    EXPECT_SOAFLOAT3_EQ(output_transforms[0].scale,
                        1.f, 1.f, 1.f, 3.f,
                        1.f, 1.f, 1.f, 7.f,
                        1.f, 1.f, 1.f, 11.f);
    EXPECT_SOAFLOAT3_EQ(output_transforms[1].translation,
                        12.f, -13.f, 0.f, 0.f,
                        16.f, -17.f, 0.f, 0.f,
                        20.f, -21.f, 0.f, 0.f);
    EXPECT_SOAFLOAT3_EQ(output_transforms[1].scale,
                        1.f, 1.f, 1.f, 1.f,
                        1.f, 1.f, 1.f, 1.f,
                        1.f, 1.f, 1.f, 1.f);
  }
  { // Null weight for the first layer.
    BlendingJob job;
    job.layers.begin = layers;
    job.layers.end = layers + 2;
    job.bind_pose.begin = bind_poses;
    job.bind_pose.end = bind_poses + 2;
    ozz::math::SoaTransform output_transforms[3];
    job.output.begin = output_transforms;
    job.output.end = output_transforms + 2;

    layers[0].weight = 0.f;
    layers[1].weight = 1.f;

    EXPECT_TRUE(job.Run());

    EXPECT_SOAFLOAT3_EQ(output_transforms[0].translation,
                        -0.f, -1.f, -2.f, 13.f,
                        -4.f, -5.f, -6.f, 17.f,
                        -8.f, -9.f, -10.f, 21.f);
    EXPECT_SOAFLOAT3_EQ(output_transforms[0].scale,
                        1.f, 1.f, 1.f, 3.f,
                        1.f, 1.f, 1.f, 7.f,
                        1.f, 1.f, 1.f, 11.f);
    EXPECT_SOAFLOAT3_EQ(output_transforms[1].translation,
                        0.f, -13.f, -14.f, -15.f,
                        0.f, -17.f, -18.f, -19.f,
                        0.f, -21.f, -22.f, -23.f);
    EXPECT_SOAFLOAT3_EQ(output_transforms[1].scale,
                        0.f, 1.f, 1.f, 1.f,
                        8.f, 1.f, 1.f, 1.f,
                        16.f, 1.f, 1.f, 1.f);
  }
}

TEST(Normalize, BlendingJob) {
  const ozz::math::SoaTransform identity = ozz::math::SoaTransform::identity();

  // Initialize inputs.
  ozz::math::SoaTransform input_transforms[2][1] = {
    {identity}, {identity}};

  // Initialize bind pose.
  ozz::math::SoaTransform bind_poses[1] = {identity};
  bind_poses[0].scale = ozz::math::SoaFloat3::Load(
    ozz::math::simd_float4::Load(0.f, 1.f, 2.f, 3.f),
    ozz::math::simd_float4::Load(4.f, 5.f, 6.f, 7.f),
    ozz::math::simd_float4::Load(8.f, 9.f, 10.f, 11.f));

  input_transforms[0][0].rotation = ozz::math::SoaQuaternion::Load(
    ozz::math::simd_float4::Load(.70710677f, 0.f, 0.f, .382683432f),
    ozz::math::simd_float4::Load(0.f, 0.f, .70710677f, 0.f),
    ozz::math::simd_float4::Load(0.f, 0.f, 0.f, 0.f),
    ozz::math::simd_float4::Load(.70710677f, 1.f, .70710677f, .9238795f));
  input_transforms[1][0].rotation = ozz::math::SoaQuaternion::Load(
    ozz::math::simd_float4::Load(0.f, .70710677f, -.70710677f, -.382683432f),
    ozz::math::simd_float4::Load(0.f, 0.f, 0.f, 0.f),
    ozz::math::simd_float4::Load(0.f, 0.f, -.70710677f, 0.f),
    ozz::math::simd_float4::Load(1.f, .70710677f, 0.f, -.9238795f));

  { // Un-normalized weights < 1.f.
    input_transforms[0][0].translation = ozz::math::SoaFloat3::Load(
      ozz::math::simd_float4::Load(2.f, 3.f, 4.f, 5.f),
      ozz::math::simd_float4::Load(6.f, 7.f, 8.f, 9.f),
      ozz::math::simd_float4::Load(10.f, 11.f, 12.f, 13.f));
    input_transforms[1][0].translation = ozz::math::SoaFloat3::Load(
      ozz::math::simd_float4::Load(3.f, 4.f, 5.f, 6.f),
      ozz::math::simd_float4::Load(7.f, 8.f, 9.f, 10.f),
      ozz::math::simd_float4::Load(11.f, 12.f, 13.f, 14.f));

    BlendingJob::Layer layers[2];
    layers[0].weight = 0.2f;
    layers[0].transform.begin = input_transforms[0];
    layers[0].transform.end = input_transforms[0] + 1;
    layers[1].weight = 0.3f;
    layers[1].transform.begin = input_transforms[1];
    layers[1].transform.end = input_transforms[1] + 1;

    BlendingJob job;
    job.layers.begin = layers;
    job.layers.end = layers + 2;
    job.bind_pose.begin = bind_poses;
    job.bind_pose.end = bind_poses + 1;
    ozz::math::SoaTransform output_transforms[3];
    job.output.begin = output_transforms;
    job.output.end = output_transforms + 1;

    EXPECT_TRUE(job.Run());

    EXPECT_SOAFLOAT3_EQ(output_transforms[0].translation,
                        2.6f, 3.6f, 4.6f, 5.6f,
                        6.6f, 7.6f, 8.6f, 9.6f,
                        10.6f, 11.6f, 12.6f, 13.6f);
    EXPECT_SOAQUATERNION_EQ_EST(output_transforms[0].rotation,
                                .30507791f, .45761687f, -.58843851f, .38268352f,
                                0.f, 0.f, .39229235f, 0.f,
                                0.f, 0.f, -.58843851f, 0.f,
                                .95224595f, .88906217f, .39229235f, .92387962f);
    EXPECT_TRUE(ozz::math::AreAllTrue(IsNormalizedEst(output_transforms[0].rotation)));
    EXPECT_SOAFLOAT3_EQ(output_transforms[0].scale,
                        1.f, 1.f, 1.f, 1.f,
                        1.f, 1.f, 1.f, 1.f,
                        1.f, 1.f, 1.f, 1.f);
  }
  { // Un-normalized weights > 1.f.
    input_transforms[0][0].translation = ozz::math::SoaFloat3::Load(
      ozz::math::simd_float4::Load(5.f, 10.f, 15.f, 20.f),
      ozz::math::simd_float4::Load(25.f, 30.f, 35.f, 40.f),
      ozz::math::simd_float4::Load(45.f, 50.f, 55.f, 60.f));
    input_transforms[1][0].translation = ozz::math::SoaFloat3::Load(
      ozz::math::simd_float4::Load(10.f, 15.f, 20.f, 25.f),
      ozz::math::simd_float4::Load(30.f, 35.f, 40.f, 45.f),
      ozz::math::simd_float4::Load(50.f, 55.f, 60.f, 65.f));

    BlendingJob::Layer layers[2];
    layers[0].weight = 2.f;
    layers[0].transform.begin = input_transforms[0];
    layers[0].transform.end = input_transforms[0] + 1;
    layers[1].weight = 3.f;
    layers[1].transform.begin = input_transforms[1];
    layers[1].transform.end = input_transforms[1] + 1;

    BlendingJob job;
    job.layers.begin = layers;
    job.layers.end = layers + 2;
    job.bind_pose.begin = bind_poses;
    job.bind_pose.end = bind_poses + 1;
    ozz::math::SoaTransform output_transforms[3];
    job.output.begin = output_transforms;
    job.output.end = output_transforms + 1;

    EXPECT_TRUE(job.Run());

    EXPECT_SOAFLOAT3_EQ(output_transforms[0].translation,
                        8.f, 13.f, 18.f, 23.f,
                        28.f, 33.f, 38.f, 43.f,
                        48.f, 53.f, 58.f, 63.f);
    EXPECT_SOAQUATERNION_EQ_EST(output_transforms[0].rotation,
                                .30507791f, .45761687f, -.58843851f, .38268352f,
                                0.f, 0.f, .39229235f, 0.f,
                                0.f, 0.f, -.58843851f, 0.f,
                                .95224595f, .88906217f, .39229235f, .92387962f);
    EXPECT_TRUE(ozz::math::AreAllTrue(IsNormalizedEst(output_transforms[0].rotation)));
    EXPECT_SOAFLOAT3_EQ(output_transforms[0].scale,
                        1.f, 1.f, 1.f, 1.f,
                        1.f, 1.f, 1.f, 1.f,
                        1.f, 1.f, 1.f, 1.f);
  }
  { // Un-normalized weights > 1.f, with per-joint weights.
    input_transforms[0][0].translation = ozz::math::SoaFloat3::Load(
      ozz::math::simd_float4::Load(5.f, 10.f, 15.f, 20.f),
      ozz::math::simd_float4::Load(25.f, 30.f, 35.f, 40.f),
      ozz::math::simd_float4::Load(45.f, 50.f, 55.f, 60.f));
    input_transforms[1][0].translation = ozz::math::SoaFloat3::Load(
      ozz::math::simd_float4::Load(10.f, 15.f, 20.f, 25.f),
      ozz::math::simd_float4::Load(30.f, 35.f, 40.f, 45.f),
      ozz::math::simd_float4::Load(50.f, 55.f, 60.f, 65.f));
    ozz::math::SimdFloat4 joint_weights[2] = {
      ozz::math::simd_float4::Load(1.f, -1.f, 2.f, .1f),
      ozz::math::simd_float4::one()};

    BlendingJob::Layer layers[2];
    layers[0].weight = 2.f;
    layers[0].transform.begin = input_transforms[0];
    layers[0].transform.end = input_transforms[0] + 1;
    layers[1].weight = 3.f;
    layers[1].transform.begin = input_transforms[1];
    layers[1].transform.end = input_transforms[1] + 1;
    layers[1].joint_weights.begin = joint_weights;
    layers[1].joint_weights.end = joint_weights + 1;

    BlendingJob job;
    job.layers.begin = layers;
    job.layers.end = layers + 2;
    job.bind_pose.begin = bind_poses;
    job.bind_pose.end = bind_poses + 1;
    ozz::math::SoaTransform output_transforms[3];
    job.output.begin = output_transforms;
    job.output.end = output_transforms + 1;

    EXPECT_TRUE(job.Run());

    EXPECT_SOAFLOAT3_EQ(output_transforms[0].translation,
                        8.f, 10.f, 150.f / 8.f, 47.5f / 2.3f,
                        28.f, 30.f, 310.f / 8.f, 93.5f / 2.3f,
                        48.f, 50.f, 470.f / 8.f, 139.5f / 2.3f);
    EXPECT_SOAFLOAT3_EQ(output_transforms[0].scale,
                        1.f, 1.f, 1.f, 1.f,
                        1.f, 1.f, 1.f, 1.f,
                        1.f, 1.f, 1.f, 1.f);
  }
}

TEST(Threshold, BlendingJob) {
  const ozz::math::SoaTransform identity = ozz::math::SoaTransform::identity();

  // Initialize inputs.
  ozz::math::SoaTransform input_transforms[2][1] = {
    {identity}, {identity}};

  // Initialize bind pose.
  ozz::math::SoaTransform bind_poses[1] = {identity};
  bind_poses[0].scale = ozz::math::SoaFloat3::Load(
    ozz::math::simd_float4::Load(0.f, 1.f, 2.f, 3.f),
    ozz::math::simd_float4::Load(4.f, 5.f, 6.f, 7.f),
    ozz::math::simd_float4::Load(8.f, 9.f, 10.f, 11.f));

  input_transforms[0][0].translation = ozz::math::SoaFloat3::Load(
    ozz::math::simd_float4::Load(2.f, 3.f, 4.f, 5.f),
    ozz::math::simd_float4::Load(6.f, 7.f, 8.f, 9.f),
    ozz::math::simd_float4::Load(10.f, 11.f, 12.f, 13.f));
  input_transforms[1][0].translation = ozz::math::SoaFloat3::Load(
    ozz::math::simd_float4::Load(3.f, 4.f, 5.f, 6.f),
    ozz::math::simd_float4::Load(7.f, 8.f, 9.f, 10.f),
    ozz::math::simd_float4::Load(11.f, 12.f, 13.f, 14.f));

  { // Threshold is not reached.
    BlendingJob::Layer layers[2];
    layers[0].weight = 0.04f;
    layers[0].transform.begin = input_transforms[0];
    layers[0].transform.end = input_transforms[0] + 1;
    layers[1].weight = 0.06f;
    layers[1].transform.begin = input_transforms[1];
    layers[1].transform.end = input_transforms[1] + 2;

    BlendingJob job;
    job.threshold = .1f;
    job.layers.begin = layers;
    job.layers.end = layers + 2;
    job.bind_pose.begin = bind_poses;
    job.bind_pose.end = bind_poses + 1;
    ozz::math::SoaTransform output_transforms[3];
    job.output.begin = output_transforms;
    job.output.end = output_transforms + 3;

    EXPECT_TRUE(job.Run());

    EXPECT_SOAFLOAT3_EQ(output_transforms[0].translation,
                        2.6f, 3.6f, 4.6f, 5.6f,
                        6.6f, 7.6f, 8.6f, 9.6f,
                        10.6f, 11.6f, 12.6f, 13.6f);
    EXPECT_SOAQUATERNION_EQ_EST(output_transforms[0].rotation,
                                0.f, 0.f, 0.f, 0.f,
                                0.f, 0.f, 0.f, 0.f,
                                0.f, 0.f, 0.f, 0.f,
                                1.f, 1.f, 1.f, 1.f);
    EXPECT_SOAFLOAT3_EQ(output_transforms[0].scale,
                        1.f, 1.f, 1.f, 1.f,
                        1.f, 1.f, 1.f, 1.f,
                        1.f, 1.f, 1.f, 1.f);
  }
  { // Threshold is reached at 100%.
    BlendingJob::Layer layers[2];
    layers[0].weight = 1e-27f;
    layers[0].transform.begin = input_transforms[0];
    layers[0].transform.end = input_transforms[0] + 2;
    layers[1].weight = 0.f;
    layers[1].transform.begin = input_transforms[1];
    layers[1].transform.end = input_transforms[1] + 1;

    BlendingJob job;
    job.threshold = .1f;
    job.layers.begin = layers;
    job.layers.end = layers + 2;
    job.bind_pose.begin = bind_poses;
    job.bind_pose.end = bind_poses + 1;
    ozz::math::SoaTransform output_transforms[2];
    job.output.begin = output_transforms;
    job.output.end = output_transforms + 1;

    EXPECT_TRUE(job.Run());

    EXPECT_SOAFLOAT3_EQ(output_transforms[0].translation,
                        0.f, 0.f, 0.f, 0.f,
                        0.f, 0.f, 0.f, 0.f,
                        0.f, 0.f, 0.f, 0.f);
    EXPECT_SOAQUATERNION_EQ_EST(output_transforms[0].rotation,
                                0.f, 0.f, 0.f, 0.f,
                                0.f, 0.f, 0.f, 0.f,
                                0.f, 0.f, 0.f, 0.f,
                                1.f, 1.f, 1.f, 1.f);
    EXPECT_SOAFLOAT3_EQ(output_transforms[0].scale,
                        0.f, 1.f, 2.f, 3.f,
                        4.f, 5.f, 6.f, 7.f,
                        8.f, 9.f, 10.f, 11.f);
  }
}
