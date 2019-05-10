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

#include "ozz/animation/runtime/animation.h"
#include "ozz/animation/runtime/ik_aim_job.h"
#include "ozz/animation/runtime/local_to_model_job.h"
#include "ozz/animation/runtime/sampling_job.h"
#include "ozz/animation/runtime/skeleton.h"

#include "ozz/base/log.h"

#include "ozz/base/maths/simd_math.h"
#include "ozz/base/maths/simd_quaternion.h"
#include "ozz/base/maths/soa_transform.h"
#include "ozz/base/maths/vec_float.h"

#include "ozz/options/options.h"

#include "framework/application.h"
#include "framework/imgui.h"
#include "framework/mesh.h"
#include "framework/renderer.h"
#include "framework/utils.h"

// Skeleton archive can be specified as an option.
OZZ_OPTIONS_DECLARE_STRING(skeleton,
                           "Path to the skeleton (ozz archive format).",
                           "media/skeleton.ozz", false)

// Animation archive can be specified as an option.
OZZ_OPTIONS_DECLARE_STRING(animation,
                           "Path to the animation (ozz archive format).",
                           "media/animation.ozz", false)

// Mesh archive can be specified as an option.
OZZ_OPTIONS_DECLARE_STRING(mesh,
                           "Path to the skinned mesh (ozz archive format).",
                           "media/mesh.ozz", false)

class LookAtSampleApplication : public ozz::sample::Application {
 public:
  LookAtSampleApplication()
      : target_(0.f, 1.f, 1.f),
        offset_(.07f, .1f, 0.f),
        do_ik_(true),
        show_skin_(true),
        show_target_(true),
        show_offset_(false),
        show_forward_(false) {}

 protected:
  // Updates current animation time and skeleton pose.
  virtual bool OnUpdate(float _dt, float) {
    // Updates current animation time.
    controller_.Update(animation_, _dt);

    // Samples optimized animation at t = animation_time_.
    ozz::animation::SamplingJob sampling_job;
    sampling_job.animation = &animation_;
    sampling_job.cache = &cache_;
    sampling_job.ratio = controller_.time_ratio();
    sampling_job.output = make_range(locals_);
    if (!sampling_job.Run()) {
      return false;
    }

    // Converts from local space to model space matrices.
    ozz::animation::LocalToModelJob ltm_job;
    ltm_job.skeleton = &skeleton_;
    ltm_job.input = make_range(locals_);
    ltm_job.output = make_range(models_);
    if (!ltm_job.Run()) {
      return false;
    }

    ozz::animation::IKAimJob ik_job;

    ik_job.offset = ozz::math::simd_float4::Load3PtrU(&offset_.x);
    ik_job.forward = ozz::math::simd_float4::y_axis();

    // TODO local space
    ik_job.target = ozz::math::simd_float4::Load3PtrU(&target_.x);

    ik_job.up = ozz::math::simd_float4::x_axis();
    ik_job.pole_vector = ozz::math::simd_float4::y_axis();
    ik_job.joint = &models_[head_];
    ik_job.weight = 1.f;
    ozz::math::SimdQuaternion correction;
    ik_job.joint_correction = &correction;
    if (!ik_job.Run()) {
      return false;
    }

    // TODO temp
    if (do_ik_) {
      // Apply IK quaternions to their respective local-space transforms.
      ozz::sample::MultiplySoATransformQuaternion(head_, correction,
                                                  make_range(locals_));

      // TODO comment
      ltm_job.from = head_;
      if (!ltm_job.Run()) {
        return false;
      }
    }
    return true;
  }

  // Samples animation, transforms to model space and renders.
  virtual bool OnDisplay(ozz::sample::Renderer* _renderer) {
    bool success = true;
    const ozz::math::Float4x4 identity = ozz::math::Float4x4::identity();
    const float kAxeScale = .1f;
    const ozz::math::Float4x4 kAxesScale =
        ozz::math::Float4x4::Scaling(ozz::math::simd_float4::Load1(kAxeScale));

    // Renders character.
    if (show_skin_) {
      // Builds skinning matrices.
      // The mesh might not use (aka be skinned by) all skeleton joints. We
      // use the joint remapping table (available from the mesh object) to
      // reorder model-space matrices and build skinning ones.
      for (size_t m = 0; m < meshes_.size(); ++m) {
        const ozz::sample::Mesh& mesh = meshes_[m];
        for (size_t i = 0; i < mesh.joint_remaps.size(); ++i) {
          skinning_matrices_[i] =
              models_[mesh.joint_remaps[i]] * mesh.inverse_bind_poses[i];
        }

        success &= _renderer->DrawSkinnedMesh(
            mesh, make_range(skinning_matrices_), identity);
      }
    } else {
      // Renders skeleton only.
      success &=
          _renderer->DrawPosture(skeleton_, make_range(models_), identity);
    }

    if (show_target_) {
      const ozz::math::Float4x4 target = ozz::math::Float4x4::Translation(
          ozz::math::simd_float4::Load3PtrU(&target_.x));
      success &= _renderer->DrawAxes(target * kAxesScale);
    }

    if (show_offset_ || show_forward_) {
      const ozz::math::Float4x4 offset =
          models_[head_] * ozz::math::Float4x4::Translation(
                               ozz::math::simd_float4::Load3PtrU(&offset_.x));
      if (show_offset_) {
        success &= _renderer->DrawAxes(offset * kAxesScale);
      }
      if (show_forward_) {
        ozz::math::Float3 begin;
        ozz::math::Store3PtrU(offset.cols[3], &begin.x);
        // TODO share setting.
        const ozz::math::Float3 forward = ozz::math::Float3::y_axis() * 10.f;

        ozz::sample::Renderer::Color color = {0xff, 0xff, 0xff, 0xff};
        success &= _renderer->DrawSegment(ozz::math::Float3::zero(), forward,
                                          color, offset);
      }
    }
    return success;
  }

