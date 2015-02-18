//============================================================================//
//                                                                            //
// ozz-animation, 3d skeletal animation libraries and tools.                  //
// https://code.google.com/p/ozz-animation/                                   //
//                                                                            //
//----------------------------------------------------------------------------//
//                                                                            //
// Copyright (c) 2012-2015 Guillaume Blanc                                    //
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

#include "ozz/animation/offline/raw_skeleton.h"
#include "ozz/animation/offline/skeleton_builder.h"

#include <cstring>

#include "gtest/gtest.h"
#include "ozz/base/gtest_helper.h"
#include "ozz/base/maths/gtest_math_helper.h"

#include "ozz/base/memory/allocator.h"
#include "ozz/animation/runtime/skeleton.h"
#include "ozz/animation/runtime/skeleton_utils.h"

using ozz::animation::Skeleton;
using ozz::animation::offline::RawSkeleton;
using ozz::animation::offline::SkeletonBuilder;

TEST(JointBindPose, SkeletonUtils) {
  // Instantiates a builder objects with default parameters.
  SkeletonBuilder builder;

  RawSkeleton raw_skeleton;
  raw_skeleton.roots.resize(1);
  RawSkeleton::Joint& r = raw_skeleton.roots[0];
  r.name = "r0";
  r.transform.translation = ozz::math::Float3::x_axis();
  r.transform.rotation = ozz::math::Quaternion::identity();
  r.transform.scale = ozz::math::Float3::zero();

  r.children.resize(2);
  RawSkeleton::Joint& c0 = r.children[0];
  c0.name = "j0";
  c0.transform.translation = ozz::math::Float3::y_axis();
  c0.transform.rotation = -ozz::math::Quaternion::identity();
  c0.transform.scale = -ozz::math::Float3::one();

  RawSkeleton::Joint& c1 = r.children[1];
  c1.name = "j1";
  c1.transform.translation = ozz::math::Float3::z_axis();
  c1.transform.rotation = Conjugate(ozz::math::Quaternion::identity());
  c1.transform.scale = ozz::math::Float3::one();
  
  EXPECT_TRUE(raw_skeleton.Validate());
  EXPECT_EQ(raw_skeleton.num_joints(), 3);

  Skeleton* skeleton = builder(raw_skeleton);
  ASSERT_TRUE(skeleton != NULL);
  EXPECT_EQ(skeleton->num_joints(), 3);

  // Out of range.
  EXPECT_ASSERTION(GetJointBindPose(*skeleton, 3), "Joint index out of range.");
  
  const ozz::math::Transform bind_pose0 =
    ozz::animation::GetJointBindPose(*skeleton, 0);
  EXPECT_FLOAT3_EQ(bind_pose0.translation, 1.f, 0.f, 0.f);
  EXPECT_QUATERNION_EQ(bind_pose0.rotation, 0.f, 0.f, 0.f, 1.f);
  EXPECT_FLOAT3_EQ(bind_pose0.scale, 0.f, 0.f, 0.f);

  const ozz::math::Transform bind_pose1 =
    ozz::animation::GetJointBindPose(*skeleton, 1);
  EXPECT_FLOAT3_EQ(bind_pose1.translation, 0.f, 1.f, 0.f);
  EXPECT_QUATERNION_EQ(bind_pose1.rotation, 0.f, 0.f, 0.f, -1.f);
  EXPECT_FLOAT3_EQ(bind_pose1.scale, -1.f, -1.f, -1.f);

  const ozz::math::Transform bind_pose2 =
    ozz::animation::GetJointBindPose(*skeleton, 2);
  EXPECT_FLOAT3_EQ(bind_pose2.translation, 0.f, 0.f, 1.f);
  EXPECT_QUATERNION_EQ(bind_pose2.rotation, -0.f, -0.f, -0.f, 1.f);
  EXPECT_FLOAT3_EQ(bind_pose2.scale, 1.f, 1.f, 1.f);

  ozz::memory::default_allocator()->Delete(skeleton);
}

