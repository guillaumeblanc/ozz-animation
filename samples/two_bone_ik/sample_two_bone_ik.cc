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

#include "ozz/animation/runtime/ik_two_bone_job.h"
#include "ozz/animation/runtime/local_to_model_job.h"
#include "ozz/animation/runtime/skeleton.h"

#include "ozz/base/log.h"

#include "ozz/base/maths/box.h"
#include "ozz/base/maths/simd_math.h"
#include "ozz/base/maths/soa_transform.h"

// TODO Move
#include "ozz/base/maths/simd_quaternion.h"

#include "ozz/base/memory/allocator.h"

#include "ozz/options/options.h"

#include "framework/application.h"
#include "framework/imgui.h"
#include "framework/renderer.h"
#include "framework/utils.h"

// TEMP
#include "ozz/animation/offline/raw_skeleton.h"
#include "ozz/animation/offline/skeleton_builder.h"

ozz::math::SimdFloat4 g_target_pos;

void ApplyQuaternion(int _joint, const ozz::math::SimdQuaternion& _quat,
                     const ozz::Range<ozz::math::SoaTransform>& _transforms) {
  // Convert soa to aos in order to perform quaternion multiplication, and gets
  // back to soa.

  ozz::math::SoaTransform& soa_transform_ref = _transforms[_joint / 4];
  ozz::math::SimdQuaternion aos_quats[4];
  ozz::math::Transpose4x4(&soa_transform_ref.rotation.x, &aos_quats->xyzw);

  ozz::math::SimdQuaternion& aos_joint_quat_ref = aos_quats[_joint & 3];
  aos_joint_quat_ref = aos_joint_quat_ref * _quat;

  ozz::math::Transpose4x4(&aos_quats->xyzw, &soa_transform_ref.rotation.x);
}

// Skeleton archive can be specified as an option.
OZZ_OPTIONS_DECLARE_STRING(skeleton,
                           "Path to the skeleton (ozz archive format).",
                           "media/skeleton.ozz", false)

class TwoBoneIKSampleApplication : public ozz::sample::Application {
 public:
  TwoBoneIKSampleApplication()
      : skeleton_(NULL),
        start_joint_(-1),
        mid_joint_(-1),
        end_joint_(-1),
        doik(true),
        root_translation_(0.f, 0.f, 0.f),
        root_euler_(0.f, 0.f, 0.f),
        root_scale_(1.f),
        pole_vector(0.f, 1.f, 0.f),
        weight_(1.f),
        soften_(1.f),
        twist_angle_(0.f),
        extent(0.f),
        target_offset(1.f, .1f, 0.f),
        use_loaded_skeleton_(true),
        show_target_(true),
        show_joints_(false),
        show_pole_vector_(false) {}

 protected:
  virtual bool OnUpdate(float _dt) {
    (void)_dt;

    // Reset locals to skeleton bind pose (for sample purpose).
    for (size_t i = 0; i < locals_.size(); ++i) {
      locals_[i] = skeleton_->joint_bind_poses()[i];
    }

    // Build root matrix
    using ozz::math::Float4x4;

    // Converts from local space to model space matrices.
    ozz::animation::LocalToModelJob ltm_job;
    ltm_job.skeleton = skeleton_;
    ltm_job.from = ozz::animation::Skeleton::kNoParent;  // TODO explain
    ltm_job.to = end_joint_;
    ltm_job.input = make_range(locals_);
    ltm_job.output = make_range(models_);
    if (!ltm_job.Run()) {
      return false;
    }

    static float time = 0.f;
    time += _dt;
    g_target_pos = ozz::math::simd_float4::Load(
        target_offset.x + extent * sinf(time),
        target_offset.y + extent * cosf(time * 2.f),
        target_offset.z + extent * cosf(time / 2.f), 0);

    if (!IK()) {
      return false;
    }

    ltm_job.from = start_joint_;                        // TODO explain
    ltm_job.to = ozz::animation::Skeleton::kMaxJoints;  // end_joint_;
    if (!ltm_job.Run()) {
      return false;
    }

    return true;
  }

