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
#include "ozz/animation/runtime/skeleton_utils.h"

#include "ozz/base/log.h"

#include "ozz/base/containers/vector.h"

#include "ozz/base/maths/simd_math.h"
#include "ozz/base/maths/soa_transform.h"
#include "ozz/base/maths/vec_float.h"

#include "ozz/options/options.h"

#include "framework/application.h"
#include "framework/imgui.h"
#include "framework/renderer.h"
#include "framework/utils.h"

#include <cstring>

// Skeleton archive can be specified as an option.
OZZ_OPTIONS_DECLARE_STRING(skeleton,
                           "Path to the skeleton (ozz archive format).",
                           "media/skeleton.ozz", false)

// Lower body animation archive can be specified as an option.
OZZ_OPTIONS_DECLARE_STRING(
    lower_body_animation,
    "Path to the lower body animation(ozz archive format).",
    "media/animation_base.ozz", false)

// Upper body animation archive can be specified as an option.
OZZ_OPTIONS_DECLARE_STRING(
    upper_body_animation,
    "Path to the upper body animation (ozz archive format).",
    "media/animation_partial.ozz", false)

class PartialBlendSampleApplication : public ozz::sample::Application {
 public:
  PartialBlendSampleApplication()
      : upper_body_root_(0),
        threshold_(ozz::animation::BlendingJob().threshold) {}

