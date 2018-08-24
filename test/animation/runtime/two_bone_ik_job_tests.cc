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

#include "ozz/base/maths/simd_math.h"
#include "ozz/base/maths/simd_quaternion.h"

#include "gtest/gtest.h"
#include "ozz/base/maths/gtest_math_helper.h"

TEST(JobValidity, TwoBoneIKJob) {
  {  // Default is invalid
    ozz::animation::TwoBoneIKJob job;
    EXPECT_FALSE(job.Validate());
  }

  {  // Missing start joint matrix
    ozz::animation::TwoBoneIKJob job;
    ozz::math::Float4x4 mat;
    job.mid_joint = &mat;
    job.end_joint = &mat;
    ozz::math::SimdQuaternion quat;
    job.start_joint_correction = &quat;
    job.mid_joint_correction = &quat;
    EXPECT_FALSE(job.Validate());
  }

  {  // Missing mid joint matrix
    ozz::animation::TwoBoneIKJob job;
    ozz::math::Float4x4 mat;
    job.start_joint = &mat;
    job.end_joint = &mat;
    ozz::math::SimdQuaternion quat;
    job.start_joint_correction = &quat;
    job.mid_joint_correction = &quat;
    EXPECT_FALSE(job.Validate());
  }

  {  // Missing end joint matrix
    ozz::animation::TwoBoneIKJob job;
    ozz::math::Float4x4 mat;
    job.start_joint = &mat;
    job.mid_joint = &mat;
    ozz::math::SimdQuaternion quat;
    job.start_joint_correction = &quat;
    job.mid_joint_correction = &quat;
    EXPECT_FALSE(job.Validate());
  }

  {  // Missing start joint output quaternion
    ozz::animation::TwoBoneIKJob job;
    ozz::math::Float4x4 mat;
    job.start_joint = &mat;
    job.mid_joint = &mat;
    job.end_joint = &mat;
    ozz::math::SimdQuaternion quat;
    job.mid_joint_correction = &quat;
    EXPECT_FALSE(job.Validate());
  }

  {  // Missing mid joint output quaternion
    ozz::animation::TwoBoneIKJob job;
    ozz::math::Float4x4 mat;
    job.start_joint = &mat;
    job.mid_joint = &mat;
    job.end_joint = &mat;
    ozz::math::SimdQuaternion quat;
    job.start_joint_correction = &quat;
    EXPECT_FALSE(job.Validate());
  }

  {  // Valid
    ozz::animation::TwoBoneIKJob job;
    ozz::math::Float4x4 mat;
    job.start_joint = &mat;
    job.mid_joint = &mat;
    job.end_joint = &mat;
    ozz::math::SimdQuaternion quat;
    job.start_joint_correction = &quat;
    job.mid_joint_correction = &quat;
    EXPECT_TRUE(job.Validate());
  }
}
