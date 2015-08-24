//----------------------------------------------------------------------------//
//                                                                            //
// ozz-animation is hosted at http://github.com/guillaumeblanc/ozz-animation  //
// and distributed under the MIT License (MIT).                               //
//                                                                            //
// Copyright (c) 2015 Guillaume Blanc                                         //
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

#include "ozz/animation/runtime/skeleton.h"
#include "ozz/animation/runtime/animation.h"
#include "ozz/animation/runtime/sampling_job.h"
#include "ozz/animation/runtime/local_to_model_job.h"

#include "ozz/animation/offline/animation_builder.h"
#include "ozz/animation/offline/animation_optimizer.h"
#include "ozz/animation/offline/raw_animation.h"
#include "ozz/animation/offline/raw_animation.h"

#include "ozz/base/memory/allocator.h"

#include "ozz/base/io/archive.h"
#include "ozz/base/io/stream.h"
#include "ozz/base/log.h"

#include "ozz/base/maths/vec_float.h"
#include "ozz/base/maths/simd_math.h"
#include "ozz/base/maths/soa_transform.h"

#include "ozz/options/options.h"

#include "framework/application.h"
#include "framework/renderer.h"
#include "framework/imgui.h"
#include "framework/utils.h"

// Collada skeleton and animation file can be specified as an option.
OZZ_OPTIONS_DECLARE_STRING(
  skeleton,
  "Path to the runtime skeleton file.",
  "media/skeleton.ozz",
  false)

OZZ_OPTIONS_DECLARE_STRING(
  animation,
  "Path to the raw animation file.",
  "media/raw_animation.ozz",
  false)

namespace {
bool LoadAnimation(const char* _filename,
                   ozz::animation::offline::RawAnimation* _animation) {
  assert(_filename && _animation);
  ozz::log::Out() << "Loading raw animation archive: " << _filename <<
    "." << std::endl;
  ozz::io::File file(_filename, "rb");
  if (!file.opened()) {
    ozz::log::Err() << "Failed to open animation file " << _filename <<
      "." << std::endl;
    return false;
  }
  ozz::io::IArchive archive(&file);
  if (!archive.TestTag<ozz::animation::offline::RawAnimation>()) {
    ozz::log::Err() << "Failed to load rawvanimation instance from file " <<
      _filename << "." << std::endl;
    return false;
  }

  // Once the tag is validated, reading cannot fail.
  archive >> *_animation;

  return true;
}
}  // namespace

class OptimizeSampleApplication : public ozz::sample::Application {
 public:
  OptimizeSampleApplication()
    : selected_display_(eOptimized),
      optimize_(true),
      cache_(NULL),
      animation_opt_(NULL),
      animation_non_opt_(NULL) {
  }

 protected:
  // Updates current animation time.
  virtual bool OnUpdate(float _dt) {
    // Updates current animation time.
    controller_.Update(*animation_opt_, _dt);

    // Prepares sampling job.
    ozz::animation::SamplingJob sampling_job;
    sampling_job.cache = cache_;
    sampling_job.time = controller_.time();
    
    // Samples optimized animation (_according to the display mode).
    if (selected_display_ != eNonOptimized) {
      sampling_job.animation = animation_opt_;
      sampling_job.output = locals_opt_;
      if (!sampling_job.Run()) {
        return false;
      }
    }

    // Also samples non-optimized animation (according to the display mode).
    if (selected_display_ != eOptimized) {
      // Shares the cache even if it's not optimal.
      sampling_job.animation = animation_non_opt_;
      sampling_job.output = locals_scratch_;
      if (!sampling_job.Run()) {
        return false;
      }
    }

    // Computes difference between the optimized and non-optimized animations
    // in local space, and rebinds it to the bind pose.
    if (selected_display_ == eDifference) {
      ozz::math::SoaTransform* locals = locals_scratch_.begin;
      const ozz::math::SoaTransform* locals_opt = locals_opt_.begin;
      ozz::Range<const ozz::math::SoaTransform> bind_poses =
        skeleton_.bind_pose();
      const ozz::math::SoaTransform* bind_pose = bind_poses.begin;
      for (;
           locals < locals_scratch_.end;
           ++locals, ++locals_opt, ++bind_pose) {
        assert(locals_opt < locals_opt_.end && bind_pose < bind_poses.end);

        // Computes difference.
        const ozz::math::SoaTransform diff = {
          locals_opt->translation - locals->translation,
          locals_opt->rotation * Conjugate(locals->rotation),
          locals_opt->scale / locals->scale
        };

        // Rebinds to the bind pose in the scratch buffer.
        locals->translation = bind_pose->translation + diff.translation;
        locals->rotation = bind_pose->rotation * diff.rotation;
        locals->scale = bind_pose->scale * diff.scale;
      }
    }

    // Converts from local space to model space matrices.
    ozz::animation::LocalToModelJob ltm_job;
    ltm_job.skeleton = &skeleton_;
    ltm_job.input =
      selected_display_ == eOptimized ? locals_opt_ : locals_scratch_;
    ltm_job.output = models_;
    if (!ltm_job.Run()) {
      return false;
    }

    return true;
  }

