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

#ifndef OZZ_OZZ_ANIMATION_OFFLINE_SKELETON_BUILDER_H_
#define OZZ_OZZ_ANIMATION_OFFLINE_SKELETON_BUILDER_H_

#include "ozz/base/containers/vector.h"
#include "ozz/base/containers/string.h"

#include "ozz/base/maths/transform.h"

namespace ozz {
namespace animation {

// Forward declares the run time skeleton type.
class Skeleton;

namespace offline {

// Off-line skeleton type.
// This skeleton type is not intended to be used in run time. It is used to
// define the offline skeleton object that can be converted to the runtime
// skeleton using the SkeletonBuilder. This skeleton structure exposes joints'
// hierarchy. A joint is defined with a name, a transformation (its bind pose),
// and its children. Children are exposed as a public std::vector of joints.
// This same type is used for skeleton roots, also exposed from the public API.
// The public API exposed through std:vector's of joints can be used freely with
// the only restriction that the total number of joints does not exceed
// Skeleton::kMaxJoints.
struct RawSkeleton {
  // Construct an empty skeleton.
  RawSkeleton();

  // The destructor is responsible for deleting the roots and their hierarchy.
  ~RawSkeleton();

  // Offline skeleton joint type.
  struct Joint {
    // Type of the list of children joints.
    typedef ozz::Vector<Joint>::Std Children;

    // Children joints.
    Children children;

    // The name of the joint.
    ozz::String name;

    // Joint bind pose transformation in local space.
    math::Transform transform;
  };

  // Tests for *this validity.
  // Returns true on success or false on failure if the number of joints exceeds
  // ozz::Skeleton::kMaxJoints.
  bool Validate() const;

  // Returns the number of joints of *this animation.
  // This function is not constant time as it iterates the hierarchy of joints
  // and counts them.
  int num_joints() const;

  // Applies a specified functor to each joint in a depth-first order.
  // _Fct is of type void(const Joint& _current, const Joint* _parent) where the
  // first argument is the child of the second argument. _parent is null if the
  // _current joint is the root.
  template<typename _Fct>
  _Fct IterateJointsDF(_Fct _fct) const {
    IterHierarchyDF(roots, NULL, _fct);
    return _fct;
  }

  // Applies a specified functor to each joint in a breadth-first order.
  // _Fct is of type void(const Joint& _current, const Joint* _parent) where the
  // first argument is the child of the second argument. _parent is null if the
  // _current joint is the root.
  template<typename _Fct>
  _Fct IterateJointsBF(_Fct _fct) const {
    IterHierarchyBF(roots, NULL, _fct);
    return _fct;
  }

  // Declares the skeleton's roots. Can be empty if the skeleton has no joint.
  Joint::Children roots;

 private:
  // Internal function used to iterate through joint hierarchy depth-first.
  template<typename _Fct>
  static void IterHierarchyDF(const RawSkeleton::Joint::Children& _children,
                              const RawSkeleton::Joint* _parent,
                              _Fct& _fct) {
    for (std::size_t i = 0; i < _children.size(); ++i) {
      const RawSkeleton::Joint& current = _children[i];
      _fct(current, _parent);
      IterHierarchyDF(current.children, &current, _fct);
    }
  }

  // Internal function used to iterate through joint hierarchy breadth-first.
  template<typename _Fct>
  static void IterHierarchyBF(const RawSkeleton::Joint::Children& _children,
                              const RawSkeleton::Joint* _parent,
                              _Fct& _fct) {
    for (std::size_t i = 0; i < _children.size(); ++i) {
      const RawSkeleton::Joint& current = _children[i];
      _fct(current, _parent);
    }
    for (std::size_t i = 0; i < _children.size(); i++) {
      const RawSkeleton::Joint& current = _children[i];
      IterHierarchyBF(current.children, &current, _fct);
    }
  }
};

// Defines the class responsible of building Skeleton instances.
class SkeletonBuilder {
 public:
  // Creates a Skeleton based on _raw_skeleton and *this builder parameters.
  // Returns a Skeleton instance on success which will then be deleted using
  // the default allocator Delete() function.
  // Returns NULL on failure. See RawSkeleton::Validate() for more details about
  // failure reasons.
  Skeleton* operator()(const RawSkeleton& _raw_skeleton) const;
};
}  // offline
}  // animation
}  // ozz
#endif  // OZZ_OZZ_ANIMATION_OFFLINE_SKELETON_BUILDER_H_
