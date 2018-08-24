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

#include "ozz/base/maths/simd_quaternion.h"

namespace ozz {
namespace animation {

TwoBoneIKJob::TwoBoneIKJob()
    : start_joint(NULL),
      mid_joint(NULL),
      end_joint(NULL),
      handle(math::simd_float4::zero()),
      pole_vector(math::simd_float4::y_axis()),
      start_joint_correction(NULL),
      mid_joint_correction(NULL) {}

bool TwoBoneIKJob::Validate() const {
  bool valid = true;
  valid &= start_joint && mid_joint && end_joint;
  valid &= start_joint_correction && mid_joint_correction;
  return valid;
}

namespace {

bool isParallel(ozz::math::_SimdFloat4 _a, ozz::math::_SimdFloat4 _b) {
  const ozz::math::SimdFloat4 dot =
      ozz::math::Dot3(_a, _a) * ozz::math::Dot3(_b, _b) -
      ozz::math::Dot3(_a, _b) * ozz::math::Dot3(_a, _b);
  return (ozz::math::AreAllTrue1(ozz::math::CmpLe(
      ozz::math::Abs(dot), ozz::math::simd_float4::Load1(.0001f))));
}
/*
const ozz::math::SimdFloat4 dot =
ozz::math::Dot3(_a, _b) * ozz::math::RSqrtEstX(ozz::math::Length3Sqr(_a) *
                                               ozz::math::Length3Sqr(_b));
const ozz::math::SimdFloat4 cmp =
ozz::math::Abs(dot);
return (ozz::math::AreAllTrue1(ozz::math::And(
ozz::math::CmpGe(cmp, ozz::math::simd_float4::Load1(.9999f)),
ozz::math::CmpLe(cmp, ozz::math::simd_float4::Load1(1.0001f)))));
   
} */ /*
 bool isParallel(ozz::math::_SimdFloat4 _a, ozz::math::_SimdFloat4 _b) {
   const ozz::math::SimdFloat4 dot =
       ozz::math::Dot3(ozz::math::Normalize3(_a), ozz::math::Normalize3(_b));
   const ozz::math::SimdFloat4 dot_abs = ozz::math::Abs(dot);
   return (ozz::math::AreAllTrue1(ozz::math::And(
       ozz::math::CmpGe(dot_abs, ozz::math::simd_float4::Load1(.9999f)),
       ozz::math::CmpLe(dot_abs, ozz::math::simd_float4::Load1(1.0001f)))));
 }*/

// https://math.oregonstate.edu/home/programs/undergrad/CalculusQuestStudyGuides/vcalc/dotprod/dotprod.html
// Vector projection is the unit vector a/|a| times the scalar
// projection of b onto a (dot product).
ozz::math::SimdFloat4 project(ozz::math::_SimdFloat4 _a,
                              ozz::math::_SimdFloat4 _b) {
  return _a *
         ozz::math::SplatX(ozz::math::Dot3(_a, _b) *
                           ozz::math::RcpEstXNR(ozz::math::Length3Sqr(_a)));
}

ozz::math::SimdFloat4 project(ozz::math::_SimdFloat4 _a,
                              ozz::math::_SimdFloat4 _b,
                              ozz::math::_SimdFloat4 _rcp_norm_a_sq) {
  return _a * ozz::math::SplatX(ozz::math::Dot3(_a, _b) * _rcp_norm_a_sq);
}

ozz::math::SimdFloat4 ortho(ozz::math::_SimdFloat4 _a,
                            ozz::math::_SimdFloat4 _b) {
  return _a - project(_b, _a);
}

ozz::math::SimdFloat4 ortho(ozz::math::_SimdFloat4 _a,
                            ozz::math::_SimdFloat4 _b,
                            ozz::math::_SimdFloat4 _rcp_norm_a_sq) {
  return _a - project(_b, _a, _rcp_norm_a_sq);
}
}  // namespace

bool TwoBoneIKJob::Run() const {
  if (!Validate()) {
    return false;
  }

  using namespace ozz::math;

  const SimdFloat4 zero = simd_float4::zero();
  const SimdFloat4 one = simd_float4::one();

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
  const SimdFloat4& start_handle_ss = handle_ss;
  const SimdFloat4 start_mid_ss_len2 = Length3Sqr(start_mid_ss);
  const SimdFloat4 mid_end_ss_len2 = Length3Sqr(mid_end_ss);
  const SimdFloat4 start_end_ss_len2 = Length3Sqr(start_end_ss);
  const SimdFloat4 start_handle_ss_len2 = Length3Sqr(start_handle_ss);

  // Calculate mid_rot_local quaternion which solves for the mid_ss joint
  // rotation.
  // --------------------------------------------------------------------------

  // Computes expected angle at mid_ss joint, using law of cosine (generalized
  // Pythagorean).
  // c^2 = a^2 + b^2 - 2ab cosC
  // cosC = (a^2 + b^2 - c^2) / 2ab
  // Computes both corrected and initial mid joint angles
  // cosine within a single SimdFloat4 (corrected is x component, initial is y).
  const SimdFloat4 start_mid_end_sum_ss_len2 =
      start_mid_ss_len2 + mid_end_ss_len2;
  const SimdFloat4 start_mid_end_ss_half_rlen =
      simd_float4::Load1(.5f) *
      SplatX(RSqrtEstX(start_mid_ss_len2 * mid_end_ss_len2));  // 1 / 2ab
  // Cos value needs to be clamped, as it will exit expected range if
  // start_handle_ss_len2 is longer than the triangle can be (start_mid_ss +
  // mid_end_ss).
  const SimdFloat4 mid_cos_angles =
      Clamp(-one,
            (SplatX(start_mid_end_sum_ss_len2) -
             SetY(start_handle_ss_len2, start_end_ss_len2)) *
                start_mid_end_ss_half_rlen,
            one);

  // Computes corrected and initial angles difference.
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

  // Calculate mid joint hinge axis
  // TODO safe ?
  const SimdFloat4 mid_hinge_ms = Normalize3(Cross3(start_mid_ms, mid_end_ms));

  // Flip rotation direction if distance to handle is longer than initial
  // distance to end_ss (aka opposite angle).
  const SimdInt4 flip = SplatX(CmpLt(start_end_ss_len2, start_handle_ss_len2));
  const SimdFloat4 mid_hinge_ms_flipped =
      Xor(mid_hinge_ms, And(flip, simd_int4::mask_sign()));

  // Mid joint rotation.
  const SimdQuaternion mid_rot_ms = SimdQuaternion::FromAxisCosAngle(
      mid_hinge_ms_flipped, mid_cos_angle_diff);

  // Calculates end_to_handle_rot_ss quaternion which solves for effector
  // rotating onto the handle.
  // -----------------------------------------------------------------

  // start_mid_ss with quaternion mid_rot_ms applied.
  const SimdFloat4 mid_end_ss_corrected = TransformVector(
      inv_start_joint,
      TransformVector(*mid_joint, TransformVector(mid_rot_ms, mid_end_ms)));
  const SimdFloat4 start_end_ss_corrected = start_mid_ss + mid_end_ss_corrected;

  // Quaternion for rotating the effector onto the handle
  const SimdQuaternion end_to_handle_rot_ss =
      SimdQuaternion::FromVectors(start_end_ss_corrected, start_handle_ss);

  // Calculates rotate_plane_rot quaternion which solves for the rotate plane.
  // -------------------------------------------------

  // start_mid_ss with quaternion end_to_handle_rot_ss applied
  SimdFloat4 start_mid_ss_corrected =
      TransformVector(end_to_handle_rot_ss, start_mid_ss);

  if (isParallel(start_mid_ss_corrected, start_handle_ss)) {
    // TODO doesn't work if start_mid_ss and start_end_ss are //

    // Singular case, use orthogonal component of the start_mid_ss orthogonal
    // to the start_end_ss instead. Needs to recompute mid_ss - start because
    // start_mid_ss was changed.
    /* const */ SimdFloat4 start_mid_ortho_ss = ortho(mid_ss, end_ss);
    //     start_mid_ortho_ss =
    //         Xor(start_mid_ortho_ss, And(CmpLt(SplatX(Dot3(start_mid_ortho_ss,
    //         start_mid_ms)), zero), simd_int4::mask_sign()));
    start_mid_ss_corrected =
        TransformVector(end_to_handle_rot_ss, start_mid_ortho_ss);
  }

  // Quaternion for rotate plane
  SimdQuaternion rotate_plane_ss;
  if (!isParallel(pole_ss, start_handle_ss) &&
      (GetX(start_handle_ss_len2) != 0.f)) {
    const SimdFloat4 rcp_start_handle_len2 = RcpEstXNR(start_handle_ss_len2);

    // Computes component of start_mid_ss_corrected orthogonal to
    // start_handle_ss
    const SimdFloat4 vectorN =
        ortho(start_mid_ss_corrected, start_handle_ss, rcp_start_handle_len2);

    // Computes component of pole orthogonal to start_handle_ss
    const SimdFloat4 vectorP =
        ortho(pole_ss, start_handle_ss, rcp_start_handle_len2);

    // TODO check divide by 0?
    const SimdFloat4 dotNP =
        Dot3(vectorN, vectorP) *
        RSqrtEstXNR(Length3Sqr(vectorN) * Length3Sqr(vectorP));
    if (AreAllTrue1(
            CmpLt(Abs(dotNP + one),
                  simd_float4::Load1(.0001f)))) {  // TODO kParallelTolerance
      // Singular case, rotate halfway (cos pi) around start_handle_ss
      // TODO Normalize3 safe ?
      rotate_plane_ss =
          SimdQuaternion::FromAxisCosAngle(Normalize3(start_handle_ss), -one);
    } else {
      rotate_plane_ss = SimdQuaternion::FromVectors(vectorN, vectorP);
    }
  } else {
    rotate_plane_ss = SimdQuaternion::identity();
  }

  // Output start and mid joint rotation corrections.
  *mid_joint_correction = mid_rot_ms;
  *start_joint_correction = rotate_plane_ss * end_to_handle_rot_ss;

  return true;
}
}  // namespace animation
}  // namespace ozz
#if 0
  bool IKGlobal() {
    const ozz::math::SimdFloat4 pole_vector = ozz::math::simd_float4::y_axis();

    const ozz::math::SimdFloat4 zero = ozz::math::simd_float4::zero();
    const ozz::math::SimdFloat4 one = ozz::math::simd_float4::one();

    // TODO ensure joints are in the same hierarchy
    const ozz::math::SimdFloat4& start = models_[start_joint_].cols[3];
    const ozz::math::SimdFloat4& mid = models_[mid_joint_].cols[3];
    const ozz::math::SimdFloat4& end = models_[end_joint_].cols[3];

    // Computes bones vectors and length
    ozz::math::SimdFloat4 start_mid = mid - start;
    ozz::math::SimdFloat4 mid_end = end - mid;
    ozz::math::SimdFloat4 start_end = end - start;
    const ozz::math::SimdFloat4 start_handle = g_handle_pos - start;
    const ozz::math::SimdFloat4 start_mid_len2 =
        ozz::math::Length3Sqr(start_mid);
    const ozz::math::SimdFloat4 mid_end_len2 = ozz::math::Length3Sqr(mid_end);
    const ozz::math::SimdFloat4 start_end_len2 =
        ozz::math::Length3Sqr(start_end);
    const ozz::math::SimdFloat4 start_handle_len2 =
        ozz::math::Length3Sqr(start_handle);

    // Calculate mil_local_rot quaternion which solves for the mid joint
    // rotation.
    // --------------------------------------------------------------------------

    // Computes expected angle at mid joint, using law of cosine (generalized
    // Pythagorean).
    // c^2 = a^2 + b^2 - 2ab cosC
    // cosC = (a^2 + b^2 - c^2) / 2ab
    const ozz::math::SimdFloat4 start_mid_end_rlen =
        ozz::math::RSqrtEstX(start_mid_len2 * mid_end_len2);  // 1/(ab)
    const ozz::math::SimdFloat4 mid_cos_angle = ozz::math::Clamp(
        -one,
        (start_handle_len2 - start_mid_len2 - mid_end_len2) *
            ozz::math::simd_float4::Load1(.5f) * start_mid_end_rlen,
        one);

    // Computes initial angle at mid joint.
    const ozz::math::SimdFloat4 mid_cos_initial_angle =
        ozz::math::Dot3(start_mid, mid_end) * start_mid_end_rlen;

    // TODO swap -

    // Computes expected and initial angles difference.
    // Using the formulas for the differences of angles (below) and building the
    // quaternion form cosine avoids computing the 2 ACos here, and the Sin and
    // Cos in quaternion construction.
    // cos(A - B) = cosA * cosB + sinA * sinB
    // cos(A - B) = cosA * cosB + Sqrt((1 - cosA^2) * (1 - cosB^2))
    const ozz::math::SimdFloat4 mid_cos_angle_diff =
        mid_cos_angle * mid_cos_initial_angle +
        ozz::math::SqrtX(ozz::math::Max(
            zero, (one - mid_cos_angle * mid_cos_angle) *
                      (one - mid_cos_initial_angle * mid_cos_initial_angle)));

    const ozz::math::SimdFloat4 mid_local_hingle =
        ozz::math::Normalize3(ozz::math::Cross3(start_mid, mid_end));

    const ozz::math::SimdInt4 flip =
        ozz::math::SplatX(ozz::math::CmpLt(start_end_len2, start_handle_len2));
    const ozz::math::SimdFloat4 mid_local_hingle_flipped =
        ozz::math::Xor(mid_local_hingle,
                       ozz::math::And(flip, ozz::math::simd_int4::mask_sign()));

    // Rotates mid joint
    const ozz::math::SimdQuaternion mid_local_rot =
        ozz::math::SimdQuaternion::FromAxisCosAngle(
            mid_local_hingle_flipped,
            ozz::math::Clamp(-one, mid_cos_angle_diff, one));

    // Calculates end_to_handle_rot quaternion which solves for effector
    // rotating onto the handle.
    // -----------------------------------------------------------------

    // mid_end with quaternion qMid applied
    mid_end = TransformVector(mid_local_rot, mid_end);
    // start_end with quaternion qMid applied
    start_end = start_mid + mid_end;
    // Quaternion for rotating the effector onto the handle
    const ozz::math::SimdQuaternion end_to_handle_rot =
        ozz::math::SimdQuaternion::FromVectors(start_end, start_handle);

    // Calculates qNP which solves for the rotate plane.
    // -------------------------------------------------

    // start_mid with quaternion end_to_handle_rot applied
    start_mid = TransformVector(end_to_handle_rot, start_mid);

    if (isParallel(start_mid, start_handle)) {
      // TODO doesn't work if start_mid and start_end are //

      // Singular case, use orthogonal component of the start_mid orthogonal to
      // the start_end instead. Needs to recompute mid - start because start_mid
      // was changed.
      const ozz::math::SimdFloat4 start_mid_initial = mid - start;
      const ozz::math::SimdFloat4 start_end_initial = end - start;
      const ozz::math::SimdFloat4 start_mid_ortho =
          start_mid_initial - project(start_end_initial, start_mid_initial);
      start_mid = TransformVector(end_to_handle_rot, start_mid_ortho);
    }

    // Quaternion for rotate plane
    ozz::math::SimdQuaternion rotate_plane_rot;
    if (!isParallel(pole_vector, start_handle) &&
        (ozz::math::GetX(start_handle_len2) != 0.f)) {
      const ozz::math::SimdFloat4 rcp_start_handle_len2 =
          ozz::math::RcpEstXNR(start_handle_len2);

      // Computes component of start_mid orthogonal to start_handle
      const ozz::math::SimdFloat4 vectorN =
          start_mid - project(start_handle, start_mid, rcp_start_handle_len2);

      // Computes component of pole_vector orthogonal to start_handle
      const ozz::math::SimdFloat4 vectorP =
          pole_vector -
          project(start_handle, pole_vector, rcp_start_handle_len2);

      const ozz::math::SimdFloat4 dotNP =
          ozz::math::Dot3(vectorN, vectorP) *
          ozz::math::RSqrtEstXNR(ozz::math::Length3Sqr(vectorN) *
                                 ozz::math::Length3Sqr(vectorP));
      if (ozz::math::AreAllTrue1(
              ozz::math::CmpLt(ozz::math::Abs(dotNP + one),
                               ozz::math::simd_float4::Load1(.001f)))) {
        //        assert(false && "first try");
        // singular case, rotate halfway around start_handle
        // TODO Normalize3 safe ?
        rotate_plane_rot = ozz::math::SimdQuaternion::FromAxisCosAngle(
            ozz::math::Normalize3(start_handle), -one);  // -> cos pi
      } else {
        rotate_plane_rot =
            ozz::math::SimdQuaternion::FromVectors(vectorN, vectorP);
      }
    } else {
      //   assert(false && "first try");
      rotate_plane_rot = ozz::math::SimdQuaternion::identity();
    }

    const ozz::math::Float4x4& mid_matrix = models_[mid_joint_];
    const ozz::math::Float4x4 inv_mid_matrix = Invert(mid_matrix);

    const ozz::math::Float4x4 rotm =
        ozz::math::Float4x4::FromQuaternion((mid_local_rot).xyzw);
    const ozz::math::Float4x4 lrotm = inv_mid_matrix * rotm * mid_matrix;
    ozz::math::SimdFloat4 t, r, s;
    ToAffine(lrotm, &t, &r, &s);
    ozz::math::SimdQuaternion sqm = {r /*ToQuaternion(lrotm)*/};

    // Update mid joint pose
    SetQuaternion(mid_joint_, sqm, make_range(locals_));
    // SetScale(mid_joint_, s, make_range(locals_));

    const ozz::math::Float4x4& start_matrix = models_[start_joint_];
    const ozz::math::Float4x4 inv_start_matrix = Invert(start_matrix);

    const ozz::math::Float4x4 rot1 = ozz::math::Float4x4::FromQuaternion(
        (rotate_plane_rot * end_to_handle_rot).xyzw);
    const ozz::math::Float4x4 lrot1 = inv_start_matrix * rot1 * start_matrix;
    ToAffine(lrot1, &t, &r, &s);
    ozz::math::SimdQuaternion sq1 = {r /*ToQuaternion(lrot1)*/};

    /*
        const ozz::math::Float4x4 rot2 = ozz::math::Float4x4::FromQuaternion(
            (end_to_handle_rot).xyzw);
        const ozz::math::Float4x4 lrot2 = inv_start_matrix * rot2 *
       start_matrix; ToAffine(lrot2, &t, &r, &s); ozz::math::SimdQuaternion sq2
       = {r/ *ToQuaternion(lrot2)* /};*/

    SetQuaternion(start_joint_, sq1 /* * sq2*/, make_range(locals_));
    // SetScale(start_joint_, s, make_range(locals_));

    // SetQuaternion(start_joint_, rotate_plane_rot * end_to_handle_rot,
    // locals_);

    ozz::animation::LocalToModelJob ltm_job;
    ltm_job.skeleton = skeleton_;
    ltm_job.input = make_range(locals_);
    ltm_job.output = make_range(models_);
    if (!ltm_job.Run()) {
      return false;
    }

    return true;
  }
#endif
