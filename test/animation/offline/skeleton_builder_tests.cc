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

#include "ozz/animation/offline/raw_skeleton.h"
#include "ozz/animation/offline/skeleton_builder.h"

#include <cstring>

#include "gtest/gtest.h"

#include "ozz/animation/runtime/skeleton.h"
#include "ozz/base/maths/simd_math.h"
#include "ozz/base/maths/soa_transform.h"
#include "ozz/base/memory/unique_ptr.h"

#include "ozz/base/maths/gtest_math_helper.h"

using ozz::animation::Skeleton;
using ozz::animation::offline::RawSkeleton;
using ozz::animation::offline::SkeletonBuilder;

TEST(Error, SkeletonBuilder) {
  // Instantiates a builder objects with default parameters.
  SkeletonBuilder builder;

  // The default raw skeleton is valid. It has no joint.
  {
    RawSkeleton raw_skeleton;
    EXPECT_TRUE(raw_skeleton.Validate());
    EXPECT_EQ(raw_skeleton.num_joints(), 0);

    ozz::unique_ptr<Skeleton> skeleton(builder(raw_skeleton));
    ASSERT_TRUE(skeleton);
    EXPECT_EQ(skeleton->num_joints(), 0);
  }
}

namespace {
// Tester object that ensure order (depth-first) of joints iteration.
class RawSkeletonIterateDFTester {
 public:
  RawSkeletonIterateDFTester() : num_joint_(0) {}
  void operator()(const RawSkeleton::Joint& _current,
                  const RawSkeleton::Joint* _parent) {
    switch (num_joint_) {
      case 0: {
        EXPECT_TRUE(_current.name == "root" && _parent == nullptr);
        break;
      }
      case 1: {
        EXPECT_TRUE(_current.name == "j0" && _parent->name == "root");
        break;
      }
      case 2: {
        EXPECT_TRUE(_current.name == "j1" && _parent->name == "root");
        break;
      }
      case 3: {
        EXPECT_TRUE(_current.name == "j2" && _parent->name == "j1");
        break;
      }
      case 4: {
        EXPECT_TRUE(_current.name == "j3" && _parent->name == "j1");
        break;
      }
      case 5: {
        EXPECT_TRUE(_current.name == "j4" && _parent->name == "root");
        break;
      }
      default: {
        EXPECT_TRUE(false);
        break;
      }
    }
    ++num_joint_;
  }

 private:
  int num_joint_;
};

// Tester object that ensure order (breadth-first) of joints iteration.
class RawSkeletonIterateBFTester {
 public:
  RawSkeletonIterateBFTester() : num_joint_(0) {}
  void operator()(const RawSkeleton::Joint& _current,
                  const RawSkeleton::Joint* _parent) {
    switch (num_joint_) {
      case 0: {
        EXPECT_TRUE(_current.name == "root" && _parent == nullptr);
        break;
      }
      case 1: {
        EXPECT_TRUE(_current.name == "j0" && _parent->name == "root");
        break;
      }
      case 2: {
        EXPECT_TRUE(_current.name == "j1" && _parent->name == "root");
        break;
      }
      case 3: {
        EXPECT_TRUE(_current.name == "j4" && _parent->name == "root");
        break;
      }
      case 4: {
        EXPECT_TRUE(_current.name == "j2" && _parent->name == "j1");
        break;
      }
      case 5: {
        EXPECT_TRUE(_current.name == "j3" && _parent->name == "j1");
        break;
      }
      default: {
        EXPECT_TRUE(false);
        break;
      }
    }
    ++num_joint_;
  }

 private:
  int num_joint_;
};
}  // namespace

TEST(Iterate, SkeletonBuilder) {
  /*
  5 joints

     *
     |
    root
    / |  \
   j0 j1 j4
      / \
     j2 j3
  */
  RawSkeleton raw_skeleton;
  raw_skeleton.roots.resize(1);
  RawSkeleton::Joint& root = raw_skeleton.roots[0];
  root.name = "root";

  root.children.resize(3);
  root.children[0].name = "j0";
  root.children[1].name = "j1";
  root.children[2].name = "j4";

  root.children[1].children.resize(2);
  root.children[1].children[0].name = "j2";
  root.children[1].children[1].name = "j3";

  EXPECT_TRUE(raw_skeleton.Validate());
  EXPECT_EQ(raw_skeleton.num_joints(), 6);

  IterateJointsDF(raw_skeleton, RawSkeletonIterateDFTester());
  IterateJointsBF(raw_skeleton, RawSkeletonIterateBFTester());
}

