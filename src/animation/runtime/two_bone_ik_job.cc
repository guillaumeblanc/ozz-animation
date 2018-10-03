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

#include "ozz/animation/runtime/two_bone_ik_job.h"

#include <cassert>

#include "ozz/base/log.h"
#include "ozz/base/maths/math_ex.h"
#include "ozz/base/maths/simd_quaternion.h"

using namespace ozz::math;

namespace ozz {
namespace animation {
TwoBoneIKJob::TwoBoneIKJob()
    : handle(math::simd_float4::zero()),
      pole_vector(math::simd_float4::y_axis()),
      mid_axis_fallback(math::simd_float4::z_axis()),
      soften(1.f),
      twist_angle(0.f),
      start_joint(NULL),
      mid_joint(NULL),
      end_joint(NULL),
      start_joint_correction(NULL),
      mid_joint_correction(NULL) {}

bool TwoBoneIKJob::Validate() const {
  bool valid = true;
  valid &= start_joint && mid_joint && end_joint;
  valid &= start_joint_correction && mid_joint_correction;
  valid &=
      ozz::math::AreAllTrue1(ozz::math::IsNormalizedEst3(mid_axis_fallback));
  return valid;
}

namespace {
// Smoothen handle position when it's further that a ratio of the joint chain
// length, and start to handle length isn't 0.
// Inspired from http://www.softimageblog.com/archives/108
bool SoftenHandle(_SimdFloat4 _start_mid_ss_len2, _SimdFloat4 _mid_end_ss_len2,
                  _SimdFloat4 _handle_ss, float _soften,
                  SimdFloat4* _start_handle_ss,
                  SimdFloat4* _start_handle_ss_len2) {
  const SimdFloat4& start_handle_original_ss = _handle_ss;
  const SimdFloat4 start_handle_original_ss_len2 = Length3Sqr(_handle_ss);

  const SimdFloat4 bones_len = Sqrt(SetY(_start_mid_ss_len2, _mid_end_ss_len2));
  const SimdFloat4 bones_chain_len = bones_len + SplatY(bones_len);
  const SimdFloat4 da =
      bones_chain_len * simd_float4::LoadX(Clamp(_soften, 0.f, 1.f));
  const SimdFloat4 ds = bones_chain_len - da;

  // Sotftens handle position if it is further than a ratio (_soften) of the
  // whole bone chain length. Needs to check also ds and
  // start_handle_original_ss_len2 are != 0, because they're used as a
  // denominator. Note that da.yzw == 0
  const SimdFloat4 comperand = SetZ(SplatX(start_handle_original_ss_len2), ds);
  bool needs_softening = AreAllTrue3(CmpGt(comperand, da * da));
  if (needs_softening) {
    // Finds interpolation ratio (aka alpha).
    const SimdFloat4 start_handle_original_ss_inv_len =
        RSqrtEstX(start_handle_original_ss_len2);
    const SimdFloat4 start_handle_original_ss_len = //  x^.5 = x^2 / (x^2)^.5
        start_handle_original_ss_len2 * start_handle_original_ss_inv_len;
    const SimdFloat4 alpha = (start_handle_original_ss_len - da) * RcpEstX(ds);
    // Approximate an exponential function with : 1-(3^4)/(alpha+3)^4
    // The derivative must be 1 for x = 0, and y must never exceeds 1.
    // Negative x aren't used.
    const SimdFloat4 three = simd_float4::Load1(3.f);
    const SimdFloat4 op = SetY(three, alpha + three);
    const SimdFloat4 op2 = op * op;
    const SimdFloat4 op4 = op2 * op2;
    const SimdFloat4 smoothed_ratio =
        simd_float4::one() - op4 * RcpEstX(SplatY(op4));

    // Recomputes start_handle_ss vector and length.
    const SimdFloat4 start_handle_ss_len = ds * smoothed_ratio + da;
    *_start_handle_ss_len2 = start_handle_ss_len * start_handle_ss_len;
    *_start_handle_ss =
        start_handle_original_ss *
        SplatX(start_handle_ss_len * start_handle_original_ss_inv_len);
  } else {
    *_start_handle_ss = start_handle_original_ss;
    *_start_handle_ss_len2 = start_handle_original_ss_len2;
  }

  // If handle position is softened, then it means that the real handle isn't
  // reached.
  return !needs_softening;
}
}  // namespace

bool TwoBoneIKJob::Run() const {
  if (!Validate()) {
    return false;
  }

  // Prepares constants
  const SimdFloat4 zero = simd_float4::zero();
  const SimdFloat4 one = simd_float4::one();
  const SimdInt4 mask_sign = simd_int4::mask_sign();

  // Computes inverse matrices required to change to start and mid spaces.
  const Float4x4 inv_start_joint = Invert(*start_joint);
  const Float4x4 inv_mid_joint = Invert(*mid_joint);

  // Transform some positions to mid joint space (_ms)
  const SimdFloat4 start_ms =
      TransformPoint(inv_mid_joint, start_joint->cols[3]);
  const SimdFloat4 end_ms = TransformPoint(inv_mid_joint, end_joint->cols[3]);

  // Transform some positions to start joint space (_ss)
  const SimdFloat4 mid_ss = TransformPoint(inv_start_joint, mid_joint->cols[3]);
  const SimdFloat4 end_ss = TransformPoint(inv_start_joint, end_joint->cols[3]);
  const SimdFloat4 handle_ss = TransformPoint(inv_start_joint, handle);
  const SimdFloat4 pole_ss = TransformVector(inv_start_joint, pole_vector);

  // Computes bones vectors and length in mid and start spaces.
  // Start joint position will be treated as 0 because all joints are
  // expressed in start joint space.
  const SimdFloat4 start_mid_ms = -start_ms;
  const SimdFloat4& mid_end_ms = end_ms;
  const SimdFloat4& start_mid_ss = mid_ss;
  const SimdFloat4 mid_end_ss = end_ss - mid_ss;
  const SimdFloat4& start_end_ss = end_ss;
  const SimdFloat4 start_mid_ss_len2 = Length3Sqr(start_mid_ss);
  const SimdFloat4 mid_end_ss_len2 = Length3Sqr(mid_end_ss);
  const SimdFloat4 start_end_ss_len2 = Length3Sqr(start_end_ss);

  // Finds soften handle position.
  SimdFloat4 start_handle_ss;
  SimdFloat4 start_handle_ss_len2;
  SoftenHandle(start_mid_ss_len2, mid_end_ss_len2, handle_ss, soften,
               &start_handle_ss, &start_handle_ss_len2);

  // Calculate mid_rot_local quaternion which solves for the mid_ss joint
  // rotation.
  // --------------------------------------------------------------------------

  // Computes expected angle at mid_ss joint, using law of cosine (generalized
  // Pythagorean).
  // c^2 = a^2 + b^2 - 2ab cosC
  // cosC = (a^2 + b^2 - c^2) / 2ab
  // Computes both final and initial mid joint angles
  // cosine within a single SimdFloat4 (corrected is x component, initial is y).
  const SimdFloat4 start_mid_end_sum_ss_len2 =
      start_mid_ss_len2 + mid_end_ss_len2;
  const SimdFloat4 start_mid_end_ss_half_rlen =
      SplatX(simd_float4::Load1(.5f) *
             RSqrtEstXNR(start_mid_ss_len2 * mid_end_ss_len2));
  // Cos value needs to be clamped, as it will exit expected range if
  // start_handle_ss_len2 is longer than the triangle can be (start_mid_ss +
  // mid_end_ss).
  const SimdFloat4 mid_cos_angles =
      Clamp(-one,
            (SplatX(start_mid_end_sum_ss_len2) -
             SetY(start_handle_ss_len2, start_end_ss_len2)) *
                start_mid_end_ss_half_rlen,
            one);

  // Computes final and initial angles difference.
  // Using the formulas for the differences of angles (below) and building
  // the quaternion form cosine avoids computing the 2 ACos here, and the
  // Sin and Cos in quaternion construction.
  // cos(A - B) = cosA * cosB + sinA * sinB
  // cos(A - B) = cosA * cosB + Sqrt((1 - cosA^2) * (1 - cosB^2))
  const SimdFloat4 one_minus_mid_cos_angles2 =
      one - mid_cos_angles * mid_cos_angles;
  const SimdFloat4 mid_cos_angle_diff_unclamped =
      mid_cos_angles * SplatY(mid_cos_angles) +
      SqrtX(Max(zero,
                one_minus_mid_cos_angles2 * SplatY(one_minus_mid_cos_angles2)));
  const SimdFloat4 mid_cos_angle_diff =
      Clamp(-one, mid_cos_angle_diff_unclamped, one);

  // Calculate mid joint axis
  // Falls back to mid_axis_fallback if cross product isn't meaningful,
  // aka start, mid and end are aligned.
  const SimdFloat4 mid_axis_ms =
      NormalizeSafeEst3(Cross3(start_mid_ms, mid_end_ms), mid_axis_fallback);

  // Flip rotation direction if distance to handle is longer than initial
  // distance to end_ss (aka opposite angle).
  const SimdInt4 mid_axis_flip =
      SplatX(CmpLt(start_end_ss_len2, start_handle_ss_len2));
  const SimdFloat4 mid_axis_ms_flipped =
      Xor(mid_axis_ms, And(mid_axis_flip, mask_sign));

  // Mid joint rotation.
  const SimdQuaternion mid_rot_ms =
      SimdQuaternion::FromAxisCosAngle(mid_axis_ms_flipped, mid_cos_angle_diff);

  // Output start and mid joint rotation corrections.
  *mid_joint_correction = mid_rot_ms;

  // Calculates end_to_handle_rot_ss quaternion which solves for effector
  // rotating onto the handle.
  // -----------------------------------------------------------------

  // start_mid_ss with quaternion mid_rot_ms applied.
  const SimdFloat4 mid_end_ss_final = TransformVector(
      inv_start_joint,
      TransformVector(*mid_joint, TransformVector(mid_rot_ms, mid_end_ms)));
  const SimdFloat4 start_end_ss_final = start_mid_ss + mid_end_ss_final;

  // Quaternion for rotating the effector onto the handle
  const SimdQuaternion end_to_handle_rot_ss =
      SimdQuaternion::FromVectors(start_end_ss_final, start_handle_ss);

  // Calculates rotate_plane_rot quaternion which aligns joint chain plane to
  // the reference plane (pole vector). This can only be computed if start
  // handle axis is valid (not 0 length)
  // -------------------------------------------------
  if (AreAllTrue1(CmpGt(start_handle_ss_len2, zero))) {
    // Computes each plane normal.
    const ozz::math::SimdFloat4 ref_plane_normal_ss =
        Cross3(start_handle_ss, pole_ss);
    const ozz::math::SimdFloat4 ref_plane_normal_ss_len2 =
        ozz::math::Length3Sqr(ref_plane_normal_ss);
    // Computes joint chain plane normal, which is the same as mid joint axis
    // (same triangle). Uses -mid_axis_ms because it was computed in the other
    // direction.
    const ozz::math::SimdFloat4 mid_axis_ss = TransformVector(
        inv_start_joint, TransformVector(*mid_joint, -mid_axis_ms));
    const ozz::math::SimdFloat4 joint_plane_normal_ss =
        TransformVector(end_to_handle_rot_ss, mid_axis_ss);
    const ozz::math::SimdFloat4 joint_plane_normal_ss_len2 =
        ozz::math::Length3Sqr(joint_plane_normal_ss);
    // Computes all reciprocal square roots at once.
    const SimdFloat4 rsqrts =
        RSqrtEstNR(SetZ(SetY(start_handle_ss_len2, ref_plane_normal_ss_len2),
                        joint_plane_normal_ss_len2));

    // Computes angle cosine between the 2 normalized normals.
    const SimdFloat4 rotate_plane_cos_angle =
        ozz::math::Dot3(ref_plane_normal_ss * SplatY(rsqrts),
                        joint_plane_normal_ss * SplatZ(rsqrts));

    // Computes rotation axis, which is either start_handle_ss or
    // -start_handle_ss depending on rotation direction.
    const SimdFloat4 rotate_plane_axis_ss = start_handle_ss * SplatX(rsqrts);
    const SimdFloat4 start_axis_flip =
        And(SplatX(Dot3(joint_plane_normal_ss, pole_ss)), mask_sign);
    const SimdFloat4 rotate_plane_axis_flipped_ss =
        Xor(rotate_plane_axis_ss, start_axis_flip);

    // Builds quaternion along rotation axis.
    const SimdQuaternion rotate_plane_ss = SimdQuaternion::FromAxisCosAngle(
        rotate_plane_axis_flipped_ss, Clamp(-one, rotate_plane_cos_angle, one));

    if (twist_angle != 0.f) {
      // If a twist angle is provided, rotation angle is rotated along
      // rotation plane axis.
      const SimdQuaternion twist_ss = SimdQuaternion::FromAxisAngle(
          rotate_plane_axis_ss, simd_float4::LoadX(twist_angle));
      *start_joint_correction =
          twist_ss * rotate_plane_ss * end_to_handle_rot_ss;
    } else {
      *start_joint_correction = rotate_plane_ss * end_to_handle_rot_ss;
    }
  } else {
    // Can't apply pole vector correction.
    *start_joint_correction = end_to_handle_rot_ss;
  }

  return true;
}
}  // namespace animation
}  // namespace ozz
