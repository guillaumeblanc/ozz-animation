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

#include "gtest/gtest.h"
#include "ozz/animation/runtime/blending_job.h"
#include "ozz/base/maths/gtest_math_helper.h"
#include "ozz/base/maths/soa_transform.h"

using ozz::animation::BlendingJob;

TEST(JobValidity, BlendingJob) {
  const ozz::math::SoaTransform identity = ozz::math::SoaTransform::identity();
  const ozz::math::SimdFloat4 zero = ozz::math::simd_float4::zero();
  BlendingJob::Layer layers[2];
  const ozz::math::SoaTransform bind_poses[3] = {identity, identity, identity};
  const ozz::math::SoaTransform input_transforms[3] = {identity, identity,
                                                       identity};
  ozz::math::SoaTransform output_transforms[3] = {identity, identity, identity};
  ozz::math::SimdFloat4 joint_weights[3] = {zero, zero, zero};

  layers[0].transform = input_transforms;
  layers[1].transform = {input_transforms, input_transforms + 2};

  {  // Empty/default job.
    BlendingJob job;
    EXPECT_FALSE(job.Validate());
    EXPECT_FALSE(job.Run());
  }

  {  // Invalid output.
    BlendingJob job;
    job.layers = {layers, layers + 2};
    job.bind_pose = {bind_poses, bind_poses + 2};
    EXPECT_FALSE(job.Validate());
    EXPECT_FALSE(job.Run());
  }
  {  // Layers are optional.
    BlendingJob job;
    job.bind_pose = {bind_poses, bind_poses + 2};
    job.output = {output_transforms, output_transforms + 2};
    EXPECT_TRUE(job.Validate());
    EXPECT_TRUE(job.Run());
  }
  {  // Invalid bind pose.
    BlendingJob job;
    job.layers = {layers, layers + 2};
    job.output = {output_transforms, output_transforms + 2};
    EXPECT_FALSE(job.Validate());
    EXPECT_FALSE(job.Run());
  }
  {  // Invalid layer input range, too small.
    BlendingJob::Layer invalid_layers[2];
    invalid_layers[0].transform = {input_transforms, input_transforms + 1};
    invalid_layers[1].transform = {input_transforms, input_transforms + 2};

    BlendingJob job;
    job.layers = invalid_layers;
    job.bind_pose = {bind_poses, bind_poses + 2};
    job.output = {output_transforms, output_transforms + 2};
    EXPECT_FALSE(job.Validate());
    EXPECT_FALSE(job.Run());
  }
  {  // Invalid output range, smaller output.
    BlendingJob job;
    job.layers = {layers, layers + 2};
    job.bind_pose = {bind_poses, bind_poses + 2};
    job.output = {output_transforms, output_transforms + 1};
    EXPECT_FALSE(job.Validate());
    EXPECT_FALSE(job.Run());
  }

  {  // Invalid smaller input.
    BlendingJob job;
    job.layers = {layers, layers + 2};
    job.bind_pose = {bind_poses, bind_poses + 3};
    job.output = {output_transforms, output_transforms + 3};
    EXPECT_FALSE(job.Validate());
    EXPECT_FALSE(job.Run());
  }

  {  // Invalid threshold.
    BlendingJob job;
    job.threshold = 0.f;
    job.layers = {layers, layers + 2};
    job.bind_pose = {bind_poses, bind_poses + 2};
    job.output = {output_transforms, output_transforms + 2};
    EXPECT_FALSE(job.Validate());
    EXPECT_FALSE(job.Run());
  }

  {  // Invalid joint weights range.
    layers[0].joint_weights = {joint_weights, joint_weights + 1};

    BlendingJob job;
    job.layers = {layers, layers + 2};
    job.bind_pose = {bind_poses, bind_poses + 2};
    job.output = {output_transforms, output_transforms + 2};
    EXPECT_FALSE(job.Validate());
    EXPECT_FALSE(job.Run());
  }

  {  // Valid job.
    layers[0].joint_weights = {nullptr, nullptr};

    BlendingJob job;
    job.layers = {layers, layers + 2};
    job.bind_pose = {bind_poses, bind_poses + 2};
    job.output = {output_transforms, output_transforms + 2};
    EXPECT_TRUE(job.Validate());
    EXPECT_TRUE(job.Run());
  }

  {  // Valid joint weights range.
    layers[0].joint_weights = {joint_weights, joint_weights + 2};

    BlendingJob job;
    job.layers = {layers, layers + 2};
    job.bind_pose = {bind_poses, bind_poses + 2};
    job.output = {output_transforms, output_transforms + 2};
    EXPECT_TRUE(job.Validate());
    EXPECT_TRUE(job.Run());
  }

  {  // Valid job, bigger output.
    layers[0].joint_weights = {joint_weights, joint_weights + 2};

    BlendingJob job;
    job.layers = {layers, layers + 2};
    job.bind_pose = {bind_poses, bind_poses + 2};
    job.output = {output_transforms, output_transforms + 3};
    EXPECT_TRUE(job.Validate());
    EXPECT_TRUE(job.Run());
  }

  {  // Valid no layers.
    BlendingJob job;
    job.bind_pose = {bind_poses, bind_poses + 2};
    job.output = {output_transforms, output_transforms + 2};
    EXPECT_TRUE(job.Validate());
    EXPECT_TRUE(job.Run());
  }
}

