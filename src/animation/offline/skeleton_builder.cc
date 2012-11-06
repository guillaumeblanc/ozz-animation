//============================================================================//
// Copyright (c) <2012> <Guillaume Blanc>                                     //
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
//============================================================================//

#include "ozz/animation/offline/skeleton_builder.h"

#include <cstring>
#include <algorithm>

#include "ozz/base/containers/vector.h"
#include "ozz/base/maths/soa_transform.h"
#include "ozz/base/memory/allocator.h"
#include "ozz/animation/skeleton.h"

namespace ozz {
namespace animation {
namespace offline {

RawSkeleton::RawSkeleton() {
}

RawSkeleton::~RawSkeleton() {
}

bool RawSkeleton::Validate() const {
  if (num_joints() > Skeleton::kMaxJoints) {
    return false;
  }
  return true;
}

namespace {
struct JointCounter {
  JointCounter() :
    num_joints(0) {
  }
  void operator()(const RawSkeleton::Joint&, const RawSkeleton::Joint*) {
    num_joints++;
  }
  int num_joints;
};
}  // namespace

// Iterates through all the root children and count them.
int RawSkeleton::num_joints() const {
  return IterateJointsDF(JointCounter()).num_joints;
}

namespace {
// Stores each traversed joint to a vector.
struct JointLister {
  explicit JointLister(int _num_joints) {
    linear_joints.reserve(_num_joints);
  }
  void operator()(const RawSkeleton::Joint& _current,
                  const RawSkeleton::Joint* _parent) {
    // Looks for the "lister" parent.
    int parent = -1;
    if (_parent) {
      // Start searching from the last joint.
      int j = static_cast<int>(linear_joints.size()) - 1;
      for (; j >= 0; j--) {
        if (linear_joints[j].joint == _parent) {
          parent = j;
          break;
        }
      }
      assert(parent >= 0);
    }
    const Joint listed = {&_current, parent};
    linear_joints.push_back(listed);
  }
  struct Joint {
    const RawSkeleton::Joint* joint;
    int parent;
  };
  // Array of joints in the traversed DAG order.
  ozz::Vector<Joint>::Std linear_joints;
};
}  // namespace

// Validates the RawSkeleton and fills a Skeleton.
// Uses RawSkeleton::IterateJointsBF to traverse in DAG breadth-first order.
// This favors cache coherency (when traversing joints) and reduces
// Load-Hit-Stores (reusing the parent that has just been computed).
Skeleton* SkeletonBuilder::operator()(const RawSkeleton& _raw_skeleton) const {
  // Tests _raw_skeleton validity.
  if (!_raw_skeleton.Validate()) {
    return NULL;
  }

  // Everything is fine, allocates and fills the skeleton.
  // Will not fail.
  Skeleton* skeleton = memory::default_allocator().New<Skeleton>();
  const int num_joints = _raw_skeleton.num_joints();
  skeleton->num_joints_ = num_joints;

  // Iterates through all the joint of the raw skeleton and fills a sorted joint
  // list.
  JointLister lister(num_joints);
  _raw_skeleton.IterateJointsBF<JointLister&>(lister);
  assert(static_cast<int>(lister.linear_joints.size()) == num_joints);

  // Transfers sorted joints hierarchy to the new skeleton.
  skeleton->joint_properties_ =
    memory::default_allocator().Allocate<Skeleton::JointProperties>(num_joints);
  for (int i = 0; i < num_joints; i++) {
    skeleton->joint_properties_[i].parent = lister.linear_joints[i].parent;
    skeleton->joint_properties_[i].is_leaf =
      lister.linear_joints[i].joint->children.empty();
  }
  // Transfers joint's names: First computes name's buffer size, then allocate
  // and do the copy.
  std::size_t buffer_size = num_joints * sizeof(char*);
  for (int i = 0; i < num_joints; i++) {
    const RawSkeleton::Joint& current = *lister.linear_joints[i].joint;
    buffer_size += (current.name.size() + 1) * sizeof(char);
  }
  skeleton->joint_names_ =
    memory::default_allocator().Allocate<char*>(buffer_size);
  char* cursor = reinterpret_cast<char*>(skeleton->joint_names_ + num_joints);
  for (int i = 0; i < num_joints; i++) {
    const RawSkeleton::Joint& current = *lister.linear_joints[i].joint;
    skeleton->joint_names_[i] = cursor;
    strcpy(cursor, current.name.c_str());
    cursor += (current.name.size() + 1) * sizeof(char);
  }

  // Transfers t-poses.
  const int num_soa_joints = (num_joints + 3) / 4;
  skeleton->bind_pose_ =
    memory::default_allocator().Allocate<math::SoaTransform>(num_soa_joints);

  const math::SimdFloat4 w_axis = math::simd_float4::w_axis();
  const math::SimdFloat4 zero = math::simd_float4::zero();
  const math::SimdFloat4 one = math::simd_float4::one();

  for (int i = 0; i < num_soa_joints; i++) {
    math::SimdFloat4 translations[4];
    math::SimdFloat4 scales[4];
    math::SimdFloat4 rotations[4];
    for (int j = 0; j < 4; j++) {
      if (i * 4 + j < num_joints) {
        const RawSkeleton::Joint& src_joint =
          *lister.linear_joints[i * 4 + j].joint;
        translations[j] =
          math::simd_float4::Load3PtrU(&src_joint.transform.translation.x);
        rotations[j] = math::NormalizeSafe4(
          math::simd_float4::LoadPtrU(&src_joint.transform.rotation.x), w_axis);
        scales[j] =
          math::simd_float4::Load3PtrU(&src_joint.transform.scale.x);
      } else {
        translations[j] = zero;
        rotations[j] = w_axis;
        scales[j] = one;
      }
    }
    // Fills the SoaTransform structure.
    math::Transpose4x3(translations, &skeleton->bind_pose_[i].translation.x);
    math::Transpose4x4(rotations, &skeleton->bind_pose_[i].rotation.x);
    math::Transpose4x3(scales, &skeleton->bind_pose_[i].scale.x);
  }

  return skeleton;  // Success.
}
}  // offline
}  // animation
}  // ozz