 protected:
  // Updates current animation time and skeleton pose.
  virtual bool OnUpdate(float _dt, float) {
    // Updates and samples both animations to their respective local space
    // transform buffers.
    for (int i = 0; i < kNumLayers; ++i) {
      Sampler& sampler = samplers_[i];

      // Updates animations time.
      sampler.controller.Update(sampler.animation, _dt);

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
      layers[i].weight = samplers_[i].weight_setting;

      // Set per-joint weights for the partially blended layer.
      layers[i].joint_weights = make_span(samplers_[i].joint_weights);
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

    // Run ltm job.
    if (!ltm_job.Run()) {
      return false;
    }

    return true;
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
    const char* filenames[] = {OPTIONS_lower_body_animation,
                               OPTIONS_upper_body_animation};
    for (int i = 0; i < kNumLayers; ++i) {
      Sampler& sampler = samplers_[i];

      if (!ozz::sample::LoadAnimation(filenames[i], &sampler.animation)) {
        return false;
      }

      // Allocates sampler runtime buffers.
      sampler.locals.resize(num_soa_joints);

      // Allocates per-joint weights used for the partial animation. Note that
      // this is a Soa structure.
      sampler.joint_weights.resize(num_soa_joints);

      // Allocates a cache that matches animation requirements.
      sampler.cache.Resize(num_joints);
    }

    // Default weight settings.
    Sampler& lower_body_sampler = samplers_[kLowerBody];
    lower_body_sampler.weight_setting = 1.f;
    lower_body_sampler.joint_weight_setting = 0.f;

    Sampler& upper_body_sampler = samplers_[kUpperBody];
    upper_body_sampler.weight_setting = 1.f;
    upper_body_sampler.joint_weight_setting = 1.f;

    // Allocates local space runtime buffers of blended data.
    blended_locals_.resize(num_soa_joints);

    // Allocates model space runtime buffers of blended data.
    models_.resize(num_joints);

    // Finds the "Spine1" joint in the joint hierarchy.
    for (int i = 0; i < num_joints; ++i) {
      if (std::strstr(skeleton_.joint_names()[i], "Spine1")) {
        upper_body_root_ = i;
        break;
      }
    }
    SetupPerJointWeights();

    return true;
  }

  // Helper functor used to set weights while traversing joints hierarchy.
  struct WeightSetupIterator {
    WeightSetupIterator(ozz::vector<ozz::math::SimdFloat4>* _weights,
                        float _weight_setting)
        : weights(_weights), weight_setting(_weight_setting) {}
    void operator()(int _joint, int) {
      ozz::math::SimdFloat4& soa_weight = weights->at(_joint / 4);
      soa_weight = ozz::math::SetI(
          soa_weight, ozz::math::simd_float4::Load1(weight_setting),
          _joint % 4);
    }
    ozz::vector<ozz::math::SimdFloat4>* weights;
    float weight_setting;
  };

  void SetupPerJointWeights() {
    // Setup partial animation mask. This mask is defined by a weight_setting
    // assigned to each joint of the hierarchy. Joint to disable are set to a
    // weight_setting of 0.f, and enabled joints are set to 1.f.
    // Per-joint weights of lower and upper body layers have opposed values
    // (weight_setting and 1 - weight_setting) in order for a layer to select
    // joints that are rejected by the other layer.
    Sampler& lower_body_sampler = samplers_[kLowerBody];
    Sampler& upper_body_sampler = samplers_[kUpperBody];

    // Disables all joints: set all weights to 0.
    for (int i = 0; i < skeleton_.num_soa_joints(); ++i) {
      lower_body_sampler.joint_weights[i] = ozz::math::simd_float4::one();
      upper_body_sampler.joint_weights[i] = ozz::math::simd_float4::zero();
    }

    // Sets the weight_setting of all the joints children of the lower and upper
    // body weights. Note that they are stored in SoA format.
    WeightSetupIterator lower_it(&lower_body_sampler.joint_weights,
                                 lower_body_sampler.joint_weight_setting);
    ozz::animation::IterateJointsDF(skeleton_, lower_it, upper_body_root_);

    WeightSetupIterator upper_it(&upper_body_sampler.joint_weights,
                                 upper_body_sampler.joint_weight_setting);
    ozz::animation::IterateJointsDF(skeleton_, upper_it, upper_body_root_);
  }

  virtual void OnDestroy() {}

  virtual bool OnGui(ozz::sample::ImGui* _im_gui) {
    // Exposes blending parameters.
    {
      static bool open = true;
      ozz::sample::ImGui::OpenClose oc(_im_gui, "Blending parameters", &open);
      if (open) {
        char label[64];

        static bool automatic = true;
        _im_gui->DoCheckBox("Use automatic blending settings", &automatic);

        static float coeff = 1.f;  // All power to the partial animation.
        std::sprintf(label, "Upper body weight: %.2f", coeff);
        _im_gui->DoSlider(label, 0.f, 1.f, &coeff, 1.f, automatic);

        Sampler& lower_body_sampler = samplers_[kLowerBody];
        Sampler& upper_body_sampler = samplers_[kUpperBody];

        if (automatic) {
          // Blending values are forced when "automatic" mode is selected.
          lower_body_sampler.weight_setting = 1.f;
          lower_body_sampler.joint_weight_setting = 1.f - coeff;
          upper_body_sampler.weight_setting = 1.f;
          upper_body_sampler.joint_weight_setting = coeff;
        }

        _im_gui->DoLabel("Manual settings:");
        _im_gui->DoLabel("Lower body layer:");
        std::sprintf(label, "Layer weight: %.2f",
                     lower_body_sampler.weight_setting);
        _im_gui->DoSlider(label, 0.f, 1.f, &lower_body_sampler.weight_setting,
                          1.f, !automatic);
        std::sprintf(label, "Joints weight: %.2f",
                     lower_body_sampler.joint_weight_setting);
        _im_gui->DoSlider(label, 0.f, 1.f,
                          &lower_body_sampler.joint_weight_setting, 1.f,
                          !automatic);
        _im_gui->DoLabel("Upper body layer:");
        std::sprintf(label, "Layer weight: %.2f",
                     upper_body_sampler.weight_setting);
        _im_gui->DoSlider(label, 0.f, 1.f, &upper_body_sampler.weight_setting,
                          1.f, !automatic);
        std::sprintf(label, "Joints weight: %.2f",
                     upper_body_sampler.joint_weight_setting);
        _im_gui->DoSlider(label, 0.f, 1.f,
                          &upper_body_sampler.joint_weight_setting, 1.f,
                          !automatic);
        _im_gui->DoLabel("Global settings:");
        std::sprintf(label, "Threshold: %.2f", threshold_);
        _im_gui->DoSlider(label, .01f, 1.f, &threshold_);

        SetupPerJointWeights();
      }
    }
    // Exposes selection of the root of the partial blending hierarchy.
    {
      static bool open = true;
      ozz::sample::ImGui::OpenClose oc(_im_gui, "Root", &open);
      if (open && skeleton_.num_joints() != 0) {
        _im_gui->DoLabel("Root of the upper body hierarchy:",
                         ozz::sample::ImGui::kLeft, false);
        char label[64];
        std::sprintf(label, "%s (%d)",
                     skeleton_.joint_names()[upper_body_root_],
                     upper_body_root_);
        if (_im_gui->DoSlider(label, 0, skeleton_.num_joints() - 1,
                              &upper_body_root_)) {
          SetupPerJointWeights();
        }
      }
    }
    // Exposes animations runtime playback controls.
    {
      static bool oc_open = true;
      ozz::sample::ImGui::OpenClose oc(_im_gui, "Animation control", &oc_open);
      if (oc_open) {
        static bool open[kNumLayers] = {true, true};
        const char* oc_names[kNumLayers] = {"Lower body animation",
                                            "Upper body animation"};
        for (int i = 0; i < kNumLayers; ++i) {
          Sampler& sampler = samplers_[i];
          ozz::sample::ImGui::OpenClose loc(_im_gui, oc_names[i], nullptr);
          if (open[i]) {
            sampler.controller.OnGui(sampler.animation, _im_gui);
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

  // The number of layers to blend.
  enum {
    kLowerBody = 0,
    kUpperBody = 1,
    kNumLayers = 2,
  };

  // Sampler structure contains all the data required to sample a single
  // animation.
  struct Sampler {
    // Constructor, default initialization.
    Sampler() : weight_setting(1.f), joint_weight_setting(1.f) {}

    // Playback animation controller. This is a utility class that helps with
    // controlling animation playback time.
    ozz::sample::PlaybackController controller;

    // Blending weight_setting for the layer.
    float weight_setting;

    // Blending weight_setting setting of the joints of this layer that are
    // affected
    // by the masking.
    float joint_weight_setting;

    // Runtime animation.
    ozz::animation::Animation animation;

    // Sampling cache.
    ozz::animation::SamplingCache cache;

    // Buffer of local transforms as sampled from animation_.
    ozz::vector<ozz::math::SoaTransform> locals;

    // Per-joint weights used to define the partial animation mask. Allows to
    // select which joints are considered during blending, and their individual
    // weight_setting.
    ozz::vector<ozz::math::SimdFloat4> joint_weights;
  } samplers_[kNumLayers];  // kNumLayers animations to blend.

  // Index of the joint at the base of the upper body hierarchy.
  int upper_body_root_;

  // Blending job bind pose threshold.
  float threshold_;

  // Buffer of local transforms which stores the blending result.
  ozz::vector<ozz::math::SoaTransform> blended_locals_;

  // Buffer of model space matrices. These are computed by the local-to-model
  // job after the blending stage.
  ozz::vector<ozz::math::Float4x4> models_;
};

int main(int _argc, const char** _argv) {
  const char* title = "Ozz-animation sample: Partial animations blending";
  return PartialBlendSampleApplication().Run(_argc, _argv, "1.0", title);
}
