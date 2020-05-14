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

#include "ozz/animation/runtime/animation.h"
#include "ozz/animation/runtime/ik_aim_job.h"
#include "ozz/animation/runtime/local_to_model_job.h"
#include "ozz/animation/runtime/sampling_job.h"
#include "ozz/animation/runtime/skeleton.h"

#include "ozz/base/log.h"

#include "ozz/base/maths/box.h"
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

// Defines IK chain joint names.
// Joints must be from the same hierarchy (all ancestors of the first joint
// listed) and ordered from child to parent.
const char* kJointNames[] = {"Head", "Spine3", "Spine2", "Spine1"};
const size_t kMaxChainLength = OZZ_ARRAY_SIZE(kJointNames);

// Forward vector in head local-space.
const ozz::math::SimdFloat4 kHeadForward = ozz::math::simd_float4::y_axis();

// Defines Up vectors for each joint. This is skeleton/rig dependant.
const ozz::math::SimdFloat4 kJointUpVectors[] = {
    ozz::math::simd_float4::x_axis(), ozz::math::simd_float4::x_axis(),
    ozz::math::simd_float4::x_axis(), ozz::math::simd_float4::x_axis()};
static_assert(OZZ_ARRAY_SIZE(kJointUpVectors) == kMaxChainLength,
              "Array size mismatch.");

class LookAtSampleApplication : public ozz::sample::Application {
 public:
  LookAtSampleApplication()
      : target_offset_(.2f, 1.5f, -.3f),
        target_extent_(1.f),
        eyes_offset_(.07f, .1f, 0.f),
        enable_ik_(true),
        chain_length_(kMaxChainLength),
        joint_weight_(.5f),
        chain_weight_(1.f),
        show_skin_(true),
        show_joints_(false),
        show_target_(true),
        show_eyes_offset_(false),
        show_forward_(false) {}