  bool IK() {
    // Get root matrix, which is used at render time to convert object from
    // model-space to world space.
    const ozz::math::Float4x4 root = ComputeRootTransform();

    // Target should be in model-space, so it must be converted from
    // wolrd-space.
    const ozz::math::Float4x4 invert_root = Invert(root);
    const ozz::math::SimdFloat4 target_ls =
        TransformPoint(invert_root, g_target_pos);
    const ozz::math::SimdFloat4 pole_vector_ls = TransformPoint(
        invert_root, ozz::math::simd_float4::Load3PtrU(&pole_vector.x));

    ozz::animation::IKTwoBoneJob ik_job;
    ik_job.target = target_ls;
    ik_job.pole_vector = pole_vector_ls;
    ik_job.mid_axis = ozz::math::simd_float4::z_axis();
    ik_job.weight = weight_;
    ik_job.soften = soften_;
    ik_job.twist_angle = twist_angle_;
    ik_job.start_joint = &models_[start_joint_];
    ik_job.mid_joint = &models_[mid_joint_];
    ik_job.end_joint = &models_[end_joint_];
    ozz::math::SimdQuaternion start_correction;
    ik_job.start_joint_correction = &start_correction;
    ozz::math::SimdQuaternion mid_correction;
    ik_job.mid_joint_correction = &mid_correction;
    if (!ik_job.Run()) {
      return false;
    }

    if (doik) {
      ApplyQuaternion(mid_joint_, mid_correction, make_range(locals_));
      ApplyQuaternion(start_joint_, start_correction, make_range(locals_));
    } else {
      for (size_t i = 0; i < locals_.size(); ++i) {
        locals_[i] = skeleton_->joint_bind_poses()[i];
      }
    }

    return true;
  }

  // Samples animation, transforms to model space and renders.
  virtual bool OnDisplay(ozz::sample::Renderer* _renderer) {
    bool success = true;

    const float kBoxHalfSize = .005f;
    const ozz::math::Float4x4 kAxesScale =
        ozz::math::Float4x4::Scaling(ozz::math::simd_float4::Load1(.1f));

    const ozz::math::Float4x4 root = ComputeRootTransform();

    // Displays target
    if (show_target_) {
      const ozz::sample::Renderer::Color colors[2] = {{0xff, 0xff, 0xff, 0xff},
                                                      {0xff, 0, 0, 0xff}};
      success &= _renderer->DrawBoxIm(
          ozz::math::Box(ozz::math::Float3(-kBoxHalfSize),
                         ozz::math::Float3(kBoxHalfSize)),
          ozz::math::Float4x4::Translation(g_target_pos), colors);
    }

    // Displays pole vector
    if (show_pole_vector_) {
      const ozz::sample::Renderer::Color color = {0xff, 0xff, 0xff, 0xff};
      float pos[3];
      ozz::math::Store3PtrU(models_[start_joint_].cols[3], pos);
      success &= _renderer->DrawVectors(
          ozz::Range<const float>(pos, 3), 6,
          ozz::Range<const float>(&pole_vector.x, 3), 6, 1, 1.f, color,
          ozz::math::Float4x4::identity());
    }

    // Showing joints
    if (show_joints_) {
      for (size_t i = 0; i < 3; ++i) {
        const ozz::sample::Renderer::Color colors[6] = {
            {0xff, 0, 0, 0xff}, {0xff, 0xff, 0xff, 0xff},
            {0, 0xff, 0, 0xff}, {0xff, 0xff, 0xff, 0xff},
            {0, 0, 0xff, 0xff}, {0xff, 0xff, 0xff, 0xff}};
        const int joints[3] = {start_joint_, mid_joint_, end_joint_};
        const ozz::math::Float4x4& transform = root * models_[joints[i]];
        success &= _renderer->DrawBoxIm(
            ozz::math::Box(ozz::math::Float3(-kBoxHalfSize),
                           ozz::math::Float3(kBoxHalfSize)),
            transform, colors + i * 2);
        success &= _renderer->DrawAxes(transform * kAxesScale);
      }
    }

    // Draws the animated skeleton posture.
    success &= _renderer->DrawPosture(*skeleton_, make_range(models_), root);

    return success;
  }

