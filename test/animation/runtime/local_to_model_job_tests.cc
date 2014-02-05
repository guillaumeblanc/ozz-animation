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

#include "ozz/animation/runtime/local_to_model_job.h"

#include "gtest/gtest.h"

#include "ozz/base/memory/allocator.h"
#include "ozz/base/maths/gtest_math_helper.h"
#include "ozz/base/maths/soa_transform.h"

#include "ozz/animation/offline/skeleton_builder.h"
#include "ozz/animation/runtime/skeleton.h"

using ozz::animation::Skeleton;
using ozz::animation::LocalToModelJob;
using ozz::animation::offline::RawSkeleton;
using ozz::animation::offline::SkeletonBuilder;

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

  ozz::math::SoaTransform input[2] = {
    ozz::math::SoaTransform::identity(),
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
    job.output.begin = output;
    job.output.end = output + 4;
    EXPECT_FALSE(job.Validate());
    EXPECT_FALSE(job.Run());
  }
  // Valid job.
  {
    LocalToModelJob job;
    job.skeleton = skeleton;
    job.input.begin = input;
    job.input.end = input + 1;
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
    job.input.begin = input;
    job.input.end = input + 2;
    job.output.begin = output;
    job.output.end = output + 5;
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
  ozz::math::SoaTransform input[2] = {  // Stores up to 8 inputs, needs 6.
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

  // Allocates output.
  ozz::math::Float4x4 output[6];

  // Prepares the job.
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
  ozz::memory::default_allocator()->Delete(skeleton);
}
