//============================================================================//
// Copyright (c) <2012> <Guillaume Blanc>                                     //
//                                                                            //
// This software is provided 'as-is', without any express or implied          //
// warranty. In no event will the authors be held liable for any damages      //
// arising from the use of this software.                                     //
//                                                                            //
// Permission is granted to anyone to use this software for any purpose,      //
// including commercial applications, and to alter it and redistribute it     //
// freely, subject to the following restrictions:                             //
//                                                                            //
// 1. The origin of this software must not be misrepresented; you must not    //
// claim that you wrote the original software. If you use this software       //
// in a product, an acknowledgment in the product documentation would be      //
// appreciated but is not required.                                           //
//                                                                            //
// 2. Altered source versions must be plainly marked as such, and must not be //
// misrepresented as being the original software.                             //
//                                                                            //
// 3. This notice may not be removed or altered from any source               //
// distribution.                                                              //
//============================================================================//

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>

#include "ozz/animation/animation.h"
#include "ozz/animation/skeleton.h"
#include "ozz/animation/sampling_job.h"
#include "ozz/animation/local_to_model_job.h"
#include "ozz/animation/utils.h"

#include "ozz/animation/offline/animation_builder.h"
#include "ozz/animation/offline/skeleton_builder.h"

#include "ozz/base/maths/vec_float.h"
#include "ozz/base/maths/quaternion.h"

#include "ozz/base/memory/allocator.h"

#include "framework/application.h"
#include "framework/renderer.h"
#include "framework/imgui.h"
#include "framework/utils.h"

class MillipedeAplication : public ozz::demo::Application {
 public:
  MillipedeAplication()
    : animation_time_(0.f),
      playback_speed_(1.f),
      slice_count_(27),
      skeleton_(NULL),
      animation_(NULL),
      cache_(NULL),
      locals_(NULL),
      locals_end_(NULL),
      models_(NULL),
      models_end_(NULL) {
  }

 protected:
  virtual bool OnUpdate(float _dt) {
    // Updates current animation time
    const float new_time = animation_time_ + _dt * playback_speed_;
    const float loops = new_time / animation_->duration();
    animation_time_ = new_time - floorf(loops) * animation_->duration();

    // Samples animation at t = animation_time_.
    ozz::animation::SamplingJob sampling_job;
    sampling_job.animation = animation_;
    sampling_job.cache = cache_;
    sampling_job.time = animation_time_;
    sampling_job.output_begin = locals_;
    sampling_job.output_end = locals_end_;
    if (!sampling_job.Run()) {
      return false;
    }

    // Converts from local space to model space matrices.
    ozz::animation::LocalToModelJob ltm_job;
    ltm_job.skeleton = skeleton_;
    ltm_job.input_begin = locals_;
    ltm_job.input_end = locals_end_;
    ltm_job.output_begin = models_;
    ltm_job.output_end = models_end_;
    return ltm_job.Run();
  }

  virtual bool OnDisplay(ozz::demo::Renderer* _renderer) {
    // Renders the animated posture.
    return _renderer->DrawPosture(*skeleton_, models_, models_end_, true);
  }

  virtual bool OnInitialize() {
    return Build();
  }

  virtual void OnDestroy() {
    Destroy();
  }

  virtual bool OnGui(ozz::demo::ImGui* _im_gui) {
    // Rebuilds all if the number of joints has changed.
    const float joints = slice_count_ * 5 + 1.f;
    char szJoints[64];
    std::sprintf(szJoints, "Joints count: %.0f", joints);

    // Uses an exponential scale in the slider to maintain enough precision in
    // the lowest values.
    const float max = static_cast<float>(ozz::animation::Skeleton::kMaxJoints);
    float new_joints = joints;
    if (_im_gui->DoSlider(szJoints,
                          6.f, max, &new_joints, .3f,
                          true)) {
      const int new_slice_count = (static_cast<int>(new_joints) - 1) / 5;
      if (new_slice_count != slice_count_) {  // Slider use floats !
        slice_count_ = new_slice_count;
        Destroy();
        if (!Build()) {
          return false;
        }
      }
    }

    // Sets playback speed.
    char szSpeed[64];
    std::sprintf(szSpeed, "Playback speed: %.2f", playback_speed_);
    _im_gui->DoSlider(szSpeed, -2.f, 2.f, &playback_speed_);

    return true;
  }

