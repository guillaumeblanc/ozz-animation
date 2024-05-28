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

#include "framework/application.h"
#include "framework/imgui.h"
#include "framework/motion_utils.h"
#include "framework/renderer.h"
#include "framework/utils.h"
#include "ozz/animation/runtime/animation.h"
#include "ozz/animation/runtime/blending_job.h"
#include "ozz/animation/runtime/local_to_model_job.h"
#include "ozz/animation/runtime/motion_blending_job.h"
#include "ozz/animation/runtime/sampling_job.h"
#include "ozz/animation/runtime/skeleton.h"
#include "ozz/base/log.h"
#include "ozz/base/maths/box.h"
#include "ozz/base/maths/math_ex.h"
#include "ozz/base/maths/simd_math.h"
#include "ozz/base/maths/soa_transform.h"
#include "ozz/base/maths/vec_float.h"
#include "ozz/options/options.h"

// Skeleton archive can be specified as an option.
OZZ_OPTIONS_DECLARE_STRING(skeleton,
                           "Path to the skeleton (ozz archive format).",
                           "media/skeleton.ozz", false)

// First animation archive can be specified as an option.
OZZ_OPTIONS_DECLARE_STRING(animation1,
                           "Path to the first animation (ozz archive format).",
                           "media/animation1.ozz", false)

// Second animation archive can be specified as an option.
OZZ_OPTIONS_DECLARE_STRING(animation2,
                           "Path to the second animation (ozz archive format).",
                           "media/animation2.ozz", false)

// Third animation archive can be specified as an option.
OZZ_OPTIONS_DECLARE_STRING(animation3,
                           "Path to the third animation (ozz archive format).",
                           "media/animation3.ozz", false)

// First motion track archive can be specified as an option.
OZZ_OPTIONS_DECLARE_STRING(motion1,
                           "Path to the first motion (ozz archive format).",
                           "media/motion1.ozz", false)

// Second motion track archive can be specified as an option.
OZZ_OPTIONS_DECLARE_STRING(motion2,
                           "Path to the first motion (ozz archive format).",
                           "media/motion2.ozz", false)

// Third motion track archive can be specified as an option.
OZZ_OPTIONS_DECLARE_STRING(motion3,
                           "Path to the first motion (ozz archive format).",
                           "media/motion3.ozz", false)