TEST(JobValidityAdditive, BlendingJob) {
  const ozz::math::SoaTransform identity = ozz::math::SoaTransform::identity();
  const ozz::math::SimdFloat4 zero = ozz::math::simd_float4::zero();
  BlendingJob::Layer layers[2];
  BlendingJob::Layer additive_layers[2];

  const ozz::math::SoaTransform bind_poses[3] = {identity, identity, identity};
  const ozz::math::SoaTransform input_transforms[3] = {identity, identity,
                                                       identity};
  ozz::math::SoaTransform output_transforms[3] = {identity, identity, identity};
  ozz::math::SimdFloat4 joint_weights[3] = {zero, zero, zero};

  layers[0].transform = input_transforms;
  layers[1].transform = input_transforms;

  additive_layers[0].transform = input_transforms;
  additive_layers[1].transform = input_transforms;

  {  // Valid additive job, no normal blending.
    BlendingJob job;
    job.additive_layers = additive_layers;
    job.bind_pose = bind_poses;
    job.output = output_transforms;
    EXPECT_TRUE(job.Validate());
    EXPECT_TRUE(job.Run());
  }

  {  // Valid additive job, with normal blending also.

    BlendingJob job;
    job.layers = layers;
    job.additive_layers = additive_layers;
    job.bind_pose = bind_poses;
    job.output = output_transforms;
    EXPECT_TRUE(job.Validate());
    EXPECT_TRUE(job.Run());
  }

  {  // Invalid layer input range, too small.
    BlendingJob::Layer invalid_layers[2];
    invalid_layers[0].transform = {input_transforms, input_transforms + 1};
    invalid_layers[1].transform = {input_transforms, input_transforms + 2};

    BlendingJob job;
    job.layers = layers;
    job.additive_layers = invalid_layers;
    job.bind_pose = {bind_poses, bind_poses + 2};
    job.output = {output_transforms, output_transforms + 2};
    EXPECT_FALSE(job.Validate());
    EXPECT_FALSE(job.Run());
  }

  {  // Valid additive job, with per-joint weights.
    layers[0].joint_weights = {joint_weights, joint_weights + 2};

    BlendingJob job;
    job.additive_layers = additive_layers;
    job.bind_pose = {bind_poses, bind_poses + 2};
    job.output = {output_transforms, output_transforms + 2};
    EXPECT_TRUE(job.Validate());
    EXPECT_TRUE(job.Run());
  }
}

TEST(Empty, BlendingJob) {
  const ozz::math::SoaTransform identity = ozz::math::SoaTransform::identity();

  // Initialize bind pose.
  ozz::math::SoaTransform bind_poses[2] = {identity, identity};
  bind_poses[0].translation = ozz::math::SoaFloat3::Load(
      ozz::math::simd_float4::Load(0.f, 1.f, 2.f, 3.f),
      ozz::math::simd_float4::Load(4.f, 5.f, 6.f, 7.f),
      ozz::math::simd_float4::Load(8.f, 9.f, 10.f, 11.f));
  bind_poses[0].scale = ozz::math::SoaFloat3::Load(
      ozz::math::simd_float4::Load(0.f, 10.f, 20.f, 30.f),
      ozz::math::simd_float4::Load(40.f, 50.f, 60.f, 70.f),
      ozz::math::simd_float4::Load(80.f, 90.f, 100.f, 110.f));
  bind_poses[1].translation = bind_poses[0].translation *
                              ozz::math::simd_float4::Load(2.f, 2.f, 2.f, 2.f);
  bind_poses[1].scale =
      bind_poses[0].scale * ozz::math::simd_float4::Load(2.f, 2.f, 2.f, 2.f);

  BlendingJob job;
  job.bind_pose = bind_poses;
  ozz::math::SoaTransform output_transforms[2];
  job.output = output_transforms;

  EXPECT_TRUE(job.Validate());
  EXPECT_TRUE(job.Run());

  EXPECT_SOAFLOAT3_EQ(output_transforms[0].translation, 0.f, 1.f, 2.f, 3.f, 4.f,
                      5.f, 6.f, 7.f, 8.f, 9.f, 10.f, 11.f);
  EXPECT_SOAFLOAT3_EQ(output_transforms[0].scale, 0.f, 10.f, 20.f, 30.f, 40.f,
                      50.f, 60.f, 70.f, 80.f, 90.f, 100.f, 110.f);
  EXPECT_SOAFLOAT3_EQ(output_transforms[1].translation, 0.f, 2.f, 4.f, 6.f, 8.f,
                      10.f, 12.f, 14.f, 16.f, 18.f, 20.f, 22.f);
  EXPECT_SOAFLOAT3_EQ(output_transforms[1].scale, 0.f, 20.f, 40.f, 60.f, 80.f,
                      100.f, 120.f, 140.f, 160.f, 180.f, 200.f, 220.f);
}