  bool Build() {
    using ozz::math::Float3;
    using ozz::math::Float4;
    using ozz::math::Quaternion;
    using ozz::math::SoaTransform;
    using ozz::math::Float4x4;
    using ozz::animation::offline::RawSkeleton;
    using ozz::animation::offline::RawAnimation;

    const int num_joints = slice_count_ * 5 + 1;
    const float kDuration = 6.f;
    const float kSpinLength = .5f;
    const float kWalkCycleLength = 2.f;
    const int kWalkCycleCount = 4;
    const float kSpinLoop =
      2 * kWalkCycleCount * kWalkCycleLength / kSpinLength;

    /* A millipede slice is 2 legs and a spine.
         *
         |
         sp
        / \              sp
       lu ru         lu__.__ru
      /     \        |       |
     ld     rd       ld  *  rd
    */
    const Float3 t_u = Float3(0.f, 0.f, 0.f);
    const Float3 t_d = Float3(0.f, 0.f, 1.f);

    const Quaternion r_lu =
      Quaternion::FromAxisAngle(Float4(0.f, 1.f, 0.f, -ozz::math::kPi_2));
    const Quaternion r_ld =
      Quaternion::FromAxisAngle(Float4(1.f, 0.f, 0.f, ozz::math::kPi_2)) *
      Quaternion::FromAxisAngle(Float4(0.f, 1.f, 0.f, -ozz::math::kPi_2));
    const Quaternion r_ru =
      Quaternion::FromAxisAngle(Float4(0.f, 1.f, 0.f, ozz::math::kPi_2));
    const Quaternion r_rd =
      Quaternion::FromAxisAngle(Float4(1.f, 0.f, 0.f, ozz::math::kPi_2)) *
      Quaternion::FromAxisAngle(Float4(0.f, 1.f, 0.f, -ozz::math::kPi_2));

    const RawAnimation::TranslationKey tkeys[] = {
      {0.f * kDuration, Float3(.25f * kWalkCycleLength, 0.f, 0.f)},
      {.125f * kDuration, Float3(-.25f * kWalkCycleLength, 0.f, 0.f)},
      {.145f * kDuration, Float3(-.17f * kWalkCycleLength, 0.3f, 0.f)},
      {.23f * kDuration, Float3(.17f * kWalkCycleLength, 0.3f, 0.f)},
      {.25f * kDuration, Float3(.25f * kWalkCycleLength, 0.f, 0.f)},
      {.375f * kDuration, Float3(-.25f * kWalkCycleLength, 0.f, 0.f)},
      {.395f * kDuration, Float3(-.17f * kWalkCycleLength, 0.3f, 0.f)},
      {.48f * kDuration, Float3(.17f * kWalkCycleLength, 0.3f, 0.f)},
      {.5f * kDuration, Float3(.25f * kWalkCycleLength, 0.f, 0.f)},
      {.625f * kDuration, Float3(-.25f * kWalkCycleLength, 0.f, 0.f)},
      {.645f * kDuration, Float3(-.17f * kWalkCycleLength, 0.3f, 0.f)},
      {.73f * kDuration, Float3(.17f * kWalkCycleLength, 0.3f, 0.f)},
      {.75f * kDuration, Float3(.25f * kWalkCycleLength, 0.f, 0.f)},
      {.875f * kDuration, Float3(-.25f * kWalkCycleLength, 0.f, 0.f)},
      {.895f * kDuration, Float3(-.17f * kWalkCycleLength, 0.3f, 0.f)},
      {.98f * kDuration, Float3(.17f * kWalkCycleLength, 0.3f, 0.f)}};
    const int tkey_count = OZZ_ARRAY_SIZE(tkeys);

    // Initializes the root. The root pointer will change from a spine to the
    // next for each centiped slice.
    RawSkeleton raw_skeleton;
    raw_skeleton.roots.resize(1);
    RawSkeleton::Joint* root = &raw_skeleton.roots[0];
    root->name = "root";
    root->transform.translation = Float3(0.f, 1.f, -slice_count_ * kSpinLength);
    root->transform.rotation = Quaternion::identity();
    root->transform.scale = Float3::one();

    char buf[16];
    for (int i = 0; i < slice_count_; i++) {
      // Format joint number.
      std::sprintf(buf, "%d", i);

      root->children.resize(3);

      // Left leg.
      RawSkeleton::Joint& lu = root->children[0];
      lu.name = "lu";
      lu.name += buf;
      lu.transform.translation = t_u;
      lu.transform.rotation = r_lu;
      lu.transform.scale = Float3::one();

      lu.children.resize(1);
      RawSkeleton::Joint& ld = lu.children[0];
      ld.name = "ld";
      ld.name += buf;
      ld.transform.translation = t_d;
      ld.transform.rotation = r_ld;
      ld.transform.scale = Float3::one();

      // Right leg.
      RawSkeleton::Joint& ru = root->children[1];
      ru.name = "ru";
      ru.name += buf;
      ru.transform.translation = t_u;
      ru.transform.rotation = r_ru;
      ru.transform.scale = Float3::one();

      ru.children.resize(1);
      RawSkeleton::Joint& rd = ru.children[0];
      rd.name = "rd";
      rd.name += buf;
      rd.transform.translation = t_d;
      rd.transform.rotation = r_rd;
      rd.transform.scale = Float3::one();

      // Spine.
      RawSkeleton::Joint& sp = root->children[2];
      sp.name = "sp";
      sp.name += buf;
      sp.transform.translation = Float3(0.f, 0.f, kSpinLength);
      sp.transform.rotation = Quaternion::identity();
      sp.transform.scale = Float3::one();

      root = &sp;
    }
    assert(raw_skeleton.num_joints() == num_joints);

    // Build the run time skeleton.
    ozz::animation::offline::SkeletonBuilder skeleton_builder;
    skeleton_ = skeleton_builder(raw_skeleton);
    if (!skeleton_) {
      return false;
    }

    // Build a walk animation.
    RawAnimation raw_animation;
    raw_animation.duration = kDuration;
    raw_animation.tracks.resize(skeleton_->num_joints());

    for (int i = 0; i < raw_animation.num_tracks(); i++) {
      RawAnimation::JointTrack& track = raw_animation.tracks[i];
      const char* joint_name = skeleton_->joint_names()[i];

      if (strstr(joint_name, "ld") || strstr(joint_name, "rd")) {
        bool left = joint_name[0] == 'l';  // First letter of "ld".

        // Copy original keys while taking into consideration the spine number
        // as a phase.
        const int spine_number = std::atoi(joint_name + 2);
        const float offset = kDuration * (slice_count_ - spine_number) /
          kSpinLoop;
        const float phase = std::fmod(offset, kDuration);

        // Loop to find animation start.
        int i_offset = 0;
        while (i_offset < tkey_count && tkeys[i_offset].time < phase) {
          i_offset++;
        }

        // Push key with their corrected time.
        track.translations.reserve(tkey_count);
        for (int j = i_offset; j < i_offset + tkey_count; j++) {
          const RawAnimation::TranslationKey& rkey = tkeys[j % tkey_count];
          float new_time = rkey.time - phase;
          if (new_time < 0.f) {
            new_time = kDuration - phase + rkey.time;
          }

          if (left) {
            const RawAnimation::TranslationKey tkey =
              {new_time, t_d + rkey.value};
            track.translations.push_back(tkey);
          } else {
            const RawAnimation::TranslationKey tkey =
              {new_time, Float3(t_d.x - rkey.value.x,
               t_d.y + rkey.value.y,
               t_d.z + rkey.value.z)};
            track.translations.push_back(tkey);
          }
        }

        // Pushes rotation key-frame.
        if (left) {
          const RawAnimation::RotationKey rkey = {0.f, r_ld};
          track.rotations.push_back(rkey);
        } else {
          const RawAnimation::RotationKey rkey = {0.f, r_rd};
          track.rotations.push_back(rkey);
        }
      } else if (strstr(joint_name, "lu")) {
        const RawAnimation::TranslationKey tkey = {0.f, t_u};
        track.translations.push_back(tkey);

        const RawAnimation::RotationKey rkey = {0.f, r_lu};
        track.rotations.push_back(rkey);

      } else if (strstr(joint_name, "ru")) {
        const RawAnimation::TranslationKey tkey0 = {0.f, t_u};
        track.translations.push_back(tkey0);

        const RawAnimation::RotationKey rkey0 = {0.f, r_ru};
        track.rotations.push_back(rkey0);
      } else if (strstr(joint_name, "sp")) {
        const RawAnimation::TranslationKey skey = {
          0.f,
          Float3(0.f, 0.f, kSpinLength)};
        track.translations.push_back(skey);

        const RawAnimation::RotationKey rkey = {
          0.f,
          ozz::math::Quaternion::FromAxisAngle(Float4(0.f, 1.f, 0.f, 0.f))};
        track.rotations.push_back(rkey);
      } else if (strstr(joint_name, "root")) {
        const RawAnimation::TranslationKey tkey0 = {
          0.f,
          Float3(0.f, 1.f, -slice_count_ * kSpinLength)};
        track.translations.push_back(tkey0);
        const RawAnimation::TranslationKey tkey1 = {
          kDuration,
          Float3(0.f, 1.f, kWalkCycleCount * kWalkCycleLength + tkey0.value.z)};
        track.translations.push_back(tkey1);
      }

      // Make sure begin and end keys are looping.
      if (track.translations.front().time != 0.f) {
        const RawAnimation::TranslationKey& front = track.translations.front();
        const RawAnimation::TranslationKey& back = track.translations.back();
        const float lerp_time =
          front.time / (front.time + kDuration - back.time);
        const RawAnimation::TranslationKey tkey = {
          0.f,
          Lerp(front.value, back.value, lerp_time)};
        track.translations.insert(track.translations.begin(), tkey);
      }
      if (track.translations.back().time != kDuration) {
        const RawAnimation::TranslationKey& front = track.translations.front();
        const RawAnimation::TranslationKey& back = track.translations.back();
        const float lerp_time =
          (kDuration - back.time) / (front.time + kDuration - back.time);
        const RawAnimation::TranslationKey tkey = {
          kDuration, Lerp(back.value, front.value, lerp_time)};
        track.translations.push_back(tkey);
      }
    }

    // Build the run time animation from the raw animation.
    ozz::animation::offline::AnimationBuilder animation_builder;
    animation_ = animation_builder(raw_animation);
    if (!animation_) {
      return false;
    }

    // Allocates runtime buffers.
    ozz::memory::Allocator& allocator = ozz::memory::default_allocator();

    ozz::animation::LocalsAlloc locals_alloc =
      ozz::animation::AllocateLocals(&allocator, num_joints);
    locals_ = locals_alloc.begin;
    locals_end_ = locals_alloc.end;

    ozz::animation::ModelsAlloc models_alloc =
      ozz::animation::AllocateModels(&allocator, num_joints);
    models_ = models_alloc.begin;
    models_end_ = models_alloc.end;

    // Allocates a cache that matches new animation requirements.
    cache_ =
      allocator.New<ozz::animation::SamplingCache>(num_joints);
    return true;
  }