  virtual bool OnInitialize() {
    // Reading skeleton.
    if (!ozz::sample::LoadSkeleton(OPTIONS_skeleton, &skeleton_)) {
      return false;
    }

    // Look for head joint.
    head_ = -1;
    for (int i = 0; i < skeleton_.num_joints(); ++i) {
      const char* joint_name = skeleton_.joint_names()[i];
      if (std::strcmp(joint_name, "Head") == 0) {
        head_ = i;
      }
    }
    if (head_ == -1) {
      return false;
    }

    // Reading animation.
    if (!ozz::sample::LoadAnimation(OPTIONS_animation, &animation_)) {
      return false;
    }

    // Allocates runtime buffers.
    const int num_soa_joints = skeleton_.num_soa_joints();
    locals_.resize(num_soa_joints);
    const int num_joints = skeleton_.num_joints();
    models_.resize(num_joints);

    // Allocates a cache that matches animation requirements.
    cache_.Resize(num_joints);

    // Reading character mesh.
    if (!ozz::sample::LoadMeshes(OPTIONS_mesh, &meshes_)) {
      return false;
    }

    // The number of joints of the mesh needs to match skeleton.
    for (size_t m = 0; m < meshes_.size(); ++m) {
      const ozz::sample::Mesh& mesh = meshes_[m];
      if (num_joints < mesh.highest_joint_index()) {
        ozz::log::Err() << "The provided mesh doesn't match skeleton "
                           "(joint count mismatch)."
                        << std::endl;
        return false;
      }
    }
    skinning_matrices_.resize(num_joints);

    return true;
  }

  virtual void OnDestroy() {}

  virtual bool OnGui(ozz::sample::ImGui* _im_gui) {
    char txt[64];

    _im_gui->DoCheckBox("do ik", &do_ik_);

    // Exposes animation runtime playback controls.
    {
      static bool open = true;
      ozz::sample::ImGui::OpenClose oc(_im_gui, "Animation control", &open);
      if (open) {
        controller_.OnGui(animation_, _im_gui);
      }
    }

    {  // Target position
      static bool opened = true;
      ozz::sample::ImGui::OpenClose oc(_im_gui, "Target position", &opened);
      if (opened) {
        const float kTargetRange = 2.f;
        sprintf(txt, "x %.2g", target_.x);
        _im_gui->DoSlider(txt, -kTargetRange, kTargetRange, &target_.x);
        sprintf(txt, "y %.2g", target_.y);
        _im_gui->DoSlider(txt, -kTargetRange, kTargetRange, &target_.y);
        sprintf(txt, "z %.2g", target_.z);
        _im_gui->DoSlider(txt, -kTargetRange, kTargetRange, &target_.z);
      }
    }

    {  // Offset position
      static bool opened = true;
      ozz::sample::ImGui::OpenClose oc(_im_gui, "Offset position", &opened);
      if (opened) {
        const float kOffsetRange = .5f;
        sprintf(txt, "x %.2g", offset_.x);
        _im_gui->DoSlider(txt, -kOffsetRange, kOffsetRange, &offset_.x);
        sprintf(txt, "y %.2g", offset_.y);
        _im_gui->DoSlider(txt, -kOffsetRange, kOffsetRange, &offset_.y);
        sprintf(txt, "z %.2g", offset_.z);
        _im_gui->DoSlider(txt, -kOffsetRange, kOffsetRange, &offset_.z);
      }
    }

    // Options
    {
      _im_gui->DoCheckBox("Show skin", &show_skin_);
      _im_gui->DoCheckBox("Show target", &show_target_);
      _im_gui->DoCheckBox("Show offset", &show_offset_);
      _im_gui->DoCheckBox("Show forward", &show_forward_);
    }

    return true;
  }

  virtual void GetSceneBounds(ozz::math::Box* _bound) const {
    ozz::sample::ComputePostureBounds(make_range(models_), _bound);
  }

 private:
  // Playback animation controller. This is a utility class that helps with
  // controlling animation playback time.
  ozz::sample::PlaybackController controller_;

  // Runtime skeleton.
  ozz::animation::Skeleton skeleton_;

  // Runtime animation.
  ozz::animation::Animation animation_;

  // Sampling cache.
  ozz::animation::SamplingCache cache_;

  // Buffer of local transforms as sampled from animation_.
  ozz::Vector<ozz::math::SoaTransform>::Std locals_;

  // Buffer of model space matrices.
  ozz::Vector<ozz::math::Float4x4>::Std models_;

  // Buffer of skinning matrices, result of the joint multiplication of the
  // inverse bind pose with the model space matrix.
  ozz::Vector<ozz::math::Float4x4>::Std skinning_matrices_;

  // The mesh used by the sample.
  ozz::Vector<ozz::sample::Mesh>::Std meshes_;

  // TODO
  int head_;

  ozz::math::Float3 target_;
  ozz::math::Float3 offset_;

  bool do_ik_;

  bool show_skin_;
  bool show_target_;
  bool show_offset_;
  bool show_forward_;
};

int main(int _argc, const char** _argv) {
  const char* title = "Ozz-animation sample: Look at";
  return LookAtSampleApplication().Run(_argc, _argv, "1.0", title);
}