class MotionBlendSampleApplication : public ozz::sample::Application {
 protected:
  // Updates current animation time and skeleton pose.
  virtual bool OnUpdate(float _dt, float) {
    // Updates blending parameters and synchronizes animations if control mode
    // is not manual.
    if (!manual_) {
      UpdateRuntimeParameters();
    }

    // Samples animation and motion tracks.
    for (auto& sampler : samplers_) {
      // Updates animations time.
      int loops = sampler.controller.Update(sampler.animation, _dt);

      // Updates motion.
      //-----------------------------------------------------------------------
      // Always update motion sampler (accumulation needs to happen to keep
      // consistent last-current transforms). A more elaborated implementation
      // could teleport the accumulator as soon as the animation become useful
      // (aka weight > 0).
      // Updates motion accumulator.
      if (!sampler.motion_sampler.Update(
              sampler.motion_track, sampler.controller.time_ratio(), loops)) {
        return false;
      }

      // Updates animation
      //-----------------------------------------------------------------------

      // Early out if this sampler weight makes it irrelevant during animation
      // blending.
      if (sampler.weight <= 0.f) {
        continue;
      }

      // Setup sampling job.
      ozz::animation::SamplingJob sampling_job;
      sampling_job.animation = &sampler.animation;
      sampling_job.context = &sampler.context;
      sampling_job.ratio = sampler.controller.time_ratio();
      sampling_job.output = make_span(sampler.locals);

      // Samples animation.
      if (!sampling_job.Run()) {
        return false;
      }
    }

    // Blends motion.
    //-------------------------------------------------------------------------
    {
      // Fills job layers with delta transforms and weights.
      ozz::animation::MotionBlendingJob::Layer layers[kNumLayers];
      for (size_t i = 0; i < kNumLayers; ++i) {
        const auto& sampler = samplers_[i];
        layers[i].delta =
            &sampler.motion_sampler.delta;  // Uses delta transform from motion
                                            // sampler accumulator.
        layers[i].weight = sampler.weight;  // Reuses animation weight.
      }

      // Setup blending job.
      ozz::math::Transform delta;
      ozz::animation::MotionBlendingJob motion_blend_job;
      motion_blend_job.layers = layers;
      motion_blend_job.output = &delta;

      // Executes blending job.
      if (!motion_blend_job.Run()) {
        return false;
      }

      // Applies blended delta to the character accumulator.
      const auto rotation = FrameRotation(_dt);
      accumulator_.Update(delta, rotation);
    }

    // Updates the character transform matrix.
    const auto& transform = accumulator_.current;
    transform_ = ozz::math::Float4x4::FromAffine(
        transform.translation, transform.rotation, transform.scale);

    // Blends animations.
    //-------------------------------------------------------------------------
    // Blends the local spaces transforms computed by sampling all animations
    // (1st stage just above), and outputs the result to the local space
    // transform buffer locals_

    {
      // Prepares blending layers.
      ozz::animation::BlendingJob::Layer layers[kNumLayers];
      for (size_t i = 0; i < kNumLayers; ++i) {
        layers[i].transform = make_span(samplers_[i].locals);
        layers[i].weight = samplers_[i].weight;
      }

      // Setups blending job.
      ozz::animation::BlendingJob blend_job;
      blend_job.layers = layers;
      blend_job.rest_pose = skeleton_.joint_rest_poses();
      blend_job.output = make_span(locals_);

      // Blends.
      if (!blend_job.Run()) {
        return false;
      }
    }
    // Converts from local space to model space matrices.
    // Gets the output of the blending stage, and converts it to model space.

    // Setup local-to-model conversion job.
    ozz::animation::LocalToModelJob ltm_job;
    ltm_job.skeleton = &skeleton_;
    ltm_job.input = make_span(locals_);
    ltm_job.output = make_span(models_);

    // Runs ltm job.
    if (!ltm_job.Run()) {
      return false;
    }

    return true;
  }

  // Compute rotation to apply for the given _duration
  ozz::math::Quaternion FrameRotation(float _duration) const {
    const float angle = angular_velocity_ * _duration;
    return ozz::math::Quaternion::FromEuler({angle, 0, 0});
  }

  // Computes blending weight and synchronizes playback speed when the
  // "manual" option is off.
  void UpdateRuntimeParameters() {
    // Computes weight parameters for all samplers.
    const float kNumIntervals = kNumLayers - 1;
    const float kInterval = 1.f / kNumIntervals;
    for (size_t i = 0; i < kNumLayers; ++i) {
      const float med = i * kInterval;
      const float x = blend_ratio_ - med;
      const float y = ((x < 0.f ? x : -x) + kInterval) * kNumIntervals;
      samplers_[i].weight = ozz::math::Max(0.f, y);
    }

    // Synchronizes animations.
    // Selects the 2 samplers that define interval that contains blend_ratio_.
    const float clamped_ratio = ozz::math::Clamp(0.f, blend_ratio_, .999f);
    const size_t lower = static_cast<size_t>(clamped_ratio * (kNumLayers - 1));
    const Sampler& sampler_l = samplers_[lower];
    const Sampler& sampler_r = samplers_[lower + 1];

    // Interpolates animation durations using their respective weights, to
    // find the loop cycle duration that matches blend_ratio_.
    const float loop_duration =
        sampler_l.animation.duration() * sampler_l.weight +
        sampler_r.animation.duration() * sampler_r.weight;

    // Finally finds the speed coefficient for all samplers.
    const float inv_loop_duration = 1.f / loop_duration;
    for (auto& sampler : samplers_) {
      const float speed = sampler.animation.duration() * inv_loop_duration;
      sampler.controller.set_playback_speed(speed);
    }
  }

