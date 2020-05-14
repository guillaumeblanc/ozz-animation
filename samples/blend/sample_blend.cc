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
#include "ozz/animation/runtime/blending_job.h"
#include "ozz/animation/runtime/local_to_model_job.h"
#include "ozz/animation/runtime/sampling_job.h"
#include "ozz/animation/runtime/skeleton.h"

#include "ozz/base/log.h"

#include "ozz/base/maths/math_ex.h"
#include "ozz/base/maths/simd_math.h"
#include "ozz/base/maths/soa_transform.h"
#include "ozz/base/maths/vec_float.h"

#include "ozz/options/options.h"

#include "framework/application.h"
#include "framework/imgui.h"
#include "framework/renderer.h"
#include "framework/utils.h"

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
                           "Path to the second animation (ozz archive format).",
                           "media/animation3.ozz", false)

class BlendSampleApplication : public ozz::sample::Application {
 public:
  BlendSampleApplication()
      : blend_ratio_(.3f),
        manual_(false),
        threshold_(ozz::animation::BlendingJob().threshold) {}

 protected:
  // Updates current animation time and skeleton pose.
  virtual bool OnUpdate(float _dt, float) {
    // Updates blending parameters and synchronizes animations if control mode
    // is not manual.
    if (!manual_) {
      UpdateRuntimeParameters();
    }

    // Updates and samples all animations to their respective local space
    // transform buffers.
    for (int i = 0; i < kNumLayers; ++i) {
      Sampler& sampler = samplers_[i];

      // Updates animations time.
      sampler.controller.Update(sampler.animation, _dt);

      // Early out if this sampler weight makes it irrelevant during blending.
      if (samplers_[i].weight <= 0.f) {
        continue;
      }

      // Setup sampling job.
      ozz::animation::SamplingJob sampling_job;
      sampling_job.animation = &sampler.animation;
      sampling_job.cache = &sampler.cache;
      sampling_job.ratio = sampler.controller.time_ratio();
      sampling_job.output = make_span(sampler.locals);

      // Samples animation.
      if (!sampling_job.Run()) {
        return false;
      }
    }

    // Blends animations.
    // Blends the local spaces transforms computed by sampling all animations
    // (1st stage just above), and outputs the result to the local space
    // transform buffer blended_locals_

    // Prepares blending layers.
    ozz::animation::BlendingJob::Layer layers[kNumLayers];
    for (int i = 0; i < kNumLayers; ++i) {
      layers[i].transform = make_span(samplers_[i].locals);
      layers[i].weight = samplers_[i].weight;
    }

    // Setups blending job.
    ozz::animation::BlendingJob blend_job;
    blend_job.threshold = threshold_;
    blend_job.layers = layers;
    blend_job.bind_pose = skeleton_.joint_bind_poses();
    blend_job.output = make_span(blended_locals_);

    // Blends.
    if (!blend_job.Run()) {
      return false;
    }

    // Converts from local space to model space matrices.
    // Gets the output of the blending stage, and converts it to model space.

    // Setup local-to-model conversion job.
    ozz::animation::LocalToModelJob ltm_job;
    ltm_job.skeleton = &skeleton_;
    ltm_job.input = make_span(blended_locals_);
    ltm_job.output = make_span(models_);

    // Runs ltm job.
    if (!ltm_job.Run()) {
      return false;
    }

    return true;
  }

  // Computes blending weight and synchronizes playback speed when the "manual"
  // option is off.
  void UpdateRuntimeParameters() {
    // Computes weight parameters for all samplers.
    const float kNumIntervals = kNumLayers - 1;
    const float kInterval = 1.f / kNumIntervals;
    for (int i = 0; i < kNumLayers; ++i) {
      const float med = i * kInterval;
      const float x = blend_ratio_ - med;
      const float y = ((x < 0.f ? x : -x) + kInterval) * kNumIntervals;
      samplers_[i].weight = ozz::math::Max(0.f, y);
    }

    // Synchronizes animations.
    // First computes loop cycle duration. Selects the 2 samplers that define
    // interval that contains blend_ratio_.
    // Uses a maximum value smaller that 1.f (-epsilon) to ensure that
    // (relevant_sampler + 1) is always valid.
    const int relevant_sampler =
        static_cast<int>((blend_ratio_ - 1e-3f) * (kNumLayers - 1));
    assert(relevant_sampler + 1 < kNumLayers);
    Sampler& sampler_l = samplers_[relevant_sampler];
    Sampler& sampler_r = samplers_[relevant_sampler + 1];

    // Interpolates animation durations using their respective weights, to
    // find the loop cycle duration that matches blend_ratio_.
    const float loop_duration =
        sampler_l.animation.duration() * sampler_l.weight +
        sampler_r.animation.duration() * sampler_r.weight;

    // Finally finds the speed coefficient for all samplers.
    const float inv_loop_duration = 1.f / loop_duration;
    for (int i = 0; i < kNumLayers; ++i) {
      Sampler& sampler = samplers_[i];
      const float speed = sampler.animation.duration() * inv_loop_duration;
      sampler.controller.set_playback_speed(speed);
    }
  }

  // Samples animation, transforms to model space and renders.
  virtual bool OnDisplay(ozz::sample::Renderer* _renderer) {
    return _renderer->DrawPosture(skeleton_, make_span(models_),
                                  ozz::math::Float4x4::identity());
  }

