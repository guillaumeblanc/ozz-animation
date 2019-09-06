//----------------------------------------------------------------------------//
//                                                                            //
// ozz-animation is hosted at http://github.com/guillaumeblanc/ozz-animation  //
// and distributed under the MIT License (MIT).                               //
//                                                                            //
// Copyright (c) 2019 Guillaume Blanc                                         //
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
#include "framework/mesh.h"
#include "framework/renderer.h"
#include "framework/utils.h"

#include <cstring>

// Skeleton archive can be specified as an option.
OZZ_OPTIONS_DECLARE_STRING(skeleton,
                           "Path to the skeleton (ozz archive format).",
                           "media/skeleton.ozz", false)

// MAin animation archive can be specified as an option.
OZZ_OPTIONS_DECLARE_STRING(animation,
                           "Path to the main animation(ozz archive format).",
                           "media/animation_base.ozz", false)

// Additive animation archive can be specified as an option.
OZZ_OPTIONS_DECLARE_STRING(
    additive_animation, "Path to the additive animation (ozz archive format).",
    "media/animation_additive.ozz", false)

// Mesh archive can be specified as an option.
OZZ_OPTIONS_DECLARE_STRING(mesh,
                           "Path to the skinned mesh (ozz archive format).",
                           "media/mesh.ozz", false)

class AdditiveBlendSampleApplication : public ozz::sample::Application {
 public:
  AdditiveBlendSampleApplication()
      : upper_body_root_(0),
        upper_body_mask_enable_(true),
        upper_body_joint_weight_setting_(1.f),
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
      sampling_job.output = make_range(sampler.locals);