  virtual bool OnDisplay(ozz::sample::Renderer* _renderer) {
    bool success = true;
    // Renders character at transform_ location.
    success |=
        _renderer->DrawPosture(skeleton_, make_span(models_), transform_);

    // Draw a box at character's root.
    if (show_box_) {
      const ozz::math::Box box(ozz::math::Float3(-.25f, 0, -.25f),
                               ozz::math::Float3(.25f, 1.8f, .25f));
      success &= _renderer->DrawBoxIm(box, transform_, ozz::sample::kWhite);
    }

    // Renders motion tracks at transform_ location.
    if (show_motion_) {
      const float kStep = 1.f / 60.f;
      for (const auto& sampler : samplers_) {
        const auto rotation =
            FrameRotation(kStep * sampler.animation.duration());
        const float at = sampler.controller.time_ratio();
        success |= ozz::sample::DrawMotion(
            _renderer, sampler.motion_track, at - 1.f, at, at + 1.f, kStep,
            transform_, rotation, sampler.weight);
      }
    }
    return success;
  }

  virtual bool OnInitialize() {
    // Reading skeleton.
    if (!ozz::sample::LoadSkeleton(OPTIONS_skeleton, &skeleton_)) {
      return false;
    }

    const int num_joints = skeleton_.num_joints();
    const int num_soa_joints = skeleton_.num_soa_joints();

    // Reading animations.
    const char* animations[] = {OPTIONS_animation1, OPTIONS_animation2,
                                OPTIONS_animation3};
    const char* motions[] = {OPTIONS_motion1, OPTIONS_motion2, OPTIONS_motion3};
    static_assert(OZZ_ARRAY_SIZE(animations) == kNumLayers &&
                      OZZ_ARRAY_SIZE(motions) == kNumLayers,
                  "Arrays mismatch.");
    for (size_t i = 0; i < kNumLayers; ++i) {
      Sampler& sampler = samplers_[i];

      if (!ozz::sample::LoadAnimation(animations[i], &sampler.animation)) {
        return false;
      }

      if (!ozz::sample::LoadMotionTrack(motions[i], &sampler.motion_track)) {
        return false;
      }

      // Allocates sampler runtime buffers.
      sampler.locals.resize(num_soa_joints);

      // Allocates a context that matches animation requirements.
      sampler.context.Resize(num_joints);
    }

    // Allocates local space runtime buffers of blended data.
    locals_.resize(num_soa_joints);

    // Allocates model space runtime buffers of blended data.
    models_.resize(num_joints);

    return true;
  }