  void Destroy() {
    ozz::memory::Allocator& allocator = ozz::memory::default_allocator();
    allocator.Delete(skeleton_);
    skeleton_ = NULL;
    allocator.Delete(animation_);
    animation_ = NULL;
    allocator.Deallocate(locals_);
    locals_ = NULL;
    allocator.Deallocate(models_);
    models_ = NULL;
    allocator.Delete(cache_);
    cache_ = NULL;
  }

  virtual bool GetSceneBounds(ozz::math::Box* _bound) const {
    return ozz::demo::ComputePostureBounds(models_, models_end_, _bound);
  }

  virtual const char* GetTitle() const {
    return "ozz_animation millipede demo application";
  }

 private:
  // Current animation time.
  float animation_time_;

  // Playback speed, can be negative in order to play the animation backward.
  float playback_speed_;

  // Millipede skeleton number of slices. 5 joints per slice.
  int slice_count_;

  // The millipede skeleton.
  ozz::animation::Skeleton* skeleton_;

  // The millipede procedural walk animation.
  ozz::animation::Animation* animation_;

  // Sampling cache, as used by SamplingJob.
  ozz::animation::SamplingCache* cache_;

  // Buffer of local transforms as sampled from animation_.
  // These are shared between sampling output and local-to-model input.
  ozz::math::SoaTransform* locals_;
  const ozz::math::SoaTransform* locals_end_;

  // Buffer of model matrices (local-to-model output).
  ozz::math::Float4x4* models_;
  const ozz::math::Float4x4* models_end_;
};

int main(int _argc, const char** _argv) {
  return MillipedeAplication().Run(
    _argc, _argv,
    "1.0",
    "Procedurally generates a millipede skeleton and walk animation. "
    "This demo allows to test skeletons with 6 to 16381 joints.\n"
    "Update time is interesting as it measures sampling and local-to-"
    "model workloads. FPS and render time are not very important as they "
    "measure skeleton rendering.");
}