TEST(Build, SkeletonBuilder) {
  // Instantiates a builder objects with default parameters.
  SkeletonBuilder builder;

  // 1 joint: the root.
  {
    RawSkeleton raw_skeleton;
    raw_skeleton.roots.resize(1);
    RawSkeleton::Joint& root = raw_skeleton.roots[0];
    root.name = "root";

    EXPECT_TRUE(raw_skeleton.Validate());
    EXPECT_EQ(raw_skeleton.num_joints(), 1);

    ozz::unique_ptr<Skeleton> skeleton(builder(raw_skeleton));
    ASSERT_TRUE(skeleton);
    EXPECT_EQ(skeleton->num_joints(), 1);
    EXPECT_EQ(skeleton->joint_parents()[0], Skeleton::kNoParent);
  }

  /*
   2 joints

     *
     |
    j0
     |
    j1
  */
  {
    RawSkeleton raw_skeleton;
    raw_skeleton.roots.resize(1);
    RawSkeleton::Joint& root = raw_skeleton.roots[0];
    root.name = "j0";

    root.children.resize(1);
    root.children[0].name = "j1";

    EXPECT_TRUE(raw_skeleton.Validate());
    EXPECT_EQ(raw_skeleton.num_joints(), 2);

    ozz::unique_ptr<Skeleton> skeleton(builder(raw_skeleton));
    ASSERT_TRUE(skeleton);
    EXPECT_EQ(skeleton->num_joints(), 2);
    for (int i = 0; i < skeleton->num_joints(); ++i) {
      const int parent_index = skeleton->joint_parents()[i];
      if (std::strcmp(skeleton->joint_names()[i], "j0") == 0) {
        EXPECT_EQ(parent_index, Skeleton::kNoParent);
      } else if (std::strcmp(skeleton->joint_names()[i], "j1") == 0) {
        EXPECT_TRUE(std::strcmp(skeleton->joint_names()[parent_index], "j0") ==
                    0);
      } else {
        EXPECT_TRUE(false);
      }
    }
  }

  /*
   3 joints

     *
     |
    j0
    / \
   j1 j2
  */
  {
    RawSkeleton raw_skeleton;
    raw_skeleton.roots.resize(1);
    RawSkeleton::Joint& root = raw_skeleton.roots[0];
    root.name = "j0";

    root.children.resize(2);
    root.children[0].name = "j1";
    root.children[1].name = "j2";

    EXPECT_TRUE(raw_skeleton.Validate());
    EXPECT_EQ(raw_skeleton.num_joints(), 3);

    ozz::unique_ptr<Skeleton> skeleton(builder(raw_skeleton));
    ASSERT_TRUE(skeleton);
    EXPECT_EQ(skeleton->num_joints(), 3);
    for (int i = 0; i < skeleton->num_joints(); ++i) {
      const int parent_index = skeleton->joint_parents()[i];
      if (std::strcmp(skeleton->joint_names()[i], "j0") == 0) {
        EXPECT_EQ(parent_index, Skeleton::kNoParent);
      } else if (std::strcmp(skeleton->joint_names()[i], "j1") == 0) {
        EXPECT_STREQ(skeleton->joint_names()[parent_index], "j0");
      } else if (std::strcmp(skeleton->joint_names()[i], "j2") == 0) {
        EXPECT_STREQ(skeleton->joint_names()[parent_index], "j0");
      } else {
        EXPECT_TRUE(false);
      }
    }
  }

  /*
   4 joints

     *
     |
    j0
    / \
   j1 j3
    |
   j2
  */
  {
    RawSkeleton raw_skeleton;
    raw_skeleton.roots.resize(1);
    RawSkeleton::Joint& root = raw_skeleton.roots[0];
    root.name = "j0";

    root.children.resize(2);
    root.children[0].name = "j1";
    root.children[1].name = "j3";

    root.children[0].children.resize(1);
    root.children[0].children[0].name = "j2";

    EXPECT_TRUE(raw_skeleton.Validate());
    EXPECT_EQ(raw_skeleton.num_joints(), 4);

    ozz::unique_ptr<Skeleton> skeleton(builder(raw_skeleton));
    ASSERT_TRUE(skeleton);
    EXPECT_EQ(skeleton->num_joints(), 4);
    for (int i = 0; i < skeleton->num_joints(); ++i) {
      const int parent_index = skeleton->joint_parents()[i];
      if (std::strcmp(skeleton->joint_names()[i], "j0") == 0) {
        EXPECT_EQ(parent_index, Skeleton::kNoParent);
      } else if (std::strcmp(skeleton->joint_names()[i], "j1") == 0) {
        EXPECT_STREQ(skeleton->joint_names()[parent_index], "j0");
      } else if (std::strcmp(skeleton->joint_names()[i], "j2") == 0) {
        EXPECT_STREQ(skeleton->joint_names()[parent_index], "j1");
      } else if (std::strcmp(skeleton->joint_names()[i], "j3") == 0) {
        EXPECT_STREQ(skeleton->joint_names()[parent_index], "j0");
      } else {
        EXPECT_TRUE(false);
      }
    }
  }

  /*
   4 joints

     *
     |
    j0
    / \
   j1 j2
       |
      j3
  */
  {
    RawSkeleton raw_skeleton;
    raw_skeleton.roots.resize(1);
    RawSkeleton::Joint& root = raw_skeleton.roots[0];
    root.name = "j0";

    root.children.resize(2);
    root.children[0].name = "j1";
    root.children[1].name = "j2";

    root.children[1].children.resize(1);
    root.children[1].children[0].name = "j3";

    EXPECT_TRUE(raw_skeleton.Validate());
    EXPECT_EQ(raw_skeleton.num_joints(), 4);

    ozz::unique_ptr<Skeleton> skeleton(builder(raw_skeleton));
    ASSERT_TRUE(skeleton);
    EXPECT_EQ(skeleton->num_joints(), 4);
    for (int i = 0; i < skeleton->num_joints(); ++i) {
      const int parent_index = skeleton->joint_parents()[i];
      if (std::strcmp(skeleton->joint_names()[i], "j0") == 0) {
        EXPECT_EQ(parent_index, Skeleton::kNoParent);
      } else if (std::strcmp(skeleton->joint_names()[i], "j1") == 0) {
        EXPECT_STREQ(skeleton->joint_names()[parent_index], "j0");
      } else if (std::strcmp(skeleton->joint_names()[i], "j2") == 0) {
        EXPECT_STREQ(skeleton->joint_names()[parent_index], "j0");
      } else if (std::strcmp(skeleton->joint_names()[i], "j3") == 0) {
        EXPECT_STREQ(skeleton->joint_names()[parent_index], "j2");
      } else {
        EXPECT_TRUE(false);
      }
    }
  }

  /*
   5 joints

     *
     |
    j0
    / \
   j1 j2
      / \
     j3 j4
  */
  {
    RawSkeleton raw_skeleton;
    raw_skeleton.roots.resize(1);
    RawSkeleton::Joint& root = raw_skeleton.roots[0];
    root.name = "j0";

    root.children.resize(2);
    root.children[0].name = "j1";
    root.children[1].name = "j2";

    root.children[1].children.resize(2);
    root.children[1].children[0].name = "j3";
    root.children[1].children[1].name = "j4";

    EXPECT_TRUE(raw_skeleton.Validate());
    EXPECT_EQ(raw_skeleton.num_joints(), 5);

    ozz::unique_ptr<Skeleton> skeleton(builder(raw_skeleton));
    ASSERT_TRUE(skeleton);
    EXPECT_EQ(skeleton->num_joints(), 5);
    for (int i = 0; i < skeleton->num_joints(); ++i) {
      const int parent_index = skeleton->joint_parents()[i];
      if (std::strcmp(skeleton->joint_names()[i], "j0") == 0) {
        EXPECT_EQ(parent_index, Skeleton::kNoParent);
      } else if (std::strcmp(skeleton->joint_names()[i], "j1") == 0) {
        EXPECT_STREQ(skeleton->joint_names()[parent_index], "j0");
      } else if (std::strcmp(skeleton->joint_names()[i], "j2") == 0) {
        EXPECT_STREQ(skeleton->joint_names()[parent_index], "j0");
      } else if (std::strcmp(skeleton->joint_names()[i], "j3") == 0) {
        EXPECT_STREQ(skeleton->joint_names()[parent_index], "j2");
      } else if (std::strcmp(skeleton->joint_names()[i], "j4") == 0) {
        EXPECT_STREQ(skeleton->joint_names()[parent_index], "j2");
      } else {
        EXPECT_TRUE(false);
      }
    }
  }

  /*
   6 joints

     *
     |
    j0
    /  \
   j1  j3
    |  / \
   j2 j4 j5
  */
  {
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

    ozz::unique_ptr<Skeleton> skeleton(builder(raw_skeleton));
    ASSERT_TRUE(skeleton);
    EXPECT_EQ(skeleton->num_joints(), 6);
    for (int i = 0; i < skeleton->num_joints(); ++i) {
      const int parent_index = skeleton->joint_parents()[i];
      if (std::strcmp(skeleton->joint_names()[i], "j0") == 0) {
        EXPECT_EQ(parent_index, Skeleton::kNoParent);
      } else if (std::strcmp(skeleton->joint_names()[i], "j1") == 0) {
        EXPECT_STREQ(skeleton->joint_names()[parent_index], "j0");
      } else if (std::strcmp(skeleton->joint_names()[i], "j2") == 0) {
        EXPECT_STREQ(skeleton->joint_names()[parent_index], "j1");
      } else if (std::strcmp(skeleton->joint_names()[i], "j3") == 0) {
        EXPECT_STREQ(skeleton->joint_names()[parent_index], "j0");
      } else if (std::strcmp(skeleton->joint_names()[i], "j4") == 0) {
        EXPECT_STREQ(skeleton->joint_names()[parent_index], "j3");
      } else if (std::strcmp(skeleton->joint_names()[i], "j5") == 0) {
        EXPECT_STREQ(skeleton->joint_names()[parent_index], "j3");
      } else {
        EXPECT_TRUE(false);
      }
    }

    // Skeleton joins should be sorted "per parent" and maintain original
    // children joint order.
    EXPECT_EQ(skeleton->joint_parents()[0], Skeleton::kNoParent);
    EXPECT_STREQ(skeleton->joint_names()[0], "j0");
    EXPECT_EQ(skeleton->joint_parents()[1], 0);
    EXPECT_STREQ(skeleton->joint_names()[1], "j1");
    EXPECT_EQ(skeleton->joint_parents()[2], 1);
    EXPECT_STREQ(skeleton->joint_names()[2], "j2");
    EXPECT_EQ(skeleton->joint_parents()[3], 0);
    EXPECT_STREQ(skeleton->joint_names()[3], "j3");
    EXPECT_EQ(skeleton->joint_parents()[4], 3);
    EXPECT_STREQ(skeleton->joint_names()[4], "j4");
    EXPECT_EQ(skeleton->joint_parents()[5], 3);
    EXPECT_STREQ(skeleton->joint_names()[5], "j5");
  }
}

