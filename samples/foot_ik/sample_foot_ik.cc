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
#include "ozz/animation/runtime/ik_two_bone_job.h"
#include "ozz/animation/runtime/local_to_model_job.h"
#include "ozz/animation/runtime/sampling_job.h"
#include "ozz/animation/runtime/skeleton.h"

#include "ozz/base/log.h"

#include "ozz/base/maths/box.h"
#include "ozz/base/maths/math_ex.h"
#include "ozz/base/maths/simd_math.h"
#include "ozz/base/maths/simd_quaternion.h"
#include "ozz/base/maths/soa_transform.h"
#include "ozz/base/maths/vec_float.h"

#include "ozz/options/options.h"

#include "framework/application.h"
#include "framework/imgui.h"
#include "framework/mesh.h"
#include "framework/utils.h"

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

// Mesh archive can be specified as an option.
OZZ_OPTIONS_DECLARE_STRING(floor,
                           "Path to the floor mesh (ozz archive format).",
                           "media/floor.ozz", false)

// Structure used to store each leg setup data.
struct LegSetup {
  int hip;
  int knee;
  int ankle;
};

struct LegRayInfo {
  ozz::math::Float3 start;
  ozz::math::Float3 dir;

  bool hit;
  ozz::math::Float3 hit_point;
  ozz::math::Float3 hit_normal;
};

class FootIKSampleApplication : public ozz::sample::Application {
 public:
  FootIKSampleApplication()
      : pelvis_offset(0.f),
        root_translation_(1.f, 2.f, 0.f),
        root_yaw_(0.f),
        auto_character_height_(true),
        pelvis_correction_(true),
        two_bone_ik_(true),
        aim_ik_(true),
        show_skin_(true),
        show_joints_(false),
        show_raycast_(false),
        show_root_(false),
        show_offseted_root_(false) {}

 protected:
  // Updates current animation time and skeleton pose.
  virtual bool OnUpdate(float _dt, float) {
    const ozz::math::Float3 down(0.f, -1.f, 0.f);

    // Updates current animation time.
    controller_.Update(animation_, _dt);

    if (auto_character_height_) {
      ozz::sample::RayIntersectsMesh(
          root_translation_ + ozz::math::Float3(0.f, 5.f, 0.f), down, floor_,
          &root_translation_, NULL);
    }

    const ozz::math::Float4x4 root = GetRootTransform();
    const ozz::math::Float4x4 inv_root = Invert(root);

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

    // Raycast down from the ankle to the floor.
    bool any_leg_hit = false;
    const ozz::math::Float3 foot_height(0.f, .12f, 0.f);
    const ozz::math::Float3 height_offset(0.f, .5f, 0.f);

    for (size_t l = 0; l < kLegsCount; ++l) {
      const LegSetup& leg = legs_setup_[l];
      LegRayInfo& ray = rays_info_[l];

      // Start ray from ankle position, down y axis.
      ozz::math::Float3 ankle_pos_ws;
      ozz::math::Store3PtrU(TransformPoint(root, models_[leg.ankle].cols[3]),
                            &ankle_pos_ws.x);
      ray.start = ankle_pos_ws + height_offset;
      ray.dir = down;
      ray.hit = ozz::sample::RayIntersectsMesh(ray.start, ray.dir, floor_,
                                               &ray.hit_point, &ray.hit_normal);
      any_leg_hit |= ray.hit;
    }

    // Recomputes pelvis offset.
    // Strategy is to move the pelvis along "down" axis (ray axis), enough for
    // the foot lowest from its original position to touch the floor. The other
    // foot will be ik-ed.
    pelvis_offset = ozz::math::Float3(0.f);
    if (any_leg_hit) {
      if (pelvis_correction_) {
        float max_dot = 0.f;
        for (size_t i = 0; i < kLegsCount; ++i) {
          const LegRayInfo& ray = rays_info_[i];
          const ozz::math::Float3 ankle_to_ground =
              foot_height + ray.hit_point - (ray.start - height_offset);
          const float dot = Dot(ankle_to_ground, down);
          if (i == 0 || dot > max_dot) {
            max_dot = dot;
            pelvis_offset = ankle_to_ground;
          }
        }
      }

      // Perform IK
      for (size_t i = 0; i < kLegsCount; ++i) {
        const LegRayInfo& ray = rays_info_[i];
        if (!ray.hit) {
          continue;
        }

        const LegSetup& leg = legs_setup_[i];
        const ozz::math::Float3 target(foot_height + ray.hit_point -
                                       pelvis_offset);
        if (two_bone_ik_ && !ApplyLegTwoBoneIK(leg, target, inv_root)) {
          return false;
        }

        // TODO comment
        ltm_job.from = leg.hip;
        ltm_job.to = leg.ankle;
        if (!ltm_job.Run()) {
          return false;
        }

        if (aim_ik_ && !ApplyAnkleAimIK(leg, ray.hit_normal, inv_root)) {
          return false;
        }

        // TODO comment
        ltm_job.from = leg.ankle;
        ltm_job.to = ozz::animation::Skeleton::kMaxJoints;
        if (!ltm_job.Run()) {
          return false;
        }
      }
    }

    return true;
  }

