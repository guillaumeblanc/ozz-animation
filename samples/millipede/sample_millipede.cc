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

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "ozz/animation/runtime/animation.h"
#include "ozz/animation/runtime/local_to_model_job.h"
#include "ozz/animation/runtime/sampling_job.h"
#include "ozz/animation/runtime/skeleton.h"

#include "ozz/animation/offline/animation_builder.h"
#include "ozz/animation/offline/raw_animation.h"
#include "ozz/animation/offline/raw_skeleton.h"
#include "ozz/animation/offline/skeleton_builder.h"

#include "ozz/base/maths/quaternion.h"
#include "ozz/base/maths/simd_math.h"
#include "ozz/base/maths/soa_transform.h"
#include "ozz/base/maths/vec_float.h"

#include "framework/application.h"
#include "framework/imgui.h"
#include "framework/renderer.h"
#include "framework/utils.h"

using ozz::animation::offline::RawAnimation;
using ozz::animation::offline::RawSkeleton;
using ozz::math::Float3;
using ozz::math::Float4;
using ozz::math::Float4x4;
using ozz::math::Quaternion;
using ozz::math::SoaTransform;

// A millipede slice is 2 legs and a spine.
// Each slice is made of 7 joints, organized as follows.
//          * root
//             |
//           spine                                   spine
//         |       |                                   |
//     left_up    right_up        left_down - left_u - . - right_u - right_down
//       |           |                  |                                    |
//   left_down     right_down     left_foot         * root            right_foot
//     |               |
// left_foot        right_foot

// The following constants are used to define the millipede skeleton and
// animation.
// Skeleton constants.
const Float3 kTransUp = Float3(0.f, 0.f, 0.f);
const Float3 kTransDown = Float3(0.f, 0.f, 1.f);
const Float3 kTransFoot = Float3(1.f, 0.f, 0.f);

const Quaternion kRotLeftUp =
    Quaternion::FromAxisAngle(Float3::y_axis(), -ozz::math::kPi_2);
const Quaternion kRotLeftDown =
    Quaternion::FromAxisAngle(Float3::x_axis(), ozz::math::kPi_2) *
    Quaternion::FromAxisAngle(Float3::y_axis(), -ozz::math::kPi_2);
const Quaternion kRotRightUp =
    Quaternion::FromAxisAngle(Float3::y_axis(), ozz::math::kPi_2);
const Quaternion kRotRightDown =
    Quaternion::FromAxisAngle(Float3::x_axis(), ozz::math::kPi_2) *
    Quaternion::FromAxisAngle(Float3::y_axis(), -ozz::math::kPi_2);

// Animation constants.
const float kDuration = 6.f;
const float kSpinLength = .5f;
const float kWalkCycleLength = 2.f;
const int kWalkCycleCount = 4;
const float kSpinLoop = 2 * kWalkCycleCount * kWalkCycleLength / kSpinLength;

const RawAnimation::TranslationKey kPrecomputedKeys[] = {
    {0.f * kDuration, Float3(.25f * kWalkCycleLength, 0.f, 0.f)},
    {.125f * kDuration, Float3(-.25f * kWalkCycleLength, 0.f, 0.f)},
    {.145f * kDuration, Float3(-.17f * kWalkCycleLength, .3f, 0.f)},
    {.23f * kDuration, Float3(.17f * kWalkCycleLength, .3f, 0.f)},
    {.25f * kDuration, Float3(.25f * kWalkCycleLength, 0.f, 0.f)},
    {.375f * kDuration, Float3(-.25f * kWalkCycleLength, 0.f, 0.f)},
    {.395f * kDuration, Float3(-.17f * kWalkCycleLength, .3f, 0.f)},
    {.48f * kDuration, Float3(.17f * kWalkCycleLength, .3f, 0.f)},
    {.5f * kDuration, Float3(.25f * kWalkCycleLength, 0.f, 0.f)},
    {.625f * kDuration, Float3(-.25f * kWalkCycleLength, 0.f, 0.f)},
    {.645f * kDuration, Float3(-.17f * kWalkCycleLength, .3f, 0.f)},
    {.73f * kDuration, Float3(.17f * kWalkCycleLength, .3f, 0.f)},
    {.75f * kDuration, Float3(.25f * kWalkCycleLength, 0.f, 0.f)},
    {.875f * kDuration, Float3(-.25f * kWalkCycleLength, 0.f, 0.f)},
    {.895f * kDuration, Float3(-.17f * kWalkCycleLength, .3f, 0.f)},
    {.98f * kDuration, Float3(.17f * kWalkCycleLength, .3f, 0.f)}};