TEST(JointOrder, SkeletonBuilder) {
  // Instantiates a builder objects with default parameters.
  SkeletonBuilder builder;

  /*
   7 joints

        *
        |
        j0
     /  |  \
   j1   j3  j7
    |  / \
   j2 j4 j5
         |
         j6
  */
  RawSkeleton raw_skeleton;
  raw_skeleton.roots.resize(1);
  RawSkeleton::Joint& root = raw_skeleton.roots[0];
  root.name = "j0";

  root.children.resize(3);
  root.children[0].name = "j1";
  root.children[1].name = "j3";
  root.children[2].name = "j7";

  root.children[0].children.resize(1);
  root.children[0].children[0].name = "j2";

  root.children[1].children.resize(2);
  root.children[1].children[0].name = "j4";
  root.children[1].children[1].name = "j5";

  root.children[1].children[1].children.resize(1);
  root.children[1].children[1].children[0].name = "j6";

  EXPECT_TRUE(raw_skeleton.Validate());
  EXPECT_EQ(raw_skeleton.num_joints(), 8);

  ozz::unique_ptr<Skeleton> skeleton(builder(raw_skeleton));
  ASSERT_TRUE(skeleton);
  EXPECT_EQ(skeleton->num_joints(), 8);

  // Skeleton joints should be sorted "per parent" and maintain original
  // children joint order.
  EXPECT_EQ(skeleton->joint_parents()[0], Skeleton::kNoParent);
  EXPECT_STREQ(skeleton->joint_names()[0], "j0");
  EXPECT_EQ(skeleton->joint_parents()[1], 0);
  EXPECT_STREQ(skeleton->joint_names()[1], "j1");
  EXPECT_EQ(skeleton->joint_parents()[3], 0);
  EXPECT_STREQ(skeleton->joint_names()[3], "j3");
  EXPECT_EQ(skeleton->joint_parents()[7], 0);
  EXPECT_STREQ(skeleton->joint_names()[7], "j7");
  EXPECT_EQ(skeleton->joint_parents()[2], 1);
  EXPECT_STREQ(skeleton->joint_names()[2], "j2");
  EXPECT_EQ(skeleton->joint_parents()[4], 3);
  EXPECT_STREQ(skeleton->joint_names()[4], "j4");
  EXPECT_EQ(skeleton->joint_parents()[5], 3);
  EXPECT_STREQ(skeleton->joint_names()[5], "j5");
  EXPECT_EQ(skeleton->joint_parents()[6], 5);
  EXPECT_STREQ(skeleton->joint_names()[6], "j6");
}

