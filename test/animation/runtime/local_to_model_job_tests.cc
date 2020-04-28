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
#include "ozz/animation/offline/raw_skeleton.h"
#include "ozz/animation/offline/skeleton_builder.h"
#include "ozz/animation/runtime/local_to_model_job.h"
#include "ozz/animation/runtime/skeleton.h"
#include "ozz/base/maths/gtest_math_helper.h"
#include "ozz/base/maths/soa_transform.h"
#include "ozz/base/memory/unique_ptr.h"

using ozz::animation::LocalToModelJob;
using ozz::animation::Skeleton;
using ozz::animation::offline::RawSkeleton;
using ozz::animation::offline::SkeletonBuilder;

TEST(JobValidity, LocalToModel) {
  RawSkeleton raw_skeleton;
  SkeletonBuilder builder;

  // Empty skeleton.
  ozz::unique_ptr<Skeleton> empty_skeleton(builder(raw_skeleton));
  ASSERT_TRUE(empty_skeleton);

  // Adds 2 joints.
  raw_skeleton.roots.resize(1);
  RawSkeleton::Joint& root = raw_skeleton.roots[0];
  root.name = "root";
  root.children.resize(1);

  ozz::unique_ptr<Skeleton> skeleton(builder(raw_skeleton));
  ASSERT_TRUE(skeleton);

  ozz::math::SoaTransform input[2] = {ozz::math::SoaTransform::identity(),
                                      ozz::math::SoaTransform::identity()};
  ozz::math::Float4x4 output[5];

  // Default job
  {
    LocalToModelJob job;
    EXPECT_FALSE(job.Validate());
    EXPECT_FALSE(job.Run());
  }

  // nullptr output
  {
    LocalToModelJob job;
    job.skeleton = skeleton.get();
    job.input = {input, input + 1};
    EXPECT_FALSE(job.Validate());
    EXPECT_FALSE(job.Run());
  }
  // nullptr input
  {
    LocalToModelJob job;
    job.skeleton = skeleton.get();
    job.output = {output, output + 2};
    EXPECT_FALSE(job.Validate());
    EXPECT_FALSE(job.Run());
  }
  // Null skeleton.
  {
    LocalToModelJob job;
    job.input = {input, input + 1};
    job.output = {output, output + 4};
    EXPECT_FALSE(job.Validate());
    EXPECT_FALSE(job.Run());
  }
  // Invalid output range: end < begin.
  {
    LocalToModelJob job;
    job.skeleton = skeleton.get();
    job.input = {input, input + 1};
    job.output = {output, output + 1};
    EXPECT_FALSE(job.Validate());
    EXPECT_FALSE(job.Run());
  }
  // Invalid output range: too small.
  {
    LocalToModelJob job;
    job.skeleton = skeleton.get();
    job.input = {input, input + 1};
    job.output = {output, output + 1};
    EXPECT_FALSE(job.Validate());
    EXPECT_FALSE(job.Run());
  }
  // Invalid input range: too small.
  {
    LocalToModelJob job;
    job.skeleton = skeleton.get();
    job.input = {input, input + 0};
    job.output = output;
    EXPECT_FALSE(job.Validate());
    EXPECT_FALSE(job.Run());
  }
  // Valid job.
  {
    LocalToModelJob job;
    job.skeleton = skeleton.get();
    job.input = input;
    job.output = {output, output + 2};
    EXPECT_TRUE(job.Validate());
    EXPECT_TRUE(job.Run());
  }
  // Valid job with root matrix.
  {
    LocalToModelJob job;
    const ozz::math::SimdFloat4 v =
        ozz::math::simd_float4::Load(4.f, 3.f, 2.f, 1.f);
    ozz::math::Float4x4 world = ozz::math::Float4x4::Translation(v);
    job.skeleton = skeleton.get();
    job.root = &world;
    job.input = input;
    job.output = {output, output + 2};
    EXPECT_TRUE(job.Validate());
    EXPECT_TRUE(job.Run());
  }
  // Valid out-of-bound from.
  {
    LocalToModelJob job;
    job.skeleton = skeleton.get();
    job.from = 93;
    job.input = input;
    job.output = {output, output + 2};
    EXPECT_TRUE(job.Validate());
    EXPECT_TRUE(job.Run());
  }
  // Valid out-of-bound from.
  {
    LocalToModelJob job;
    job.skeleton = skeleton.get();
    job.from = -93;
    job.input = input;
    job.output = {output, output + 2};
    EXPECT_TRUE(job.Validate());
    EXPECT_TRUE(job.Run());
  }
  // Valid out-of-bound to.
  {
    LocalToModelJob job;
    job.skeleton = skeleton.get();
    job.from = 93;
    job.input = input;
    job.output = {output, output + 2};
    EXPECT_TRUE(job.Validate());
    EXPECT_TRUE(job.Run());
  }
  // Valid out-of-bound to.
  {
    LocalToModelJob job;
    job.skeleton = skeleton.get();
    job.from = -93;
    job.input = input;
    job.output = {output, output + 2};
    EXPECT_TRUE(job.Validate());
    EXPECT_TRUE(job.Run());
  }
  // Valid job with empty skeleton.
  {
    LocalToModelJob job;
    job.skeleton = empty_skeleton.get();
    job.input = {input, input + 0};
    job.output = {output, output + 0};
    EXPECT_TRUE(job.Validate());
    EXPECT_TRUE(job.Run());
  }
  // Valid job. Bigger input & output
  {
    LocalToModelJob job;
    job.skeleton = skeleton.get();
    job.input = input;
    job.output = output;
    EXPECT_TRUE(job.Validate());
    EXPECT_TRUE(job.Run());
  }
}