  // Samples animation, transforms to model space and renders.
  virtual bool OnDisplay(ozz::sample::Renderer* _renderer) {
    return _renderer->
      DrawPosture(skeleton_, models_, ozz::math::Float4x4::identity());
  }

  virtual bool OnInitialize() {
    // Imports offline skeleton from a binary file.
    if (!ozz::sample::LoadSkeleton(OPTIONS_skeleton, &skeleton_)) {
      return false;
    }

    // Imports offline animation from a binary file.
    if (!LoadAnimation(OPTIONS_animation, &raw_animation_)) {
      return false;
    }

    // Builds the runtime animation from the raw one imported from Collada.
    if (!BuildAnimations()) {
      return false;
    }

    // Allocates runtime buffers.
    ozz::memory::Allocator* allocator = ozz::memory::default_allocator();
    const int num_joints = skeleton_.num_joints();
    const int num_soa_joints = skeleton_.num_soa_joints();

    locals_opt_ =
      allocator->AllocateRange<ozz::math::SoaTransform>(num_soa_joints);
    locals_scratch_ =
      allocator->AllocateRange<ozz::math::SoaTransform>(num_soa_joints);
    models_ = allocator->AllocateRange<ozz::math::Float4x4>(num_joints);

    // Allocates a cache that matches animation requirements.
    cache_ = allocator->New<ozz::animation::SamplingCache>(num_joints);

    return true;
  }

