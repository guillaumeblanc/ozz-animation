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

#ifndef OZZ_OZZ_ANIMATION_RUNTIME_SKELETON_UTILS_H_
#define OZZ_OZZ_ANIMATION_RUNTIME_SKELETON_UTILS_H_

#include "skeleton.h"

#include "ozz/base/maths/transform.h"

namespace ozz {
namespace animation {

// Get bind-pose of a skeleton joint.
ozz::math::Transform GetJointBindPose(const Skeleton& _skeleton, int _joint);

// Defines the iterator structure used by IterateJointsDF to traverse joint
// hierarchy.
struct JointsIterator {
  uint16_t joints[Skeleton::kMaxJoints];
  int num_joints;
};

// Fills _iterator with the index of the joints of _skeleton traversed in depth-
// first order.
// _from indicates the join from which the joint hierarchy traversal begins. Use
// Skeleton::kNoParentIndex to traverse the whole hierarchy, even if there are
// multiple joints.
// This function does not use a recursive implementation, to enforce a
// predictable stack usage, independent off the data (joint hierarchy) being
// processed.
void IterateJointsDF(const Skeleton& _skeleton,
                     int _from,
                     JointsIterator* _iterator);

// Applies a specified functor to each joint in a depth-first order.
// _Fct is of type void(int _current, int _parent) where the first argument is
// the child of the second argument. _parent is kNoParentIndex if the _current
// joint is the root.
// _from indicates the join from which the joint hierarchy traversal begins. Use
// Skeleton::kNoParentIndex to traverse the whole hierarchy, even if there are
// multiple joints.
// This implementation is based on IterateJointsDF(*, *, JointsIterator$)
// variant.
template<typename _Fct>
inline _Fct IterateJointsDF(const Skeleton& _skeleton, int _from, _Fct _fct) {
  // Iterates and fills iterator.
  JointsIterator iterator;
  IterateJointsDF(_skeleton, _from, &iterator);

  // Consumes iterator and call _fct.
  Range<const Skeleton::JointProperties> properties =
    _skeleton.joint_properties();
  for (int i = 0; i < iterator.num_joints; ++i) {
    const int joint = iterator.joints[i];
    _fct(joint, properties.begin[joint].parent);
  }
  return _fct;
}
}  // animation
}  // ozz
#endif  // OZZ_OZZ_ANIMATION_RUNTIME_SKELETON_UTILS_H_