TEST(Transformation, LocalToModel) {
  // Builds the skeleton
  /*
   6 joints
     j0
    /  \
   j1  j3
    |  / \
   j2 j4 j5
  */
  RawSkeleton raw_skeleton;
  raw_skeleton.roots.resize(1);
  RawSkeleton::Joint& root = raw_skeleton.roots[0];
  root.name = "j0";

  root.children.resize(2);
  root.children[0].name = "j1";
  root.children[1].name = "j3";

  root.children[0].children.resize(1);
  root.children[0].children[0].name = "j2";

  root.children[1].children.resize(2);
  root.children[1].children[0].name = "j4";
  root.children[1].children[1].name = "j5";

  EXPECT_TRUE(raw_skeleton.Validate());
  EXPECT_EQ(raw_skeleton.num_joints(), 6);

  SkeletonBuilder builder;
  ozz::unique_ptr<Skeleton> skeleton(builder(raw_skeleton));
  ASSERT_TRUE(skeleton);

  // Initializes an input transformation.
  ozz::math::SoaTransform input[2] = {
      // Stores up to 8 inputs, needs 6.
      {{ozz::math::simd_float4::Load(2.f, 0.f, 1.f, -2.f),
        ozz::math::simd_float4::Load(2.f, 0.f, 2.f, -2.f),
        ozz::math::simd_float4::Load(2.f, 0.f, 4.f, -2.f)},
       {ozz::math::simd_float4::Load(0.f, 0.f, 0.f, 0.f),
        ozz::math::simd_float4::Load(0.f, .70710677f, 0.f, 0.f),
        ozz::math::simd_float4::Load(0.f, 0.f, 0.f, 0.f),
        ozz::math::simd_float4::Load(1.f, .70710677f, 1.f, 1.f)},
       {ozz::math::simd_float4::Load(1.f, 1.f, 1.f, 10.f),
        ozz::math::simd_float4::Load(1.f, 1.f, 1.f, 10.f),
        ozz::math::simd_float4::Load(1.f, 1.f, 1.f, 10.f)}},
      {{ozz::math::simd_float4::Load(12.f, 0.f, 0.f, 0.f),
        ozz::math::simd_float4::Load(46.f, 0.f, 0.f, 0.f),
        ozz::math::simd_float4::Load(-12.f, 0.f, 0.f, 0.f)},
       {ozz::math::simd_float4::Load(0.f, 0.f, 0.f, 0.f),
        ozz::math::simd_float4::Load(0.f, 0.f, 0.f, 0.f),
        ozz::math::simd_float4::Load(0.f, 0.f, 0.f, 0.f),
        ozz::math::simd_float4::Load(1.f, 1.f, 1.f, 1.f)},
       {ozz::math::simd_float4::Load(1.f, -.1f, 1.f, 1.f),
        ozz::math::simd_float4::Load(1.f, -.1f, 1.f, 1.f),
        ozz::math::simd_float4::Load(1.f, -.1f, 1.f, 1.f)}}};

  {
    // Prepares the job with root == nullptr (default identity matrix)
    ozz::math::Float4x4 output[6];
    LocalToModelJob job;
    job.skeleton = skeleton.get();
    job.input = input;
    job.output = output;
    EXPECT_TRUE(job.Validate());
    EXPECT_TRUE(job.Run());
    EXPECT_FLOAT4x4_EQ(output[0], 1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f,
                       0.f, 1.f, 0.f, 2.f, 2.f, 2.f, 1.f);
    EXPECT_FLOAT4x4_EQ(output[1], 0.f, 0.f, -1.f, 0.f, 0.f, 1.f, 0.f, 0.f, 1.f,
                       0.f, 0.f, 0.f, 2.f, 2.f, 2.f, 1.f);
    EXPECT_FLOAT4x4_EQ(output[2], 0.f, 0.f, -1.f, 0.f, 0.f, 1.f, 0.f, 0.f, 1.f,
                       0.f, 0.f, 0.f, 6.f, 4.f, 1.f, 1.f);
    EXPECT_FLOAT4x4_EQ(output[3], 10.f, 0.f, 0.f, 0.f, 0.f, 10.f, 0.f, 0.f, 0.f,
                       0.f, 10.f, 0.f, 0.f, 0.f, 0.f, 1.f);
    EXPECT_FLOAT4x4_EQ(output[4], 10.f, 0.f, 0.f, 0.f, 0.f, 10.f, 0.f, 0.f, 0.f,
                       0.f, 10.f, 0.f, 120.f, 460.f, -120.f, 1.f);
    EXPECT_FLOAT4x4_EQ(output[5], -1.f, 0.f, 0.f, 0.f, 0.f, -1.f, 0.f, 0.f, 0.f,
                       0.f, -1.f, 0.f, 0.f, 0.f, 0.f, 1.f);
  }

  {
    // Prepares the job with root == Translation(4,3,2,1)
    ozz::math::Float4x4 output[6];
    const ozz::math::SimdFloat4 v =
        ozz::math::simd_float4::Load(4.f, 3.f, 2.f, 1.f);
    ozz::math::Float4x4 world = ozz::math::Float4x4::Translation(v);
    LocalToModelJob job;
    job.skeleton = skeleton.get();
    job.root = &world;
    job.input = input;
    job.output = output;
    EXPECT_TRUE(job.Validate());
    EXPECT_TRUE(job.Run());
    EXPECT_FLOAT4x4_EQ(output[0], 1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f,
                       0.f, 1.f, 0.f, 6.f, 5.f, 4.f, 1.f);
    EXPECT_FLOAT4x4_EQ(output[1], 0.f, 0.f, -1.f, 0.f, 0.f, 1.f, 0.f, 0.f, 1.f,
                       0.f, 0.f, 0.f, 6.f, 5.f, 4.f, 1.f);
    EXPECT_FLOAT4x4_EQ(output[2], 0.f, 0.f, -1.f, 0.f, 0.f, 1.f, 0.f, 0.f, 1.f,
                       0.f, 0.f, 0.f, 10.f, 7.f, 3.f, 1.f);
    EXPECT_FLOAT4x4_EQ(output[3], 10.f, 0.f, 0.f, 0.f, 0.f, 10.f, 0.f, 0.f, 0.f,
                       0.f, 10.f, 0.f, 4.f, 3.f, 2.f, 1.f);
    EXPECT_FLOAT4x4_EQ(output[4], 10.f, 0.f, 0.f, 0.f, 0.f, 10.f, 0.f, 0.f, 0.f,
                       0.f, 10.f, 0.f, 124.f, 463.f, -118.f, 1.f);
    EXPECT_FLOAT4x4_EQ(output[5], -1.f, 0.f, 0.f, 0.f, 0.f, -1.f, 0.f, 0.f, 0.f,
                       0.f, -1.f, 0.f, 4.f, 3.f, 2.f, 1.f);
  }
}

