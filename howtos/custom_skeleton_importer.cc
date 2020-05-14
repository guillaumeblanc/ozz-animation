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

#include "ozz/animation/runtime/skeleton.h"

#include <cstdlib>

// Code for ozz-animation HowTo: "How to write a custon skeleton importer?"

int main(int argc, char const* argv[]) {
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

  // Setup root joints bind-pose/rest transformation, in joint local-space.
  // This is the default skeleton posture (most of the time a T-pose). It's
  // used as a fallback when there's no animation for a joint.
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
  // This operation will fail and return an empty unique_ptr if the RawSkeleton
  // isn't valid.
  ozz::unique_ptr<ozz::animation::Skeleton> skeleton = builder(raw_skeleton);

  // ...use the skeleton as you want...

  return EXIT_SUCCESS;
}
