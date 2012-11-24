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
  animation,
  "Path to the animation (ozz archive format).",
  "media/animation.ozz",
  false)

class LoadDemoAplication : public ozz::demo::Application {
 public:
  LoadDemoAplication()
    : skeleton_(NULL),
      animation_(NULL),
      cache_(NULL),
      locals_(NULL),
      locals_end_(NULL),
      models_(NULL),
      models_end_(NULL) {
    // Set default application settings.
    set_auto_framing(true);
  }

 protected:
  // Updates current animation time.
  virtual bool OnUpdate(float _dt) {
    // Updates current animation time.
    controller_.Update(*animation_, _dt);

    // Samples optimized animation at t = animation_time_.
    ozz::animation::SamplingJob sampling_job;
    sampling_job.animation = animation_;
    sampling_job.cache = cache_;
    sampling_job.time = controller_.time();
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

    // Reading animation.
    {
      ozz::log::Out() << "Loading animation archive." << std::endl;
      ozz::io::File file(OPTIONS_animation, "rb");
      if (!file.opened()) {
        ozz::log::Err() << "Failed to open animation file " << OPTIONS_animation <<
          "." << std::endl;
        return false;
      }
      ozz::io::IArchive archive(&file);
      if (!archive.TestTag<ozz::animation::Animation>()) {
        ozz::log::Err() << "Failed to load animation instance from file " << OPTIONS_animation <<
          "." << std::endl;
        return false;
      }
      // Once the tag is validated, reading cannot fail.
      animation_ = allocator.New<ozz::animation::Animation>();
      archive >> *animation_;
    }

    // Allocates runtime buffers.
    const int num_joints = skeleton_->num_joints();

    ozz::animation::LocalsAlloc locals_alloc =
      ozz::animation::AllocateLocals(&allocator, num_joints);
    locals_ = locals_alloc.begin;
    locals_end_ = locals_alloc.end;

    ozz::animation::ModelsAlloc models_alloc =
      ozz::animation::AllocateModels(&allocator, num_joints);
    models_ = models_alloc.begin;
    models_end_ = models_alloc.end;

    // Allocates a cache that matches animation requirements.
    cache_ = allocator.New<ozz::animation::SamplingCache>(num_joints);

    return true;
  }

  virtual bool OnGui(ozz::demo::ImGui* _im_gui) {
    // Exposes animation runtime playback controls.
    {
      static bool open = true;
      ozz::demo::ImGui::OpenClose oc(_im_gui, "Animation control", &open);
      if (open) {
        controller_.OnGui(*animation_, _im_gui);
      }
    }
    return true;
  }

  virtual void OnDestroy() {
    ozz::memory::Allocator& allocator = ozz::memory::default_allocator();
    allocator.Delete(skeleton_);
    skeleton_ = NULL;
    allocator.Delete(animation_);
    animation_ = NULL;
    allocator.Deallocate(locals_);
    locals_end_ = locals_ = NULL;
    allocator.Deallocate(models_);
    models_end_ = models_ = NULL;
    allocator.Delete(cache_);
    cache_ = NULL;
  }

  virtual bool GetSceneBounds(ozz::math::Box* _bound) const {
    return ozz::demo::ComputePostureBounds(models_, models_end_, _bound);
  }

  // Gets the title to display.
  virtual const char* GetTitle() const {
    return "Skeleton and animation loader demo application.";
  }

 private:

  // Playback animation controller. This is a utility class that helps with
  // controlling animation playback time.
  ozz::demo::PlaybackController controller_;

  // Runtime skeleton.
  ozz::animation::Skeleton* skeleton_;

  // Runtime animation.
  ozz::animation::Animation* animation_;

  // Sampling cache.
  ozz::animation::SamplingCache* cache_;

  // Buffer of local transforms as sampled from animation_.
  ozz::math::SoaTransform* locals_;
  const ozz::math::SoaTransform* locals_end_;

  // Buffer of model space matrices.
  ozz::math::Float4x4* models_;
  const ozz::math::Float4x4* models_end_;
};

int main(int _argc, const char** _argv) {
  return LoadDemoAplication().Run(
    _argc, _argv,
    "1.0",
    "Loads a skeleton and an animation from ozz binary archives.\n"
    "Animation time and playback speed can be tweaked.\n");
}
