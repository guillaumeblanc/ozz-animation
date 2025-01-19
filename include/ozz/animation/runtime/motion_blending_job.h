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

#ifndef OZZ_OZZ_ANIMATION_RUNTIME_MOTION_BLENDING_JOB_H_
#define OZZ_OZZ_ANIMATION_RUNTIME_MOTION_BLENDING_JOB_H_

#include "ozz/animation/runtime/export.h"
#include "ozz/base/span.h"

namespace ozz {

// Forward declaration of math structures.
namespace math {
struct Transform;
}

namespace animation {

// ozz::animation::MotionBlendingJob is in charge of blending delta motions
// according to their respective weight. MotionBlendingJob is usually done to
// blend the motion resulting from the motion extraction process, in parallel to
// blending animations.
struct OZZ_ANIMATION_DLL MotionBlendingJob {
  // Validates job parameters.
  // Returns true for a valid job, false otherwise:
  // -if a layer transform pointer is null.
  // -if output transform pointer is null.
  bool Validate() const;

  // Runs job's blending task.
  // The job is validated before any operation is performed, see Validate() for
  // more details.
  // Returns false if *this job is not valid.
  bool Run() const;

  // Defines a layer of blending input data and its weight.
  struct OZZ_ANIMATION_DLL Layer {
    // Blending weight of this layer. Negative values are considered as 0.
    // Normalization is performed at the end of the blending stage, so weight
    // can be in any range, even though range [0:1] is optimal.
    float weight = 0.f;

    // The motion delta transform to be blended.
    const math::Transform* delta = nullptr;
  };

  // Job input layers, can be empty or nullptr.
  // The range of layers that must be blended.
  span<const Layer> layers;

  // Job output.
  ozz::math::Transform* output = nullptr;
};
}  // namespace animation
}  // namespace ozz
#endif  // OZZ_OZZ_ANIMATION_RUNTIME_MOTION_BLENDING_JOB_H_