      // Samples animation.
      if (!sampling_job.Run()) {
        return false;
      }
    }

    // Blends animations.
    // Blends the local spaces transforms computed by sampling all animations
    // (1st stage just above), and outputs the result to the local space
    // transform buffer blended_locals_

    // Prepares standard blending layers.
    ozz::animation::BlendingJob::Layer layers[1];
    layers[0].transform = make_range(samplers_[kMainAnimation].locals);
    layers[0].weight = samplers_[kMainAnimation].weight_setting;

    // Prepares additive blending layers.
    ozz::animation::BlendingJob::Layer additive_layers[1];
    additive_layers[0].transform =
        make_range(samplers_[kAdditiveAnimation].locals);
    additive_layers[0].weight = samplers_[kAdditiveAnimation].weight_setting;

    // Set per-joint weights for the additive blended layer.
    if (upper_body_mask_enable_) {
      additive_layers[0].joint_weights = make_range(upper_body_joint_weights_);
    }

    // Setups blending job.
    ozz::animation::BlendingJob blend_job;
    blend_job.threshold = threshold_;
    blend_job.layers = layers;
    blend_job.additive_layers = additive_layers;
    blend_job.bind_pose = skeleton_.joint_bind_poses();
    blend_job.output = make_range(blended_locals_);

    // Blends.
    if (!blend_job.Run()) {
      return false;
    }

    // Gets the output of the blending stage, and converts it to model space.

    // Setup local-to-model conversion job.
    ozz::animation::LocalToModelJob ltm_job;
    ltm_job.skeleton = &skeleton_;
    ltm_job.input = make_range(blended_locals_);
    ltm_job.output = make_range(models_);

    // Run ltm job.
    if (!ltm_job.Run()) {
      return false;
    }

    return true;
  }

  // Samples animation, transforms to model space and renders.
  virtual bool OnDisplay(ozz::sample::Renderer* _renderer) {
    // Builds skinning matrices, based on the output of the animation stage.
    // The mesh might not use (aka be skinned by) all skeleton joints. We use
    // the joint remapping table (available from the mesh object) to reorder
    // model-space matrices and build skinning ones.
    for (size_t i = 0; i < mesh_.joint_remaps.size(); ++i) {
      skinning_matrices_[i] =
          models_[mesh_.joint_remaps[i]] * mesh_.inverse_bind_poses[i];
    }

    // Renders skin.
    return _renderer->DrawSkinnedMesh(mesh_, make_range(skinning_matrices_),
                                      ozz::math::Float4x4::identity(),
                                      render_options_);
  }

  virtual bool OnInitialize() {
    // Reading skeleton.
    if (!ozz::sample::LoadSkeleton(OPTIONS_skeleton, &skeleton_)) {
      return false;
    }
    const int num_soa_joints = skeleton_.num_soa_joints();
    const int num_joints = skeleton_.num_joints();

    // Reading skinned mesh.
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

    // Reading animations.
    const char* filenames[] = {OPTIONS_animation, OPTIONS_additive_animation};
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

    // Default weight settings.
    samplers_[kMainAnimation].weight_setting = 1.f;

    upper_body_joint_weight_setting_ = 1.f;
    samplers_[kAdditiveAnimation].weight_setting = 1.f;

    // Allocates local space runtime buffers of blended data.
    blended_locals_.resize(num_soa_joints);

    // Allocates model space runtime buffers of blended data.
    models_.resize(num_joints);
    skinning_matrices_.resize(num_joints);

    // Allocates per-joint weights used for the partial additive animation.
    // Note that this is a Soa structure.
    upper_body_joint_weights_.resize(num_soa_joints);

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
    WeightSetupIterator(ozz::Vector<ozz::math::SimdFloat4>::Std* _weights,
                        float _weight_setting)
        : weights(_weights), weight_setting(_weight_setting) {}
    void operator()(int _joint, int) {
      ozz::math::SimdFloat4& soa_weight = weights->at(_joint / 4);
      soa_weight = ozz::math::SetI(
          soa_weight, ozz::math::simd_float4::Load1(weight_setting),
          _joint % 4);
    }
    ozz::Vector<ozz::math::SimdFloat4>::Std* weights;
    float weight_setting;
  };

  void SetupPerJointWeights() {
    // Setup partial animation mask. This mask is defined by a weight_setting
    // assigned to each joint of the hierarchy. Joint to disable are set to a
    // weight_setting of 0.f, and enabled joints are set to 1.f.

    // Disables all joints: set all weights to 0.
    for (int i = 0; i < skeleton_.num_soa_joints(); ++i) {
      upper_body_joint_weights_[i] = ozz::math::simd_float4::zero();
    }

    // Extracts the list of children of the shoulder
    WeightSetupIterator it(&upper_body_joint_weights_,
                           upper_body_joint_weight_setting_);
    IterateJointsDF(skeleton_, it, upper_body_root_);
  }

  virtual void OnDestroy() {}

  virtual bool OnGui(ozz::sample::ImGui* _im_gui) {
    char label[64];

    // Exposes blending parameters.
    {
      static bool open = true;
      ozz::sample::ImGui::OpenClose oc(_im_gui, "Blending parameters", &open);
      if (open) {
        _im_gui->DoLabel("Main layer:");
        std::sprintf(label, "Layer weight: %.2f",
                     samplers_[kMainAnimation].weight_setting);
        _im_gui->DoSlider(label, 0.f, 1.f,
                          &samplers_[kMainAnimation].weight_setting, 1.f);
        _im_gui->DoLabel("Additive layer:");
        std::sprintf(label, "Layer weight: %.2f",
                     samplers_[kAdditiveAnimation].weight_setting);
        _im_gui->DoSlider(label, -1.f, 1.f,
                          &samplers_[kAdditiveAnimation].weight_setting, 1.f);
        _im_gui->DoLabel("Global settings:");
        std::sprintf(label, "Threshold: %.2f", threshold_);
        _im_gui->DoSlider(label, .01f, 1.f, &threshold_);
      }
    }
    // Exposes selection of the root of the partial blending hierarchy.
    {
      static bool open = true;
      ozz::sample::ImGui::OpenClose oc(_im_gui, "Upper body masking", &open);

      if (open) {
        bool rebuild_joint_weights = false;
        rebuild_joint_weights |=
            _im_gui->DoCheckBox("Enable mask", &upper_body_mask_enable_);

        std::sprintf(label, "Joints weight: %.2f",
                     upper_body_joint_weight_setting_);
        rebuild_joint_weights |= _im_gui->DoSlider(
            label, 0.f, 1.f, &upper_body_joint_weight_setting_, 1.f,
            upper_body_mask_enable_);

        if (skeleton_.num_joints() != 0) {
          _im_gui->DoLabel("Root of the upper body hierarchy:",
                           ozz::sample::ImGui::kLeft, false);
          std::sprintf(label, "%s (%d)",
                       skeleton_.joint_names()[upper_body_root_],
                       upper_body_root_);

          rebuild_joint_weights |= _im_gui->DoSlider(
              label, 0, skeleton_.num_joints() - 1, &upper_body_root_, 1.f,
              upper_body_mask_enable_);
        }

        // Rebuilds per-joint weights if something has changed.
        if (rebuild_joint_weights) {
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
        const char* oc_names[kNumLayers] = {"Main animation",
                                            "Additive animation"};
        for (int i = 0; i < kNumLayers; ++i) {
          Sampler& sampler = samplers_[i];
          ozz::sample::ImGui::OpenClose loc(_im_gui, oc_names[i], NULL);
          if (open[i]) {
            sampler.controller.OnGui(sampler.animation, _im_gui);
          }
        }
      }
    }
    // Expose mesh rendering options
    {
      static bool oc_open = false;
      ozz::sample::ImGui::OpenClose oc(_im_gui, "Rendering options", &oc_open);
      if (oc_open) {
        _im_gui->DoCheckBox("Show texture", &render_options_.texture);
        _im_gui->DoCheckBox("Show normals", &render_options_.normals);
        _im_gui->DoCheckBox("Show tangents", &render_options_.tangents);
        _im_gui->DoCheckBox("Show binormals", &render_options_.binormals);
        _im_gui->DoCheckBox("Show colors", &render_options_.colors);
        _im_gui->DoCheckBox("Skip skinning", &render_options_.skip_skinning);
      }
    }

    return true;
  }

  virtual void GetSceneBounds(ozz::math::Box* _bound) const {
    ozz::sample::ComputePostureBounds(make_range(models_), _bound);
  }

 private:
  // Runtime skeleton.
  ozz::animation::Skeleton skeleton_;

  // The number of layers to blend.
  enum {
    kMainAnimation = 0,
    kAdditiveAnimation = 1,
    kNumLayers = 2,
  };

  // Sampler structure contains all the data required to sample a single
  // animation.
  struct Sampler {
    // Constructor, default initialization.
    Sampler() : weight_setting(1.f) {}

    // Playback animation controller. This is a utility class that helps with
    // controlling animation playback time.
    ozz::sample::PlaybackController controller;

    // Blending weight_setting for the layer.
    float weight_setting;

    // Runtime animation.
    ozz::animation::Animation animation;

    // Sampling cache.
    ozz::animation::SamplingCache cache;

    // Buffer of local transforms as sampled from animation_.
    ozz::Vector<ozz::math::SoaTransform>::Std locals;

  } samplers_[kNumLayers];  // kNumLayers animations to blend.

  // Index of the joint at the base of the upper body hierarchy.
  int upper_body_root_;

  // Enables upper boddy per-joint weights.
  bool upper_body_mask_enable_;

  // Blending weight_setting setting of the joints of this layer that are
  // affected
  // by the masking.
  float upper_body_joint_weight_setting_;

  // Per-joint weights used to define the partial animation mask. Allows to
  // select which joints are considered during blending, and their individual
  // weight_setting.
  ozz::Vector<ozz::math::SimdFloat4>::Std upper_body_joint_weights_;

  // Blending job bind pose threshold.
  float threshold_;

  // Buffer of local transforms which stores the blending result.
  ozz::Vector<ozz::math::SoaTransform>::Std blended_locals_;

  // Buffer of model space matrices. These are computed by the local-to-model
  // job after the blending stage.
  ozz::Vector<ozz::math::Float4x4>::Std models_;

  // Buffer of skinning matrices, result of the joint multiplication of the
  // inverse bind pose with the model space matrix.
  ozz::Vector<ozz::math::Float4x4>::Std skinning_matrices_;

  // The mesh used by the sample.
  ozz::sample::Mesh mesh_;

  // Mesh rendering options.
  ozz::sample::Renderer::Options render_options_;
};

int main(int _argc, const char** _argv) {
  const char* title = "Ozz-animation sample: Additive animations blending";
  return AdditiveBlendSampleApplication().Run(_argc, _argv, "1.0", title);
}