const int kPrecomputedKeyCount = OZZ_ARRAY_SIZE(kPrecomputedKeys);

class MillipedeSampleApplication : public ozz::sample::Application {
 public:
  MillipedeSampleApplication() : slice_count_(26) {}

 protected:
  virtual bool OnUpdate(float _dt, float) {
    // Updates current animation time
    controller_.Update(*animation_, _dt);

    // Samples animation at t = animation_time_.
    ozz::animation::SamplingJob sampling_job;
    sampling_job.animation = animation_.get();
    sampling_job.cache = &cache_;
    sampling_job.ratio = controller_.time_ratio();
    sampling_job.output = make_span(locals_);
    if (!sampling_job.Run()) {
      return false;
    }

    // Converts from local space to model space matrices.
    ozz::animation::LocalToModelJob ltm_job;
    ltm_job.skeleton = skeleton_.get();
    ltm_job.input = make_span(locals_);
    ltm_job.output = make_span(models_);
    return ltm_job.Run();
  }

  virtual bool OnDisplay(ozz::sample::Renderer* _renderer) {
    // Renders the animated posture.
    return _renderer->DrawPosture(*skeleton_, make_span(models_),
                                  ozz::math::Float4x4::identity());
  }

  virtual bool OnInitialize() { return Build(); }

  virtual void OnDestroy() {}

  virtual bool OnGui(ozz::sample::ImGui* _im_gui) {
    // Rebuilds all if the number of joints has changed.
    int joints = skeleton_->num_joints();
    char label[64];
    std::sprintf(label, "Joints count: %d", joints);

    // Uses an exponential scale in the slider to maintain enough precision in
    // the lowest values.
    if (_im_gui->DoSlider(label, 8, ozz::animation::Skeleton::kMaxJoints,
                          &joints, .3f, true)) {
      const int new_slice_count = (joints - 1) / 7;
      // Slider use floats, we need to check if it has really changed.
      if (new_slice_count != slice_count_) {
        slice_count_ = new_slice_count;
        if (!Build()) {
          return false;
        }
      }
    }

    // Updates controller Gui.
    controller_.OnGui(*animation_, _im_gui);

    return true;
  }

  // Procedurally builds millipede skeleton and walk animation
  bool Build() {
    // Initializes the root. The root pointer will change from a spine to the
    // next for each slice.
    RawSkeleton raw_skeleton;
    CreateSkeleton(&raw_skeleton);
    const int num_joints = raw_skeleton.num_joints();

    // Build the run time skeleton.
    ozz::animation::offline::SkeletonBuilder skeleton_builder;
    skeleton_ = skeleton_builder(raw_skeleton);
    if (!skeleton_) {
      return false;
    }

    // Build a walk animation.
    RawAnimation raw_animation;
    CreateAnimation(&raw_animation);

    // Build the run time animation from the raw animation.
    ozz::animation::offline::AnimationBuilder animation_builder;
    animation_ = animation_builder(raw_animation);
    if (!animation_) {
      return false;
    }

    // Allocates runtime buffers.
    const int num_soa_joints = skeleton_->num_soa_joints();
    locals_.resize(num_soa_joints);
    models_.resize(num_joints);

    // Allocates a cache that matches new animation requirements.
    cache_.Resize(num_joints);

    return true;
  }

