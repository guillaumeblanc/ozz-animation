//----------------------------------------------------------------------------//
//                                                                            //
// ozz-animation is hosted at http://github.com/guillaumeblanc/ozz-animation  //
// and distributed under the MIT License (MIT).                               //
//                                                                            //
// Copyright (c) 2017 Guillaume Blanc                                         //
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
#include "ozz/animation/runtime/local_to_model_job.h"
#include "ozz/animation/runtime/sampling_job.h"
#include "ozz/animation/runtime/skeleton.h"

#include "ozz/animation/runtime/track.h"
#include "ozz/animation/runtime/track_sampling_job.h"
#include "ozz/animation/runtime/track_triggering_job.h"

#include "ozz/base/log.h"

#include "ozz/base/memory/allocator.h"

#include "ozz/base/maths/box.h"
#include "ozz/base/maths/simd_math.h"
#include "ozz/base/maths/soa_transform.h"
#include "ozz/base/maths/vec_float.h"

#include "ozz/options/options.h"

#include "framework/application.h"
#include "framework/imgui.h"
#include "framework/renderer.h"
#include "framework/utils.h"

// Some scene constants.
const ozz::math::Box kBox(ozz::math::Float3(-.01f, -.1f, -.05f),
                          ozz::math::Float3(.01f, .1f, .05f));

const ozz::math::SimdFloat4 kBoxInitialPosition =
    ozz::math::simd_float4::Load(0.f, .1f, .3f, 0.f);

const ozz::sample::Renderer::Color kBoxColor = {0x80, 0x80, 0x80, 0xff};

// Skeleton archive can be specified as an option.
OZZ_OPTIONS_DECLARE_STRING(skeleton,
                           "Path to the skeleton (ozz archive format).",
                           "media/skeleton.ozz", false)

// Animation archive can be specified as an option.
OZZ_OPTIONS_DECLARE_STRING(animation,
                           "Path to the animation (ozz archive format).",
                           "media/animation.ozz", false)

// Track archive can be specified as an option.
OZZ_OPTIONS_DECLARE_STRING(track, "Path to the track (ozz archive format).",
                           "media/track.ozz", false)

class LoadSampleApplication : public ozz::sample::Application {
 public:
  LoadSampleApplication()
      : cache_(NULL),
        method_(0),
        attached_(false),
        attach_joint_(0),
        box_world_transform_(
            ozz::math::Float4x4::Translation(kBoxInitialPosition)),
        box_local_transform_(ozz::math::Float4x4::identity()) {}

 protected:
  virtual bool OnUpdate(float _dt) {
    // Updates current animation time.
    bool looped = controller_.Update(animation_, _dt);

    // Reset box position to its initial location. Note that it depends on
    // animation playback direction.
    if (looped) {
      box_world_transform_ =
          ozz::math::Float4x4::Translation(kBoxInitialPosition);
    }

    // Samples optimized animation at t = animation_time_.
    ozz::animation::SamplingJob sampling_job;
    sampling_job.animation = &animation_;
    sampling_job.cache = cache_;
    sampling_job.time = controller_.time();
    sampling_job.output = locals_;
    if (!sampling_job.Run()) {
      return false;
    }

    // Converts from local space to model space matrices.
    ozz::animation::LocalToModelJob ltm_job;
    ltm_job.skeleton = &skeleton_;
    ltm_job.input = locals_;
    ltm_job.output = models_;
    if (!ltm_job.Run()) {
      return false;
    }

    // Stores previous attachment state to be able to detect a change.
    const bool previously_attached = attached_;

    // Update attachment state depending on the selected method, aka sample or
    // triggering.
    if (method_ == 0) {
      if (!Update_SamplingMethod(_dt)) {
        return false;
      }
    } else {
      if (!Update_TriggeringMethod(_dt)) {
        return false;
      }
    }

    // Updates box transform based on attachment state.
    if (attached_) {
      // Find the relative transform of the box relative to the joint if the box
      // is being attached now.
      if (!previously_attached) {
        box_local_transform_ =
            Invert(models_[attach_joint_]) * box_world_transform_;
      }
      // Compute new (attached) box position.
      box_world_transform_ = models_[attach_joint_] * box_local_transform_;
    } else {
      // Lets the box where it is.
    }

    return true;
  }

  bool Update_SamplingMethod(float _dt) {
    (void)_dt;
    // Samples the track in order to know if the box should be attached to the
    // skeleton joint (hand).
    ozz::animation::FloatTrackSamplingJob job;

    // Tracks have a unit length duration. They are thus sampled with a ratio
    // (rather than a time), which is computed based on the duration of the
    // animation they refer to.
    job.time = controller_.time() / animation_.duration();
    job.track = &track_;
    float attached;
    job.result = &attached;
    if (!job.Run()) {
      return false;
    }
    attached_ = attached != 0.f;

    return true;
  }