  bool BuildSkeleton() {
    // Delete previous skeleton
    ozz::memory::default_allocator()->Delete(skeleton_);
    skeleton_ = NULL;

    // Generates or loads skeleton
    if (use_loaded_skeleton_) {
      ozz::memory::Allocator* allocator = ozz::memory::default_allocator();
      skeleton_ = allocator->New<ozz::animation::Skeleton>();
      if (!skeleton_ ||
          !ozz::sample::LoadSkeleton(OPTIONS_skeleton, skeleton_)) {
        return false;
      }
    } else {
      // Generates runtime skeleton from editable raw skeleton
      ozz::animation::offline::SkeletonBuilder builder;
      skeleton_ = builder(editable_skeleton_);
      if (!skeleton_) {
        return false;
      }
    }

    // Allocates runtime buffers.
    const int num_soa_joints = skeleton_->num_soa_joints();
    locals_.resize(num_soa_joints);
    const int num_joints = skeleton_->num_joints();
    models_.resize(num_joints);

    // Find the 3 joints in skeleton hierarchy.
    start_joint_ = mid_joint_ = end_joint_ = -1;
    for (int i = 0; i < skeleton_->num_joints(); i++) {
      const char* joint_name = skeleton_->joint_names()[i];
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
      return false;
    }

    // Initialize locals from skeleton bind pose
    for (size_t i = 0; i < locals_.size(); ++i) {
      locals_[i] = skeleton_->joint_bind_poses()[i];
    }
    return true;
  }

  virtual bool OnInitialize() {
    // Prepares editable skeleton once for all
    SetupEditableSkeleton();

    // Builds current skeleton.
    return BuildSkeleton();
  }

  virtual void OnDestroy() {
    ozz::memory::Allocator* allocator = ozz::memory::default_allocator();
    allocator->Delete(skeleton_);
  }

  virtual bool OnGui(ozz::sample::ImGui* _im_gui) {
    char txt[32];

    _im_gui->DoSlider("extent", 0.f, 5.f, &extent);

    // IK parameters

    {  // Handle
      static bool opened = true;
      ozz::sample::ImGui::OpenClose oc_pole(_im_gui, "Handle position",
                                            &opened);
      const float kOffsetRange = 2.f;
      sprintf(txt, "x %.2g", target_offset.x);
      _im_gui->DoSlider(txt, -kOffsetRange, kOffsetRange, &target_offset.x);
      sprintf(txt, "y %.2g", target_offset.y);
      _im_gui->DoSlider(txt, -kOffsetRange, kOffsetRange, &target_offset.y);
      sprintf(txt, "z %.2g", target_offset.z);
      _im_gui->DoSlider(txt, -kOffsetRange, kOffsetRange, &target_offset.z);
    }

    {
      // Pole vector
      static bool opened = false;
      ozz::sample::ImGui::OpenClose oc_pole(_im_gui, "Pole vector", &opened);
      sprintf(txt, "x %.2g", pole_vector.x);
      _im_gui->DoSlider(txt, -1.f, 1.f, &pole_vector.x);
      sprintf(txt, "y %.2g", pole_vector.y);
      _im_gui->DoSlider(txt, -1.f, 1.f, &pole_vector.y);
      sprintf(txt, "z %.2g", pole_vector.z);
      _im_gui->DoSlider(txt, -1.f, 1.f, &pole_vector.z);
    }
    sprintf(txt, "Weight: %.2g", weight_);
    _im_gui->DoSlider(txt, 0.f, 1.f, &weight_);
    sprintf(txt, "Soften: %.2g", soften_);
    _im_gui->DoSlider(txt, 0.f, 1.f, &soften_, 2.f);
    sprintf(txt, "Twist angle: %.0f",
            twist_angle_ * ozz::math::kRadianToDegree);
    _im_gui->DoSlider(txt, -ozz::math::kPi, ozz::math::kPi, &twist_angle_);

    {  // Root

      static bool opened = false;
      ozz::sample::ImGui::OpenClose oc_pole(_im_gui, "Root transformation",
                                            &opened);
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
        sprintf(txt, "x %.3g", euler.x);
        _im_gui->DoSlider(txt, -180.f, 180.f, &euler.x);
        sprintf(txt, "y %.3g", euler.y);
        _im_gui->DoSlider(txt, -180.f, 180.f, &euler.y);
        sprintf(txt, "z %.3g", euler.z);
        _im_gui->DoSlider(txt, -180.f, 180.f, &euler.z);
        root_euler_ = euler * ozz::math::kDegreeToRadian;

        // Scale (must be uniform and not 0)
        _im_gui->DoLabel("Scale");
        sprintf(txt, "%.2g", root_scale_);
        _im_gui->DoSlider(txt, -1.f, 1.f, &root_scale_);
        // root_scale_ = root_scale_ != 0.f ? scale.x : .01f;
      }
    }
    {  // Display options
      static bool opened = true;
      ozz::sample::ImGui::OpenClose oc_pole(_im_gui, "Display options",
                                            &opened);
      _im_gui->DoCheckBox("Show target", &show_target_);
      _im_gui->DoCheckBox("Show joints", &show_joints_);
      _im_gui->DoCheckBox("Show pole vector", &show_pole_vector_);
    }

