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

#ifndef OZZ_OZZ_ANIMATION_OFFLINE_SKELETON_BUILDER_H_
#define OZZ_OZZ_ANIMATION_OFFLINE_SKELETON_BUILDER_H_

#include "ozz/base/containers/vector.h"
#include "ozz/base/containers/string.h"

#include "ozz/base/maths/transform.h"

namespace ozz {
namespace animation {

// Forward declares the runtime skeleton type.
class Skeleton;

namespace offline {

// Forward declares the offline skeleton type.
struct RawSkeleton;

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
