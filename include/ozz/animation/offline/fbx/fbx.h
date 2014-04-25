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

#ifndef OZZ_OZZ_ANIMATION_OFFLINE_FBX_FBX_H_
#define OZZ_OZZ_ANIMATION_OFFLINE_FBX_FBX_H_

namespace ozz {
namespace animation {

//  Forward declares ozz runtime skeleton type.
class Skeleton;

namespace offline {

//  Forward declares ozz offline animation and skeleton types.
struct RawSkeleton;
struct RawAnimation;

namespace fbx {

// Imports an offline skeleton from _filename fbx document.
// _skeleton must point to a valid RawSkeleton instance, that will be cleared
// and filled with skeleton data extracted from the fbx document.
bool ImportFromFile(const char* _filename, RawSkeleton* _skeleton);

// Imports an offline animation from _filename fbx document.
// _animation must point to a valid RawSkeleton instance, that will be cleared
// and filled with animation data extracted from the fbx document.
// _skeleton is a run-time Skeleton object used to select and sort animation
// tracks.
bool ImportFromFile(const char* _filename,
                    const Skeleton& _skeleton,
                    float _sampling_rate,
                    RawAnimation* _animation);
}  // fbx
}  // offline
}  // animation
}  // ozz
#endif  // OZZ_OZZ_ANIMATION_OFFLINE_FBX_FBX_H_