TEST(Weight, BlendingJob) {
  const ozz::math::SoaTransform identity = ozz::math::SoaTransform::identity();

  // Initialize inputs.
  ozz::math::SoaTransform input_transforms[2][2] = {{identity, identity},
                                                    {identity, identity}};
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
  ozz::math::SoaTransform bind_poses[2] = {identity, identity};
  bind_poses[0].scale = ozz::math::SoaFloat3::Load(
      ozz::math::simd_float4::Load(0.f, 1.f, 2.f, 3.f),
      ozz::math::simd_float4::Load(4.f, 5.f, 6.f, 7.f),
      ozz::math::simd_float4::Load(8.f, 9.f, 10.f, 11.f));
  bind_poses[1].scale =
      bind_poses[0].scale * ozz::math::simd_float4::Load(2.f, 2.f, 2.f, 2.f);

  {
    BlendingJob::Layer layers[2];
    layers[0].transform = input_transforms[0];
    layers[1].transform = input_transforms[1];

    ozz::math::SoaTransform output_transforms[2];

    BlendingJob job;
    job.layers = layers;
    job.bind_pose = bind_poses;
    job.output = output_transforms;

    // Weight 0 (a bit less must give the same result) for the first layer,
    // 1 for the second.
    layers[0].weight = -.07f;
    layers[1].weight = 1.f;

    EXPECT_TRUE(job.Run());

    EXPECT_SOAFLOAT3_EQ(output_transforms[0].translation, -0.f, -1.f, -2.f,
                        -3.f, -4.f, -5.f, -6.f, -7.f, -8.f, -9.f, -10.f, -11.f);
    EXPECT_SOAFLOAT3_EQ(output_transforms[0].scale, 1.f, 1.f, 1.f, 1.f, 1.f,
                        1.f, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f);
    EXPECT_SOAFLOAT3_EQ(output_transforms[1].translation, -12.f, -13.f, -14.f,
                        -15.f, -16.f, -17.f, -18.f, -19.f, -20.f, -21.f, -22.f,
                        -23.f);
    EXPECT_SOAFLOAT3_EQ(output_transforms[1].scale, 1.f, 1.f, 1.f, 1.f, 1.f,
                        1.f, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f);

    // Weight 1 for the first layer, 0 for the second.
    layers[0].weight = 1.f;
    layers[1].weight = 1e-27f;  // Very low weight value.

    EXPECT_TRUE(job.Run());

    EXPECT_SOAFLOAT3_EQ(output_transforms[0].translation, 0.f, 1.f, 2.f, 3.f,
                        4.f, 5.f, 6.f, 7.f, 8.f, 9.f, 10.f, 11.f);
    EXPECT_SOAFLOAT3_EQ(output_transforms[0].scale, 1.f, 1.f, 1.f, 1.f, 1.f,
                        1.f, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f);
    EXPECT_SOAFLOAT3_EQ(output_transforms[1].translation, 12.f, 13.f, 14.f,
                        15.f, 16.f, 17.f, 18.f, 19.f, 20.f, 21.f, 22.f, 23.f);
    EXPECT_SOAFLOAT3_EQ(output_transforms[1].scale, 1.f, 1.f, 1.f, 1.f, 1.f,
                        1.f, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f);

    // Weight .5 for both layers.
    layers[0].weight = .5f;
    layers[1].weight = .5f;

    EXPECT_TRUE(job.Run());

    EXPECT_SOAFLOAT3_EQ(output_transforms[0].translation, 0.f, 0.f, 0.f, 0.f,
                        0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f);
    EXPECT_SOAFLOAT3_EQ(output_transforms[0].scale, 1.f, 1.f, 1.f, 1.f, 1.f,
                        1.f, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f);
    EXPECT_SOAFLOAT3_EQ(output_transforms[1].translation, 0.f, 0.f, 0.f, 0.f,
                        0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f);
    EXPECT_SOAFLOAT3_EQ(output_transforms[1].scale, 1.f, 1.f, 1.f, 1.f, 1.f,
                        1.f, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f);
  }
}