 protected:
  // Updates current animation time and skeleton pose.
  virtual bool OnUpdate(float _dt, float _time) {
    // Animates target position.
    MoveTarget(_time);

    // Updates current animation time.
    controller_.Update(animation_, _dt);

    // Samples optimized animation at t = animation_time_.
    ozz::animation::SamplingJob sampling_job;
    sampling_job.animation = &animation_;
    sampling_job.cache = &cache_;
    sampling_job.ratio = controller_.time_ratio();
    sampling_job.output = make_span(locals_);
    if (!sampling_job.Run()) {
      return false;
    }

    // Converts from local-space to model-space matrices.
    ozz::animation::LocalToModelJob ltm_job;
    ltm_job.skeleton = &skeleton_;
    ltm_job.input = make_span(locals_);
    ltm_job.output = make_span(models_);
    if (!ltm_job.Run()) {
      return false;
    }

    // Early out if IK is disabled.
    if (!enable_ik_) {
      return true;
    }

    // IK aim job setup.
    ozz::animation::IKAimJob ik_job;

    // Pole vector and target position are constant for the whole algorithm, in
    // model-space.
    ik_job.pole_vector = ozz::math::simd_float4::y_axis();
    ik_job.target = ozz::math::simd_float4::Load3PtrU(&target_.x);

    // The same quaternion will be used each time the job is run.
    ozz::math::SimdQuaternion correction;
    ik_job.joint_correction = &correction;

    // The algorithm iteratively updates from the first joint (closer to the
    // head) to the last (the further ancestor, closer to the pelvis). Joints
    // order is already validated. For the first joint, aim IK is applied with
    // the global forward and offset, so the forward vector aligns in direction
    // of the target. If a weight lower that 1 is provided to the first joint,
    // then it will not fully align to the target. In this case further joint
    // will need to be updated. For the remaining joints, forward vector and
    // offset position are computed in each joint local-space, before IK is
    // applied:
    // 1. Rotates forward and offset position based on the result of the
    // previous joint IK.
    // 2. Brings forward and offset back in joint local-space.
    // Aim is iteratively applied up to the last selected joint of the
    // hierarchy. A weight of 1 is given to the last joint so we can guarantee
    // target is reached. Note that model-space transform of each joint doesn't
    // need to be updated between each pass, as joints are ordered from child to
    // parent.
    int previous_joint = ozz::animation::Skeleton::kNoParent;
    for (int i = 0, joint = joints_chain_[0]; i < chain_length_;
         ++i, previous_joint = joint, joint = joints_chain_[i]) {
      // Setups the model-space matrix of the joint being processed by IK.
      ik_job.joint = &models_[joint];

      // Setups joint local-space up vector.
      ik_job.up = kJointUpVectors[i];

      // Setups weights of IK job.
      // the last joint being processed needs a full weight (1.f) to ensure
      // target is reached.
      const bool last = i == chain_length_ - 1;
      ik_job.weight = chain_weight_ * (last ? 1.f : joint_weight_);

      // Setup offset and forward vector for the current joint being processed.
      if (i == 0) {
        // First joint, uses global forward and offset.
        ik_job.offset = ozz::math::simd_float4::Load3PtrU(&eyes_offset_.x);
        ik_job.forward = kHeadForward;
      } else {
        // Applies previous correction to "forward" and "offset", before
        // bringing them to model-space (_ms).
        const ozz::math::SimdFloat4 corrected_forward_ms =
            TransformVector(models_[previous_joint],
                            TransformVector(correction, ik_job.forward));
        const ozz::math::SimdFloat4 corrected_offset_ms =
            TransformPoint(models_[previous_joint],
                           TransformVector(correction, ik_job.offset));

        // Brings "forward" and "offset" to joint local-space
        const ozz::math::Float4x4 inv_joint = Invert(models_[joint]);
        ik_job.forward = TransformVector(inv_joint, corrected_forward_ms);
        ik_job.offset = TransformPoint(inv_joint, corrected_offset_ms);
      }

      // Runs IK aim job.
      if (!ik_job.Run()) {
        return false;
      }

      // Apply IK quaternion to its respective local-space transforms.
      ozz::sample::MultiplySoATransformQuaternion(joint, correction,
                                                  make_span(locals_));
    }

    // Skeleton model-space matrices need to be updated again. This re-uses the
    // already setup job, but limits the update to childs of the last joint (the
    // parent-iest of the chain).
    ltm_job.from = previous_joint;
    if (!ltm_job.Run()) {
      return false;
    }

    return true;
  }

