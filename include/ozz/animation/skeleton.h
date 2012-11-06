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

#ifndef OZZ_OZZ_ANIMATION_SKELETON_H_
#define OZZ_OZZ_ANIMATION_SKELETON_H_

#include "ozz/base/platform.h"
namespace ozz {
namespace io { class IArchive; class OArchive; }
namespace math { struct SoaTransform; }
namespace animation {

// Forward declaration of SkeletonBuilder, used to instantiate a skeleton.
namespace offline { class SkeletonBuilder; }

class Skeleton {
 public:

  // Defines the maximum number of joints, fixed by JointProperties.
  static const int kMaxJoints;

  // Builds a default skeleton.
  Skeleton();

  // Declares the public non-virtual destructor.
  ~Skeleton();

  // Returns the number of joints of *this skeleton.
  int num_joints() const {
    return num_joints_;
  }

  // Per joint properties.
  struct JointProperties {
    short parent:15;  // Parent's index. Roots (joints with no parent) have an
                      // index of -1.
    unsigned short is_leaf:1;  // Set to 1 for a leaf, 0 for a branch.
  };

  // Returns joint's parent indices.
  const JointProperties* joint_properties() const {
    return joint_properties_;
  }

  // Returns the end of the buffer of joint's parent indices.
  const JointProperties* joint_properties_end() const {
    return joint_properties_ + num_joints_;
  }

  // Returns joint's bind poses. Bind poses are stored in soa format.
  const math::SoaTransform* bind_pose() const {
    return bind_pose_;
  }

  // Returns the end of the buffer of joint's bind poses.
  const math::SoaTransform* bind_pose_end() const;

  // Returns joint's name collection.
  const char* const* joint_names() const {
    return joint_names_;
  }

  // Serialization functions.
  void Save(ozz::io::OArchive& _archive) const;
  void Load(ozz::io::IArchive& _archive, ozz::uint32 _version);

 private:

  // SkeletonBuilder class is allowed to instantiate an Skeleton.
  friend class offline::SkeletonBuilder;

  // Buffers below store joint informations in DAG order. Their size is equal to
  // the number of joints of the skeleton.

  // Array of joint properties.
  JointProperties* joint_properties_;

  // Bind pose of every joint in local space.
  math::SoaTransform* bind_pose_;

  // Stores the name of every joint in an array of c-strings.
  // Uses a single allocation to store the array and all the c strings.
  char** joint_names_;

  // The number of joints.
  int num_joints_;
};
}  // animation 
}  // ozz
#endif  // OZZ_OZZ_ANIMATION_SKELETON_H_
