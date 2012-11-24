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

#include "ozz/animation/skeleton.h"
#include "ozz/animation/animation.h"
#include "ozz/animation/sampling_job.h"
#include "ozz/animation/local_to_model_job.h"
#include "ozz/animation/utils.h"

#include "ozz/animation/offline/collada/collada.h"
#include "ozz/animation/offline/animation_builder.h"
#include "ozz/animation/offline/animation_optimizer.h"
#include "ozz/animation/offline/skeleton_builder.h"

#include "ozz/base/memory/allocator.h"
#include "ozz/base/maths/soa_transform.h"

#include "ozz/base/log.h"

#include "ozz/options/options.h"

#include "framework/application.h"
#include "framework/renderer.h"
#include "framework/imgui.h"
#include "framework/utils.h"

// Collada skeleton and animation file can be specified as an option.
OZZ_OPTIONS_DECLARE_STRING(
  skeleton,
  "Path to the Collada skeleton file.",
  "media/skeleton_yup.dae",
  false)

OZZ_OPTIONS_DECLARE_STRING(
  animation,
  "Path to the Collada animation file.",
  "media/animation.dae",
  false)

class ColladaDemoAplication : public ozz::demo::Application {
 public:
  ColladaDemoAplication()
    : selected_display_(eOptimized),
      skeleton_(NULL),
      cache_(NULL),
      animation_opt_(NULL),
      animation_non_opt_(NULL),
      locals_opt_(NULL),
      locals_opt_end_(NULL),
      locals_scratch_(NULL),
      locals_scratch_end_(NULL),
      models_(NULL),
      models_end_(NULL) {
    // Set default application settings.
    set_auto_framing(true);
  }

 protected:
  // Updates current animation time.
  virtual bool OnUpdate(float _dt) {
    // Updates current animation time.
    controller_.Update(*animation_opt_, _dt);

    // Samples optimized animation at t = animation_time_.
    ozz::animation::SamplingJob sampling_job;
    sampling_job.animation = animation_opt_;
    sampling_job.cache = cache_;
    sampling_job.time = controller_.time();
    sampling_job.output_begin = locals_opt_;
    sampling_job.output_end = locals_opt_end_;
    if (!sampling_job.Run()) {
      return false;
    }

    // Also samples non-optimized animation according to the display mode.
    if (selected_display_ != eOptimized) {
      // Shares the cache even if it's not optimal.
      sampling_job.animation = animation_non_opt_;
      sampling_job.output_begin = locals_scratch_;
      sampling_job.output_end = locals_scratch_end_;
      if (!sampling_job.Run()) {
        return false;
      }
    }

    // Computes difference between the optimized and non-optimized animations
    // in local space, and rebinds it to the bind pose.
    if (selected_display_ == eDifference) {
      ozz::math::SoaTransform* locals = locals_scratch_;
      const ozz::math::SoaTransform* locals_opt = locals_opt_;
      const ozz::math::SoaTransform* bind_pose = skeleton_->bind_pose();
      for (;
           locals < locals_scratch_end_;
           locals++, locals_opt++, bind_pose++) {
        assert(locals_opt < locals_opt_end_ &&
               bind_pose < skeleton_->bind_pose_end());

        // Computes difference.
        const ozz::math::SoaTransform diff = {
          locals_opt->translation - locals->translation,
          locals_opt->rotation * Conjugate(locals->rotation),
          locals_opt->scale - locals->scale};

        // Rebinds to the bind pose in the scratch buffer.
        locals->translation = bind_pose->translation + diff.translation;
        locals->rotation = bind_pose->rotation * diff.rotation;
        locals->scale = bind_pose->scale + diff.scale;
      }
    }

    // Converts from local space to model space matrices.
    ozz::animation::LocalToModelJob ltm_job;
    ltm_job.skeleton = skeleton_;
    ltm_job.input_begin =
      selected_display_ == eOptimized ? locals_opt_ : locals_scratch_;
    ltm_job.input_end =
      selected_display_ == eOptimized ? locals_opt_end_ : locals_scratch_end_;
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
    // Imports offline skeleton from a collada file.
    ozz::animation::offline::RawSkeleton raw_skeleton;
    if (!ozz::animation::offline::collada::ImportFromFile(
        OPTIONS_skeleton, &raw_skeleton)) {
      return false;
    }

    // Builds the runtime skeleton from the offline one.
    ozz::animation::offline::SkeletonBuilder skeleton_builder;
    skeleton_ = skeleton_builder(raw_skeleton);
    if (!skeleton_) {
      return false;
    }

    // Imports offline animation from a collada file.
    if (!ozz::animation::offline::collada::ImportFromFile(
        OPTIONS_animation, *skeleton_, &raw_animation_)) {
      return false;
    }

    // Builds the runtime animation from the raw one imported from Collada.
    if (!BuildAnimations()) {
      return false;
    }

    // Allocates runtime buffers.
    ozz::memory::Allocator& allocator = ozz::memory::default_allocator();
    const int num_joints = skeleton_->num_joints();

    ozz::animation::LocalsAlloc locals_opt_alloc =
      ozz::animation::AllocateLocals(&allocator, num_joints);
    locals_opt_ = locals_opt_alloc.begin;
    locals_opt_end_ = locals_opt_alloc.end;

    ozz::animation::LocalsAlloc locals_scratch_alloc =
      ozz::animation::AllocateLocals(&allocator, num_joints);
    locals_scratch_ = locals_scratch_alloc.begin;
    locals_scratch_end_ = locals_scratch_alloc.end;

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
      ozz::demo::ImGui::OpenClose occ(_im_gui, "Animation control", &open);
      if (open) {
        controller_.OnGui(*animation_opt_, _im_gui);
      }
    }