namespace {

class IterateDFFailTester {
 public:
  void operator()(int, int) {
    ASSERT_TRUE(false);
  }
};

/* Definition of the skeleton used by the tests. 
   10 joints, 2 roots

         *
       /   \
      r0    r1
    /  |     \
   j0  j3    j7
    |  / \
   j1 j4 j5
    |     |
   j2    j6
*/

static const uint16_t joints_df[] = {0, 2, 5, 6, 3, 7, 8, 9, 4, 1};

class IterateDFTester {
 public:

  IterateDFTester(const ozz::animation::Skeleton* _skeleton, int _start)
    : skeleton_(_skeleton),
      current_df_(_start),
       num_iterations_(0) {
  }

  void operator()(int _current, int _parent) {
    ASSERT_EQ(skeleton_->num_joints(),
              static_cast<int>(OZZ_ARRAY_SIZE(joints_df)));
    ASSERT_LT(_current, static_cast<int>(OZZ_ARRAY_SIZE(joints_df)));
    ASSERT_LT(num_iterations_, static_cast<int>(OZZ_ARRAY_SIZE(joints_df)));

    EXPECT_EQ(joints_df[current_df_], _current);
    EXPECT_EQ(skeleton_->joint_properties().begin[_current].parent, _parent);

    ++current_df_;
    ++num_iterations_;
  }

  int num_iterations() const {
    return num_iterations_;
  }
 private:

  // Iterated skeleton.
  const ozz::animation::Skeleton* skeleton_;

  // Current joint index in depth first order.
  int current_df_;

  // Number of iterations completed.
  int num_iterations_;
};
}

TEST(InterateDF, SkeletonUtils) {
  // Instantiates a builder objects with default parameters.
  SkeletonBuilder builder;

  RawSkeleton raw_skeleton;
  raw_skeleton.roots.resize(2);
  RawSkeleton::Joint& r0 = raw_skeleton.roots[0];
  r0.name = "r0";
  RawSkeleton::Joint& r1 = raw_skeleton.roots[0];
  r1.name = "r1";

  r0.children.resize(3);
  r0.children[0].name = "j0";
  r0.children[1].name = "j3";
  r0.children[2].name = "j7";

  r0.children[0].children.resize(1);
  r0.children[0].children[0].name = "j1";

  r0.children[0].children[0].children.resize(1);
  r0.children[0].children[0].children[0].name = "j2";

  r0.children[1].children.resize(2);
  r0.children[1].children[0].name = "j4";
  r0.children[1].children[1].name = "j5";

  r0.children[1].children[1].children.resize(1);
  r0.children[1].children[1].children[0].name = "j6";

  EXPECT_TRUE(raw_skeleton.Validate());
  EXPECT_EQ(raw_skeleton.num_joints(), 10);

  Skeleton* skeleton = builder(raw_skeleton);
  ASSERT_TRUE(skeleton != NULL);
  EXPECT_EQ(skeleton->num_joints(), 10);

  ozz::animation::JointsIterator it;

  ozz::animation::IterateJointsDF(*skeleton, -12, IterateDFFailTester());
  ozz::animation::IterateJointsDF(*skeleton, -12, &it);
  EXPECT_EQ(it.num_joints, 0);
  ozz::animation::IterateJointsDF(*skeleton, 12, IterateDFFailTester());
  ozz::animation::IterateJointsDF(*skeleton, 12, &it);
  EXPECT_EQ(it.num_joints, 0);

  ozz::animation::IterateJointsDF(
    *skeleton, ozz::animation::Skeleton::kNoParentIndex, &it);
  EXPECT_EQ(it.num_joints, 10);
  EXPECT_EQ(std::memcmp(joints_df, it.joints, 10 * sizeof(uint16_t)), 0);

  IterateDFTester fct_all = ozz::animation::IterateJointsDF(
    *skeleton,
    ozz::animation::Skeleton::kNoParentIndex,
    IterateDFTester(skeleton, 0));
  EXPECT_EQ(fct_all.num_iterations(), 10);

  ozz::animation::IterateJointsDF(*skeleton, 1, &it);
  EXPECT_EQ(it.num_joints, 1);
  EXPECT_EQ(std::memcmp(joints_df + 9, it.joints, 1 * sizeof(uint16_t)), 0);

  IterateDFTester fct1 = ozz::animation::IterateJointsDF(
    *skeleton, 1, IterateDFTester(skeleton, 9));
  EXPECT_EQ(fct1.num_iterations(), 1);

  ozz::animation::IterateJointsDF(*skeleton, 2, &it);
  EXPECT_EQ(it.num_joints, 3);
  EXPECT_EQ(std::memcmp(joints_df + 1, it.joints, 1 * sizeof(uint16_t)), 0);

  IterateDFTester fct2 = ozz::animation::IterateJointsDF(
    *skeleton, 2, IterateDFTester(skeleton, 1));
  EXPECT_EQ(fct2.num_iterations(), 3);

  ozz::animation::IterateJointsDF(*skeleton, 3, &it);
  EXPECT_EQ(it.num_joints, 4);
  EXPECT_EQ(std::memcmp(joints_df + 4, it.joints, 4 * sizeof(uint16_t)), 0);

  IterateDFTester fct3 = ozz::animation::IterateJointsDF(
    *skeleton, 3, IterateDFTester(skeleton, 4));
  EXPECT_EQ(fct3.num_iterations(), 4);

  ozz::animation::IterateJointsDF(*skeleton, 9, &it);
  EXPECT_EQ(it.num_joints, 1);
  EXPECT_EQ(std::memcmp(joints_df + 7, it.joints, 1 * sizeof(uint16_t)), 0);

  IterateDFTester fct4 = ozz::animation::IterateJointsDF(
    *skeleton, 9, IterateDFTester(skeleton, 7));
  EXPECT_EQ(fct4.num_iterations(), 1);

  ozz::memory::default_allocator()->Delete(skeleton);
}