  bool SetupLeg(const ozz::animation::Skeleton& _skeleton,
                const char* _joint_names[3], LegSetup* _leg) {
    int found = 0;
    int joints[3];
    for (int i = 0; i < _skeleton.num_joints() && found != 3; i++) {
      const char* joint_name = _skeleton.joint_names()[i];
      if (std::strcmp(joint_name, _joint_names[found]) == 0) {
        joints[found] = i;
        ++found;
      }
    }
    _leg->hip = joints[0];
    _leg->knee = joints[1];
    _leg->ankle = joints[2];
    return found == 3;
  }

  bool ApplyLegTwoBoneIK(const LegSetup& _leg,
                         const ozz::math::Float3& _target_ws,
                         const ozz::math::Float4x4& _inv_root) {
    // TODO model space
    const ozz::math::SimdFloat4 target_ms = TransformPoint(
        _inv_root, ozz::math::simd_float4::Load3PtrU(&_target_ws.x));
    const ozz::math::SimdFloat4 pole_vector_ms = models_[_leg.knee].cols[1];

    ozz::animation::IKTwoBoneJob ik_job;
    ik_job.target = target_ms;
    ik_job.pole_vector = pole_vector_ms;
    ik_job.mid_axis = ozz::math::simd_float4::z_axis();
    // ik_job.weight = weight_;
    // ik_job.soften = soften_;
    // ik_job.twist_angle = twist_angle_;
    ik_job.start_joint = &models_[_leg.hip];
    ik_job.mid_joint = &models_[_leg.knee];
    ik_job.end_joint = &models_[_leg.ankle];
    ozz::math::SimdQuaternion start_correction;
    ik_job.start_joint_correction = &start_correction;
    ozz::math::SimdQuaternion mid_correction;
    ik_job.mid_joint_correction = &mid_correction;
    // ik_job.reached = &reached;
    if (!ik_job.Run()) {
      return false;
    }
    // Apply IK quaternions to their respective local-space transforms.
    ozz::sample::MultiplySoATransformQuaternion(_leg.hip, start_correction, make_range(locals_));
    ozz::sample::MultiplySoATransformQuaternion(_leg.knee, mid_correction, make_range(locals_));

    return true;
  }

  bool ApplyAnkleAimIK(const LegSetup& _leg, const ozz::math::Float3& _aim_ws,
                       const ozz::math::Float4x4& _inv_root) {
    ozz::animation::IKAimJob ik_job;
    ik_job.aim = -ozz::math::simd_float4::x_axis();
    ik_job.target =
        TransformVector(_inv_root,
                        ozz::math::simd_float4::Load3PtrU(&_aim_ws.x));  // TODO
    ik_job.up = ozz::math::simd_float4::y_axis();
    ik_job.pole_vector = models_[_leg.ankle].cols[1];
    ik_job.joint = &models_[_leg.ankle];
    ozz::math::SimdQuaternion correction;
    ik_job.joint_correction = &correction;
    // ik_job.reached = &reached;
    if (!ik_job.Run()) {
      return false;
    }
    // Apply IK quaternions to their respective local-space transforms.
    ozz::sample::MultiplySoATransformQuaternion(_leg.ankle, correction, make_range(locals_));

    return true;
  }