  // Sample arbitrary target animation implementation.
  bool MoveTarget(float _time) {
    const ozz::math::Float3 animated_target(std::sin(_time * .5f),
                                            std::cos(_time * .25f),
                                            std::cos(_time) * .5f + .5f);
    target_ = target_offset_ + animated_target * target_extent_;
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
            mesh, make_span(skinning_matrices_), identity);
      }
    } else {
      // Renders skeleton only.
      success &=
          _renderer->DrawPosture(skeleton_, make_span(models_), identity);
    }

    // Showing joints
    if (show_joints_) {
      const float kSphereRadius = .02f;
      for (int i = 0; i < chain_length_; ++i) {
        const ozz::math::Float4x4& transform = models_[joints_chain_[i]];
        success &= _renderer->DrawAxes(transform * kAxesScale);
        success &= _renderer->DrawSphereIm(kSphereRadius, transform,
                                           ozz::sample::kWhite);
      }
    }

    // Showing target, as a box or axes depending on show_forward_ option.
    if (show_target_) {
      const ozz::math::Float4x4 target = ozz::math::Float4x4::Translation(
          ozz::math::simd_float4::Load3PtrU(&target_.x));
      if (show_forward_) {
        success &= _renderer->DrawAxes(target * kAxesScale);
      } else {
        success &= _renderer->DrawSphereIm(.02f, target, ozz::sample::kGreen);
      }
    }

    if (show_eyes_offset_ || show_forward_) {
      const int head = joints_chain_[0];
      const ozz::math::Float4x4 offset =
          models_[head] *
          ozz::math::Float4x4::Translation(
              ozz::math::simd_float4::Load3PtrU(&eyes_offset_.x));
      if (show_eyes_offset_) {
        success &= _renderer->DrawAxes(offset * kAxesScale);
      }
      if (show_forward_) {
        ozz::math::Float3 begin;
        ozz::math::Store3PtrU(offset.cols[3], &begin.x);
        ozz::math::Float3 forward;
        ozz::math::Store3PtrU(kHeadForward, &forward.x);
        ozz::sample::Color color = {0xff, 0xff, 0xff, 0xff};
        success &= _renderer->DrawSegment(ozz::math::Float3::zero(),
                                          forward * 10.f, color, offset);
      }
    }
    return success;
  }

  virtual bool OnInitialize() {
    // Reading skeleton.
    if (!ozz::sample::LoadSkeleton(OPTIONS_skeleton, &skeleton_)) {
      return false;
    }

    // Look for each joint in the chain.
    int found = 0;
    for (int i = 0; i < skeleton_.num_joints() && found != kMaxChainLength;
         ++i) {
      const char* joint_name = skeleton_.joint_names()[i];
      if (std::strcmp(joint_name, kJointNames[found]) == 0) {
        joints_chain_[found] = i;

        // Restart search
        ++found;
        i = 0;
      }
    }

    // Exit if all joints weren't found.
    if (found != kMaxChainLength) {
      ozz::log::Err()
          << "At least a joint wasn't found in the skeleton hierarchy."
          << std::endl;
      return false;
    }

    // Validates joints are order from child to parent of the same hierarchy.
    if (!ValidateJointsOrder(skeleton_, joints_chain_)) {
      ozz::log::Err() << "Joints aren't properly ordered, they must be from "
                         "the same hierarchy (all ancestors of the first joint "
                         "listed) and ordered from child to parent."
                      << std::endl;
      return false;
    }

    // Allocates runtime buffers.
    const int num_soa_joints = skeleton_.num_soa_joints();
    locals_.resize(num_soa_joints);
    const int num_joints = skeleton_.num_joints();
    models_.resize(num_joints);

    // Allocates a cache that matches animation requirements.
    cache_.Resize(num_joints);

    // Reading animation.
    if (!ozz::sample::LoadAnimation(OPTIONS_animation, &animation_)) {
      return false;
    }

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

  // Traverses the hierarchy from the first joint to the root, to check if
  // joints are all ancestors (same branch), and ordered.
  bool ValidateJointsOrder(const ozz::animation::Skeleton& _skeleton,
                           ozz::span<const int> _joints) {
    const size_t count = _joints.size();
    if (count == 0) {
      return true;
    }

    size_t i = 1;
    for (int joint = _joints[0], parent = _skeleton.joint_parents()[joint];
         i != count && joint != ozz::animation::Skeleton::kNoParent;
         joint = parent, parent = _skeleton.joint_parents()[joint]) {
      if (parent == _joints[i]) {
        ++i;
      }
    }

    return count == i;
  }

  virtual void OnDestroy() {}

  virtual bool OnGui(ozz::sample::ImGui* _im_gui) {
    char txt[64];

    _im_gui->DoCheckBox("Enable ik", &enable_ik_);
    sprintf(txt, "IK chain length: %d", chain_length_);
    _im_gui->DoSlider(txt, 0, kMaxChainLength, &chain_length_);
    sprintf(txt, "Joint weight %.2g", joint_weight_);
    _im_gui->DoSlider(txt, 0.f, 1.f, &joint_weight_);
    sprintf(txt, "Chain weight %.2g", chain_weight_);
    _im_gui->DoSlider(txt, 0.f, 1.f, &chain_weight_);

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
      ozz::sample::ImGui::OpenClose oc(_im_gui, "Target offset", &opened);
      if (opened) {
        const float kTargetRange = 3.f;

        _im_gui->DoLabel("Animated extent");
        sprintf(txt, "%.2g", target_extent_);
        _im_gui->DoSlider(txt, 0.f, kTargetRange, &target_extent_);

        sprintf(txt, "x %.2g", target_offset_.x);
        _im_gui->DoSlider(txt, -kTargetRange, kTargetRange, &target_offset_.x);
        sprintf(txt, "y %.2g", target_offset_.y);
        _im_gui->DoSlider(txt, -kTargetRange, kTargetRange, &target_offset_.y);
        sprintf(txt, "z %.2g", target_offset_.z);
        _im_gui->DoSlider(txt, -kTargetRange, kTargetRange, &target_offset_.z);
      }
    }

    {  // Offset position
      static bool opened = true;
      ozz::sample::ImGui::OpenClose oc(_im_gui, "Eyes offset", &opened);
      if (opened) {
        const float kOffsetRange = .5f;
        sprintf(txt, "x %.2g", eyes_offset_.x);
        _im_gui->DoSlider(txt, -kOffsetRange, kOffsetRange, &eyes_offset_.x);
        sprintf(txt, "y %.2g", eyes_offset_.y);
        _im_gui->DoSlider(txt, -kOffsetRange, kOffsetRange, &eyes_offset_.y);
        sprintf(txt, "z %.2g", eyes_offset_.z);
        _im_gui->DoSlider(txt, -kOffsetRange, kOffsetRange, &eyes_offset_.z);
      }
    }

    // Options
    {
      _im_gui->DoCheckBox("Show skin", &show_skin_);
      _im_gui->DoCheckBox("Show joints", &show_joints_);
      _im_gui->DoCheckBox("Show target", &show_target_);
      _im_gui->DoCheckBox("Show eyes offset", &show_eyes_offset_);
      _im_gui->DoCheckBox("Show forward", &show_forward_);
    }

    return true;
  }

  virtual void GetSceneBounds(ozz::math::Box* _bound) const {
    const ozz::math::Float3 radius(target_extent_ * .8f);
    _bound->min = target_offset_ - radius;
    _bound->max = target_offset_ + radius;
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
  ozz::vector<ozz::math::SoaTransform> locals_;

  // Buffer of model-space matrices.
  ozz::vector<ozz::math::Float4x4> models_;

  // Buffer of skinning matrices, result of the joint multiplication of the
  // inverse bind pose with the model-space matrix.
  ozz::vector<ozz::math::Float4x4> skinning_matrices_;

  // The mesh used by the sample.
  ozz::vector<ozz::sample::Mesh> meshes_;

  // Indices of the joints that are IKed for look-at purpose.
  // Joints must be from the same hierarchy (all ancestors of the first joint
  // listed) and ordered from child to parent.
  int joints_chain_[kMaxChainLength];

  // Sample settings

  // Target position management.
  ozz::math::Float3 target_offset_;
  float target_extent_;
  ozz::math::Float3 target_;

  // Offset of the look at position in (head) joint local-space.
  ozz::math::Float3 eyes_offset_;

  // IK settings

  // Enable IK look at.
  bool enable_ik_;

  // Set length of the chain that is IKed, between 0 and kMaxChainLength.
  int chain_length_;

  // Weight given to every joint of the chain. If any joint has a weight of 1,
  // no other following joint will contribute (as the target will be reached).
  float joint_weight_;

  // Overall weight given to the IK on the full chain. This allows blending in
  // and out of IK.
  float chain_weight_;

  // Options
  bool show_skin_;
  bool show_joints_;
  bool show_target_;
  bool show_eyes_offset_;
  bool show_forward_;
};

int main(int _argc, const char** _argv) {
  const char* title = "Ozz-animation sample: Look at";
  return LookAtSampleApplication().Run(_argc, _argv, "1.0", title);
}