    {  // Skeleton load/generate options
      if (_im_gui->DoCheckBox("Use loaded skeleton", &use_loaded_skeleton_)) {
        if (!BuildSkeleton()) {
          return false;
        }
      }
      if (!use_loaded_skeleton_) {
        static bool opened = false;
        ozz::sample::ImGui::OpenClose oc(_im_gui, "Edit skeleton joints",
                                         &opened);
        if (opened) {
          if (raw_skeleton_editor.OnGui(&editable_skeleton_, _im_gui)) {
            if (!BuildSkeleton()) {
              return false;
            }
          }
        }
      }
    }
    return true;
  }

  // Builds a simple skeleton that can be used to experiment different setups.
  void SetupEditableSkeleton() {
    const float kDefaultBoneLength = .3f;  // length

    editable_skeleton_.roots.resize(1);
    ozz::animation::offline::RawSkeleton::Joint& shoulder =
        editable_skeleton_.roots[0];
    shoulder.name = "shoulder";
    shoulder.transform.translation = ozz::math::Float3::zero();
    shoulder.transform.rotation = ozz::math::Quaternion::identity();
    shoulder.transform.scale = ozz::math::Float3::one();

    shoulder.children.resize(1);
    ozz::animation::offline::RawSkeleton::Joint& forearm = shoulder.children[0];
    forearm.name = "forearm";
    forearm.transform.translation =
        ozz::math::Float3::y_axis() * kDefaultBoneLength;
    forearm.transform.rotation = ozz::math::Quaternion::FromAxisAngle(
        ozz::math::Float3::z_axis(), ozz::math::kPi_2);
    forearm.transform.scale = ozz::math::Float3::one();

    forearm.children.resize(1);
    ozz::animation::offline::RawSkeleton::Joint& wrist = forearm.children[0];
    wrist.name = "wrist";
    wrist.transform.translation =
        ozz::math::Float3::y_axis() * kDefaultBoneLength;
    wrist.transform.rotation = ozz::math::Quaternion::identity();
    wrist.transform.scale = ozz::math::Float3::one();
  }

  virtual void GetSceneBounds(ozz::math::Box* _bound) const {
    // Computes skeleton bound
    ozz::sample::ComputePostureBounds(make_range(models_), _bound);

    // Adds target in the bounds
    *_bound = Merge(*_bound, ozz::math::Box(target_offset));
  }

  ozz::math::Float4x4 ComputeRootTransform() const {
    return ozz::math::Float4x4::Translation(
               ozz::math::simd_float4::Load3PtrU(&root_translation_.x)) *
           ozz::math::Float4x4::FromEuler(
               ozz::math::simd_float4::Load3PtrU(&root_euler_.x)) *
           ozz::math::Float4x4::Scaling(
               ozz::math::simd_float4::Load1(root_scale_));
  }

 private:
  // Runtime skeleton.
  ozz::animation::Skeleton* skeleton_;

  // Buffer of local transforms as sampled from animation_.
  ozz::Vector<ozz::math::SoaTransform>::Std locals_;

  // Buffer of model space matrices.
  ozz::Vector<ozz::math::Float4x4>::Std models_;

  // Indices of the joints in the chain.
  int start_joint_;
  int mid_joint_;
  int end_joint_;

  bool doik;

  // Root transformation.
  ozz::math::Float3 root_translation_;
  ozz::math::Float3 root_euler_;
  float root_scale_;

  // IK parameters.

  // TODO
  ozz::math::Float3 pole_vector;

  // TODO
  float weight_;
  float soften_;
  float twist_angle_;

  // Handle positioning.
  float extent;
  ozz::math::Float3 target_offset;

  // Sample skeleton mode option.

  // Uses loaded skeleton, if false then use editable skeleton below.
  bool use_loaded_skeleton_;

  // Raw skeleton that can be edited in UI
  ozz::animation::offline::RawSkeleton editable_skeleton_;
  ozz::sample::RawSkeletonEditor raw_skeleton_editor;

  // Sample display options
  bool show_target_;
  bool show_joints_;
  bool show_pole_vector_;
};

int main(int _argc, const char** _argv) {
  const char* title = "Ozz-animation sample: Two bone IK";
  return TwoBoneIKSampleApplication().Run(_argc, _argv, "1.0", title);
}