  virtual bool OnInitialize() {
    // Reading skeleton.
    if (!ozz::sample::LoadSkeleton(OPTIONS_skeleton, &skeleton_)) {
      return false;
    }

    const int num_joints = skeleton_.num_joints();
    const int num_soa_joints = skeleton_.num_soa_joints();

    // Reading animations.
    const char* filenames[] = {OPTIONS_animation1, OPTIONS_animation2,
                               OPTIONS_animation3};
    static_assert(OZZ_ARRAY_SIZE(filenames) == kNumLayers, "Arrays mistmatch.");
    for (int i = 0; i < kNumLayers; ++i) {
      Sampler& sampler = samplers_[i];

      if (!ozz::sample::LoadAnimation(filenames[i], &sampler.animation)) {
        return false;
      }

      // Allocates sampler runtime buffers.
      sampler.locals.resize(num_soa_joints);

      // Allocates a cache that matches animation requirements.
      sampler.cache.Resize(num_joints);
    }

    // Allocates local space runtime buffers of blended data.
    blended_locals_.resize(num_soa_joints);

    // Allocates model space runtime buffers of blended data.
    models_.resize(num_joints);

    return true;
  }

  virtual void OnDestroy() {}

  virtual bool OnGui(ozz::sample::ImGui* _im_gui) {
    // Exposes blending parameters.
    {
      static bool open = true;
      ozz::sample::ImGui::OpenClose oc(_im_gui, "Blending parameters", &open);
      if (open) {
        if (_im_gui->DoCheckBox("Manual settings", &manual_) && !manual_) {
          // Check-box state was changed, reset parameters.
          for (int i = 0; i < kNumLayers; ++i) {
            Sampler& sampler = samplers_[i];
            sampler.controller.Reset();
          }
        }

        char label[64];
        std::sprintf(label, "Blend ratio: %.2f", blend_ratio_);
        _im_gui->DoSlider(label, 0.f, 1.f, &blend_ratio_, 1.f, !manual_);

        for (int i = 0; i < kNumLayers; ++i) {
          Sampler& sampler = samplers_[i];
          std::sprintf(label, "Weight %d: %.2f", i, sampler.weight);
          _im_gui->DoSlider(label, 0.f, 1.f, &sampler.weight, 1.f, manual_);
        }

        std::sprintf(label, "Threshold: %.2f", threshold_);
        _im_gui->DoSlider(label, .01f, 1.f, &threshold_);
      }
    }
    // Exposes animations runtime playback controls.
    {
      static bool oc_open = true;
      ozz::sample::ImGui::OpenClose oc(_im_gui, "Animation control", &oc_open);
      if (oc_open) {
        static bool open[] = {true, true, true};
        static_assert(OZZ_ARRAY_SIZE(open) == kNumLayers,
                      "Arrays size mismatch");
        const char* oc_names[] = {"Animation 1", "Animation 2", "Animation 3"};
        static_assert(OZZ_ARRAY_SIZE(oc_names) == kNumLayers,
                      "Arrays size mismatch");
        for (int i = 0; i < kNumLayers; ++i) {
          Sampler& sampler = samplers_[i];
          ozz::sample::ImGui::OpenClose loc(_im_gui, oc_names[i], nullptr);
          if (open[i]) {
            sampler.controller.OnGui(sampler.animation, _im_gui, manual_);
          }
        }
      }
    }
    return true;
  }

  virtual void GetSceneBounds(ozz::math::Box* _bound) const {
    ozz::sample::ComputePostureBounds(make_span(models_), _bound);
  }

 private:
  // Runtime skeleton.
  ozz::animation::Skeleton skeleton_;

  // Global blend ratio in range [0,1] that controls all blend parameters and
  // synchronizes playback speeds. A value of 0 gives full weight to the first
  // animation, and 1 to the last.
  float blend_ratio_;

  // Switch to manual control of animations and blending parameters.
  bool manual_;

  // The number of layers to blend.
  enum {
    kNumLayers = 3,
  };

  // Sampler structure contains all the data required to sample a single
  // animation.
  struct Sampler {
    // Constructor, default initialization.
    Sampler() : weight(1.f) {}

    // Playback animation controller. This is a utility class that helps with
    // controlling animation playback time.
    ozz::sample::PlaybackController controller;

    // Blending weight for the layer.
    float weight;

    // Runtime animation.
    ozz::animation::Animation animation;

    // Sampling cache.
    ozz::animation::SamplingCache cache;

    // Buffer of local transforms as sampled from animation_.
    ozz::vector<ozz::math::SoaTransform> locals;
  } samplers_[kNumLayers];  // kNumLayers animations to blend.

  // Blending job bind pose threshold.
  float threshold_;

  // Buffer of local transforms which stores the blending result.
  ozz::vector<ozz::math::SoaTransform> blended_locals_;

  // Buffer of model space matrices. These are computed by the local-to-model
  // job after the blending stage.
  ozz::vector<ozz::math::Float4x4> models_;
};

int main(int _argc, const char** _argv) {
  const char* title = "Ozz-animation sample: Animation blending";
  return BlendSampleApplication().Run(_argc, _argv, "1.2", title);
}
