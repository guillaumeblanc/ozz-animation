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

#include "ozz/animation/runtime/motion_blending_job.h"

#include "ozz/base/maths/transform.h"

namespace ozz {
namespace animation {

bool MotionBlendingJob::Validate() const {
  bool valid = true;

  valid &= output != nullptr;

  // Validates layers.
  for (const Layer& layer : layers) {
    valid &= layer.delta != nullptr;
  }

  return valid;
}

bool MotionBlendingJob::Run() const {
  if (!Validate()) {
    return false;
  }

  // Lerp each layer's transform according to its weight.
  float acc_weight = 0.f;  // Accumulates weights for normalization.
  float dl = 0.f;          // Weighted translation lengths.
  ozz::math::Float3 dt = {0.f, 0.f, 0.f};  // Weighted translations directions.
  ozz::math::Quaternion dr = {0.f, 0.f, 0.f, 0.f};  // Weighted rotations.

  for (const auto& layer : layers) {
    const float weight = layer.weight;
    if (weight <= 0.f) {  // Negative weights are considered O.
      continue;
    }
    acc_weight += weight;

    // Decomposes translation into a normalized vector and a length, to limit
    // lerp error while interpolating vector length.
    const float len = Length(layer.delta->translation);
    dl = dl + len * weight;
    const float denom = (len == 0.f) ? 0.f : weight / len;
    dt = dt + layer.delta->translation * denom;

    // Accumulates weighted rotation (NLerp). Makes sure quaternions are on the
    // same hemisphere to lerp the shortest arc (using -Q otherwise).
    const float signed_weight =
        std::copysign(weight, Dot(dr, layer.delta->rotation));
    dr = dr + layer.delta->rotation * signed_weight;
  }

  // Normalizes (weights) and fills output.

  // Normalizes translation and re-applies interpolated length.
  const float denom = Length(dt) * acc_weight;
  const float norm = (denom == 0.f) ? 0.f : dl / denom;
  output->translation = dt * norm;

  // Normalizes rotation (doesn't need acc_weight).
  output->rotation = NormalizeSafe(dr, ozz::math::Quaternion::identity());

  // No scale.
  output->scale = {1.f, 1.f, 1.f};

  return true;
}
}  // namespace animation
}  // namespace ozz