TEST(InterateWorstBreadthDF, SkeletonUtils) {
  // Instantiates a builder objects with default parameters.
  SkeletonBuilder builder;

  RawSkeleton raw_skeleton;
  raw_skeleton.roots.resize(Skeleton::kMaxJoints);

  EXPECT_TRUE(raw_skeleton.Validate());
  EXPECT_EQ(raw_skeleton.num_joints(), Skeleton::kMaxJoints);

  Skeleton* skeleton = builder(raw_skeleton);
  ASSERT_TRUE(skeleton != NULL);
  EXPECT_EQ(skeleton->num_joints(), Skeleton::kMaxJoints);

  ozz::animation::JointsIterator it;
  ozz::animation::IterateJointsDF(*skeleton, Skeleton::kNoParentIndex, &it);
  EXPECT_EQ(it.num_joints, Skeleton::kMaxJoints);

  for (int i = 0; i < Skeleton::kMaxJoints; ++i) {
    EXPECT_EQ(it.joints[i], i);
  }
  ozz::memory::default_allocator()->Delete(skeleton);
}

TEST(InterateWorstDepthDF, SkeletonUtils) {
  // Instantiates a builder objects with default parameters.
  SkeletonBuilder builder;

  RawSkeleton raw_skeleton;
  RawSkeleton::Joint::Children* child = &raw_skeleton.roots;
  for (int i = 0; i < Skeleton::kMaxJoints; ++i) {
    child->resize(1);
    child = &child->at(0).children;
  }

  EXPECT_TRUE(raw_skeleton.Validate());
  EXPECT_EQ(raw_skeleton.num_joints(), Skeleton::kMaxJoints);

  Skeleton* skeleton = builder(raw_skeleton);
  ASSERT_TRUE(skeleton != NULL);
  EXPECT_EQ(skeleton->num_joints(), Skeleton::kMaxJoints);

  ozz::animation::JointsIterator it;
  ozz::animation::IterateJointsDF(*skeleton, Skeleton::kNoParentIndex, &it);
  EXPECT_EQ(it.num_joints, Skeleton::kMaxJoints);

  for (int i = 0; i < Skeleton::kMaxJoints; ++i) {
    EXPECT_EQ(it.joints[i], i);
  }
  ozz::memory::default_allocator()->Delete(skeleton);
}
