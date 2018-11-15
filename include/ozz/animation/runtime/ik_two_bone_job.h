//----------------------------------------------------------------------------//
//                                                                            //
// ozz-animation is hosted at http://github.com/guillaumeblanc/ozz-animation  //
// and distributed under the MIT License (MIT).                               //
//                                                                            //
// Copyright (c) 2017 Guillaume Blanc                                         //
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

#ifndef OZZ_OZZ_ANIMATION_RUNTIME_TWO_BONE_IK_JOB_H_
#define OZZ_OZZ_ANIMATION_RUNTIME_TWO_BONE_IK_JOB_H_

#include "ozz/base/platform.h"

#include "ozz/base/maths/simd_math.h"

namespace ozz {
// Forward declaration of math structures.
namespace math {
struct SimdQuaternion;
}

namespace animation {

struct IKTwoBoneJob {
  // Default constructor, initializes default values.
  IKTwoBoneJob();

  // Validates job parameters. Returns true for a valid job, or false otherwise:
  // -if any input pointer is NULL
  // -if mid_axis isn't normalized.
  bool Validate() const;

  // Runs job's sampling task.
  // The job is validated before any operation is performed, see Validate() for
  // more details.
  // Returns false if *this job is not valid.
  bool Run() const;

  // Job input.

  // Tagert IK position, in model-space.
  math::SimdFloat4 handle;

  // Pole vector, in model-space. The pole vector defines where the direction
  // the middle joint should point to, allowing to control IK chain orientation.
  // Note that IK chain orientation will flip when handle vector and the pole
  // vector are aligned/crossing each other. It's caller responsibility to
  // ensure that this doens't happen.
  math::SimdFloat4 pole_vector;

  // Normalized middle joint rotation axis, in middle joint local-space. Default
  // value is z axis. This axis is usually fixed for a given skeleton (as it's
  // in middle joint space). If the two bones are not aligned, then this axis
  // can be computed as the cross product of mid-to-start and mid_to-end
  // vectors. Direction of this axis is defined like this: a positive rotation
  // around this axis will open the angle between the two bones. This in turn
  // also defines which side the two joints must bend.
  math::SimdFloat4 mid_axis;

  // Weight given to the IK correction clamped in range [0,1]. This allows to
  // blend / interpolate from no IK (0 weight) to full IK (1).
  float weight;

  // soften ratio allows the chain to gradually fall behind the the handle
  // position. This prevent the joint chain from snapping into the final
  // position, softening
  float soften;

  // twist_angle rotates IK chain orientation around the vector define by start
  // to handle direction. Default is 0.
  float twist_angle;

  // Model-space matrices of the start, middle and end joints of the chain.
  // The 3 joints should be in the same hierarchy. They don't need to be direct
  // children though.
  const math::Float4x4* start_joint;
  const math::Float4x4* mid_joint;
  const math::Float4x4* end_joint;

  // Job output.

  // Local spaces correction to apply to start and middle joints in order for
  // end joint to reach handle position.
  math::SimdQuaternion* start_joint_correction;
  math::SimdQuaternion* mid_joint_correction;
};
}  // namespace animation
}  // namespace ozz
#endif  // OZZ_OZZ_ANIMATION_RUNTIME_TWO_BONE_IK_JOB_H_