TEST(TransformationFromTo, LocalToModel) {
  // Builds the skeleton
  /*
   6 joints
         *
       /   \
     j0    j7
    /  \
   j1  j3
    |  / \
   j2 j4 j6
       |
      j5
  */
  RawSkeleton raw_skeleton;
  raw_skeleton.roots.resize(2);
  RawSkeleton::Joint& j0 = raw_skeleton.roots[0];
  j0.name = "j0";
  RawSkeleton::Joint& j7 = raw_skeleton.roots[1];
  j7.name = "j7";

  j0.children.resize(2);
  j0.children[0].name = "j1";
  j0.children[1].name = "j3";

  j0.children[0].children.resize(1);
  j0.children[0].children[0].name = "j2";

  j0.children[1].children.resize(2);
  j0.children[1].children[0].name = "j4";
  j0.children[1].children[1].name = "j6";

  j0.children[1].children[0].children.resize(1);
  j0.children[1].children[0].children[0].name = "j5";

  EXPECT_TRUE(raw_skeleton.Validate());
  EXPECT_EQ(raw_skeleton.num_joints(), 8);

  SkeletonBuilder builder;
  ozz::unique_ptr<Skeleton> skeleton(builder(raw_skeleton));
  ASSERT_TRUE(skeleton);

  // Initializes an input transformation.
  ozz::math::SoaTransform input[2] = {
      // Stores up to 8 inputs, needs 7.
      //                             j0   j1   j2    j3
      {{ozz::math::simd_float4::Load(2.f, 0.f, -2.f, 1.f),
        ozz::math::simd_float4::Load(2.f, 0.f, -2.f, 2.f),
        ozz::math::simd_float4::Load(2.f, 0.f, -2.f, 4.f)},
       {ozz::math::simd_float4::Load(0.f, 0.f, 0.f, 0.f),
        ozz::math::simd_float4::Load(0.f, .70710677f, 0.f, 0.f),
        ozz::math::simd_float4::Load(0.f, 0.f, 0.f, 0.f),
        ozz::math::simd_float4::Load(1.f, .70710677f, 1.f, 1.f)},
       {ozz::math::simd_float4::Load(1.f, 1.f, 10.f, 1.f),
        ozz::math::simd_float4::Load(1.f, 1.f, 10.f, 1.f),
        ozz::math::simd_float4::Load(1.f, 1.f, 10.f, 1.f)}},
      //                             j4    j5   j6   j7.
      {{ozz::math::simd_float4::Load(12.f, 0.f, 3.f, 6.f),
        ozz::math::simd_float4::Load(46.f, 0.f, 4.f, 7.f),
        ozz::math::simd_float4::Load(-12.f, 0.f, 5.f, 8.f)},
       {ozz::math::simd_float4::Load(0.f, 0.f, 0.f, 0.f),
        ozz::math::simd_float4::Load(0.f, 0.f, 0.f, 0.f),
        ozz::math::simd_float4::Load(0.f, 0.f, 0.f, 0.f),
        ozz::math::simd_float4::Load(1.f, 1.f, 1.f, 1.f)},
       {ozz::math::simd_float4::Load(1.f, -.1f, 1.f, 1.f),
        ozz::math::simd_float4::Load(1.f, -.1f, 1.f, 1.f),
        ozz::math::simd_float4::Load(1.f, -.1f, 1.f, 1.f)}}};

  ozz::math::Float4x4 output[8];
  LocalToModelJob job_full;
  {  // Intialize whole hierarchy output
    job_full.skeleton = skeleton.get();
    job_full.from = ozz::animation::Skeleton::kNoParent;
    job_full.input = input;
    job_full.output = output;
    EXPECT_TRUE(job_full.Validate());
    EXPECT_TRUE(job_full.Run());
    EXPECT_FLOAT4x4_EQ(output[0], 1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f,
                       0.f, 1.f, 0.f, 2.f, 2.f, 2.f, 1.f);
    EXPECT_FLOAT4x4_EQ(output[1], 0.f, 0.f, -1.f, 0.f, 0.f, 1.f, 0.f, 0.f, 1.f,
                       0.f, 0.f, 0.f, 2.f, 2.f, 2.f, 1.f);
    EXPECT_FLOAT4x4_EQ(output[2], 0.f, 0.f, -10.f, 0.f, 0.f, 10.f, 0.f, 0.f,
                       10.f, 0.f, 0.f, 0.f, 0.f, 0.f, 4.f, 1.f);
    EXPECT_FLOAT4x4_EQ(output[3], 1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f,
                       0.f, 1.f, 0.f, 3.f, 4.f, 6.f, 1.f);
    EXPECT_FLOAT4x4_EQ(output[4], 1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f,
                       0.f, 1.f, 0.f, 15.f, 50.f, -6.f, 1.f);
    EXPECT_FLOAT4x4_EQ(output[5], -.1f, 0.f, 0.f, 0.f, 0.f, -.1f, 0.f, 0.f, 0.f,
                       0.f, -.1f, 0.f, 15.f, 50.f, -6.f, 1.f);
    EXPECT_FLOAT4x4_EQ(output[6], 1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f,
                       0.f, 1.f, 0.f, 6.f, 8.f, 11.f, 1.f);
    EXPECT_FLOAT4x4_EQ(output[7], 1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f,
                       0.f, 1.f, 0.f, 6.f, 7.f, 8.f, 1.f);
  }

  {  // Updates from j0, j7 shouldn't be updated
    output[0] = output[1] = output[2] = output[3] = output[4] = output[5] =
        output[6] = output[7] = ozz::math::Float4x4::identity();

    LocalToModelJob job;
    job.skeleton = skeleton.get();
    job.from = 0;
    job.input = input;
    job.output = output;
    EXPECT_TRUE(job.Validate());
    EXPECT_TRUE(job.Run());

    EXPECT_FLOAT4x4_EQ(output[0], 1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f,
                       0.f, 1.f, 0.f, 2.f, 2.f, 2.f, 1.f);
    EXPECT_FLOAT4x4_EQ(output[1], 0.f, 0.f, -1.f, 0.f, 0.f, 1.f, 0.f, 0.f, 1.f,
                       0.f, 0.f, 0.f, 2.f, 2.f, 2.f, 1.f);
    EXPECT_FLOAT4x4_EQ(output[2], 0.f, 0.f, -10.f, 0.f, 0.f, 10.f, 0.f, 0.f,
                       10.f, 0.f, 0.f, 0.f, 0.f, 0.f, 4.f, 1.f);
    EXPECT_FLOAT4x4_EQ(output[3], 1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f,
                       0.f, 1.f, 0.f, 3.f, 4.f, 6.f, 1.f);
    EXPECT_FLOAT4x4_EQ(output[4], 1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f,
                       0.f, 1.f, 0.f, 15.f, 50.f, -6.f, 1.f);
    EXPECT_FLOAT4x4_EQ(output[5], -.1f, 0.f, 0.f, 0.f, 0.f, -.1f, 0.f, 0.f, 0.f,
                       0.f, -.1f, 0.f, 15.f, 50.f, -6.f, 1.f);
    EXPECT_FLOAT4x4_EQ(output[6], 1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f,
                       0.f, 1.f, 0.f, 6.f, 8.f, 11.f, 1.f);
    EXPECT_FLOAT4x4_EQ(output[7], 1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f,
                       0.f, 1.f, 0.f, 0.f, 0.f, 0.f, 1.f);
  }

  {  // Updates from j7, j0-6 shouldn't be updated
    output[0] = output[1] = output[2] = output[3] = output[4] = output[5] =
        output[6] = output[7] = ozz::math::Float4x4::identity();

    LocalToModelJob job;
    job.skeleton = skeleton.get();
    job.from = 7;
    job.input = input;
    job.output = output;
    EXPECT_TRUE(job.Validate());
    EXPECT_TRUE(job.Run());

    EXPECT_FLOAT4x4_EQ(output[0], 1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f,
                       0.f, 1.f, 0.f, 0.f, 0.f, 0.f, 1.f);
    EXPECT_FLOAT4x4_EQ(output[1], 1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f,
                       0.f, 1.f, 0.f, 0.f, 0.f, 0.f, 1.f);
    EXPECT_FLOAT4x4_EQ(output[2], 1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f,
                       0.f, 1.f, 0.f, 0.f, 0.f, 0.f, 1.f);
    EXPECT_FLOAT4x4_EQ(output[3], 1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f,
                       0.f, 1.f, 0.f, 0.f, 0.f, 0.f, 1.f);
    EXPECT_FLOAT4x4_EQ(output[4], 1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f,
                       0.f, 1.f, 0.f, 0.f, 0.f, 0.f, 1.f);
    EXPECT_FLOAT4x4_EQ(output[5], 1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f,
                       0.f, 1.f, 0.f, 0.f, 0.f, 0.f, 1.f);
    EXPECT_FLOAT4x4_EQ(output[6], 1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f,
                       0.f, 1.f, 0.f, 0.f, 0.f, 0.f, 1.f);
    EXPECT_FLOAT4x4_EQ(output[7], 1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f,
                       0.f, 1.f, 0.f, 6.f, 7.f, 8.f, 1.f);
  }

  {  // Updates from j1, j1-2 should be updated
    EXPECT_TRUE(job_full.Run());
    output[1] = output[2] = output[3] = output[4] = output[5] = output[6] =
        output[7] = ozz::math::Float4x4::identity();

    LocalToModelJob job;
    job.skeleton = skeleton.get();
    job.from = 1;
    job.input = input;
    job.output = output;
    EXPECT_TRUE(job.Validate());
    EXPECT_TRUE(job.Run());

    EXPECT_FLOAT4x4_EQ(output[0], 1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f,
                       0.f, 1.f, 0.f, 2.f, 2.f, 2.f, 1.f);
    EXPECT_FLOAT4x4_EQ(output[1], 0.f, 0.f, -1.f, 0.f, 0.f, 1.f, 0.f, 0.f, 1.f,
                       0.f, 0.f, 0.f, 2.f, 2.f, 2.f, 1.f);
    EXPECT_FLOAT4x4_EQ(output[2], 0.f, 0.f, -10.f, 0.f, 0.f, 10.f, 0.f, 0.f,
                       10.f, 0.f, 0.f, 0.f, 0.f, 0.f, 4.f, 1.f);
    EXPECT_FLOAT4x4_EQ(output[3], 1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f,
                       0.f, 1.f, 0.f, 0.f, 0.f, 0.f, 1.f);
    EXPECT_FLOAT4x4_EQ(output[4], 1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f,
                       0.f, 1.f, 0.f, 0.f, 0.f, 0.f, 1.f);
    EXPECT_FLOAT4x4_EQ(output[5], 1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f,
                       0.f, 1.f, 0.f, 0.f, 0.f, 0.f, 1.f);
    EXPECT_FLOAT4x4_EQ(output[6], 1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f,
                       0.f, 1.f, 0.f, 0.f, 0.f, 0.f, 1.f);
    EXPECT_FLOAT4x4_EQ(output[7], 1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f,
                       0.f, 1.f, 0.f, 0.f, 0.f, 0.f, 1.f);
  }

  {  // Updates from j3, j3-6 should be updated
    EXPECT_TRUE(job_full.Run());
    output[1] = output[2] = output[3] = output[4] = output[5] = output[6] =
        output[7] = ozz::math::Float4x4::identity();

    LocalToModelJob job;
    job.skeleton = skeleton.get();
    job.from = 3;
    job.input = input;
    job.output = output;
    EXPECT_TRUE(job.Validate());
    EXPECT_TRUE(job.Run());

    EXPECT_FLOAT4x4_EQ(output[0], 1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f,
                       0.f, 1.f, 0.f, 2.f, 2.f, 2.f, 1.f);
    EXPECT_FLOAT4x4_EQ(output[1], 1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f,
                       0.f, 1.f, 0.f, 0.f, 0.f, 0.f, 1.f);
    EXPECT_FLOAT4x4_EQ(output[2], 1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f,
                       0.f, 1.f, 0.f, 0.f, 0.f, 0.f, 1.f);
    EXPECT_FLOAT4x4_EQ(output[3], 1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f,
                       0.f, 1.f, 0.f, 3.f, 4.f, 6.f, 1.f);
    EXPECT_FLOAT4x4_EQ(output[4], 1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f,
                       0.f, 1.f, 0.f, 15.f, 50.f, -6.f, 1.f);
    EXPECT_FLOAT4x4_EQ(output[5], -.1f, 0.f, 0.f, 0.f, 0.f, -.1f, 0.f, 0.f, 0.f,
                       0.f, -.1f, 0.f, 15.f, 50.f, -6.f, 1.f);
    EXPECT_FLOAT4x4_EQ(output[6], 1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f,
                       0.f, 1.f, 0.f, 6.f, 8.f, 11.f, 1.f);
    EXPECT_FLOAT4x4_EQ(output[7], 1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f,
                       0.f, 1.f, 0.f, 0.f, 0.f, 0.f, 1.f);
  }

  {  // Updates from j5, j5 should only be updated
    EXPECT_TRUE(job_full.Run());
    output[0] = output[1] = output[2] = output[3] = output[5] = output[6] =
        output[7] = ozz::math::Float4x4::identity();

    LocalToModelJob job;
    job.skeleton = skeleton.get();
    job.from = 5;
    job.input = input;
    job.output = output;
    EXPECT_TRUE(job.Validate());
    EXPECT_TRUE(job.Run());

    EXPECT_FLOAT4x4_EQ(output[0], 1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f,
                       0.f, 1.f, 0.f, 0.f, 0.f, 0.f, 1.f);
    EXPECT_FLOAT4x4_EQ(output[1], 1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f,
                       0.f, 1.f, 0.f, 0.f, 0.f, 0.f, 1.f);
    EXPECT_FLOAT4x4_EQ(output[2], 1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f,
                       0.f, 1.f, 0.f, 0.f, 0.f, 0.f, 1.f);
    EXPECT_FLOAT4x4_EQ(output[3], 1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f,
                       0.f, 1.f, 0.f, 0.f, 0.f, 0.f, 1.f);
    EXPECT_FLOAT4x4_EQ(output[4], 1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f,
                       0.f, 1.f, 0.f, 15.f, 50.f, -6.f, 1.f);
    EXPECT_FLOAT4x4_EQ(output[5], -.1f, 0.f, 0.f, 0.f, 0.f, -.1f, 0.f, 0.f, 0.f,
                       0.f, -.1f, 0.f, 15.f, 50.f, -6.f, 1.f);
    EXPECT_FLOAT4x4_EQ(output[6], 1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f,
                       0.f, 1.f, 0.f, 0.f, 0.f, 0.f, 1.f);
    EXPECT_FLOAT4x4_EQ(output[7], 1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f,
                       0.f, 1.f, 0.f, 0.f, 0.f, 0.f, 1.f);
  }

  {  // Updates from j6, j6 should only be updated
    EXPECT_TRUE(job_full.Run());
    output[0] = output[1] = output[2] = output[4] = output[5] = output[6] =
        output[7] = ozz::math::Float4x4::identity();

    LocalToModelJob job;
    job.skeleton = skeleton.get();
    job.from = 6;
    job.input = input;
    job.output = output;
    EXPECT_TRUE(job.Validate());
    EXPECT_TRUE(job.Run());

    EXPECT_FLOAT4x4_EQ(output[0], 1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f,
                       0.f, 1.f, 0.f, 0.f, 0.f, 0.f, 1.f);
    EXPECT_FLOAT4x4_EQ(output[1], 1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f,
                       0.f, 1.f, 0.f, 0.f, 0.f, 0.f, 1.f);
    EXPECT_FLOAT4x4_EQ(output[2], 1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f,
                       0.f, 1.f, 0.f, 0.f, 0.f, 0.f, 1.f);
    EXPECT_FLOAT4x4_EQ(output[3], 1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f,
                       0.f, 1.f, 0.f, 3.f, 4.f, 6.f, 1.f);
    EXPECT_FLOAT4x4_EQ(output[4], 1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f,
                       0.f, 1.f, 0.f, 0.f, 0.f, 0.f, 1.f);
    EXPECT_FLOAT4x4_EQ(output[5], 1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f,
                       0.f, 1.f, 0.f, 0.f, 0.f, 0.f, 1.f);
    EXPECT_FLOAT4x4_EQ(output[6], 1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f,
                       0.f, 1.f, 0.f, 6.f, 8.f, 11.f, 1.f);
    EXPECT_FLOAT4x4_EQ(output[7], 1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f,
                       0.f, 1.f, 0.f, 0.f, 0.f, 0.f, 1.f);
  }

  {  // Updates from j0 to j2,
    output[0] = output[1] = output[2] = output[3] = output[4] = output[5] =
        output[6] = output[7] = ozz::math::Float4x4::identity();

    LocalToModelJob job;
    job.skeleton = skeleton.get();
    job.from = 0;
    job.to = 2;
    job.input = input;
    job.output = output;
    EXPECT_TRUE(job.Validate());
    EXPECT_TRUE(job.Run());

    EXPECT_FLOAT4x4_EQ(output[0], 1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f,
                       0.f, 1.f, 0.f, 2.f, 2.f, 2.f, 1.f);
    EXPECT_FLOAT4x4_EQ(output[1], 0.f, 0.f, -1.f, 0.f, 0.f, 1.f, 0.f, 0.f, 1.f,
                       0.f, 0.f, 0.f, 2.f, 2.f, 2.f, 1.f);
    EXPECT_FLOAT4x4_EQ(output[2], 0.f, 0.f, -10.f, 0.f, 0.f, 10.f, 0.f, 0.f,
                       10.f, 0.f, 0.f, 0.f, 0.f, 0.f, 4.f, 1.f);
    EXPECT_FLOAT4x4_EQ(output[3], 1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f,
                       0.f, 1.f, 0.f, 0.f, 0.f, 0.f, 1.f);
    EXPECT_FLOAT4x4_EQ(output[4], 1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f,
                       0.f, 1.f, 0.f, 0.f, 0.f, 0.f, 1.f);
    EXPECT_FLOAT4x4_EQ(output[5], 1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f,
                       0.f, 1.f, 0.f, 0.f, 0.f, 0.f, 1.f);
    EXPECT_FLOAT4x4_EQ(output[6], 1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f,
                       0.f, 1.f, 0.f, 0.f, 0.f, 0.f, 1.f);
    EXPECT_FLOAT4x4_EQ(output[7], 1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f,
                       0.f, 1.f, 0.f, 0.f, 0.f, 0.f, 1.f);
  }

  {  // Updates from j0 to j6, j7 shouldn't be updated
    output[0] = output[1] = output[2] = output[3] = output[4] = output[5] =
        output[6] = output[7] = ozz::math::Float4x4::identity();

    LocalToModelJob job;
    job.skeleton = skeleton.get();
    job.from = 0;
    job.to = 6;
    job.input = input;
    job.output = output;
    EXPECT_TRUE(job.Validate());
    EXPECT_TRUE(job.Run());

    EXPECT_FLOAT4x4_EQ(output[0], 1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f,
                       0.f, 1.f, 0.f, 2.f, 2.f, 2.f, 1.f);
    EXPECT_FLOAT4x4_EQ(output[1], 0.f, 0.f, -1.f, 0.f, 0.f, 1.f, 0.f, 0.f, 1.f,
                       0.f, 0.f, 0.f, 2.f, 2.f, 2.f, 1.f);
    EXPECT_FLOAT4x4_EQ(output[2], 0.f, 0.f, -10.f, 0.f, 0.f, 10.f, 0.f, 0.f,
                       10.f, 0.f, 0.f, 0.f, 0.f, 0.f, 4.f, 1.f);
    EXPECT_FLOAT4x4_EQ(output[3], 1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f,
                       0.f, 1.f, 0.f, 3.f, 4.f, 6.f, 1.f);
    EXPECT_FLOAT4x4_EQ(output[4], 1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f,
                       0.f, 1.f, 0.f, 15.f, 50.f, -6.f, 1.f);
    EXPECT_FLOAT4x4_EQ(output[5], -.1f, 0.f, 0.f, 0.f, 0.f, -.1f, 0.f, 0.f, 0.f,
                       0.f, -.1f, 0.f, 15.f, 50.f, -6.f, 1.f);
    EXPECT_FLOAT4x4_EQ(output[6], 1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f,
                       0.f, 1.f, 0.f, 6.f, 8.f, 11.f, 1.f);
    EXPECT_FLOAT4x4_EQ(output[7], 1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f,
                       0.f, 1.f, 0.f, 0.f, 0.f, 0.f, 1.f);
  }

  {  // Updates from j0 to past end, j7 shouldn't be updated
    output[0] = output[1] = output[2] = output[3] = output[4] = output[5] =
        output[6] = output[7] = ozz::math::Float4x4::identity();

    LocalToModelJob job;
    job.skeleton = skeleton.get();
    job.from = 0;
    job.to = 46;
    job.input = input;
    job.output = output;
    EXPECT_TRUE(job.Validate());
    EXPECT_TRUE(job.Run());

    EXPECT_FLOAT4x4_EQ(output[0], 1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f,
                       0.f, 1.f, 0.f, 2.f, 2.f, 2.f, 1.f);
    EXPECT_FLOAT4x4_EQ(output[1], 0.f, 0.f, -1.f, 0.f, 0.f, 1.f, 0.f, 0.f, 1.f,
                       0.f, 0.f, 0.f, 2.f, 2.f, 2.f, 1.f);
    EXPECT_FLOAT4x4_EQ(output[2], 0.f, 0.f, -10.f, 0.f, 0.f, 10.f, 0.f, 0.f,
                       10.f, 0.f, 0.f, 0.f, 0.f, 0.f, 4.f, 1.f);
    EXPECT_FLOAT4x4_EQ(output[3], 1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f,
                       0.f, 1.f, 0.f, 3.f, 4.f, 6.f, 1.f);
    EXPECT_FLOAT4x4_EQ(output[4], 1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f,
                       0.f, 1.f, 0.f, 15.f, 50.f, -6.f, 1.f);
    EXPECT_FLOAT4x4_EQ(output[5], -.1f, 0.f, 0.f, 0.f, 0.f, -.1f, 0.f, 0.f, 0.f,
                       0.f, -.1f, 0.f, 15.f, 50.f, -6.f, 1.f);
    EXPECT_FLOAT4x4_EQ(output[6], 1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f,
                       0.f, 1.f, 0.f, 6.f, 8.f, 11.f, 1.f);
    EXPECT_FLOAT4x4_EQ(output[7], 1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f,
                       0.f, 1.f, 0.f, 0.f, 0.f, 0.f, 1.f);
  }

  {  // Updates from j0 to nowehere, nothing should be updated
    output[0] = output[1] = output[2] = output[3] = output[4] = output[5] =
        output[6] = output[7] = ozz::math::Float4x4::identity();

    LocalToModelJob job;
    job.skeleton = skeleton.get();
    job.from = 0;
    job.to = -99;
    job.input = input;
    job.output = output;
    EXPECT_TRUE(job.Validate());
    EXPECT_TRUE(job.Run());

    EXPECT_FLOAT4x4_EQ(output[0], 1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f,
                       0.f, 1.f, 0.f, 0.f, 0.f, 0.f, 1.f);
    EXPECT_FLOAT4x4_EQ(output[1], 1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f,
                       0.f, 1.f, 0.f, 0.f, 0.f, 0.f, 1.f);
    EXPECT_FLOAT4x4_EQ(output[2], 1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f,
                       0.f, 1.f, 0.f, 0.f, 0.f, 0.f, 1.f);
    EXPECT_FLOAT4x4_EQ(output[3], 1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f,
                       0.f, 1.f, 0.f, 0.f, 0.f, 0.f, 1.f);
    EXPECT_FLOAT4x4_EQ(output[4], 1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f,
                       0.f, 1.f, 0.f, 0.f, 0.f, 0.f, 1.f);
    EXPECT_FLOAT4x4_EQ(output[5], 1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f,
                       0.f, 1.f, 0.f, 0.f, 0.f, 0.f, 1.f);
    EXPECT_FLOAT4x4_EQ(output[6], 1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f,
                       0.f, 1.f, 0.f, 0.f, 0.f, 0.f, 1.f);
    EXPECT_FLOAT4x4_EQ(output[7], 1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f,
                       0.f, 1.f, 0.f, 0.f, 0.f, 0.f, 1.f);
  }

  {  // Updates from out-of-range value, nothing should be updated
    EXPECT_TRUE(job_full.Run());
    output[0] = output[1] = output[2] = output[3] = output[4] = output[5] =
        output[6] = output[7] = ozz::math::Float4x4::identity();

    LocalToModelJob job;
    job.skeleton = skeleton.get();
    job.from = 93;
    job.input = input;
    job.output = output;
    EXPECT_TRUE(job.Validate());
    EXPECT_TRUE(job.Run());

    EXPECT_FLOAT4x4_EQ(output[0], 1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f,
                       0.f, 1.f, 0.f, 0.f, 0.f, 0.f, 1.f);
    EXPECT_FLOAT4x4_EQ(output[1], 1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f,
                       0.f, 1.f, 0.f, 0.f, 0.f, 0.f, 1.f);
    EXPECT_FLOAT4x4_EQ(output[2], 1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f,
                       0.f, 1.f, 0.f, 0.f, 0.f, 0.f, 1.f);
    EXPECT_FLOAT4x4_EQ(output[3], 1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f,
                       0.f, 1.f, 0.f, 0.f, 0.f, 0.f, 1.f);
    EXPECT_FLOAT4x4_EQ(output[4], 1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f,
                       0.f, 1.f, 0.f, 0.f, 0.f, 0.f, 1.f);
    EXPECT_FLOAT4x4_EQ(output[5], 1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f,
                       0.f, 1.f, 0.f, 0.f, 0.f, 0.f, 1.f);
    EXPECT_FLOAT4x4_EQ(output[6], 1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f,
                       0.f, 1.f, 0.f, 0.f, 0.f, 0.f, 1.f);
    EXPECT_FLOAT4x4_EQ(output[7], 1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f,
                       0.f, 1.f, 0.f, 0.f, 0.f, 0.f, 1.f);
  }
}