  virtual bool OnGui(ozz::sample::ImGui* _im_gui) {
    char label[64];

    // Exposes blending parameters.
    {
      static bool open = true;
      ozz::sample::ImGui::OpenClose oc(_im_gui, "Blending parameters", &open);
      if (open) {
        if (_im_gui->DoCheckBox("Manual settings", &manual_) && !manual_) {
          // Check-box state was changed, reset parameters.
          for (auto& sampler : samplers_) {
            sampler.controller.Reset();
          }
        }

        std::snprintf(label, sizeof(label), "Blend ratio: %.2f", blend_ratio_);
        _im_gui->DoSlider(label, 0.f, 1.f, &blend_ratio_, 1.f, !manual_);

        for (size_t i = 0; i < kNumLayers; ++i) {
          Sampler& sampler = samplers_[i];
          std::snprintf(label, sizeof(label), "Weight %d: %.2f",
                        static_cast<int>(i), sampler.weight);
          _im_gui->DoSlider(label, 0.f, 1.f, &sampler.weight, 1.f, manual_);
        }
      }
    }

    // Exposes animations runtime playback controls.
    {
      static bool oc_open = false;
      ozz::sample::ImGui::OpenClose oc(_im_gui, "Animation control", &oc_open);
      if (oc_open) {
        static bool open[] = {true, true, true};
        static_assert(OZZ_ARRAY_SIZE(open) == kNumLayers,
                      "Arrays size mismatch");
        const char* oc_names[] = {"Animation 1", "Animation 2", "Animation 3"};
        static_assert(OZZ_ARRAY_SIZE(oc_names) == kNumLayers,
                      "Arrays size mismatch");
        for (size_t i = 0; i < kNumLayers; ++i) {
          Sampler& sampler = samplers_[i];
          ozz::sample::ImGui::OpenClose loc(_im_gui, oc_names[i], nullptr);
          if (open[i]) {
            sampler.controller.OnGui(sampler.animation, _im_gui, manual_);
          }
        }
      }
    }

    {
      static bool open = true;
      ozz::sample::ImGui::OpenClose oc(_im_gui, "Motion control", &open);
      if (open) {
        std::snprintf(label, sizeof(label), "Angular vel: %.0f deg/s",
                      angular_velocity_ * 180.f / ozz::math::kPi);
        _im_gui->DoSlider(label, -ozz::math::kPi_2, ozz::math::kPi_2,
                          &angular_velocity_);
        if (_im_gui->DoButton("Teleport")) {
          for (auto& sampler : samplers_) {
            sampler.controller.set_time_ratio(0.f);
            sampler.motion_sampler.Teleport(ozz::math::Transform::identity());
          }
          accumulator_.Teleport(ozz::math::Transform::identity());
        }
      }
    }

    {
      static bool open = true;
      ozz::sample::ImGui::OpenClose oc(_im_gui, "Motion display", &open);
      if (open) {
        _im_gui->DoCheckBox("Show box", &show_box_);
        _im_gui->DoCheckBox("Show motion", &show_motion_);
      }
    }

    return true;
  }

  virtual void GetSceneBounds(ozz::math::Box* _bound) const {
    ozz::sample::ComputeSkeletonBounds(skeleton_, transform_, _bound);
  }

 private:
  // Runtime skeleton.
  ozz::animation::Skeleton skeleton_;

  // Global blend ratio in range [0,1] that controls all blend parameters and
  // synchronizes playback speeds. A value of 0 gives full weight to the first
  // animation, and 1 to the last.
  float blend_ratio_ = .3f;

  // Switch to manual control of animations and blending parameters.
  bool manual_ = false;

  // The number of layers to blend.
  static constexpr size_t kNumLayers = 3;

  // Sampler structure contains all the data required to sample a single
  // animation.
  struct Sampler {
    // Playback animation controller.
    ozz::sample::PlaybackController controller;

    // Blending weight for the layer.
    float weight = 1.f;

    // Runtime animation.
    ozz::animation::Animation animation;

    // Sampling context.
    ozz::animation::SamplingJob::Context context;

    // Runtime motion track.
    ozz::sample::MotionTrack motion_track;

    // Motion sampling & accumulator.
    ozz::sample::MotionSampler motion_sampler;

    // Buffer of local transforms as sampled from animation_.
    ozz::vector<ozz::math::SoaTransform> locals;
  } samplers_[kNumLayers];  // kNumLayers animations to blend.

  // Buffer of local transforms which stores the blending result.
  ozz::vector<ozz::math::SoaTransform> locals_;

  // Buffer of model space matrices. These are computed by the local-to-model
  // job after the blending stage.
  ozz::vector<ozz::math::Float4x4> models_;

  // Uses a delta accumulator to accumulate the blended delta motion of the
  // frame.
  ozz::sample::MotionDeltaAccumulator accumulator_;

  // Rotation deformation, rad/s
  float angular_velocity_ = ozz::math::kPi_4;

  // Character transform.
  ozz::math::Float4x4 transform_;

  // UI options.

  // Show motion tracks.
  bool show_motion_ = true;

  // Show box at root transform
  bool show_box_ = true;
};

int main(int _argc, const char** _argv) {
  const char* title = "Ozz-animation sample: Motion blending";
  return MotionBlendSampleApplication().Run(_argc, _argv, "1.2", title);
}