  virtual bool OnGui(ozz::sample::ImGui* _im_gui) {
    // Exposes animation runtime playback controls.
    {
      static bool open = true;
      ozz::sample::ImGui::OpenClose occ(_im_gui, "Animation control", &open);
      if (open) {
        controller_.OnGui(*animation_opt_, _im_gui);
      }
    }

    // Exposes optimizer's tolerances.
    {
      static bool open_tol = true;
      ozz::sample::ImGui::OpenClose ocb(_im_gui, "Optimization tolerances", &open_tol);
      if (open_tol) {
        bool rebuild = false;
        char label[64];

        rebuild |= _im_gui->DoCheckBox("Enable optimzations", &optimize_);

        std::sprintf(label,
                     "Translation : %0.2f cm",
                     optimizer_.translation_tolerance * 100);
        rebuild |= _im_gui->DoSlider(
          label, 0.f, .1f, &optimizer_.translation_tolerance, .5f, optimize_);

        std::sprintf(label,
                     "Rotation : %0.2f degree",
                     optimizer_.rotation_tolerance * 180.f / ozz::math::kPi);

        rebuild |= _im_gui->DoSlider(
          label, 0.f, 10.f * ozz::math::kPi / 180.f,
          &optimizer_.rotation_tolerance, .5f, optimize_);

        std::sprintf(label,
                     "Scale : %0.2f %%", optimizer_.scale_tolerance * 100.f);
        rebuild |= _im_gui->DoSlider(
          label, 0.f, .1f, &optimizer_.scale_tolerance, .5f, optimize_);

        std::sprintf(label, "Animation size : %dKB",
          static_cast<int>(animation_opt_->size()>>10));

        _im_gui->DoLabel(label);

        if (rebuild) {
          // Delete current animation and rebuild one with the new tolerances.
          ozz::memory::default_allocator()->Delete(animation_opt_);
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
      ozz::sample::ImGui::OpenClose mode(
        _im_gui, "Display mode", &open_mode);
      if (open_mode) {
        _im_gui->DoRadioButton(eOptimized, "Optimized", &selected_display_);
        _im_gui->DoRadioButton(eNonOptimized, "Non-optimized", &selected_display_);
        _im_gui->DoRadioButton(eDifference, "Difference", &selected_display_);
      }
    }
    return true;
  }

  virtual void OnDestroy() {
    ozz::memory::Allocator* allocator = ozz::memory::default_allocator();
    allocator->Delete(animation_opt_);
    allocator->Delete(animation_non_opt_);
    allocator->Deallocate(locals_opt_);
    allocator->Deallocate(locals_scratch_);
    allocator->Deallocate(models_);
    allocator->Delete(cache_);
  }

  bool BuildAnimations() {
    assert(!animation_opt_);

    // Instantiate an aniation builder.
    ozz::animation::offline::AnimationBuilder animation_builder;

    // Builds the non-optimized animation (if it's the first call).
    if (!animation_non_opt_) {
      animation_non_opt_ = animation_builder(raw_animation_);
      if (!animation_non_opt_) {
        return false;
      }
    }

    // Builds the optimized animation.
    if (optimize_) {
      // Optimzes the raw animation.
      ozz::animation::offline::RawAnimation optimized_animation;
      if (!optimizer_(raw_animation_, &optimized_animation)) {
        return false;
      }
      // Builds runtime aniamtion from the optimized one.
      animation_opt_ = animation_builder(optimized_animation);
    } else {
      // Builds runtime aniamtion from the brut one.
      animation_opt_ = animation_builder(raw_animation_);
    }

    // Check if building runtime animation was successful.
    if (!animation_opt_) {
      return false;
    }

    return true;
  }

  virtual void GetSceneBounds(ozz::math::Box* _bound) const {
    ozz::sample::ComputePostureBounds(models_, _bound);
  }

 private:

  // Selects which animation is displayed.
  enum DisplayMode {
    eOptimized,
    eNonOptimized,
    eDifference,
  };
  int selected_display_;

  // Select whether optimization should be perfomed.
  bool optimize_;

  // Imported non-optimized animation.
  ozz::animation::offline::RawAnimation raw_animation_;

  // Stores the optimizer in order to expose its parameters.
  ozz::animation::offline::AnimationOptimizer optimizer_;

  // Playback animation controller. This is a utility class that helps with
  // controlling animation playback time.
  ozz::sample::PlaybackController controller_;

  // Runtime skeleton.
  ozz::animation::Skeleton skeleton_;

  // Sampling cache, shared accros optimized and non-optimized animations. This
  // is not optimal, but it's not an issue either.
  ozz::animation::SamplingCache* cache_;

  // Runtime optimized animation.
  ozz::animation::Animation* animation_opt_;

  // Runtime non-optimized animation.
  ozz::animation::Animation* animation_non_opt_;

  // Buffer of local transforms as sampled from animation_opt_.
  ozz::Range<ozz::math::SoaTransform> locals_opt_;

  // Scratch (temporary) buffer of local transforms, used to store samples from
  // animation_non_opt_ and difference between optimized and non-optimized
  // animation samples.
  ozz::Range<ozz::math::SoaTransform> locals_scratch_;

  // Buffer of model space matrices.
  ozz::Range<ozz::math::Float4x4> models_;
};

int main(int _argc, const char** _argv) {
  const char* title = "Ozz-animation sample: Animation keyframe optimization";
  return OptimizeSampleApplication().Run(_argc, _argv, "1.0",title);
}
