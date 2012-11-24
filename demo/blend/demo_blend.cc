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

#include "ozz/animation/skeleton_serialize.h"
#include "ozz/animation/animation_serialize.h"
#include "ozz/animation/blending_job.h"
#include "ozz/animation/sampling_job.h"
#include "ozz/animation/local_to_model_job.h"
#include "ozz/animation/utils.h"

#include "ozz/base/io/archive.h"
#include "ozz/base/io/stream.h"

#include "ozz/base/log.h"

#include "ozz/base/memory/allocator.h"
#include "ozz/base/maths/soa_transform.h"

#include "ozz/options/options.h"

#include "framework/application.h"
#include "framework/renderer.h"
#include "framework/imgui.h"
#include "framework/utils.h"

// Skeleton archive can be specified as an option.
OZZ_OPTIONS_DECLARE_STRING(
  skeleton,
  "Path to the skeleton (ozz archive format).",
  "media/skeleton.ozz",
  false)

// Animation archive can be specified as an option.
OZZ_OPTIONS_DECLARE_STRING(
  animation1,
  "Path to the first animation (ozz archive format).",
  "media/animation1.ozz",
  false)

// Animation archive can be specified as an option.
OZZ_OPTIONS_DECLARE_STRING(
  animation2,
  "Path to the second animation (ozz archive format).",
  "media/animation2.ozz",
  false)

class BlendDemoAplication : public ozz::demo::Application {
 public:
  BlendDemoAplication()
    : skeleton_(NULL),
      blended_locals_(NULL),
      blended_locals_end_(NULL),
      models_(NULL),
      models_end_(NULL) {
    // Initializes blending job parameters.
    weights_[0] = 1.f;
    weights_[1] = 1.f;
    thresold_ = ozz::animation::BlendingJob().thresold;

    // Set default application settings.
    set_auto_framing(true);
  }

 protected:
  // Updates current animation time.
  virtual bool OnUpdate(float _dt) {
    // Updates and samples both animations to their respective local space
    // transform buffers.
    for (int i = 0; i < 2; i++) {
      Sampler& sampler = samplers_[i];

      // Updates animations time.
      sampler.controller.Update(*sampler.animation, _dt);

      // Samples animation.
      ozz::animation::SamplingJob sampling_job;
      sampling_job.animation = sampler.animation;
      sampling_job.cache = sampler.cache;
      sampling_job.time = sampler.controller.time();
      sampling_job.output_begin = sampler.locals;
      sampling_job.output_end = sampler.locals_end;
      if (!sampling_job.Run()) {
        return false;
      }
    }

    // Blends the 2 animations.
    // Blends the local spaces transforms computed by sampling the 2 animations
    // (1st stage just above), and outputs the result to the local space
    // transform buffer blended_locals_

    // Prepares the 2 blending layers.
    ozz::animation::BlendingJob::Layer layers[2];
    layers[0].begin = samplers_[0].locals;
    layers[0].end = samplers_[0].locals_end;
    layers[0].weight = weights_[0];
    layers[1].begin = samplers_[1].locals;
    layers[1].end = samplers_[1].locals_end;
    layers[1].weight = weights_[1];

    // Setups the job.
    ozz::animation::BlendingJob blend_job;
    blend_job.thresold = thresold_;
    blend_job.layers_begin = layers;
    blend_job.layers_end = layers + 2;
    blend_job.bind_pose_begin = skeleton_->bind_pose();
    blend_job.bind_pose_end = skeleton_->bind_pose_end();
    blend_job.output_begin = blended_locals_;
    blend_job.output_end = blended_locals_end_;
    if (!blend_job.Run()) {
      return false;
    }

    // Converts from local space to model space matrices.
    // Gets the ouput of the blending stage, and converts it to model space.
    ozz::animation::LocalToModelJob ltm_job;
    ltm_job.skeleton = skeleton_;
    ltm_job.input_begin = blended_locals_;
    ltm_job.input_end = blended_locals_end_;
    ltm_job.output_begin = models_;
    ltm_job.output_end = models_end_;
    if (!ltm_job.Run()) {
      return false;
    }

    return true;
  }

  // Samples animation, transforms to model space and renders.
  virtual bool OnDisplay(ozz::demo::Renderer* _renderer) {
    return _renderer->DrawPosture(*skeleton_, models_, models_end_, true);
  }

