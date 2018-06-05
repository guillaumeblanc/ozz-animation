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

#include "ozz/animation/runtime/local_to_model_job.h"

#include "gtest/gtest.h"

#include "ozz/base/maths/gtest_math_helper.h"
#include "ozz/base/maths/soa_transform.h"
#include "ozz/base/memory/allocator.h"

#include "ozz/animation/offline/raw_skeleton.h"
#include "ozz/animation/offline/skeleton_builder.h"
#include "ozz/animation/runtime/skeleton.h"

using ozz::animation::Skeleton;
using ozz::animation::LocalToModelJob;
using ozz::animation::offline::RawSkeleton;
using ozz::animation::offline::SkeletonBuilder;

// clang-format off

TEST(JobValidity, LocalToModel) {
  RawSkeleton raw_skeleton;
  SkeletonBuilder builder;

  // Empty skeleton.
  Skeleton* empty_skeleton = builder(raw_skeleton);
  ASSERT_TRUE(empty_skeleton != NULL);

  // Adds 2 joints.
  raw_skeleton.roots.resize(1);
  RawSkeleton::Joint& root = raw_skeleton.roots[0];
  root.name = "root";
  root.children.resize(1);

  Skeleton* skeleton = builder(raw_skeleton);
  ASSERT_TRUE(skeleton != NULL);

  ozz::math::SoaTransform input[2] = {ozz::math::SoaTransform::identity(),
                                      ozz::math::SoaTransform::identity()};
  ozz::math::Float4x4 output[5];

  // Default job
  {
    LocalToModelJob job;
    EXPECT_FALSE(job.Validate());
    EXPECT_FALSE(job.Run());
  }

  // NULL output
  {
    LocalToModelJob job;
    job.skeleton = skeleton;
    job.input.begin = input;
    job.input.end = input + 1;
    EXPECT_FALSE(job.Validate());
    EXPECT_FALSE(job.Run());
  }
  // NULL input
  {
    LocalToModelJob job;
    job.skeleton = skeleton;
    job.output.begin = output;
    job.output.end = output + 2;
    EXPECT_FALSE(job.Validate());
    EXPECT_FALSE(job.Run());
  }
  // Null skeleton.
  {
    LocalToModelJob job;
    job.input.begin = input;
    job.input.end = input + 1;
    job.output.begin = output;
    job.output.end = output + 4;
    EXPECT_FALSE(job.Validate());
    EXPECT_FALSE(job.Run());
  }
  // Invalid output range: end < begin.
  {
    LocalToModelJob job;
    job.skeleton = skeleton;
    job.input.begin = input;
    job.input.end = input + 1;
    job.output.begin = output + 1;
    job.output.end = output;
    EXPECT_FALSE(job.Validate());
    EXPECT_FALSE(job.Run());
  }
  // Invalid output range: too small.
  {
    LocalToModelJob job;
    job.skeleton = skeleton;
    job.input.begin = input;
    job.input.end = input + 1;
    job.output.begin = output;
    job.output.end = output + 1;
    EXPECT_FALSE(job.Validate());
    EXPECT_FALSE(job.Run());
  }
  // Invalid input range: end < begin.
  {
    LocalToModelJob job;
    job.skeleton = skeleton;
    job.input.begin = input + 1;
    job.input.end = input;
    job.output.begin = output;
    job.output.end = output + 4;
    EXPECT_FALSE(job.Validate());
    EXPECT_FALSE(job.Run());
  }
  // Invalid input range: too small.
  {
    LocalToModelJob job;
    job.skeleton = skeleton;
    job.input.begin = input;
    job.input.end = input;
    job.output = output;
    EXPECT_FALSE(job.Validate());
    EXPECT_FALSE(job.Run());
  }
  // Valid job.
  {
    LocalToModelJob job;
    job.skeleton = skeleton;
    job.input = input;
    job.output.begin = output;
    job.output.end = output + 2;
    EXPECT_TRUE(job.Validate());
    EXPECT_TRUE(job.Run());
  }
  // Valid job with root matrix.
  {
    LocalToModelJob job;
    const ozz::math::SimdFloat4 v = ozz::math::simd_float4::Load(4.f, 3.f, 2.f, 1.f);
    ozz::math::Float4x4 world = ozz::math::Float4x4::Translation(v);
    job.skeleton = skeleton;
    job.root = &world;
    job.input = input;
    job.output.begin = output;
    job.output.end = output + 2;
    EXPECT_TRUE(job.Validate());
    EXPECT_TRUE(job.Run());
  }
  // Valid job with empty skeleton.
  {
    LocalToModelJob job;
    job.skeleton = empty_skeleton;
    job.input.begin = input;
    job.input.end = input + 0;
    job.output.begin = output;
    job.output.end = output + 0;
    EXPECT_TRUE(job.Validate());
    EXPECT_TRUE(job.Run());
  }

  // Valid job. Bigger input & output
  {
    LocalToModelJob job;
    job.skeleton = skeleton;
    job.input = input;
    job.output = output;
    EXPECT_TRUE(job.Validate());
    EXPECT_TRUE(job.Run());
  }

  ozz::memory::default_allocator()->Delete(empty_skeleton);
  ozz::memory::default_allocator()->Delete(skeleton);
}

