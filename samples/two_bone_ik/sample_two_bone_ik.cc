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

#include "ozz/animation/runtime/local_to_model_job.h"
#include "ozz/animation/runtime/skeleton.h"
#include "ozz/animation/runtime/two_bone_ik_job.h"

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

ozz::math::Float4x4 sroot;

ozz::animation::Skeleton* TempBuildSkeleton() {
  ozz::animation::offline::RawSkeleton raw_skeleton;

  raw_skeleton.roots.resize(1);
  ozz::animation::offline::RawSkeleton::Joint& root = raw_skeleton.roots[0];
  root.name = "root";
  root.transform.translation = ozz::math::Float3::zero();
  root.transform.rotation = ozz::math::Quaternion::FromAxisAngle(
      ozz::math::Float3::x_axis(), ozz::math::kPi / 2.f);
  root.transform.rotation =
      ozz::math::Quaternion::FromEuler(ozz::math::Float3(1, 2, 3));
  // root.transform.rotation = ozz::math::Quaternion::identity();
  // root.transform.scale = ozz::math::Float3(1, 5, 1);
  root.transform.scale = ozz::math::Float3::one();

  root.children.resize(1);
  ozz::animation::offline::RawSkeleton::Joint& start = root.children[0];
  start.name = "shoulder";
  start.transform.translation = ozz::math::Float3::y_axis();
  start.transform.rotation = ozz::math::Quaternion::FromAxisAngle(
      ozz::math::Float3::x_axis(), -ozz::math::kPi / 4.f);
  // start.transform.rotation = ozz::math::Quaternion::identity();
  // start.transform.scale = ozz::math::Float3(1, 2, 1);
  start.transform.scale = ozz::math::Float3::one();

//#define STARIT
#ifdef STARIT
  start.children.resize(1);
  ozz::animation::offline::RawSkeleton::Joint& starit = start.children[0];
  starit.name = "starit";
  starit.transform.translation = ozz::math::Float3::y_axis() * .5f;
  starit.transform.rotation = ozz::math::Quaternion::FromAxisAngle(
      ozz::math::Float3::x_axis(), ozz::math::kPi / 2.);
  // starit.transform.rotation = ozz::math::Quaternion::identity();
  starit.transform.scale = ozz::math::Float3::one();
#else
  ozz::animation::offline::RawSkeleton::Joint& starit = start;
#endif

  starit.children.resize(1);
  ozz::animation::offline::RawSkeleton::Joint& mid = starit.children[0];
  mid.name = "forearm";
  mid.transform.translation = ozz::math::Float3::y_axis();
  mid.transform.rotation = ozz::math::Quaternion::FromAxisAngle(
      ozz::math::Float3::x_axis(), 2.f * ozz::math::kPi / 3.f);
  mid.transform.scale = ozz::math::Float3::one();

#define MIIDT
#ifdef MIIDT
  mid.children.resize(1);
  ozz::animation::offline::RawSkeleton::Joint& miidt = mid.children[0];
  miidt.name = "miidt";
  miidt.transform.translation = ozz::math::Float3::y_axis() * .5f;
  miidt.transform.rotation = ozz::math::Quaternion::FromAxisAngle(
      ozz::math::Float3::x_axis(), ozz::math::kPi / 2.f);
  // miidt.transform.rotation = ozz::math::Quaternion::identity();
  miidt.transform.scale = ozz::math::Float3::one();
#else
  ozz::animation::offline::RawSkeleton::Joint& miidt = mid;
#endif

  miidt.children.resize(1);
  ozz::animation::offline::RawSkeleton::Joint& end = miidt.children[0];
  end.name = "wrist";
  end.transform.translation = ozz::math::Float3::y_axis() * .5f;
  end.transform.rotation = ozz::math::Quaternion::identity();
  end.transform.scale = ozz::math::Float3::one();

  ozz::animation::offline::SkeletonBuilder builder;
  return builder(raw_skeleton);
}