  // Samples animation, transforms to model space and renders.
  virtual bool OnDisplay(ozz::sample::Renderer* _renderer) {
    const ozz::math::Float4x4 identity = ozz::math::Float4x4::identity();

    bool success = true;

    // Renders floor mesh.
    success &= _renderer->DrawMesh(floor_, identity);

    // Renders skeleton
    const ozz::math::Float4x4 root = GetRootTransform();
    const ozz::math::Float4x4 offseted_root = GetOffsetedRootTransform();

    // Renders skin.
    if (show_skin_) {
      // Builds skinning matrices.
      // The mesh might not use (aka be skinned by) all skeleton joints. We
      // use the joint remapping table (available from the mesh object) to
      // reorder model-space matrices and build skinning ones.
      for (size_t i = 0; i < mesh_.joint_remaps.size(); ++i) {
        skinning_matrices_[i] =
            models_[mesh_.joint_remaps[i]] * mesh_.inverse_bind_poses[i];
      }

      success &= _renderer->DrawSkinnedMesh(
          mesh_, make_range(skinning_matrices_), offseted_root);
    } else {
      success &=
          _renderer->DrawPosture(skeleton_, make_range(models_), offseted_root);
    }

    // Showing joints
    if (show_joints_) {
      const float kAxeScale = .1f;
      const ozz::math::Float4x4 kAxesScale = ozz::math::Float4x4::Scaling(
          ozz::math::simd_float4::Load1(kAxeScale));

      for (size_t l = 0; l < kLegsCount; ++l) {
        const LegSetup& leg = legs_setup_[l];
        for (size_t i = 0; i < 3; ++i) {
          const int joints[3] = {leg.hip, leg.knee, leg.ankle};
          const ozz::math::Float4x4& transform =
              offseted_root * models_[joints[i]];
          success &= _renderer->DrawAxes(transform * kAxesScale);
        }
      }
    }

    // Shows raycast results
    if (show_raycast_) {
      for (size_t l = 0; l < kLegsCount; ++l) {
        const LegRayInfo& ray = rays_info_[l];
        if (ray.hit) {
          const ozz::sample::Renderer::Color color = {0, 0xff, 0, 0xff};
          success &=
              _renderer->DrawSegment(ray.start, ray.hit_point, color, identity);
          const ozz::sample::Renderer::Color color_n = {0xff, 0, 0, 0xff};
          success &= _renderer->DrawSegment(
              ray.hit_point, ray.hit_point + ray.hit_normal * .4f, color_n,
              identity);
        } else {
          const ozz::sample::Renderer::Color color = {0xff, 0xff, 0xff, 0xff};
          success &= _renderer->DrawSegment(
              ray.start, ray.start + ray.dir * 10.f, color, identity);
        }
      }
    }

    if (show_root_) {
      success &= _renderer->DrawAxes(root);
    }
    if (show_offseted_root_) {
      success &= _renderer->DrawAxes(offseted_root);
    }

    return success;
  }