TEST(Transformation, LocalToModel) {
  // Builds the skeleton
  /*
   6 joints
   root
    /  \
   j0  j2
    |  / \
   j1 j3 j4
  */
  RawSkeleton raw_skeleton;
  raw_skeleton.roots.resize(1);
  RawSkeleton::Joint& root = raw_skeleton.roots[0];
  root.name = "root";

  root.children.resize(2);
  root.children[0].name = "j0";
  root.children[1].name = "j2";

  root.children[0].children.resize(1);
  root.children[0].children[0].name = "j1";

  root.children[1].children.resize(2);
  root.children[1].children[0].name = "j3";
  root.children[1].children[1].name = "j4";

  EXPECT_TRUE(raw_skeleton.Validate());
  EXPECT_EQ(raw_skeleton.num_joints(), 6);

  SkeletonBuilder builder;
  Skeleton* skeleton = builder(raw_skeleton);
  ASSERT_TRUE(skeleton != NULL);

  // Initializes an input transformation.
  ozz::math::SoaTransform input[2] = {
      // Stores up to 8 inputs, needs 6.
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
    // Prepares the job with root == NULL (default identity matrix)
    ozz::math::Float4x4 output[6];
    LocalToModelJob job;
    job.skeleton = skeleton;
    job.input.begin = input;
    job.input.end = input + 2;
    job.output.begin = output;
    job.output.end = output + 6;
    EXPECT_TRUE(job.Validate());
    EXPECT_TRUE(job.Run());
    EXPECT_FLOAT4x4_EQ(output[0], 1.f, 0.f, 0.f, 0.f,
                                  0.f, 1.f, 0.f, 0.f,
                                  0.f, 0.f, 1.f, 0.f,
                                  2.f, 2.f, 2.f, 1.f);
    EXPECT_FLOAT4x4_EQ(output[1], 0.f, 0.f, -1.f, 0.f,
                                  0.f, 1.f, 0.f, 0.f,
                                  1.f, 0.f, 0.f, 0.f,
                                  2.f, 2.f, 2.f, 1.f);
    EXPECT_FLOAT4x4_EQ(output[2], 10.f, 0.f, 0.f, 0.f,
                                  0.f, 10.f, 0.f, 0.f,
                                  0.f, 0.f, 10.f, 0.f,
                                  0.f, 0.f, 0.f, 1.f);
    EXPECT_FLOAT4x4_EQ(output[3], 0.f, 0.f, -1.f, 0.f,
                                  0.f, 1.f, 0.f, 0.f,
                                  1.f, 0.f, 0.f, 0.f,
                                  6.f, 4.f, 1.f, 1.f);
    EXPECT_FLOAT4x4_EQ(output[4], 10.f, 0.f, 0.f, 0.f,
                                  0.f, 10.f, 0.f, 0.f,
                                  0.f, 0.f, 10.f, 0.f,
                                  120.f, 460.f, -120.f, 1.f);
    EXPECT_FLOAT4x4_EQ(output[5], -1.f, 0.f, 0.f, 0.f,
                                  0.f, -1.f, 0.f, 0.f,
                                  0.f, 0.f, -1.f, 0.f,
                                  0.f, 0.f, 0.f, 1.f);
  }

  {
    // Prepares the job with root == Translation(4,3,2,1)
    ozz::math::Float4x4 output[6];
    const ozz::math::SimdFloat4 v = ozz::math::simd_float4::Load(4.f, 3.f, 2.f, 1.f);
    ozz::math::Float4x4 world = ozz::math::Float4x4::Translation(v);
    LocalToModelJob job;
    job.skeleton = skeleton;
    job.root = &world;
    job.input.begin = input;
    job.input.end = input + 2;
    job.output.begin = output;
    job.output.end = output + 6;
    EXPECT_TRUE(job.Validate());
    EXPECT_TRUE(job.Run());
    EXPECT_FLOAT4x4_EQ(output[0], 1.f, 0.f, 0.f, 0.f,
                                  0.f, 1.f, 0.f, 0.f,
                                  0.f, 0.f, 1.f, 0.f,
                                  6.f, 5.f, 4.f, 1.f);
    EXPECT_FLOAT4x4_EQ(output[1], 0.f, 0.f, -1.f, 0.f,
                                  0.f, 1.f, 0.f, 0.f,
                                  1.f, 0.f, 0.f, 0.f,
                                  6.f, 5.f, 4.f, 1.f);
    EXPECT_FLOAT4x4_EQ(output[2], 10.f, 0.f, 0.f, 0.f,
                                  0.f, 10.f, 0.f, 0.f,
                                  0.f, 0.f, 10.f, 0.f,
                                  4.f, 3.f, 2.f, 1.f);
    EXPECT_FLOAT4x4_EQ(output[3], 0.f, 0.f, -1.f, 0.f,
                                  0.f, 1.f, 0.f, 0.f,
                                  1.f, 0.f, 0.f, 0.f,
                                  10.f, 7.f, 3.f, 1.f);
    EXPECT_FLOAT4x4_EQ(output[4], 10.f, 0.f, 0.f, 0.f,
                                  0.f, 10.f, 0.f, 0.f,
                                  0.f, 0.f, 10.f, 0.f,
                                  124.f, 463.f, -118.f, 1.f);
    EXPECT_FLOAT4x4_EQ(output[5], -1.f, 0.f, 0.f, 0.f,
                                  0.f, -1.f, 0.f, 0.f,
                                  0.f, 0.f, -1.f, 0.f,
                                  4.f, 3.f, 2.f, 1.f);
  }
  ozz::memory::default_allocator()->Delete(skeleton);
}
