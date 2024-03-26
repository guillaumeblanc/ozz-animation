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

#ifndef OZZ_OZZ_ANIMATION_OFFLINE_MOTION_EXTRACTOR_H_
#define OZZ_OZZ_ANIMATION_OFFLINE_MOTION_EXTRACTOR_H_

#include "ozz/animation/offline/export.h"

namespace ozz {
namespace animation {
class Skeleton;
namespace offline {

// Forward declare offline types.
struct RawAnimation;
struct RawFloat3Track;
struct RawQuaternionTrack;

class OZZ_ANIMOFFLINE_DLL MotionExtractor {
 public:
  bool operator()(const RawAnimation& _input, const Skeleton& _skeleton,
                  RawFloat3Track* _motion_position,
                  RawQuaternionTrack* _motion_rotation,
                  RawAnimation* _output) const;

  // Index of the joint that will be used as root joint to extract root motion.
  int root_joint = 0;

  // Defines the reference transform to use while extracting root motion.
  enum class Reference {
    kIdentity,    // Identity / global reference
    kSkeleton,    // Use skeleton rest pose root bone transform
    kFirstFrame,  // Uses root transform of the animation's first frame
  };

  struct Settings {
    bool x, y, z;
    Reference reference;
    bool bake;
  };

  Settings position_settings = {true, false, true, Reference::kSkeleton,
                                true};  // X and Z projection
  Settings rotation_settings = {false, true, false, Reference::kSkeleton,
                                true};  // Y / Yaw only
};
}  // namespace offline
}  // namespace animation
}  // namespace ozz
#endif  // OZZ_OZZ_ANIMATION_OFFLINE_MOTION_EXTRACTOR_H_
