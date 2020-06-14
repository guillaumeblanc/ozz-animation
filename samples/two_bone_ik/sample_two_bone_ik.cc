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

#include "ozz/animation/runtime/ik_two_bone_job.h"
#include "ozz/animation/runtime/local_to_model_job.h"
#include "ozz/animation/runtime/skeleton.h"

#include "ozz/base/log.h"

#include "ozz/base/maths/box.h"
#include "ozz/base/maths/simd_math.h"
#include "ozz/base/maths/simd_quaternion.h"
#include "ozz/base/maths/soa_transform.h"

#include "ozz/base/memory/allocator.h"

#include "ozz/options/options.h"

#include "framework/application.h"
#include "framework/imgui.h"
#include "framework/renderer.h"
#include "framework/utils.h"

#include <algorithm>

// Skeleton archive can be specified as an option.
OZZ_OPTIONS_DECLARE_STRING(skeleton,
                           "Path to the skeleton (ozz archive format).",
                           "media/skeleton.ozz", false)

class TwoBoneIKSampleApplication : public ozz::sample::Application {
 public:
  TwoBoneIKSampleApplication()
      : start_joint_(-1),
        mid_joint_(-1),
        end_joint_(-1),
        pole_vector(0.f, 1.f, 0.f),
        weight_(1.f),
        soften_(.97f),
        twist_angle_(0.f),
        reached_(false),
        fix_initial_transform_(true),
        two_bone_ik_(true),
        show_target_(true),
        show_joints_(false),
        show_pole_vector_(false),
        root_translation_(0.f, 0.f, 0.f),
        root_euler_(0.f, 0.f, 0.f),
        root_scale_(1.f),
        target_extent_(.5f),
        target_offset_(0.f, .2f, .1f),
        target_(0.f, 0.f, 0.f) {}

 protected:
  bool ApplyTwoBoneIK() {
    // Target and pole should be in model-space, so they must be converted from
    // world-space using character inverse root matrix.
    // IK jobs must support non invertible matrices (like 0 scale matrices).
    ozz::math::SimdInt4 invertible;
    const ozz::math::Float4x4 invert_root =
        Invert(GetRootTransform(), &invertible);

    const ozz::math::SimdFloat4 target_ms = TransformPoint(
        invert_root, ozz::math::simd_float4::Load3PtrU(&target_.x));
    const ozz::math::SimdFloat4 pole_vector_ms = TransformVector(
        invert_root, ozz::math::simd_float4::Load3PtrU(&pole_vector.x));

    // Setup IK job.
    ozz::animation::IKTwoBoneJob ik_job;
    ik_job.target = target_ms;
    ik_job.pole_vector = pole_vector_ms;
    ik_job.mid_axis = ozz::math::simd_float4::z_axis();  // Middle joint
                                                         // rotation axis is
                                                         // fixed, and depends
                                                         // on skeleton rig.
    ik_job.weight = weight_;
    ik_job.soften = soften_;
    ik_job.twist_angle = twist_angle_;

    // Provides start, middle and end joints model space matrices.
    ik_job.start_joint = &models_[start_joint_];
    ik_job.mid_joint = &models_[mid_joint_];
    ik_job.end_joint = &models_[end_joint_];

    // Setup output pointers.
    ozz::math::SimdQuaternion start_correction;
    ik_job.start_joint_correction = &start_correction;
    ozz::math::SimdQuaternion mid_correction;
    ik_job.mid_joint_correction = &mid_correction;
    ik_job.reached = &reached_;

    if (!ik_job.Run()) {
      return false;
    }

    // Apply IK quaternions to their respective local-space transforms.
    ozz::sample::MultiplySoATransformQuaternion(start_joint_, start_correction,
                                                make_span(locals_));
    ozz::sample::MultiplySoATransformQuaternion(mid_joint_, mid_correction,
                                                make_span(locals_));

    // Updates model-space matrices now IK has been applied to local transforms.
    // All the ancestors of the start of the IK chain must be computed.
    ozz::animation::LocalToModelJob ltm_job;
    ltm_job.skeleton = &skeleton_;
    ltm_job.input = make_span(locals_);
    ltm_job.output = make_span(models_);
    ltm_job.from =
        start_joint_;  // Local transforms haven't changed before start_joint_.
    ltm_job.to = ozz::animation::Skeleton::kMaxJoints;

    if (!ltm_job.Run()) {
      return false;
    }

    return true;
  }

