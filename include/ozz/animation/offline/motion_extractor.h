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

// Defines the class responsible of extracting root motion from a raw animation
// object. Root motion defines how a character moves during an animation. The
// utility extracts the motion (position and rotation) from a root joint of the
// animation into separate tracks, and removes (bake) that motion from the
// original animation. User code is expected to reapply motion at runtime by
// moving the character transform, hence reconstructing the original animation.
// Position and rotation components of the extracted motion can be selected.
// This allows for example to project motion position on the XZ plane, or
// isolate rotation around y axis.
// Motion is computed as the difference from a reference, which can be the
// identity/global, the skeleton rest pose, or the animation's first frame.
class OZZ_ANIMOFFLINE_DLL MotionExtractor {
 public:
  // Executes extraction based on provided settings.
  bool operator()(const RawAnimation& _input, const Skeleton& _skeleton,
                  RawFloat3Track* _motion_position,
                  RawQuaternionTrack* _motion_rotation,
                  RawAnimation* _output) const;

  // Index of the joint that will be used as root to extract motion.
  int root_joint = 0;

  // Defines the reference transform to use while extracting root motion.
  enum class Reference {
    kAbsolute,   // Global / absolute reference.
    kSkeleton,   // Use skeleton rest pose root bone transform.
    kAnimation,  // Uses root transform of the animation's first frame.
  };

  struct Settings {
    bool x, y, z;         // Extract X, Y, Z components.
    Reference reference;  // Extracting reference.
    bool bake;            // Bake extracted data to output animation.
    bool loop;  // Makes end transformation equal to begin to make animation
                // loopable. Difference between end and begin is distributed all
                // along animation duration.
  };

  Settings position_settings = {true,
                                false,
                                true,                  // X and Z projection
                                Reference::kSkeleton,  // Reference
                                true,    // Bake extracted position
                                false};  // Don't loop position
  Settings rotation_settings = {false,
                                true,
                                false,                 // Y / Yaw only
                                Reference::kSkeleton,  // Reference
                                true,    // Bake extracted rotation
                                false};  // Don't loop rotation
};
}  // namespace offline
}  // namespace animation
}  // namespace ozz
#endif  // OZZ_OZZ_ANIMATION_OFFLINE_MOTION_EXTRACTOR_H_