TEST(JointWeights, BlendingJob) {
  const ozz::math::SoaTransform identity = ozz::math::SoaTransform::identity();

  // Initialize inputs.
  ozz::math::SoaTransform input_transforms[2][2] = {{identity, identity},
                                                    {identity, identity}};
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
  ozz::math::SimdFloat4 joint_weights[2][2] = {
      {ozz::math::simd_float4::Load(1.f, 1.f, 0.f, 0.f),
       ozz::math::simd_float4::Load(1.f, 0.f, 1.f, 1.f)},
      {ozz::math::simd_float4::Load(1.f, 1.f, 1.f, 0.f),
       ozz::math::simd_float4::Load(0.f, 1.f, 1.f, 1.f)}};
  // Initialize bind pose.
  ozz::math::SoaTransform bind_poses[2] = {identity, identity};
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
  layers[0].transform = input_transforms[0];
  layers[0].joint_weights = joint_weights[0];
  layers[1].transform = input_transforms[1];
  layers[1].joint_weights = joint_weights[1];

  {  // Weight .5 for both layers.
    ozz::math::SoaTransform output_transforms[3];

    BlendingJob job;
    job.layers = layers;
    job.bind_pose = bind_poses;
    job.output = output_transforms;

    layers[0].weight = .5f;
    layers[1].weight = .5f;

    EXPECT_TRUE(job.Run());

    EXPECT_SOAFLOAT3_EQ(output_transforms[0].translation, 0.f, 0.f, -2.f, 13.f,
                        0.f, 0.f, -6.f, 17.f, 0.f, 0.f, -10.f, 21.f);
    EXPECT_SOAFLOAT3_EQ(output_transforms[0].scale, 1.f, 1.f, 1.f, 3.f, 1.f,
                        1.f, 1.f, 7.f, 1.f, 1.f, 1.f, 11.f);
    EXPECT_SOAFLOAT3_EQ(output_transforms[1].translation, 12.f, -13.f, 0.f, 0.f,
                        16.f, -17.f, 0.f, 0.f, 20.f, -21.f, 0.f, 0.f);
    EXPECT_SOAFLOAT3_EQ(output_transforms[1].scale, 1.f, 1.f, 1.f, 1.f, 1.f,
                        1.f, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f);
  }
  {  // Null weight for the first layer.
    ozz::math::SoaTransform output_transforms[2];

    BlendingJob job;
    job.layers = layers;
    job.bind_pose = bind_poses;
    job.output = output_transforms;

    layers[0].weight = 0.f;
    layers[1].weight = 1.f;

    EXPECT_TRUE(job.Run());

    EXPECT_SOAFLOAT3_EQ(output_transforms[0].translation, -0.f, -1.f, -2.f,
                        13.f, -4.f, -5.f, -6.f, 17.f, -8.f, -9.f, -10.f, 21.f);
    EXPECT_SOAFLOAT3_EQ(output_transforms[0].scale, 1.f, 1.f, 1.f, 3.f, 1.f,
                        1.f, 1.f, 7.f, 1.f, 1.f, 1.f, 11.f);
    EXPECT_SOAFLOAT3_EQ(output_transforms[1].translation, 0.f, -13.f, -14.f,
                        -15.f, 0.f, -17.f, -18.f, -19.f, 0.f, -21.f, -22.f,
                        -23.f);
    EXPECT_SOAFLOAT3_EQ(output_transforms[1].scale, 0.f, 1.f, 1.f, 1.f, 8.f,
                        1.f, 1.f, 1.f, 16.f, 1.f, 1.f, 1.f);
  }
}