  virtual bool OnUpdate(float, float _time) {
    // Updates sample target position.
    if (!MoveTarget(_time)) {
      return false;
    }

    // Reset locals to skeleton bind pose if option is true.
    // This allows to always start IK from a fix position (required to test
    // weighting), or do IK from the latest computed pose
    if (fix_initial_transform_) {
      for (size_t i = 0; i < locals_.size(); ++i) {
        locals_[i] = skeleton_.joint_bind_poses()[i];
      }
    }

    // Updates model-space matrices from current local-space setup.
    // Model-space matrices needs to be updated up to the end joint. Any joint
    // after that will need to be recomputed after IK indeed.
    ozz::animation::LocalToModelJob ltm_job;
    ltm_job.skeleton = &skeleton_;
    ltm_job.input = make_span(locals_);
    ltm_job.output = make_span(models_);
    if (!ltm_job.Run()) {
      return false;
    }

    // Setup and run IK job.
    if (two_bone_ik_ && !ApplyTwoBoneIK()) {
      return false;
    }

    return true;
  }

  virtual bool OnDisplay(ozz::sample::Renderer* _renderer) {
    bool success = true;

    // Get skeleton root transform.
    const ozz::math::Float4x4 root = GetRootTransform();

    if (show_target_ && two_bone_ik_) {
      // Displays target
      const ozz::sample::Color colors[2][2] = {
          {ozz::sample::kRed, ozz::sample::kBlack},
          {ozz::sample::kGreen, ozz::sample::kBlack}};

      const float kBoxHalfSize = .006f;
      const ozz::math::Box box(ozz::math::Float3(-kBoxHalfSize),
                               ozz::math::Float3(kBoxHalfSize));
      success &= _renderer->DrawBoxIm(
          box,
          ozz::math::Float4x4::Translation(
              ozz::math::simd_float4::Load3PtrU(&target_.x)),
          colors[reached_]);
    }

    // Displays pole vector
    if (show_pole_vector_) {
      ozz::math::Float3 begin;
      ozz::math::Store3PtrU(TransformPoint(root, models_[mid_joint_].cols[3]),
                            &begin.x);
      success &= _renderer->DrawSegment(begin, begin + pole_vector,
                                        ozz::sample::kWhite,
                                        ozz::math::Float4x4::identity());
    }

    // Showing joints
    if (show_joints_) {
      const float kAxeScale = .1f;
      const float kSphereRadius = .009f;
      const ozz::math::Float4x4 kAxesScale = ozz::math::Float4x4::Scaling(
          ozz::math::simd_float4::Load1(kAxeScale));
      for (size_t i = 0; i < 3; ++i) {
        const int joints[3] = {start_joint_, mid_joint_, end_joint_};
        const ozz::math::Float4x4& transform = root * models_[joints[i]];
        success &= _renderer->DrawAxes(transform * kAxesScale);
        success &= _renderer->DrawSphereIm(kSphereRadius, transform,
                                           ozz::sample::kWhite);
      }
    }

    // Draws the animated skeleton posture.
    success &= _renderer->DrawPosture(skeleton_, make_span(models_), root);

    return success;
  }