  void CreateSkeleton(ozz::animation::offline::RawSkeleton* _skeleton) {
    _skeleton->roots.resize(1);
    RawSkeleton::Joint* root = &_skeleton->roots[0];
    root->name = "root";
    root->transform.translation = Float3(0.f, 1.f, -slice_count_ * kSpinLength);
    root->transform.rotation = Quaternion::identity();
    root->transform.scale = Float3::one();

    char buf[16];
    for (int i = 0; i < slice_count_; ++i) {
      // Format joint number.
      std::sprintf(buf, "%d", i);

      root->children.resize(3);

      // Left leg.
      RawSkeleton::Joint& lu = root->children[0];
      lu.name = "lu";
      lu.name += buf;
      lu.transform.translation = kTransUp;
      lu.transform.rotation = kRotLeftUp;
      lu.transform.scale = Float3::one();

      lu.children.resize(1);
      RawSkeleton::Joint& ld = lu.children[0];
      ld.name = "ld";
      ld.name += buf;
      ld.transform.translation = kTransDown;
      ld.transform.rotation = kRotLeftDown;
      ld.transform.scale = Float3::one();

      ld.children.resize(1);
      RawSkeleton::Joint& lf = ld.children[0];
      lf.name = "lf";
      lf.name += buf;
      lf.transform.translation = Float3::x_axis();
      lf.transform.rotation = Quaternion::identity();
      lf.transform.scale = Float3::one();

      // Right leg.
      RawSkeleton::Joint& ru = root->children[1];
      ru.name = "ru";
      ru.name += buf;
      ru.transform.translation = kTransUp;
      ru.transform.rotation = kRotRightUp;
      ru.transform.scale = Float3::one();

      ru.children.resize(1);
      RawSkeleton::Joint& rd = ru.children[0];
      rd.name = "rd";
      rd.name += buf;
      rd.transform.translation = kTransDown;
      rd.transform.rotation = kRotRightDown;
      rd.transform.scale = Float3::one();

      rd.children.resize(1);
      RawSkeleton::Joint& rf = rd.children[0];
      rf.name = "rf";
      rf.name += buf;
      rf.transform.translation = Float3::x_axis();
      rf.transform.rotation = Quaternion::identity();
      rf.transform.scale = Float3::one();

      // Spine.
      RawSkeleton::Joint& sp = root->children[2];
      sp.name = "sp";
      sp.name += buf;
      sp.transform.translation = Float3(0.f, 0.f, kSpinLength);
      sp.transform.rotation = Quaternion::identity();
      sp.transform.scale = Float3::one();

      root = &sp;
    }
  }