TEST(Normalize, BlendingJob) {
  const ozz::math::SoaTransform identity = ozz::math::SoaTransform::identity();

  // Initialize inputs.
  ozz::math::SoaTransform input_transforms[2][1] = {{identity}, {identity}};

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

  {  // Un-normalized weights < 1.f.
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
    layers[0].transform = input_transforms[0];
    layers[1].weight = 0.3f;
    layers[1].transform = input_transforms[1];

    ozz::math::SoaTransform output_transforms[1];

    BlendingJob job;
    job.layers = layers;
    job.bind_pose = bind_poses;
    job.output = output_transforms;

    EXPECT_TRUE(job.Run());

    EXPECT_SOAFLOAT3_EQ(output_transforms[0].translation, 2.6f, 3.6f, 4.6f,
                        5.6f, 6.6f, 7.6f, 8.6f, 9.6f, 10.6f, 11.6f, 12.6f,
                        13.6f);
    EXPECT_SOAQUATERNION_EQ_EST(output_transforms[0].rotation, .30507791f,
                                .45761687f, -.58843851f, .38268352f, 0.f, 0.f,
                                .39229235f, 0.f, 0.f, 0.f, -.58843851f, 0.f,
                                .95224595f, .88906217f, .39229235f, .92387962f);
    EXPECT_TRUE(
        ozz::math::AreAllTrue(IsNormalizedEst(output_transforms[0].rotation)));
    EXPECT_SOAFLOAT3_EQ(output_transforms[0].scale, 1.f, 1.f, 1.f, 1.f, 1.f,
                        1.f, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f);
  }
  {  // Un-normalized weights > 1.f.
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
    layers[0].transform = input_transforms[0];
    layers[1].weight = 3.f;
    layers[1].transform = input_transforms[1];

    ozz::math::SoaTransform output_transforms[1];

    BlendingJob job;
    job.layers = layers;
    job.bind_pose = bind_poses;
    job.output = output_transforms;

    EXPECT_TRUE(job.Run());

    EXPECT_SOAFLOAT3_EQ(output_transforms[0].translation, 8.f, 13.f, 18.f, 23.f,
                        28.f, 33.f, 38.f, 43.f, 48.f, 53.f, 58.f, 63.f);
    EXPECT_SOAQUATERNION_EQ_EST(output_transforms[0].rotation, .30507791f,
                                .45761687f, -.58843851f, .38268352f, 0.f, 0.f,
                                .39229235f, 0.f, 0.f, 0.f, -.58843851f, 0.f,
                                .95224595f, .88906217f, .39229235f, .92387962f);
    EXPECT_TRUE(
        ozz::math::AreAllTrue(IsNormalizedEst(output_transforms[0].rotation)));
    EXPECT_SOAFLOAT3_EQ(output_transforms[0].scale, 1.f, 1.f, 1.f, 1.f, 1.f,
                        1.f, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f);
  }
  {  // Un-normalized weights > 1.f, with per-joint weights.
    input_transforms[0][0].translation = ozz::math::SoaFloat3::Load(
        ozz::math::simd_float4::Load(5.f, 10.f, 15.f, 20.f),
        ozz::math::simd_float4::Load(25.f, 30.f, 35.f, 40.f),
        ozz::math::simd_float4::Load(45.f, 50.f, 55.f, 60.f));
    input_transforms[1][0].translation = ozz::math::SoaFloat3::Load(
        ozz::math::simd_float4::Load(10.f, 15.f, 20.f, 25.f),
        ozz::math::simd_float4::Load(30.f, 35.f, 40.f, 45.f),
        ozz::math::simd_float4::Load(50.f, 55.f, 60.f, 65.f));
    ozz::math::SimdFloat4 joint_weights[1] = {
        ozz::math::simd_float4::Load(1.f, -1.f, 2.f, .1f)};

    BlendingJob::Layer layers[2];
    layers[0].weight = 2.f;
    layers[0].transform = input_transforms[0];
    layers[1].weight = 3.f;
    layers[1].transform = input_transforms[1];
    layers[1].joint_weights = joint_weights;

    ozz::math::SoaTransform output_transforms[1];

    BlendingJob job;
    job.layers = layers;
    job.bind_pose = bind_poses;
    job.output = output_transforms;

    EXPECT_TRUE(job.Run());

    EXPECT_SOAFLOAT3_EQ(output_transforms[0].translation, 8.f, 10.f,
                        150.f / 8.f, 47.5f / 2.3f, 28.f, 30.f, 310.f / 8.f,
                        93.5f / 2.3f, 48.f, 50.f, 470.f / 8.f, 139.5f / 2.3f);
    EXPECT_SOAFLOAT3_EQ(output_transforms[0].scale, 1.f, 1.f, 1.f, 1.f, 1.f,
                        1.f, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f);
  }
}