  virtual bool OnInitialize() {
    // Loads skeleton.
    if (!ozz::sample::LoadSkeleton(OPTIONS_skeleton, &skeleton_)) {
      return false;
    }

    // Allocates runtime buffers.
    const int num_soa_joints = skeleton_.num_soa_joints();
    locals_.resize(num_soa_joints);
    const int num_joints = skeleton_.num_joints();
    models_.resize(num_joints);

    // Find the 3 joints in skeleton hierarchy.
    start_joint_ = mid_joint_ = end_joint_ = -1;
    for (int i = 0; i < skeleton_.num_joints(); i++) {
      const char* joint_name = skeleton_.joint_names()[i];
      if (std::strcmp(joint_name, "shoulder") == 0) {
        start_joint_ = i;
      } else if (std::strcmp(joint_name, "forearm") == 0) {
        mid_joint_ = i;
      } else if (std::strcmp(joint_name, "wrist") == 0) {
        end_joint_ = i;
      }
    }

    // Fails if a joint is missing.
    if (start_joint_ < 0 || mid_joint_ < 0 || end_joint_ < 0) {
      ozz::log::Err() << "Failed to find required joints." << std::endl;
      return false;
    }

    // Initialize locals from skeleton bind pose
    for (size_t i = 0; i < locals_.size(); ++i) {
      locals_[i] = skeleton_.joint_bind_poses()[i];
    }

    return true;
  }

  virtual void OnDestroy() {}

  virtual bool OnGui(ozz::sample::ImGui* _im_gui) {
    char txt[32];

    // IK parameters
    _im_gui->DoCheckBox("Fix initial transform", &fix_initial_transform_);
    _im_gui->DoCheckBox("Enable two bone ik", &two_bone_ik_);
    {
      static bool opened = true;
      ozz::sample::ImGui::OpenClose oc(_im_gui, "IK parameters", &opened);
      if (opened) {
        sprintf(txt, "Soften: %.2g", soften_);
        _im_gui->DoSlider(txt, 0.f, 1.f, &soften_, 2.f);
        sprintf(txt, "Twist angle: %.0f",
                twist_angle_ * ozz::math::kRadianToDegree);
        _im_gui->DoSlider(txt, -ozz::math::kPi, ozz::math::kPi, &twist_angle_);
        sprintf(txt, "Weight: %.2g", weight_);
        _im_gui->DoSlider(txt, 0.f, 1.f, &weight_);
        {
          // Pole vector
          static bool pole_opened = true;
          ozz::sample::ImGui::OpenClose oc_pole(_im_gui, "Pole vector",
                                                &pole_opened);
          if (pole_opened) {
            sprintf(txt, "x %.2g", pole_vector.x);
            _im_gui->DoSlider(txt, -1.f, 1.f, &pole_vector.x);
            sprintf(txt, "y %.2g", pole_vector.y);
            _im_gui->DoSlider(txt, -1.f, 1.f, &pole_vector.y);
            sprintf(txt, "z %.2g", pole_vector.z);
            _im_gui->DoSlider(txt, -1.f, 1.f, &pole_vector.z);
          }
        }
      }
    }
    {  // Target position
      static bool opened = true;
      ozz::sample::ImGui::OpenClose oc(_im_gui, "Target position", &opened);
      if (opened) {
        _im_gui->DoLabel("Target animation extent");
        sprintf(txt, "%.2g", target_extent_);
        _im_gui->DoSlider(txt, 0.f, 1.f, &target_extent_);

        _im_gui->DoLabel("Target Offset");
        const float kOffsetRange = 1.f;
        sprintf(txt, "x %.2g", target_offset_.x);
        _im_gui->DoSlider(txt, -kOffsetRange, kOffsetRange, &target_offset_.x);
        sprintf(txt, "y %.2g", target_offset_.y);
        _im_gui->DoSlider(txt, -kOffsetRange, kOffsetRange, &target_offset_.y);
        sprintf(txt, "z %.2g", target_offset_.z);
        _im_gui->DoSlider(txt, -kOffsetRange, kOffsetRange, &target_offset_.z);
      }
    }
    {  // Root
      static bool opened = false;
      ozz::sample::ImGui::OpenClose oc(_im_gui, "Root transformation", &opened);
      if (opened) {
        // Translation
        _im_gui->DoLabel("Translation");
        sprintf(txt, "x %.2g", root_translation_.x);
        _im_gui->DoSlider(txt, -1.f, 1.f, &root_translation_.x);
        sprintf(txt, "y %.2g", root_translation_.y);
        _im_gui->DoSlider(txt, -1.f, 1.f, &root_translation_.y);
        sprintf(txt, "z %.2g", root_translation_.z);
        _im_gui->DoSlider(txt, -1.f, 1.f, &root_translation_.z);

        // Rotation (in euler form)
        _im_gui->DoLabel("Rotation");
        ozz::math::Float3 euler = root_euler_ * ozz::math::kRadianToDegree;
        sprintf(txt, "yaw %.3g", euler.x);
        _im_gui->DoSlider(txt, -180.f, 180.f, &euler.x);
        sprintf(txt, "pitch %.3g", euler.y);
        _im_gui->DoSlider(txt, -180.f, 180.f, &euler.y);
        sprintf(txt, "roll %.3g", euler.z);
        _im_gui->DoSlider(txt, -180.f, 180.f, &euler.z);
        root_euler_ = euler * ozz::math::kDegreeToRadian;

        // Scale (must be uniform and not 0)
        _im_gui->DoLabel("Scale");
        sprintf(txt, "%.2g", root_scale_);
        _im_gui->DoSlider(txt, -1.f, 1.f, &root_scale_);
      }
    }
    {  // Display options
      static bool opened = true;
      ozz::sample::ImGui::OpenClose oc(_im_gui, "Display options", &opened);
      if (opened) {
        _im_gui->DoCheckBox("Show target", &show_target_);
        _im_gui->DoCheckBox("Show joints", &show_joints_);
        _im_gui->DoCheckBox("Show pole vector", &show_pole_vector_);
      }
    }

    return true;
  }

