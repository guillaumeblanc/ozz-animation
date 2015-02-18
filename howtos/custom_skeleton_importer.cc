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

#include "ozz/animation/runtime/skeleton.h"

#include <cstdlib>

// Code for ozz-animation HowTo: "How to write a custon skeleton importer?"
// "http://code.google.com/p/ozz-animation/wiki/HowTos#How_to_write_a_custom_skeleton_importer?"

int main(int argc, char const *argv[]) {
  (void)argc;
  (void)argv;

  //////////////////////////////////////////////////////////////////////////////
  // The first section builds a RawSkeleton from custom data.
  //////////////////////////////////////////////////////////////////////////////

  // Creates a RawSkeleton.
  ozz::animation::offline::RawSkeleton raw_skeleton;

  // Creates the root joint.
  raw_skeleton.roots.resize(1);
  ozz::animation::offline::RawSkeleton::Joint& root = raw_skeleton.roots[0];

  // Setup root joints name.
  root.name = "root";

  // Setup root joints bind-pose/rest transformation. It's kind of the default
  // skeleton posture (most of the time a T-pose). It's used as a fallback when
  // there's no animation for a joint.
  root.transform.translation = ozz::math::Float3(0.f, 1.f, 0.f);
  root.transform.rotation = ozz::math::Quaternion(0.f, 0.f, 0.f, 1.f);
  root.transform.scale = ozz::math::Float3(1.f, 1.f, 1.f);

  // Now adds 2 children to the root.
  root.children.resize(2);

  // Setups the 1st child name (left) and transfomation.
  ozz::animation::offline::RawSkeleton::Joint& left = root.children[0];
  left.name = "left";
  left.transform.translation = ozz::math::Float3(1.f, 0.f, 0.f);
  left.transform.rotation = ozz::math::Quaternion(0.f, 0.f, 0.f, 1.f);
  left.transform.scale = ozz::math::Float3(1.f, 1.f, 1.f);

  // Setups the 2nd child name (right) and transfomation.
  ozz::animation::offline::RawSkeleton::Joint& right = root.children[1];
  right.name = "right";
  right.transform.translation = ozz::math::Float3(-1.f, 0.f, 0.f);
  right.transform.rotation = ozz::math::Quaternion(0.f, 0.f, 0.f, 1.f);
  right.transform.scale = ozz::math::Float3(1.f, 1.f, 1.f);

  //...and so on with the whole skeleton hierarchy...

  // Test for skeleton validity.
  // The main invalidity reason is the number of joints, which must be lower
  // than ozz::animation::Skeleton::kMaxJoints.
  if (!raw_skeleton.Validate()) {
    return EXIT_FAILURE;
  }

  //////////////////////////////////////////////////////////////////////////////
  // This final section converts the RawSkeleton to a runtime Skeleton.
  //////////////////////////////////////////////////////////////////////////////

  // Creates a SkeletonBuilder instance.
  ozz::animation::offline::SkeletonBuilder builder;

  // Executes the builder on the previously prepared RawSkeleton, which returns
  // a new runtime skeleton instance.
  // This operation will fail and return NULL if the RawSkeleton isn't valid.
  ozz::animation::Skeleton* skeleton = builder(raw_skeleton);

  // ...use the skeleton as you want...

  // In the end the skeleton needs to be deleted.
  ozz::memory::default_allocator()->Delete(skeleton);

  return EXIT_SUCCESS;
}