ozz::math::SimdFloat4 g_handle_pos;

void SetQuaternion(int _joint, const ozz::math::SimdQuaternion& _quat,
                   const ozz::Range<ozz::math::SoaTransform>& _transforms) {
  const int joint_soa = _joint / 4;
  const int joint_soa_r = _joint - joint_soa * 4;
  const ozz::math::SimdInt4 kMasks[] = {
      ozz::math::simd_int4::mask_f000(), ozz::math::simd_int4::mask_0f00(),
      ozz::math::simd_int4::mask_00f0(), ozz::math::simd_int4::mask_000f()};
  const ozz::math::SimdInt4 mask = kMasks[joint_soa_r];
  ozz::math::SoaQuaternion& qlocal = _transforms[joint_soa].rotation;
  /*
  qlocal.x =
      ozz::math::Select(mask, ozz::math::SplatX(smid_rotation), qlocal.x);
  qlocal.y =
      ozz::math::Select(mask, ozz::math::SplatY(smid_rotation), qlocal.y);
  qlocal.z =
      ozz::math::Select(mask, ozz::math::SplatZ(smid_rotation), qlocal.z);
  qlocal.w =
      ozz::math::Select(mask, ozz::math::SplatW(smid_rotation), qlocal.w);
          */
  ozz::math::SoaQuaternion q = {
      ozz::math::Select(mask, ozz::math::SplatX(_quat.xyzw),
                        ozz::math::simd_float4::zero()),
      ozz::math::Select(mask, ozz::math::SplatY(_quat.xyzw),
                        ozz::math::simd_float4::zero()),
      ozz::math::Select(mask, ozz::math::SplatZ(_quat.xyzw),
                        ozz::math::simd_float4::zero()),
      ozz::math::Select(mask, ozz::math::SplatW(_quat.xyzw),
                        ozz::math::simd_float4::one())};

  qlocal = qlocal * q;
}

// Skeleton archive can be specified as an option.
OZZ_OPTIONS_DECLARE_STRING(skeleton,
                           "Path to the skeleton (ozz archive format).",
                           "media/skeleton.ozz", false)

class TwoBoneIKSampleApplication : public ozz::sample::Application {
 public:
  TwoBoneIKSampleApplication()
      : doik(true),
        plan(true),
        scale(1.f),
        twist(0.f),
        extent(.4f),
        offset(0.f, .1f, .1f),
        pole(0.f, 1.f, 0.f),
        skeleton_(NULL),
        start_joint_(-1),
        mid_joint_(-1),
        end_joint_(-1) {}

  bool doik;
  bool plan;
  float scale;
  float twist;
  float extent;
  ozz::math::Float3 offset;
  ozz::math::Float3 pole;

 protected:
  virtual bool OnUpdate(float _dt) {
    (void)_dt;

    for (size_t i = 0; i < locals_.size(); ++i) {
      locals_[i] = skeleton_->bind_pose()[i];
    }

    sroot = ozz::math::Float4x4::Scaling(ozz::math::simd_float4::Load1(scale));

    // Converts from local space to model space matrices.
    ozz::animation::LocalToModelJob ltm_job;
    ltm_job.skeleton = skeleton_;
    ltm_job.input = make_range(locals_);
    ltm_job.output = make_range(models_);
    ltm_job.root = &sroot;
    if (!ltm_job.Run()) {
      return false;
    }

    static float time = 0.f;
    time += _dt;
    g_handle_pos = ozz::math::simd_float4::Load(
        offset.x + extent * sinf(time), offset.y + extent * cosf(time * 2.f),
        offset.z + extent * cosf(time / 2.f), 0);

    if (!IK()) {
      return false;
    }

    if (!ltm_job.Run()) {
      return false;
    }

    return true;
  }