TEST(Threshold, BlendingJob) {
  const ozz::math::SoaTransform identity = ozz::math::SoaTransform::identity();

  // Initialize inputs.
  ozz::math::SoaTransform input_transforms[2][1] = {{identity}, {identity}};

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

  {  // Threshold is not reached.
    BlendingJob::Layer layers[2];
    layers[0].weight = 0.04f;
    layers[0].transform = input_transforms[0];
    layers[1].weight = 0.06f;
    layers[1].transform = input_transforms[1];

    ozz::math::SoaTransform output_transforms[1];

    BlendingJob job;
    job.threshold = .1f;
    job.layers = layers;
    job.bind_pose = bind_poses;
    job.output = output_transforms;

    EXPECT_TRUE(job.Run());

    EXPECT_SOAFLOAT3_EQ(output_transforms[0].translation, 2.6f, 3.6f, 4.6f,
                        5.6f, 6.6f, 7.6f, 8.6f, 9.6f, 10.6f, 11.6f, 12.6f,
                        13.6f);
    EXPECT_SOAQUATERNION_EQ_EST(output_transforms[0].rotation, 0.f, 0.f, 0.f,
                                0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f,
                                1.f, 1.f, 1.f, 1.f);
    EXPECT_SOAFLOAT3_EQ(output_transforms[0].scale, 1.f, 1.f, 1.f, 1.f, 1.f,
                        1.f, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f);
  }
  {  // Threshold is reached at 100%.
    BlendingJob::Layer layers[2];
    layers[0].weight = 1e-27f;
    layers[0].transform = input_transforms[0];
    layers[1].weight = 0.f;
    layers[1].transform = input_transforms[1];

    ozz::math::SoaTransform output_transforms[1];

    BlendingJob job;
    job.threshold = .1f;
    job.layers = layers;
    job.bind_pose = bind_poses;
    job.output = output_transforms;

    EXPECT_TRUE(job.Run());

    EXPECT_SOAFLOAT3_EQ(output_transforms[0].translation, 0.f, 0.f, 0.f, 0.f,
                        0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f);
    EXPECT_SOAQUATERNION_EQ_EST(output_transforms[0].rotation, 0.f, 0.f, 0.f,
                                0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f,
                                1.f, 1.f, 1.f, 1.f);
    EXPECT_SOAFLOAT3_EQ(output_transforms[0].scale, 0.f, 1.f, 2.f, 3.f, 4.f,
                        5.f, 6.f, 7.f, 8.f, 9.f, 10.f, 11.f);
  }
}

