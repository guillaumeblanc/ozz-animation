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

#include "ozz/animation/runtime/ik_aim_job.h"

#include <cassert>

// TEMP

#include "ozz/base/maths/quaternion.h"

#include "ozz/base/log.h"
#include "ozz/base/maths/math_ex.h"
#include "ozz/base/maths/simd_quaternion.h"

using namespace ozz::math;

namespace ozz {
namespace animation {
IKAimJob::IKAimJob()
    : target(math::simd_float4::zero()),
      aim(math::simd_float4::x_axis()),
      up(math::simd_float4::y_axis()),
      pole_vector(math::simd_float4::y_axis()),
      twist_angle(0.f),
      weight(1.f),
      joint(NULL),
      joint_correction(NULL) {}

bool IKAimJob::Validate() const {
  bool valid = true;
  valid &= joint != NULL;
  valid &= joint_correction != NULL;
  return valid;
}

bool IKAimJob::Run() const {
  if (!Validate()) {
    return false;
  }

  using math::Float4x4;
  using math::SimdFloat4;
  using math::SimdQuaternion;

  // If matrices aren't invertible, they'll be all 0 (ozz::math
  // implementation), which will result in identity correction quaternions.
  SimdInt4 invertible;
  const Float4x4 inv_joint = Invert(*joint, &invertible);

  // Computes joint to target vector, in joint local-space (js).
  const SimdFloat4 joint_to_target_js = TransformPoint(inv_joint, target);
  const SimdFloat4 joint_to_target_js_len2 = Length3Sqr(joint_to_target_js);

  if (AreAllTrue1(CmpEq(joint_to_target_js_len2, simd_float4::zero()))) {
    // Target is too close to joint position to find a direction.
    *joint_correction = SimdQuaternion::identity();
  } else {
    // Calculates joint_to_target_rot_ss quaternion which solves for aim vector
    // rotating onto the target.
    const SimdQuaternion joint_to_target_rot_js =
        SimdQuaternion::FromVectors(aim, joint_to_target_js);

    // Calculates rotate_plane_js quaternion which aligns joint up to the pole
    // vector.
    const SimdFloat4 corrected_up_js =
        TransformVector(joint_to_target_rot_js, up);

    // Compute (and normalize) reference and pole planes normals.
    const SimdFloat4 pole_vector_js = TransformVector(inv_joint, pole_vector);
    const SimdFloat4 ref_joint_normal_js =
        Cross3(pole_vector_js, joint_to_target_js);
    const SimdFloat4 joint_normal_js =
        Cross3(corrected_up_js, joint_to_target_js);
    const SimdFloat4 ref_joint_normal_js_len2 = Length3Sqr(ref_joint_normal_js);
    const SimdFloat4 joint_normal_js_len2 = Length3Sqr(joint_normal_js);
    const SimdFloat4 rsqrts =
        RSqrtEstNR(SetZ(SetY(joint_to_target_js_len2, joint_normal_js_len2),
                        ref_joint_normal_js_len2));

    // Computes angle cosine between the 2 normalized plane normals.
    const SimdFloat4 rotate_plane_cos_angle = ozz::math::Dot3(
        joint_normal_js * SplatY(rsqrts), ref_joint_normal_js * SplatZ(rsqrts));

    // Computes rotation axis, which is either joint_to_target_js or
    // -joint_to_target_js depending on rotation direction.
    const SimdFloat4 rotate_plane_axis_js = joint_to_target_js * SplatX(rsqrts);
    const SimdFloat4 axis_flip =
        And(SplatX(Dot3(ref_joint_normal_js, corrected_up_js)),
            ozz::math::simd_int4::mask_sign());
    const SimdFloat4 rotate_plane_axis_flipped_js =
        Xor(rotate_plane_axis_js, axis_flip);

    // Builds quaternion along rotation axis.
    const SimdQuaternion rotate_plane_js = SimdQuaternion::FromAxisCosAngle(
        rotate_plane_axis_flipped_js,
        Clamp(-ozz::math::simd_float4::one(), rotate_plane_cos_angle,
              ozz::math::simd_float4::one()));

    // Twists rotation plane.
    SimdQuaternion twisted;
    if (twist_angle != 0.f) {
      // If a twist angle is provided, rotation angle is rotated aim vector.
      const SimdQuaternion twist_ss = SimdQuaternion::FromAxisAngle(
          rotate_plane_axis_js, simd_float4::Load1(twist_angle));
      twisted = twist_ss * rotate_plane_js * joint_to_target_rot_js;
    } else {
      twisted = rotate_plane_js * joint_to_target_rot_js;
    }

    // Weights output quaternion.

    // Fix up quaternions so w is always positive, which is required for NLerp
    // (with identity quaternion) to lerp the shortest path.
    const SimdFloat4 twisted_fu = Xor(
        twisted.xyzw, And(simd_int4::mask_sign(),
                          CmpLt(SplatW(twisted.xyzw), simd_float4::zero())));

    if (weight < 1.f) {
      // NLerp start and mid joint rotations.
      const SimdFloat4 identity = simd_float4::w_axis();
      const SimdFloat4 simd_weight = Max0(simd_float4::Load1(weight));
      joint_correction->xyzw =
          NormalizeEst4(Lerp(identity, twisted.xyzw, simd_weight));
    } else {
      // Quatenions don't need interpolation
      joint_correction->xyzw = twisted_fu;
    }
  }

  return true;
}
}  // namespace animation
}  // namespace ozz