  virtual void GetSceneBounds(ozz::math::Box* _bound) const {
    const ozz::math::Float3 radius(target_extent_ * .5f);
    _bound->min = target_offset_ - radius;
    _bound->max = target_offset_ + radius;
  }

 private:
  bool MoveTarget(float _time) {
    const float anim_extent = (1.f - std::cos(_time)) * .5f * target_extent_;
    const int floor = static_cast<int>(std::fabs(_time) / ozz::math::k2Pi);

    target_ = target_offset_;
    (&target_.x)[floor % 3] += anim_extent;
    return true;
  }

  ozz::math::Float4x4 GetRootTransform() const {
    return ozz::math::Float4x4::Translation(
               ozz::math::simd_float4::Load3PtrU(&root_translation_.x)) *
           ozz::math::Float4x4::FromEuler(
               ozz::math::simd_float4::Load3PtrU(&root_euler_.x)) *
           ozz::math::Float4x4::Scaling(
               ozz::math::simd_float4::Load1(root_scale_));
  }

  // Runtime skeleton.
  ozz::animation::Skeleton skeleton_;

  // Buffer of local transforms as sampled from animation_.
  ozz::vector<ozz::math::SoaTransform> locals_;

  // Buffer of model space matrices.
  ozz::vector<ozz::math::Float4x4> models_;

  // Two bone IK setup. Indices of the relevant joints in the chain.
  int start_joint_;
  int mid_joint_;
  int end_joint_;

  // Two bone IK parameters.
  ozz::math::Float3 pole_vector;
  float weight_;
  float soften_;
  float twist_angle_;

  // Two bone IK job "reched" output value.
  bool reached_;

  // Sample options
  bool fix_initial_transform_;
  bool two_bone_ik_;

  // Sample display options
  bool show_target_;
  bool show_joints_;
  bool show_pole_vector_;

  // Root transformation.
  ozz::math::Float3 root_translation_;
  ozz::math::Float3 root_euler_;
  float root_scale_;

  // Target positioning and animation.
  float target_extent_;
  ozz::math::Float3 target_offset_;
  ozz::math::Float3 target_;
};

int main(int _argc, const char** _argv) {
  const char* title = "Ozz-animation sample: Two bone IK";
  return TwoBoneIKSampleApplication().Run(_argc, _argv, "1.0", title);
}
