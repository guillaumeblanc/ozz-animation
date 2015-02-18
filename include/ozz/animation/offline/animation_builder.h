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

#ifndef OZZ_OZZ_ANIMATION_OFFLINE_ANIMATION_BUILDER_H_
#define OZZ_OZZ_ANIMATION_OFFLINE_ANIMATION_BUILDER_H_

namespace ozz {
namespace animation {

// Forward declares the runtime animation type.
class Animation;

namespace offline {

// Forward declares the offline animation type.
struct RawAnimation;

// Defines the class responsible of building runtime animation instances from
// offline raw animations.
// No optimization at all is performed on the raw animation.
class AnimationBuilder {
 public:
  // Creates an Animation based on _raw_animation and *this builder parameters.
  // Returns a valid Animation on success
  // The returned animation will then need to be deleted using the default 
  // allocator Delete() function.
  // See RawAnimation::Validate() for more details about failure reasons.
  Animation* operator()(const RawAnimation& _raw_animation) const;
};
}  // offline
}  // animation
}  // ozz
#endif  // OZZ_OZZ_ANIMATION_OFFLINE_ANIMATION_BUILDER_H_