  void CreateAnimation(ozz::animation::offline::RawAnimation* _animation) {
    _animation->duration = kDuration;
    _animation->tracks.resize(skeleton_->num_joints());

    for (int i = 0; i < _animation->num_tracks(); ++i) {
      RawAnimation::JointTrack& track = _animation->tracks[i];
      const char* joint_name = skeleton_->joint_names()[i];

      if (strstr(joint_name, "ld") || strstr(joint_name, "rd")) {
        bool left = joint_name[0] == 'l';  // First letter of "ld".

        // Copy original keys while taking into consideration the spine number
        // as a phase.
        const int spine_number = std::atoi(joint_name + 2);
        const float offset =
            kDuration * (slice_count_ - spine_number) / kSpinLoop;
        const float phase = std::fmod(offset, kDuration);

        // Loop to find animation start.
        int i_offset = 0;
        while (i_offset < kPrecomputedKeyCount &&
               kPrecomputedKeys[i_offset].time < phase) {
          i_offset++;
        }

        // Push key with their corrected time.
        track.translations.reserve(kPrecomputedKeyCount);
        for (int j = i_offset; j < i_offset + kPrecomputedKeyCount; ++j) {
          const RawAnimation::TranslationKey& rkey =
              kPrecomputedKeys[j % kPrecomputedKeyCount];
          float new_time = rkey.time - phase;
          if (new_time < 0.f) {
            new_time = kDuration - phase + rkey.time;
          }

          if (left) {
            const RawAnimation::TranslationKey tkey = {new_time,
                                                       kTransDown + rkey.value};
            track.translations.push_back(tkey);
          } else {
            const RawAnimation::TranslationKey tkey = {
                new_time,
                Float3(kTransDown.x - rkey.value.x, kTransDown.y + rkey.value.y,
                       kTransDown.z + rkey.value.z)};
            track.translations.push_back(tkey);
          }
        }

        // Pushes rotation key-frame.
        if (left) {
          const RawAnimation::RotationKey rkey = {0.f, kRotLeftDown};
          track.rotations.push_back(rkey);
        } else {
          const RawAnimation::RotationKey rkey = {0.f, kRotRightDown};
          track.rotations.push_back(rkey);
        }
      } else if (strstr(joint_name, "lu")) {
        const RawAnimation::TranslationKey tkey = {0.f, kTransUp};
        track.translations.push_back(tkey);

        const RawAnimation::RotationKey rkey = {0.f, kRotLeftUp};
        track.rotations.push_back(rkey);

      } else if (strstr(joint_name, "ru")) {
        const RawAnimation::TranslationKey tkey0 = {0.f, kTransUp};
        track.translations.push_back(tkey0);

        const RawAnimation::RotationKey rkey0 = {0.f, kRotRightUp};
        track.rotations.push_back(rkey0);
      } else if (strstr(joint_name, "lf")) {
        const RawAnimation::TranslationKey tkey = {0.f, kTransFoot};
        track.translations.push_back(tkey);
      } else if (strstr(joint_name, "rf")) {
        const RawAnimation::TranslationKey tkey0 = {0.f, kTransFoot};
        track.translations.push_back(tkey0);
      } else if (strstr(joint_name, "sp")) {
        const RawAnimation::TranslationKey skey = {
            0.f, Float3(0.f, 0.f, kSpinLength)};
        track.translations.push_back(skey);

        const RawAnimation::RotationKey rkey = {
            0.f, ozz::math::Quaternion::identity()};
        track.rotations.push_back(rkey);
      } else if (strstr(joint_name, "root")) {
        const RawAnimation::TranslationKey tkey0 = {
            0.f, Float3(0.f, 1.f, -slice_count_ * kSpinLength)};
        track.translations.push_back(tkey0);
        const RawAnimation::TranslationKey tkey1 = {
            kDuration,
            Float3(0.f, 1.f,
                   kWalkCycleCount * kWalkCycleLength + tkey0.value.z)};
        track.translations.push_back(tkey1);
      }

      // Make sure begin and end keys are looping.
      if (track.translations.front().time != 0.f) {
        const RawAnimation::TranslationKey& front = track.translations.front();
        const RawAnimation::TranslationKey& back = track.translations.back();
        const float lerp_time =
            front.time / (front.time + kDuration - back.time);
        const RawAnimation::TranslationKey tkey = {
            0.f, Lerp(front.value, back.value, lerp_time)};
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
  }

  virtual void GetSceneBounds(ozz::math::Box* _bound) const {
    ozz::sample::ComputePostureBounds(make_span(models_), _bound);
  }

 private:
  // Playback animation controller. This is a utility class that helps with
  // controlling animation playback time.
  ozz::sample::PlaybackController controller_;

  // Millipede skeleton number of slices. 7 joints per slice.
  int slice_count_;

  // The millipede skeleton.
  ozz::unique_ptr<ozz::animation::Skeleton> skeleton_;

  // The millipede procedural walk animation.
  ozz::unique_ptr<ozz::animation::Animation> animation_;

  // Sampling cache, as used by SamplingJob.
  ozz::animation::SamplingCache cache_;

  // Buffer of local transforms as sampled from animation_.
  // These are shared between sampling output and local-to-model input.
  ozz::vector<ozz::math::SoaTransform> locals_;

  // Buffer of model matrices (local-to-model output).
  ozz::vector<ozz::math::Float4x4> models_;
};

int main(int _argc, const char** _argv) {
  const char* title = "Ozz-animation sample: RawAnimation/RawSkeleton building";
  return MillipedeSampleApplication().Run(_argc, _argv, "1.0", title);
}