TEST(TransformationFromToExclude, LocalToModel) {
  // Builds the skeleton
  /*
   6 joints
         *
       /   \
     j0    j7
    /  \
   j1  j3
    |  / \
   j2 j4 j6
       |
      j5
  */
  RawSkeleton raw_skeleton;
  raw_skeleton.roots.resize(2);
  RawSkeleton::Joint& j0 = raw_skeleton.roots[0];
  j0.name = "j0";
  RawSkeleton::Joint& j7 = raw_skeleton.roots[1];
  j7.name = "j7";

  j0.children.resize(2);
  j0.children[0].name = "j1";
  j0.children[1].name = "j3";

  j0.children[0].children.resize(1);
  j0.children[0].children[0].name = "j2";

  j0.children[1].children.resize(2);
  j0.children[1].children[0].name = "j4";
  j0.children[1].children[1].name = "j6";

  j0.children[1].children[0].children.resize(1);
  j0.children[1].children[0].children[0].name = "j5";

  EXPECT_TRUE(raw_skeleton.Validate());
  EXPECT_EQ(raw_skeleton.num_joints(), 8);

  SkeletonBuilder builder;
  ozz::unique_ptr<Skeleton> skeleton(builder(raw_skeleton));
  ASSERT_TRUE(skeleton);

  // Initializes an input transformation.
  ozz::math::SoaTransform input[2] = {
      // Stores up to 8 inputs, needs 7.
      //                             j0   j1   j2    j3
      {{ozz::math::simd_float4::Load(2.f, 0.f, -2.f, 1.f),
        ozz::math::simd_float4::Load(2.f, 0.f, -2.f, 2.f),
        ozz::math::simd_float4::Load(2.f, 0.f, -2.f, 4.f)},
       {ozz::math::simd_float4::Load(0.f, 0.f, 0.f, 0.f),
        ozz::math::simd_float4::Load(0.f, .70710677f, 0.f, 0.f),
        ozz::math::simd_float4::Load(0.f, 0.f, 0.f, 0.f),
        ozz::math::simd_float4::Load(1.f, .70710677f, 1.f, 1.f)},
       {ozz::math::simd_float4::Load(1.f, 1.f, 10.f, 1.f),
        ozz::math::simd_float4::Load(1.f, 1.f, 10.f, 1.f),
        ozz::math::simd_float4::Load(1.f, 1.f, 10.f, 1.f)}},
      //                             j4    j5   j6   j7.
      {{ozz::math::simd_float4::Load(12.f, 0.f, 3.f, 6.f),
        ozz::math::simd_float4::Load(46.f, 0.f, 4.f, 7.f),
        ozz::math::simd_float4::Load(-12.f, 0.f, 5.f, 8.f)},
       {ozz::math::simd_float4::Load(0.f, 0.f, 0.f, 0.f),
        ozz::math::simd_float4::Load(0.f, 0.f, 0.f, 0.f),
        ozz::math::simd_float4::Load(0.f, 0.f, 0.f, 0.f),
        ozz::math::simd_float4::Load(1.f, 1.f, 1.f, 1.f)},
       {ozz::math::simd_float4::Load(1.f, -.1f, 1.f, 1.f),
        ozz::math::simd_float4::Load(1.f, -.1f, 1.f, 1.f),
        ozz::math::simd_float4::Load(1.f, -.1f, 1.f, 1.f)}}};

  ozz::math::Float4x4 output[8];
  LocalToModelJob job_full;
  {  // Intialize whole hierarchy output
    job_full.skeleton = skeleton.get();
    job_full.from = ozz::animation::Skeleton::kNoParent;
    job_full.from_excluded = true;
    job_full.input = input;
    job_full.output = output;
    EXPECT_TRUE(job_full.Validate());
    EXPECT_TRUE(job_full.Run());
    EXPECT_FLOAT4x4_EQ(output[0], 1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f,
                       0.f, 1.f, 0.f, 2.f, 2.f, 2.f, 1.f);
    EXPECT_FLOAT4x4_EQ(output[1], 0.f, 0.f, -1.f, 0.f, 0.f, 1.f, 0.f, 0.f, 1.f,
                       0.f, 0.f, 0.f, 2.f, 2.f, 2.f, 1.f);
    EXPECT_FLOAT4x4_EQ(output[2], 0.f, 0.f, -10.f, 0.f, 0.f, 10.f, 0.f, 0.f,
                       10.f, 0.f, 0.f, 0.f, 0.f, 0.f, 4.f, 1.f);
    EXPECT_FLOAT4x4_EQ(output[3], 1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f,
                       0.f, 1.f, 0.f, 3.f, 4.f, 6.f, 1.f);
    EXPECT_FLOAT4x4_EQ(output[4], 1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f,
                       0.f, 1.f, 0.f, 15.f, 50.f, -6.f, 1.f);
    EXPECT_FLOAT4x4_EQ(output[5], -.1f, 0.f, 0.f, 0.f, 0.f, -.1f, 0.f, 0.f, 0.f,
                       0.f, -.1f, 0.f, 15.f, 50.f, -6.f, 1.f);
    EXPECT_FLOAT4x4_EQ(output[6], 1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f,
                       0.f, 1.f, 0.f, 6.f, 8.f, 11.f, 1.f);
    EXPECT_FLOAT4x4_EQ(output[7], 1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f,
                       0.f, 1.f, 0.f, 6.f, 7.f, 8.f, 1.f);
  }

  {  // Updates from j0 excluded, j7 shouldn't be updated
    output[0] = ozz::math::Float4x4::Scaling(
        ozz::math::simd_float4::Load(2.f, 2.f, 2.f, 0.f));
    output[1] = output[2] = output[3] = output[4] = output[5] = output[6] =
        output[7] = ozz::math::Float4x4::identity();

    LocalToModelJob job;
    job.skeleton = skeleton.get();
    job.from = 0;
    job.from_excluded = true;
    job.input = input;
    job.output = output;
    EXPECT_TRUE(job.Validate());
    EXPECT_TRUE(job.Run());

    EXPECT_FLOAT4x4_EQ(output[0], 2.f, 0.f, 0.f, 0.f, 0.f, 2.f, 0.f, 0.f, 0.f,
                       0.f, 2.f, 0.f, 0.f, 0.f, 0.f, 1.f);
    EXPECT_FLOAT4x4_EQ(output[1], 0.f, 0.f, -2.f, 0.f, 0.f, 2.f, 0.f, 0.f, 2.f,
                       0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 1.f);
    EXPECT_FLOAT4x4_EQ(output[2], 0.f, 0.f, -20.f, 0.f, 0.f, 20.f, 0.f, 0.f,
                       20.f, 0.f, 0.f, 0.f, -4.f, -4.f, 4.f, 1.f);
    EXPECT_FLOAT4x4_EQ(output[3], 2.f, 0.f, 0.f, 0.f, 0.f, 2.f, 0.f, 0.f, 0.f,
                       0.f, 2.f, 0.f, 2.f, 4.f, 8.f, 1.f);
    EXPECT_FLOAT4x4_EQ(output[4], 2.f, 0.f, 0.f, 0.f, 0.f, 2.f, 0.f, 0.f, 0.f,
                       0.f, 2.f, 0.f, 26.f, 96.f, -16.f, 1.f);
    EXPECT_FLOAT4x4_EQ(output[5], -.2f, 0.f, 0.f, 0.f, 0.f, -.2f, 0.f, 0.f, 0.f,
                       0.f, -.2f, 0.f, 26.f, 96.f, -16.f, 1.f);
    EXPECT_FLOAT4x4_EQ(output[6], 2.f, 0.f, 0.f, 0.f, 0.f, 2.f, 0.f, 0.f, 0.f,
                       0.f, 2.f, 0.f, 8.f, 12.f, 18.f, 1.f);
    EXPECT_FLOAT4x4_EQ(output[7], 1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f,
                       0.f, 1.f, 0.f, 0.f, 0.f, 0.f, 1.f);
  }

  {  // Updates from j1 exclude, j2 should be updated
    EXPECT_TRUE(job_full.Run());
    output[0] = output[2] = output[3] = output[4] = output[5] = output[6] =
        output[7] = ozz::math::Float4x4::identity();
    output[1] = ozz::math::Float4x4::Scaling(
        ozz::math::simd_float4::Load(2.f, 2.f, 2.f, 0.f));

    LocalToModelJob job;
    job.skeleton = skeleton.get();
    job.from = 1;
    job.from_excluded = true;
    job.input = input;
    job.output = output;
    EXPECT_TRUE(job.Validate());
    EXPECT_TRUE(job.Run());

    EXPECT_FLOAT4x4_EQ(output[0], 1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f,
                       0.f, 1.f, 0.f, 0.f, 0.f, 0.f, 1.f);
    EXPECT_FLOAT4x4_EQ(output[1], 2.f, 0.f, 0.f, 0.f, 0.f, 2.f, 0.f, 0.f, 0.f,
                       0.f, 2.f, 0.f, 0.f, 0.f, 0.f, 1.f);
    EXPECT_FLOAT4x4_EQ(output[2], 20.f, 0.f, 0.f, 0.f, 0.f, 20.f, 0.f, 0.f, 0.f,
                       0.f, 20.f, 0.f, -4.f, -4.f, -4.f, 1.f);
    EXPECT_FLOAT4x4_EQ(output[3], 1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f,
                       0.f, 1.f, 0.f, 0.f, 0.f, 0.f, 1.f);
    EXPECT_FLOAT4x4_EQ(output[4], 1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f,
                       0.f, 1.f, 0.f, 0.f, 0.f, 0.f, 1.f);
    EXPECT_FLOAT4x4_EQ(output[5], 1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f,
                       0.f, 1.f, 0.f, 0.f, 0.f, 0.f, 1.f);
    EXPECT_FLOAT4x4_EQ(output[6], 1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f,
                       0.f, 1.f, 0.f, 0.f, 0.f, 0.f, 1.f);
    EXPECT_FLOAT4x4_EQ(output[7], 1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f,
                       0.f, 1.f, 0.f, 0.f, 0.f, 0.f, 1.f);
  }

  {  // Updates from j2 excluded, no joint should be updated
    EXPECT_TRUE(job_full.Run());
    output[0] = output[1] = output[3] = output[4] = output[5] = output[6] =
        output[7] = ozz::math::Float4x4::identity();
    output[2] = ozz::math::Float4x4::Scaling(
        ozz::math::simd_float4::Load(2.f, 2.f, 2.f, 0.f));

    LocalToModelJob job;
    job.skeleton = skeleton.get();
    job.from = 2;
    job.from_excluded = true;
    job.input = input;
    job.output = output;
    EXPECT_TRUE(job.Validate());
    EXPECT_TRUE(job.Run());

    EXPECT_FLOAT4x4_EQ(output[0], 1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f,
                       0.f, 1.f, 0.f, 0.f, 0.f, 0.f, 1.f);
    EXPECT_FLOAT4x4_EQ(output[1], 1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f,
                       0.f, 1.f, 0.f, 0.f, 0.f, 0.f, 1.f);
    EXPECT_FLOAT4x4_EQ(output[2], 2.f, 0.f, 0.f, 0.f, 0.f, 2.f, 0.f, 0.f, 0.f,
                       0.f, 2.f, 0.f, 0.f, 0.f, 0.f, 1.f);
    EXPECT_FLOAT4x4_EQ(output[3], 1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f,
                       0.f, 1.f, 0.f, 0.f, 0.f, 0.f, 1.f);
    EXPECT_FLOAT4x4_EQ(output[4], 1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f,
                       0.f, 1.f, 0.f, 0.f, 0.f, 0.f, 1.f);
    EXPECT_FLOAT4x4_EQ(output[5], 1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f,
                       0.f, 1.f, 0.f, 0.f, 0.f, 0.f, 1.f);
    EXPECT_FLOAT4x4_EQ(output[6], 1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f,
                       0.f, 1.f, 0.f, 0.f, 0.f, 0.f, 1.f);
    EXPECT_FLOAT4x4_EQ(output[7], 1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f,
                       0.f, 1.f, 0.f, 0.f, 0.f, 0.f, 1.f);
  }

  {  // Updates from j7 excluded, no joint should be updated
    output[0] = output[1] = output[2] = output[3] = output[4] = output[5] =
        output[6] = ozz::math::Float4x4::identity();
    output[7] = ozz::math::Float4x4::Scaling(
        ozz::math::simd_float4::Load(2.f, 2.f, 2.f, 0.f));

    LocalToModelJob job;
    job.skeleton = skeleton.get();
    job.from = 7;
    job.from_excluded = true;
    job.input = input;
    job.output = output;
    EXPECT_TRUE(job.Validate());
    EXPECT_TRUE(job.Run());

    EXPECT_FLOAT4x4_EQ(output[0], 1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f,
                       0.f, 1.f, 0.f, 0.f, 0.f, 0.f, 1.f);
    EXPECT_FLOAT4x4_EQ(output[1], 1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f,
                       0.f, 1.f, 0.f, 0.f, 0.f, 0.f, 1.f);
    EXPECT_FLOAT4x4_EQ(output[2], 1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f,
                       0.f, 1.f, 0.f, 0.f, 0.f, 0.f, 1.f);
    EXPECT_FLOAT4x4_EQ(output[3], 1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f,
                       0.f, 1.f, 0.f, 0.f, 0.f, 0.f, 1.f);
    EXPECT_FLOAT4x4_EQ(output[4], 1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f,
                       0.f, 1.f, 0.f, 0.f, 0.f, 0.f, 1.f);
    EXPECT_FLOAT4x4_EQ(output[5], 1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f,
                       0.f, 1.f, 0.f, 0.f, 0.f, 0.f, 1.f);
    EXPECT_FLOAT4x4_EQ(output[6], 1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f,
                       0.f, 1.f, 0.f, 0.f, 0.f, 0.f, 1.f);
    EXPECT_FLOAT4x4_EQ(output[7], 2.f, 0.f, 0.f, 0.f, 0.f, 2.f, 0.f, 0.f, 0.f,
                       0.f, 2.f, 0.f, 0.f, 0.f, 0.f, 1.f);
  }

  {  // Updates from j6 excluded, no joint should be updated
    EXPECT_TRUE(job_full.Run());
    output[0] = output[1] = output[2] = output[3] = output[4] = output[5] =
        output[7] = ozz::math::Float4x4::identity();
    output[6] = ozz::math::Float4x4::Scaling(
        ozz::math::simd_float4::Load(2.f, 2.f, 2.f, 0.f));

    LocalToModelJob job;
    job.skeleton = skeleton.get();
    job.from = 6;
    job.from_excluded = true;
    job.input = input;
    job.output = output;
    EXPECT_TRUE(job.Validate());
    EXPECT_TRUE(job.Run());

    EXPECT_FLOAT4x4_EQ(output[0], 1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f,
                       0.f, 1.f, 0.f, 0.f, 0.f, 0.f, 1.f);
    EXPECT_FLOAT4x4_EQ(output[1], 1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f,
                       0.f, 1.f, 0.f, 0.f, 0.f, 0.f, 1.f);
    EXPECT_FLOAT4x4_EQ(output[2], 1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f,
                       0.f, 1.f, 0.f, 0.f, 0.f, 0.f, 1.f);
    EXPECT_FLOAT4x4_EQ(output[3], 1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f,
                       0.f, 1.f, 0.f, 0.f, 0.f, 0.f, 1.f);
    EXPECT_FLOAT4x4_EQ(output[4], 1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f,
                       0.f, 1.f, 0.f, 0.f, 0.f, 0.f, 1.f);
    EXPECT_FLOAT4x4_EQ(output[5], 1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f,
                       0.f, 1.f, 0.f, 0.f, 0.f, 0.f, 1.f);
    EXPECT_FLOAT4x4_EQ(output[6], 2.f, 0.f, 0.f, 0.f, 0.f, 2.f, 0.f, 0.f, 0.f,
                       0.f, 2.f, 0.f, 0.f, 0.f, 0, 1.f);
    EXPECT_FLOAT4x4_EQ(output[7], 1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f,
                       0.f, 1.f, 0.f, 0.f, 0.f, 0.f, 1.f);
  }
}

TEST(Empty, LocalToModel) {
  Skeleton skeleton;

  {  // From root
    LocalToModelJob job;
    job.skeleton = &skeleton;
    EXPECT_TRUE(job.Validate());
    EXPECT_TRUE(job.Run());
  }

  {  // From joint 0
    LocalToModelJob job;
    job.from = 0;
    job.skeleton = &skeleton;
    EXPECT_TRUE(job.Validate());
    EXPECT_TRUE(job.Run());
  }
}
