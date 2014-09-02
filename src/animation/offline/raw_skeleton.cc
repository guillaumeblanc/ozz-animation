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

#include "ozz/animation/offline/raw_skeleton.h"

#include "ozz/animation/runtime/skeleton.h"

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
    ++num_joints;
  }
  int num_joints;
};
}  // namespace

// Iterates through all the root children and count them.
int RawSkeleton::num_joints() const {
  return IterateJointsDF(JointCounter()).num_joints;
}
}  // offline
}  // animation
}  // ozz