TEST(AdditiveWeight, BlendingJob) {
  const ozz::math::SoaTransform identity = ozz::math::SoaTransform::identity();

  // Initialize inputs.
  ozz::math::SoaTransform input_transforms[2][1] = {{identity}, {identity}};
  input_transforms[0][0].translation = ozz::math::SoaFloat3::Load(
      ozz::math::simd_float4::Load(0.f, 1.f, 2.f, 3.f),
      ozz::math::simd_float4::Load(4.f, 5.f, 6.f, 7.f),
      ozz::math::simd_float4::Load(8.f, 9.f, 10.f, 11.f));
  input_transforms[0][0].rotation = ozz::math::SoaQuaternion::Load(
      ozz::math::simd_float4::Load(.70710677f, 0.f, 0.f, .382683432f),
      ozz::math::simd_float4::Load(0.f, 0.f, .70710677f, 0.f),
      ozz::math::simd_float4::Load(0.f, 0.f, 0.f, 0.f),
      ozz::math::simd_float4::Load(.70710677f, 1.f, -.70710677f, .9238795f));
  input_transforms[0][0].scale = ozz::math::SoaFloat3::Load(
      ozz::math::simd_float4::Load(12.f, 13.f, 14.f, 15.f),
      ozz::math::simd_float4::Load(16.f, 17.f, 18.f, 19.f),
      ozz::math::simd_float4::Load(20.f, 21.f, 22.f, 23.f));
  input_transforms[1][0].translation = -input_transforms[0][0].translation;
  input_transforms[1][0].rotation = Conjugate(input_transforms[0][0].rotation);
  input_transforms[1][0].scale = -input_transforms[0][0].scale;

  // Initialize bind pose.
  ozz::math::SoaTransform bind_poses[1] = {identity};

  {
    BlendingJob::Layer layers[1];
    layers[0].transform = input_transforms[0];

    ozz::math::SoaTransform output_transforms[1];

    BlendingJob job;
    job.additive_layers = layers;
    job.bind_pose = bind_poses;
    job.output = output_transforms;

    // No weight for the 1st layer.
    layers[0].weight = 0.f;
    EXPECT_TRUE(job.Run());

    EXPECT_SOAFLOAT3_EQ(output_transforms[0].translation, 0.f, 0.f, 0.f, 0.f,
                        0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f);
    EXPECT_SOAQUATERNION_EQ_EST(output_transforms[0].rotation, 0.f, 0.f, 0.f,
                                0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f,
                                1.f, 1.f, 1.f, 1.f);
    EXPECT_SOAFLOAT3_EQ(output_transforms[0].scale, 1.f, 1.f, 1.f, 1.f, 1.f,
                        1.f, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f);

    // .5 weight for the 1st layer.
    layers[0].weight = .5f;
    EXPECT_TRUE(job.Run());

    EXPECT_SOAFLOAT3_EQ(output_transforms[0].translation, 0.f, .5f, 1.f, 1.5f,
                        2.f, 2.5f, 3.f, 3.5f, 4.f, 4.5f, 5.f, 5.5f);
    EXPECT_SOAQUATERNION_EQ_EST(output_transforms[0].rotation, .3826834f, 0.f,
                                0.f, .19509032f, 0.f, 0.f, -.3826834f, 0.f, 0.f,
                                0.f, 0.f, 0.f, .9238795f, 1.f, .9238795f,
                                .98078528f);
    EXPECT_SOAFLOAT3_EQ(output_transforms[0].scale, 6.5f, 7.f, 7.5f, 8.f, 8.5f,
                        9.f, 9.5f, 10.f, 10.5f, 11.f, 11.5f, 12.f);

    // Full weight for the 1st layer.
    layers[0].weight = 1.f;
    EXPECT_TRUE(job.Run());

    EXPECT_SOAFLOAT3_EQ(output_transforms[0].translation, 0.f, 1.f, 2.f, 3.f,
                        4.f, 5.f, 6.f, 7.f, 8.f, 9.f, 10.f, 11.f);
    EXPECT_SOAQUATERNION_EQ_EST(output_transforms[0].rotation, .70710677f, 0.f,
                                0.f, .382683432f, 0.f, 0.f, -.70710677f, 0.f,
                                0.f, 0.f, 0.f, 0.f, .70710677f, 1.f, .70710677f,
                                .9238795f);
    EXPECT_SOAFLOAT3_EQ(output_transforms[0].scale, 12.f, 13.f, 14.f, 15.f,
                        16.f, 17.f, 18.f, 19.f, 20.f, 21.f, 22.f, 23.f);
  }

  {
    BlendingJob::Layer layers[2];
    layers[0].transform = input_transforms[0];
    layers[1].transform = input_transforms[1];

    ozz::math::SoaTransform output_transforms[1];

    BlendingJob job;
    job.additive_layers = layers;
    job.bind_pose = bind_poses;
    job.output = output_transforms;

    // No weight for the 1st layer.
    layers[0].weight = 0.f;
    layers[1].weight = 1.f;

    EXPECT_TRUE(job.Run());

    EXPECT_SOAFLOAT3_EQ(output_transforms[0].translation, -0.f, -1.f, -2.f,
                        -3.f, -4.f, -5.f, -6.f, -7.f, -8.f, -9.f, -10.f, -11.f);
    EXPECT_SOAQUATERNION_EQ_EST(output_transforms[0].rotation, -.70710677f,
                                -0.f, -0.f, -.382683432f, -0.f, -0.f,
                                .70710677f, -0.f, -0.f, -0.f, -0.f, -0.f,
                                .70710677f, 1.f, .70710677f, .9238795f);
    EXPECT_SOAFLOAT3_EQ(output_transforms[0].scale, -12.f, -13.f, -14.f, -15.f,
                        -16.f, -17.f, -18.f, -19.f, -20.f, -21.f, -22.f, -23.f);

    // Full weight for the both layer.
    layers[0].weight = 1.f;
    layers[1].weight = 1.f;

    EXPECT_TRUE(job.Run());

    EXPECT_SOAFLOAT3_EQ(output_transforms[0].translation, 0.f, 0.f, 0.f, 0.f,
                        0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f);
    EXPECT_SOAQUATERNION_EQ_EST(output_transforms[0].rotation, 0.f, 0.f, 0.f,
                                0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f,
                                1.f, 1.f, 1.f, 1.f);
    EXPECT_SOAFLOAT3_EQ(output_transforms[0].scale, -144.f, -169.f, -196.f,
                        -225.f, -256.f, -289.f, -324.f, -361.f, -400.f, -441.f,
                        -484.f, -529.f);

    // Subtract second layer.
    layers[0].weight = .5f;
    layers[1].transform = input_transforms[0];
    layers[1].weight = -.5f;

    EXPECT_TRUE(job.Run());

    EXPECT_SOAFLOAT3_EQ(output_transforms[0].translation, 0.f, 0.f, 0.f, 0.f,
                        0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f);
    EXPECT_SOAQUATERNION_EQ_EST(output_transforms[0].rotation, 0.f, 0.f, 0.f,
                                0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f,
                                1.f, 1.f, 1.f, 1.f);
    EXPECT_SOAFLOAT3_EQ_EST(output_transforms[0].scale, 1.f, 1.f, 1.f, 1.f, 1.f,
                            1.f, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f);
  }
}

