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

#include "ozz/animation/runtime/skeleton_utils.h"

#include "ozz/base/maths/soa_transform.h"

#include <assert.h>

namespace ozz {
namespace animation {

// Unpacks skeleton bind pose stored in soa format by the skeleton.
ozz::math::Transform GetJointLocalBindPose(const Skeleton& _skeleton,
                                           int _joint) {
  assert(_joint >= 0 && _joint < _skeleton.num_joints() &&
         "Joint index out of range.");

  const ozz::math::SoaTransform& soa_transform =
      _skeleton.joint_bind_poses()[_joint / 4];

  // Transpose SoA data to AoS.
  ozz::math::SimdFloat4 translations[4];
  ozz::math::Transpose3x4(&soa_transform.translation.x, translations);
  ozz::math::SimdFloat4 rotations[4];
  ozz::math::Transpose4x4(&soa_transform.rotation.x, rotations);
  ozz::math::SimdFloat4 scales[4];
  ozz::math::Transpose3x4(&soa_transform.scale.x, scales);

  // Stores to the Transform object.
  math::Transform bind_pose;
  const int offset = _joint % 4;
  ozz::math::Store3PtrU(translations[offset], &bind_pose.translation.x);
  ozz::math::StorePtrU(rotations[offset], &bind_pose.rotation.x);
  ozz::math::Store3PtrU(scales[offset], &bind_pose.scale.x);

  return bind_pose;
}
}  // namespace animation
}  // namespace ozz