    // Exposes optimizer's tolerances.
    {
      static bool open_tol = true;
      ozz::demo::ImGui::OpenClose ocb(_im_gui, "Optimization tolerances", &open_tol);
      if (open_tol) {
        bool rebuild = false;
        char label[64];

        sprintf(label,
                "Translation : %0.2f cm",
                optimizer_.translation_tolerance * 100);
        rebuild |= _im_gui->DoSlider(
          label, 0.f, .1f, &optimizer_.translation_tolerance, .5f);

        sprintf(label,
                "Rotation : %0.2f degree",
                optimizer_.rotation_tolerance * 180.f / ozz::math::kPi);
        rebuild |= _im_gui->DoSlider(
          label, 0.f, 10.f * ozz::math::kPi / 180.f,
          &optimizer_.rotation_tolerance, .5f);

        sprintf(label,
                "Scale : %0.2f %%", optimizer_.scale_tolerance * 100.f);
        rebuild |= _im_gui->DoSlider(
          label, 0.f, .1f, &optimizer_.scale_tolerance, .5f);

        sprintf(label, "Animation size : %zuKB", animation_opt_->size()>>10);
        _im_gui->DoLabel(label, ozz::demo::ImGui::kLeft, true);

        if (rebuild) {
          // Delete current animation and rebuild one with the new tolerances.
          ozz::memory::default_allocator().Delete(animation_opt_);
          animation_opt_ = NULL;

          // Invalidates the cache in case the new animation has the same
          // address as the previous one. Other cases are automatic handled by
          // the cache. See SamplingCache::Invalidate for more details.
          cache_->Invalidate();

          // Rebuilds a new runtime animation.
          if (!BuildAnimations()) {
            return false;
          }
        }
      }

      // Selects display mode.
      static bool open_mode = true;
      ozz::demo::ImGui::OpenClose mode(
        _im_gui, "Display mode", &open_mode);
      if (open_mode) {
        _im_gui->DoRadioButton(eOptimized, "Optimized", &selected_display_);
        _im_gui->DoRadioButton(eNonOptimized, "Non-optimized", &selected_display_);
        _im_gui->DoRadioButton(eDifference, "Difference", &selected_display_);
      }
    }
    return true;
  }

  bool BuildAnimations() {
    assert(!animation_opt_);

    // Optimzes the raw animation.
    ozz::animation::offline::RawAnimation optimized_animation;
    if (!optimizer_(raw_animation_, &optimized_animation)) {
      return false;
    }

    // Builds the runtime animation from the offline one.
    ozz::animation::offline::AnimationBuilder animation_builder;
    animation_opt_ = animation_builder(optimized_animation);
    if (!animation_opt_) {
      return false;
    }

    // Builds the non-optimized animation if it's the first call.
    if (!animation_non_opt_) {
      animation_non_opt_ = animation_builder(raw_animation_);
      if (!animation_non_opt_) {
        return false;
      }
    }
    return true;
  }

  virtual void OnDestroy() {
    ozz::memory::Allocator& allocator = ozz::memory::default_allocator();
    allocator.Delete(skeleton_);
    skeleton_ = NULL;
    allocator.Delete(animation_opt_);
    animation_opt_ = NULL;
    allocator.Delete(animation_non_opt_);
    animation_non_opt_ = NULL;
    allocator.Deallocate(locals_opt_);
    locals_opt_end_ = locals_opt_ = NULL;
    allocator.Deallocate(locals_scratch_);
    locals_scratch_end_ = locals_scratch_ = NULL;
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
    return "Collada animation import demo application";
  }

 private:

  // Selects which animation is displayed.
  enum DisplayMode {
    eOptimized,
    eNonOptimized,
    eDifference,
  };
  int selected_display_;

  // Imported non-optimized animation.
  ozz::animation::offline::RawAnimation raw_animation_;

  // Stores the optimizer in order to expose its parameters.
  ozz::animation::offline::AnimationOptimizer optimizer_;

  // Playback animation controller. This is a utility class that helps with
  // controlling animation playback time.
  ozz::demo::PlaybackController controller_;

  // Runtime skeleton.
  ozz::animation::Skeleton* skeleton_;

  // Sampling cache, shared accros optimized and non-optimized animations. This
  // is not optimal, but it's not an issue either.
  ozz::animation::SamplingCache* cache_;

  // Runtime optimized animation.
  ozz::animation::Animation* animation_opt_;

  // Runtime non-optimized animation.
  ozz::animation::Animation* animation_non_opt_;

  // Buffer of local transforms as sampled from animation_opt_.
  ozz::math::SoaTransform* locals_opt_;
  const ozz::math::SoaTransform* locals_opt_end_;

  // Scratch (temporary) buffer of local transforms, used to store samples from
  // animation_non_opt_ and difference between optimized and non-optimized
  // animation samples.
  ozz::math::SoaTransform* locals_scratch_;
  const ozz::math::SoaTransform* locals_scratch_end_;

  // Buffer of model space matrices.
  ozz::math::Float4x4* models_;
  const ozz::math::Float4x4* models_end_;
};

int main(int _argc, const char** _argv) {
  return ColladaDemoAplication().Run(
    _argc, _argv,
    "1.0",
    "Imports a skeleton and an animation from a Collada document.\n"
    "Key-frame optimizations are performed based on tolerance settings exposed "
    "in application GUI.");
}