TEST(MultiRoots, SkeletonBuilder) {
  // Instantiates a builder objects with default parameters.
  SkeletonBuilder builder;

  /*
  6 joints (2 roots)
     *
    /  \
   j0   j2
   |    |  \
   j1  j3  j5
        |
       j4
  */
  RawSkeleton raw_skeleton;
  raw_skeleton.roots.resize(2);

  raw_skeleton.roots[0].name = "j0";
  raw_skeleton.roots[0].children.resize(1);
  raw_skeleton.roots[0].children[0].name = "j1";

  raw_skeleton.roots[1].name = "j2";
  raw_skeleton.roots[1].children.resize(2);
  raw_skeleton.roots[1].children[0].name = "j3";
  raw_skeleton.roots[1].children[1].name = "j5";

  raw_skeleton.roots[1].children[0].children.resize(1);
  raw_skeleton.roots[1].children[0].children[0].name = "j4";

  EXPECT_TRUE(raw_skeleton.Validate());
  EXPECT_EQ(raw_skeleton.num_joints(), 6);

  ozz::unique_ptr<Skeleton> skeleton(builder(raw_skeleton));
  ASSERT_TRUE(skeleton);
  EXPECT_EQ(skeleton->num_joints(), 6);
  for (int i = 0; i < skeleton->num_joints(); i++) {
    const int parent_index = skeleton->joint_parents()[i];
    if (std::strcmp(skeleton->joint_names()[i], "j0") == 0) {
      EXPECT_EQ(parent_index, Skeleton::kNoParent);
    } else if (std::strcmp(skeleton->joint_names()[i], "j1") == 0) {
      EXPECT_STREQ(skeleton->joint_names()[parent_index], "j0");
    } else if (std::strcmp(skeleton->joint_names()[i], "j2") == 0) {
      EXPECT_EQ(parent_index, Skeleton::kNoParent);
    } else if (std::strcmp(skeleton->joint_names()[i], "j3") == 0) {
      EXPECT_STREQ(skeleton->joint_names()[parent_index], "j2");
    } else if (std::strcmp(skeleton->joint_names()[i], "j4") == 0) {
      EXPECT_STREQ(skeleton->joint_names()[parent_index], "j3");
    } else if (std::strcmp(skeleton->joint_names()[i], "j5") == 0) {
      EXPECT_STREQ(skeleton->joint_names()[parent_index], "j2");
    } else {
      EXPECT_TRUE(false);
    }
  }
}