  bool IK() {
    ozz::animation::TwoBoneIKJob ik_job;
    ik_job.handle = g_handle_pos;
    ik_job.pole_vector = ozz::math::simd_float4::Load3PtrU(&pole.x);
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
      SetQuaternion(mid_joint_, mid_correction, make_range(locals_));
      SetQuaternion(start_joint_, start_correction, make_range(locals_));
    }

    return true;
  }

  // Samples animation, transforms to model space and renders.
  virtual bool OnDisplay(ozz::sample::Renderer* _renderer) {
    bool success = true;

    const ozz::sample::Renderer::Color colors[2] = {{0xff, 0, 0, 0xff},
                                                    {0, 0xff, 0, 0xff}};

    const float kBoxHalfSize = .01f;
    success &= _renderer->DrawBoxIm(
        ozz::math::Box(ozz::math::Float3(-kBoxHalfSize),
                       ozz::math::Float3(kBoxHalfSize)),
        ozz::math::Float4x4::Translation(g_handle_pos), colors);

    // Draws the animated skeleton.
    success &= _renderer->DrawPosture(*skeleton_, make_range(models_),
                                      ozz::math::Float4x4::identity());

    return success;
  }

  virtual bool OnInitialize() {
// Reading skeleton.
#define LOAD
#ifdef LOAD
    ozz::memory::Allocator* allocator = ozz::memory::default_allocator();
    skeleton_ = allocator->New<ozz::animation::Skeleton>();
    if (!ozz::sample::LoadSkeleton(OPTIONS_skeleton, skeleton_)) {
      return false;
    }
#else
    skeleton_ = TempBuildSkeleton();
#endif

    // Allocates runtime buffers.
    const int num_soa_joints = skeleton_->num_soa_joints();
    locals_.resize(num_soa_joints);
    const int num_joints = skeleton_->num_joints();
    models_.resize(num_joints);

    //
    for (int i = 0; i < skeleton_->num_joints(); i++) {
      const char* joint_name = skeleton_->joint_names()[i];
      if (std::strcmp(joint_name, "shoulder" /*"start"*/) == 0) {
        start_joint_ = i;
      } else if (std::strcmp(joint_name, "forearm" /*"mid"*/) == 0) {
        mid_joint_ = i;
      } else if (std::strcmp(joint_name, "wrist" /*"end"*/) == 0) {
        end_joint_ = i;
      }
    }

    if (start_joint_ < 0 || mid_joint_ < 0 || end_joint_ < 0) {
      return false;
    }

    return true;
  }

  virtual void OnDestroy() {
    ozz::memory::Allocator* allocator = ozz::memory::default_allocator();
    allocator->Delete(skeleton_);
  }

  virtual bool OnGui(ozz::sample::ImGui* _im_gui) {
    _im_gui->DoCheckBox("ik", &doik);
    _im_gui->DoCheckBox("plan", &plan);
    _im_gui->DoSlider("scale", -2.f, 2.f, &scale);
    if (scale == 0.f) scale = .01f;
    _im_gui->DoSlider("twist", -5.f, 5.f, &twist);
    _im_gui->DoSlider("extent", 0.f, 5.f, &extent);
    _im_gui->DoSlider("offset x", -1.f, 1.f, &offset.x);
    _im_gui->DoSlider("offset y", -1.f, 1.f, &offset.y);
    _im_gui->DoSlider("offset z", -1.f, 1.f, &offset.z);
    _im_gui->DoSlider("pole x", -2.f, 2.f, &pole.x);
    _im_gui->DoSlider("pole y", -2.f, 2.f, &pole.y);
    _im_gui->DoSlider("pole z", -2.f, 2.f, &pole.z);

    return true;
  }

  virtual void GetSceneBounds(ozz::math::Box* _bound) const {
    ozz::sample::ComputePostureBounds(make_range(models_), _bound);
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
};

int main(int _argc, const char** _argv) {
  const char* title = "Ozz-animation sample: Two bone IK";
  return TwoBoneIKSampleApplication().Run(_argc, _argv, "1.0", title);
}