  virtual bool OnInitialize() {
    // Reading skeleton.
    if (!ozz::sample::LoadSkeleton(OPTIONS_skeleton, &skeleton_)) {
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

    // Finds left and right joints.
    const char* left_joints[] = {"LeftUpLeg", "LeftLeg", "LeftFoot"};
    if (!SetupLeg(skeleton_, left_joints, &legs_setup_[kLeft])) {
      return false;
    }
    const char* right_joints[] = {"RightUpLeg", "RightLeg", "RightFoot"};
    if (!SetupLeg(skeleton_, right_joints, &legs_setup_[kRight])) {
      return false;
    }

    // Reading character mesh.
    if (!ozz::sample::LoadMesh(OPTIONS_mesh, &mesh_)) {
      return false;
    }

    // The number of joints of the mesh needs to match skeleton.
    if (num_joints < mesh_.highest_joint_index()) {
      ozz::log::Err() << "The provided mesh doesn't match skeleton "
                         "(joint count mismatch)."
                      << std::endl;
      return false;
    }
    skinning_matrices_.resize(num_joints);

    // Reading floor mesh.
    if (!ozz::sample::LoadMesh(OPTIONS_floor, &floor_)) {
      return false;
    }

    return true;
  }

  virtual void OnDestroy() {}

  virtual bool OnGui(ozz::sample::ImGui* _im_gui) {
    char txt[32];
    // Exposes animation runtime playback controls.
    {
      static bool open = true;
      ozz::sample::ImGui::OpenClose oc(_im_gui, "Animation control", &open);
      if (open) {
        controller_.OnGui(animation_, _im_gui);
      }
    }

    {  // Root
      static bool opened = true;
      ozz::sample::ImGui::OpenClose oc(_im_gui, "Root transformation", &opened);
      if (opened) {
        // Translation
        _im_gui->DoLabel("Translation");
        sprintf(txt, "x %.2g", root_translation_.x);
        _im_gui->DoSlider(txt, -10.f, 10.f, &root_translation_.x);
        sprintf(txt, "y %.2g", root_translation_.y);
        _im_gui->DoSlider(txt, -1.f, 3.f, &root_translation_.y);
        sprintf(txt, "z %.2g", root_translation_.z);
        _im_gui->DoSlider(txt, -10.f, 10.f, &root_translation_.z);

        // Rotation (in euler form)
        _im_gui->DoLabel("Rotation");
        sprintf(txt, "yaw %.3g", root_yaw_ * ozz::math::kRadianToDegree);
        _im_gui->DoSlider(txt, -ozz::math::kPi, ozz::math::kPi, &root_yaw_);
      }
    }

    // Options
    {
      _im_gui->DoCheckBox("Auto character height", &auto_character_height_);
      _im_gui->DoCheckBox("Pelvis correction", &pelvis_correction_);
      _im_gui->DoCheckBox("Two bone IK (legs)", &two_bone_ik_);
      _im_gui->DoCheckBox("Aim IK (ankles)", &aim_ik_);

      _im_gui->DoCheckBox("Show skin", &show_skin_);
      _im_gui->DoCheckBox("Show joints", &show_joints_);
      _im_gui->DoCheckBox("Show raycasts", &show_raycast_);
      _im_gui->DoCheckBox("Show root", &show_root_);
      _im_gui->DoCheckBox("Show offseted root", &show_offseted_root_);
    }
    return true;
  }

  virtual void GetSceneBounds(ozz::math::Box* _bound) const {
    ozz::math::Box posture_bound;
    ozz::sample::ComputePostureBounds(make_range(models_), &posture_bound);
    *_bound = TransformBox(GetRootTransform(), posture_bound);
  }

  ozz::math::Float4x4 GetRootTransform() const {
    return ozz::math::Float4x4::Translation(
               ozz::math::simd_float4::Load3PtrU(&root_translation_.x)) *
           ozz::math::Float4x4::FromEuler(
               ozz::math::simd_float4::LoadX(root_yaw_));
  }

  ozz::math::Float4x4 GetOffsetedRootTransform() const {
    if (!pelvis_correction_) {
      return GetRootTransform();
    }

    const ozz::math::Float3 offseted_translation =
        pelvis_offset + root_translation_;

    return ozz::math::Float4x4::Translation(
               ozz::math::simd_float4::Load3PtrU(&offseted_translation.x)) *
           ozz::math::Float4x4::FromEuler(
               ozz::math::simd_float4::LoadX(root_yaw_));
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
  ozz::sample::Mesh mesh_;

  enum { kLeft, kRight };
  enum { kLegsCount = 2 };

  LegSetup legs_setup_[kLegsCount];
  LegRayInfo rays_info_[kLegsCount];

  LegRayInfo capsule;

  ozz::math::Float3 pelvis_offset;

  // The floor mesh used by the sample.
  ozz::sample::Mesh floor_;

  // Root transformation.
  ozz::math::Float3 root_translation_;
  float root_yaw_;

  bool auto_character_height_;
  bool pelvis_correction_;
  bool two_bone_ik_;
  bool aim_ik_;

  bool show_skin_;
  bool show_joints_;
  bool show_raycast_;
  bool show_root_;
  bool show_offseted_root_;
};

int main(int _argc, const char** _argv) {
  const char* title = "Ozz-animation sample: Foot IK";
  return FootIKSampleApplication().Run(_argc, _argv, "1.0", title);
}