TEST(AdditiveJointWeight, BlendingJob) {
  const ozz::math::SoaTransform identity = ozz::math::SoaTransform::identity();

  // Initialize inputs.
  ozz::math::SoaTransform input_transforms[1] = {identity};
  input_transforms[0].translation = ozz::math::SoaFloat3::Load(
      ozz::math::simd_float4::Load(0.f, 1.f, 2.f, 3.f),
      ozz::math::simd_float4::Load(4.f, 5.f, 6.f, 7.f),
      ozz::math::simd_float4::Load(8.f, 9.f, 10.f, 11.f));
  input_transforms[0].rotation = ozz::math::SoaQuaternion::Load(
      ozz::math::simd_float4::Load(.70710677f, 0.f, 0.f, .382683432f),
      ozz::math::simd_float4::Load(0.f, 0.f, .70710677f, 0.f),
      ozz::math::simd_float4::Load(0.f, 0.f, 0.f, 0.f),
      ozz::math::simd_float4::Load(.70710677f, 1.f, -.70710677f, .9238795f));
  input_transforms[0].scale = ozz::math::SoaFloat3::Load(
      ozz::math::simd_float4::Load(12.f, 13.f, 14.f, 15.f),
      ozz::math::simd_float4::Load(16.f, 17.f, 18.f, 19.f),
      ozz::math::simd_float4::Load(20.f, 21.f, 22.f, 23.f));

  ozz::math::SimdFloat4 joint_weights[1] = {
      ozz::math::simd_float4::Load(1.f, .5f, 0.f, -1.f)};

  // Initialize bind pose.
  ozz::math::SoaTransform bind_poses[1] = {identity};

  {
    BlendingJob::Layer layers[1];
    layers[0].transform = input_transforms;
    layers[0].joint_weights = joint_weights;

    ozz::math::SoaTransform output_transforms[1];

    BlendingJob job;
    job.additive_layers = layers;
    job.bind_pose = bind_poses;
    job.output = output_transforms;

    // No weight for the 1st layer.
    layers[0].weight = 0.f;
    EXPECT_TRUE(job.Run());

    EXPECT_SOAFLOAT3_EQ(output_transforms[0].translation, 0.f, 0.f, 0.f, 0.f,
                        0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f);
    EXPECT_SOAQUATERNION_EQ_EST(output_transforms[0].rotation, 0.f, 0.f, 0.f,
                                0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f,
                                1.f, 1.f, 1.f, 1.f);
    EXPECT_SOAFLOAT3_EQ(output_transforms[0].scale, 1.f, 1.f, 1.f, 1.f, 1.f,
                        1.f, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f);

    // .5 weight for the 1st layer.
    layers[0].weight = .5f;
    EXPECT_TRUE(job.Run());

    EXPECT_SOAFLOAT3_EQ(output_transforms[0].translation, 0.f, .25f, 0.f, 0.f,
                        2.f, 1.25f, 0.f, 0.f, 4.f, 2.25f, 0.f, 0.f);
    EXPECT_SOAQUATERNION_EQ_EST(output_transforms[0].rotation, .3826834f, 0.f,
                                0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f,
                                0.f, .9238795f, 1.f, 1.f, 1.f);
    EXPECT_SOAFLOAT3_EQ(output_transforms[0].scale, 6.5f, 4.f, 1.f, 1.f, 8.5f,
                        5.f, 1.f, 1.f, 10.5f, 6.f, 1.f, 1.f);

    // Full weight for the 1st layer.
    layers[0].weight = 1.f;
    EXPECT_TRUE(job.Run());

    EXPECT_SOAFLOAT3_EQ(output_transforms[0].translation, 0.f, .5f, 0.f, 0.f,
                        4.f, 2.5f, 0.f, 0.f, 8.f, 4.5f, 0.f, 0.f);
    EXPECT_SOAQUATERNION_EQ_EST(output_transforms[0].rotation, .70710677f, 0.f,
                                0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f,
                                0.f, .70710677f, 1.f, 1.f, 1.f);
    EXPECT_SOAFLOAT3_EQ(output_transforms[0].scale, 12.f, 7.f, 1.f, 1.f, 16.f,
                        9.f, 1.f, 1.f, 20.f, 11.f, 1.f, 1.f);

    // Subtract layer.
    layers[0].weight = -1.f;

    EXPECT_TRUE(job.Run());

    EXPECT_SOAFLOAT3_EQ(output_transforms[0].translation, -0.f, -.5f, 0.f, 0.f,
                        -4.f, -2.5f, 0.f, 0.f, -8.f, -4.5f, 0.f, 0.f);
    EXPECT_SOAQUATERNION_EQ_EST(output_transforms[0].rotation, -.70710677f, 0.f,
                                0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f,
                                0.f, .70710677f, 1.f, 1.f, 1.f);
    EXPECT_SOAFLOAT3_EQ_EST(output_transforms[0].scale, 1.f / 12.f, 1.f / 7.f,
                            1.f, 1.f, 1.f / 16.f, 1.f / 9.f, 1.f, 1.f,
                            1.f / 20.f, 1.f / 11.f, 1.f, 1.f);
  }
}