  bool Update_TriggeringMethod(float _dt) {
    // Walks through the track to find edges, aka when the box should be
    // attached or detached.
    ozz::animation::FloatTrackTriggeringJob job;

    // Tracks have a unit length duration. They are thus sampled with a ratio
    // (rather than a time), which is computed based on the duration of the
    // animation they refer to.
    job.from = (controller_.time() - _dt) / animation_.duration();
    job.to = controller_.time() / animation_.duration();
    job.track = &track_;
    job.threshold =
        .5f;  // Considered attached as soon as the value is greater than this.
    ozz::animation::FloatTrackTriggeringJob::Edge edges_buffer[8];
    ozz::animation::FloatTrackTriggeringJob::Edges edges(edges_buffer);
    job.edges = &edges;
    if (!job.Run()) {
      return false;
    }

    // Rising edges put "attached" state on. In this case we're only interested
    // in the last edge (if any).
    if (edges.Count() != 0) {
      const ozz::animation::FloatTrackTriggeringJob::Edge& last_edge =
          edges[edges.Count() - 1];
      attached_ = last_edge.rising;

      // Knowing exact edge time, joint position could be re-sampled in order to
      // get attachment joint position at the precise attachment time. This
      // makes the algorithm frame rate independent.
    }
    return true;
  }

  // Samples animation, transforms to model space and renders.
  virtual bool OnDisplay(ozz::sample::Renderer* _renderer) {
    bool success = true;

    // Draw box at the position computed during update.
    success &= _renderer->DrawBoxShaded(kBox, box_world_transform_, kBoxColor);

    // Draws a sphere at hand position, which shows "attached" flag status.
    const ozz::sample::Renderer::Color colors[] = {{0, 0xff, 0, 0xff},
                                                   {0xff, 0, 0, 0xff}};
    _renderer->DrawSphereIm(.01f, models_[attach_joint_], colors[attached_]);

    // Draws the animated skeleton.
    success &= _renderer->DrawPosture(skeleton_, models_,
                                      ozz::math::Float4x4::identity());
    return success;
  }

  virtual bool OnInitialize() {
    ozz::memory::Allocator* allocator = ozz::memory::default_allocator();

    // Reading skeleton.
    if (!ozz::sample::LoadSkeleton(OPTIONS_skeleton, &skeleton_)) {
      return false;
    }

    // Finds the hand joint where the box should be attached.
    // If not found, let it be 0.
    for (int i = 0; i < skeleton_.num_joints(); i++) {
      if (std::strstr(skeleton_.joint_names()[i], "thumb2")) {
        attach_joint_ = i;
        break;
      }
    }

    // Reading animation.
    if (!ozz::sample::LoadAnimation(OPTIONS_animation, &animation_)) {
      return false;
    }

    // Allocates runtime buffers.
    const int num_soa_joints = skeleton_.num_soa_joints();
    locals_ = allocator->AllocateRange<ozz::math::SoaTransform>(num_soa_joints);
    const int num_joints = skeleton_.num_joints();
    models_ = allocator->AllocateRange<ozz::math::Float4x4>(num_joints);

    // Allocates a cache that matches animation requirements.
    cache_ = allocator->New<ozz::animation::SamplingCache>(num_joints);

    // Reading track.
    if (!ozz::sample::LoadTrack(OPTIONS_track, &track_)) {
      return false;
    }

    return true;
  }

  virtual void OnDestroy() {
    ozz::memory::Allocator* allocator = ozz::memory::default_allocator();
    allocator->Deallocate(locals_);
    allocator->Deallocate(models_);
    allocator->Delete(cache_);
  }

  virtual bool OnGui(ozz::sample::ImGui* _im_gui) {
    // Exposes sample specific parameters.
    {
      static bool open = true;
      ozz::sample::ImGui::OpenClose oc(_im_gui, "Track access method", &open);
      _im_gui->DoRadioButton(0, "Sampling", &method_);
      _im_gui->DoRadioButton(1, "Triggering", &method_);
    }
    // Exposes animation runtime playback controls.
    {
      static bool open = true;
      ozz::sample::ImGui::OpenClose oc(_im_gui, "Animation control", &open);
      if (open) {
        _im_gui->DoLabel(
            "Note that changing playback time could break box attachment state",
            ozz::sample::ImGui::kLeft, false);
        controller_.OnGui(animation_, _im_gui);
      }
    }
    return true;
  }

  virtual void GetSceneBounds(ozz::math::Box* _bound) const {
    ozz::sample::ComputePostureBounds(models_, _bound);
  }

 private:
  // Playback animation controller. This is a utility class that helps with
  // controlling animation playback time.
  ozz::sample::PlaybackController controller_;

  // Runtime skeleton.
  ozz::animation::Skeleton skeleton_;

  // Runtime animation.
  ozz::animation::Animation animation_;

  // Sampling cache.
  ozz::animation::SamplingCache* cache_;

  // Buffer of local transforms as sampled from animation_.
  ozz::Range<ozz::math::SoaTransform> locals_;

  // Buffer of model space matrices.
  ozz::Range<ozz::math::Float4x4> models_;

  // Runtime float track.
  // Stores whether the box should be attached to the hand.
  ozz::animation::FloatTrack track_;

  // Track reading method, aka sampling (0) or triggering (1).
  int method_;

  // Stores whether the box is currently attached. This flag is computed during
  // update. This is only used for debug display purpose.
  bool attached_;

  // Index of the joint where the box must be attached.
  int attach_joint_;

  // Box current transformation.
  ozz::math::Float4x4 box_world_transform_;

  // Box transformation relative to the attached bone.
  ozz::math::Float4x4 box_local_transform_;
};

int main(int _argc, const char** _argv) {
  const char* title = "Ozz-animation sample: User channels";
  return LoadSampleApplication().Run(_argc, _argv, "1.0", title);
}