  virtual bool OnInitialize() {
    ozz::memory::Allocator& allocator = ozz::memory::default_allocator();

    // Reading skeleton.
    {
      ozz::log::Out() << "Loading skeleton archive " << OPTIONS_skeleton <<
        "." << std::endl;
      ozz::io::File file(OPTIONS_skeleton, "rb");
      if (!file.opened()) {
        ozz::log::Err() << "Failed to open skeleton file " << OPTIONS_skeleton << "."
          << std::endl;
        return false;
      }
      ozz::io::IArchive archive(&file);
      if (!archive.TestTag<ozz::animation::Skeleton>()) {
        ozz::log::Err() << "Failed to load skeleton instance from file " <<
          OPTIONS_skeleton << "." << std::endl;
        return false;
      }
      // Once the tag is validated, reading cannot fail.
      skeleton_ = allocator.New<ozz::animation::Skeleton>();
      archive >> *skeleton_;
    }
    const int num_joints = skeleton_->num_joints();

    // Reading animations.
    for (int i = 0; i < 2; i++) {
      Sampler& sampler = samplers_[i];

      const char* filename = i == 0 ? OPTIONS_animation1 : OPTIONS_animation2;
      ozz::log::Out() << "Loading animation archive: " << filename <<
        "." << std::endl;
      ozz::io::File file(filename, "rb");
      if (!file.opened()) {
        ozz::log::Err() << "Failed to open animation file " << filename <<
          "." << std::endl;
        return false;
      }
      ozz::io::IArchive archive(&file);
      if (!archive.TestTag<ozz::animation::Animation>()) {
        ozz::log::Err() << "Failed to load animation instance from file " <<
          filename << "." << std::endl;
        return false;
      }

      // Once the tag is validated, reading cannot fail.
      sampler.animation = allocator.New<ozz::animation::Animation>();
      archive >> *sampler.animation;

      // Allocates sampler runtime buffers.
      ozz::animation::LocalsAlloc locals_alloc =
        ozz::animation::AllocateLocals(&allocator, num_joints);
      sampler.locals = locals_alloc.begin;
      sampler.locals_end = locals_alloc.end;

      // Allocates a cache that matches animation requirements.
      sampler.cache = allocator.New<ozz::animation::SamplingCache>(num_joints);
    }

    // Allocates local space runtime buffers of blended data.
    ozz::animation::LocalsAlloc blended_locals_alloc =
      ozz::animation::AllocateLocals(&allocator, num_joints);
    blended_locals_ = blended_locals_alloc.begin;
    blended_locals_end_ = blended_locals_alloc.end;

    // Allocates model space runtime buffers of blended data.
    ozz::animation::ModelsAlloc models_alloc =
      ozz::animation::AllocateModels(&allocator, num_joints);
    models_ = models_alloc.begin;
    models_end_ = models_alloc.end;

    return true;
  }

  virtual bool OnGui(ozz::demo::ImGui* _im_gui) {
    // Exposes animations runtime playback controls.
    {
      static bool open[2] = {true, true};
      const char* oc_names[2] = {"Animation control 1", "Animation control 2"};
      for (int i = 0; i < 2; i++) {
        Sampler& sampler = samplers_[i];
        ozz::demo::ImGui::OpenClose oc(_im_gui, oc_names[i], &open[i]);
        if (open[i]) {
          sampler.controller.OnGui(*sampler.animation, _im_gui);
        }
      }
    }

    // Exposes blending parameters.
    {
      static bool open = true;
      ozz::demo::ImGui::OpenClose oc(_im_gui, "Blending parameters", &open);
      char label[64];
      std::sprintf(label, "Blend weight 1: %.2f", weights_[0]);
      _im_gui->DoSlider(label, 0.f, 1.f, &weights_[0]);
      std::sprintf(label, "Blend weight 2: %.2f", weights_[1]);
      _im_gui->DoSlider(label, 0.f, 1.f, &weights_[1]);
      std::sprintf(label, "Thresold: %.2f", thresold_);
      _im_gui->DoSlider(label, .01f, 1.f, &thresold_);
    }
    return true;
  }

  virtual void OnDestroy() {
    ozz::memory::Allocator& allocator = ozz::memory::default_allocator();
    allocator.Delete(skeleton_);
    skeleton_ = NULL;
    for (int i = 0; i < 2; i++) {
      Sampler& sampler = samplers_[i];
      allocator.Delete(sampler.animation);
      sampler.animation = NULL;
      allocator.Deallocate(sampler.locals);
      sampler.locals_end = sampler.locals = NULL;
      allocator.Delete(sampler.cache);
      sampler.cache = NULL;
    }
    allocator.Deallocate(blended_locals_);
    blended_locals_end_ = blended_locals_ = NULL;
    allocator.Deallocate(models_);
    models_end_ = models_ = NULL;
  }

  virtual bool GetSceneBounds(ozz::math::Box* _bound) const {
    return ozz::demo::ComputePostureBounds(models_, models_end_, _bound);
  }

  // Gets the title to display.
  virtual const char* GetTitle() const {
    return "Animations blending demo application.";
  }

 private:

  // Runtime skeleton.
  ozz::animation::Skeleton* skeleton_;

  // Sampler structure contains all the data required to sample a single
  // animation.
  struct Sampler {
    // Constructor, default initialization.
    Sampler() 
     :  animation(NULL),
        cache(NULL),
        locals(NULL),
        locals_end(NULL) {
    }

    // Playback animation controller. This is a utility class that helps with
    // controlling animation playback time.
    ozz::demo::PlaybackController controller;

    // Runtime animation.
    ozz::animation::Animation* animation;

    // Sampling cache.
    ozz::animation::SamplingCache* cache;

    // Buffer of local transforms as sampled from animation_.
    ozz::math::SoaTransform* locals;
    const ozz::math::SoaTransform* locals_end;
  } samplers_[2];  // 2 animations to blend.

  // Blending weights of the 2 animations.
  float weights_[2];

  // Blending job bind pose thresold.
  float thresold_;

  // Buffer of local transforms which stores the blending result.
  ozz::math::SoaTransform* blended_locals_;
  const ozz::math::SoaTransform* blended_locals_end_;

  // Buffer of model space matrices. These are computed by the local-to-model
  // job after the blending stage.
  ozz::math::Float4x4* models_;
  const ozz::math::Float4x4* models_end_;
};

int main(int _argc, const char** _argv) {
  return BlendDemoAplication().Run(
    _argc, _argv,
    "1.0",
    "Loads and blends two animations.\n"
    "Blending weights and bind pose thresold can be tweaked.\n");
}