TEST(BindPose, SkeletonBuilder) {
  using ozz::math::Float3;
  using ozz::math::Float4;
  using ozz::math::Quaternion;
  using ozz::math::Transform;

  // Instantiates a builder objects with default parameters.
  SkeletonBuilder builder;

  /*
   3 joints

     *
     |
    j0
    / \
   j1 j2
  */

  RawSkeleton raw_skeleton;
  raw_skeleton.roots.resize(1);
  RawSkeleton::Joint& root = raw_skeleton.roots[0];
  root.name = "j0";
  root.transform = Transform::identity();
  root.transform.translation = Float3(1.f, 2.f, 3.f);
  root.transform.rotation = Quaternion(1.f, 0.f, 0.f, 0.f);

  root.children.resize(2);
  root.children[0].name = "j1";
  root.children[0].transform = Transform::identity();
  root.children[0].transform.rotation = Quaternion(0.f, 1.f, 0.f, 0.f);
  root.children[0].transform.translation = Float3(4.f, 5.f, 6.f);
  root.children[1].name = "j2";
  root.children[1].transform = Transform::identity();
  root.children[1].transform.translation = Float3(7.f, 8.f, 9.f);
  root.children[1].transform.scale = Float3(-27.f, 46.f, 9.f);

  EXPECT_TRUE(raw_skeleton.Validate());
  EXPECT_EQ(raw_skeleton.num_joints(), 3);

  ozz::unique_ptr<Skeleton> skeleton(builder(raw_skeleton));
  ASSERT_TRUE(skeleton);

  // Convert bind pose back to aos.
  ozz::math::SimdFloat4 translations[4];
  ozz::math::SimdFloat4 scales[4];
  ozz::math::SimdFloat4 rotations[4];
  const ozz::math::SoaTransform& bind_pose = skeleton->joint_bind_poses()[0];
  ozz::math::Transpose3x4(&bind_pose.translation.x, translations);
  ozz::math::Transpose4x4(&bind_pose.rotation.x, rotations);
  ozz::math::Transpose3x4(&bind_pose.scale.x, scales);

  for (int i = 0; i < skeleton->num_joints(); ++i) {
    if (std::strcmp(skeleton->joint_names()[i], "j0") == 0) {
      EXPECT_SIMDFLOAT_EQ(translations[i], 1.f, 2.f, 3.f, 0.f);
      EXPECT_SIMDFLOAT_EQ(rotations[i], 1.f, 0.f, 0.f, 0.f);
      EXPECT_SIMDFLOAT_EQ(scales[i], 1.f, 1.f, 1.f, 0.f);
    } else if (std::strcmp(skeleton->joint_names()[i], "j1") == 0) {
      EXPECT_SIMDFLOAT_EQ(translations[i], 4.f, 5.f, 6.f, 0.f);
      EXPECT_SIMDFLOAT_EQ(rotations[i], 0.f, 1.f, 0.f, 0.f);
      EXPECT_SIMDFLOAT_EQ(scales[i], 1.f, 1.f, 1.f, 0.f);
    } else if (std::strcmp(skeleton->joint_names()[i], "j2") == 0) {
      EXPECT_SIMDFLOAT_EQ(translations[i], 7.f, 8.f, 9.f, 0.f);
      EXPECT_SIMDFLOAT_EQ(rotations[i], 0.f, 0.f, 0.f, 1.f);
      EXPECT_SIMDFLOAT_EQ(scales[i], -27.f, 46.f, 9.f, 0.f);
    } else {
      EXPECT_TRUE(false);
    }
  }

  // Unused joint from the SoA structure must be properly initialized
  EXPECT_SIMDFLOAT_EQ(translations[3], 0.f, 0.f, 0.f, 0.f);
  EXPECT_SIMDFLOAT_EQ(rotations[3], 0.f, 0.f, 0.f, 1.f);
  EXPECT_SIMDFLOAT_EQ(scales[3], 1.f, 1.f, 1.f, 0.f);
}

TEST(MaxJoints, SkeletonBuilder) {
  // Instantiates a builder objects with default parameters.
  SkeletonBuilder builder;

  {  // Inside the domain.
    RawSkeleton raw_skeleton;
    raw_skeleton.roots.resize(Skeleton::kMaxJoints);

    EXPECT_TRUE(raw_skeleton.Validate());
    EXPECT_EQ(raw_skeleton.num_joints(), Skeleton::kMaxJoints);

    ozz::unique_ptr<Skeleton> skeleton(builder(raw_skeleton));
    EXPECT_TRUE(skeleton);
  }

  {  // Outside of the domain.
    RawSkeleton raw_skeleton;
    raw_skeleton.roots.resize(Skeleton::kMaxJoints + 1);

    EXPECT_FALSE(raw_skeleton.Validate());
    EXPECT_EQ(raw_skeleton.num_joints(), Skeleton::kMaxJoints + 1);

    EXPECT_TRUE(!builder(raw_skeleton));
  }
}
