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
#include "framework/renderer.h"
#include "framework/utils.h"
#include "ozz/animation/runtime/animation.h"
#include "ozz/animation/runtime/local_to_model_job.h"
#include "ozz/animation/runtime/sampling_job.h"
#include "ozz/animation/runtime/skeleton.h"
#include "ozz/animation/runtime/track.h"
#include "ozz/animation/runtime/track_sampling_job.h"
#include "ozz/animation/runtime/track_triggering_job.h"
#include "ozz/base/log.h"
#include "ozz/base/maths/box.h"
#include "ozz/base/maths/simd_math.h"
#include "ozz/base/maths/soa_transform.h"
#include "ozz/base/maths/vec_float.h"
#include "ozz/options/options.h"

// Some scene constants.
const ozz::math::Box kBox(ozz::math::Float3(-.01f, -.1f, -.05f),
                          ozz::math::Float3(.01f, .1f, .05f));

const ozz::math::SimdFloat4 kBoxInitialPosition =
    ozz::math::simd_float4::Load(0.f, .1f, .3f, 0.f);

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

class UserChannelSampleApplication : public ozz::sample::Application {
 public:
  UserChannelSampleApplication()
      : method_(kTriggering),  // Triggering is the most robust method.
        attached_(false),
        attach_joint_(0) {
    ResetState();
  }

 protected:
  // Resets everything to it's initial state.
  void ResetState() {
    controller_.set_time_ratio(0.f);
    attached_ = false;
    box_local_transform_ = ozz::math::Float4x4::identity();
    box_world_transform_ =
        ozz::math::Float4x4::Translation(kBoxInitialPosition);
  }

  virtual bool OnUpdate(float _dt, float) {
    // Updates current animation time.
    controller_.Update(animation_, _dt);

    // Update attachment state depending on the selected method, aka sample or
    // triggering.
    if (method_ == kSampling) {
      if (!Update_SamplingMethod()) {
        return false;
      }
    } else {
      if (!Update_TriggeringMethod()) {
        return false;
      }
    }

    // Updates box transform based on attachment state.
    if (attached_) {
      box_world_transform_ = models_[attach_joint_] * box_local_transform_;
    } else {
      // Lets the box where it is.
    }

    return true;
  }

  bool Update_SamplingMethod() {
    // Updates animation and computes new joints position at current frame time.
    if (!Update_Joints(controller_.time_ratio())) {
      return false;
    }

    // Samples the track in order to know if the box should be attached to the
    // skeleton joint (hand).
    ozz::animation::FloatTrackSamplingJob job;

    // Tracks have a unit length duration. They are thus sampled with a ratio
    // (rather than a time), which is computed based on the duration of the
    // animation they refer to.
    job.ratio = controller_.time_ratio();
    job.track = &track_;
    float attached;
    job.result = &attached;
    if (!job.Run()) {
      return false;
    }

    bool previously_attached = attached_;
    attached_ = attached != 0.f;

    // If box is being attached, then computes it's relative position with the
    // attachment joint.
    if (attached_ && !previously_attached) {
      box_local_transform_ =
          Invert(models_[attach_joint_]) * box_world_transform_;
    }

    return true;
  }

  bool Update_TriggeringMethod() {
    // Walks through the track to find edges, aka when the box should be
    // attached or detached.
    ozz::animation::TrackTriggeringJob job;

    // Tracks have a unit length duration. They are thus sampled with a ratio
    // (rather than a time), which is computed based on the duration of the
    // animation they refer to.
    // Its important to use exact "previous time" here, because if we recompute
    // it, we might end up using a time range that is not exactly the previous
    // one, leading to missed or redundant edges.
    // Previous_time can be higher that current time, in case of a loop. It's
    // not a problem. Edges will be triggered backward (rewinding track in
    // time), so the "attachment" state remains valid. It's not the shortest or
    // optimum path though.
    job.from = controller_.previous_time_ratio();
    job.to = controller_.time_ratio();
    job.track = &track_;
    job.threshold = 0.f;  // Considered attached as soon as the value is
                          // greater than 0, aka different from 0.
    ozz::animation::TrackTriggeringJob::Iterator iterator;
    job.iterator = &iterator;
    if (!job.Run()) {
      return false;
    }

    // Iteratively evaluates all edges.
    // Edges are lazily evaluated on iterator increments.
    for (const ozz::animation::TrackTriggeringJob::Iterator end = job.end();
         iterator != end; ++iterator) {
      const ozz::animation::TrackTriggeringJob::Edge& edge = *iterator;

      // Updates attachment state.
      // Triggering job ensures rising and falling edges symmetry, this can be
      // asserted.
      assert(attached_ != edge.rising);
      attached_ = edge.rising;

      // Knowing exact edge ratio, joint position can be re-sampled in order
      // to get attachment joint position at the precise attachment time. This
      // makes the algorithm frame rate independent.
      // Sampling is cached so this intermediate updates don't have a big
      // performance impact.
      if (!Update_Joints(edge.ratio)) {
        return false;
      }

      if (edge.rising) {
        // Box is being attached on rising edges.
        // Find the relative transform of the box to the attachment joint at the
        // exact time of the rising edge.
        box_local_transform_ =
            Invert(models_[attach_joint_]) * box_world_transform_;
      } else {
        // Box is being detached on falling edges.
        // Compute box position when at the exact time it is released.
        box_world_transform_ = models_[attach_joint_] * box_local_transform_;
      }
    }

    // Finally updates animation and computes joints position at current frame
    // time.
    return Update_Joints(controller_.time_ratio());
  }

  bool Update_Joints(float _ratio) {
    // Samples animation at r = _ratio.
    ozz::animation::SamplingJob sampling_job;
    sampling_job.animation = &animation_;
    sampling_job.cache = &cache_;
    sampling_job.ratio = _ratio;
    sampling_job.output = make_span(locals_);
    if (!sampling_job.Run()) {
      return false;
    }

    // Converts from local space to model space matrices.
    ozz::animation::LocalToModelJob ltm_job;
    ltm_job.skeleton = &skeleton_;
    ltm_job.input = make_span(locals_);
    ltm_job.output = make_span(models_);
    if (!ltm_job.Run()) {
      return false;
    }

    return true;
  }

  // Samples animation, transforms to model space and renders.
  virtual bool OnDisplay(ozz::sample::Renderer* _renderer) {
    bool success = true;

    // Draw box at the position computed during update.
    success &= _renderer->DrawBoxShaded(
        kBox, ozz::span<ozz::math::Float4x4>(box_world_transform_),
        ozz::sample::kGrey);

    // Draws a sphere at hand position, which shows "attached" flag status.
    const ozz::sample::Color colors[] = {{0, 0xff, 0, 0xff},
                                         {0xff, 0, 0, 0xff}};
    _renderer->DrawSphereIm(.01f, models_[attach_joint_], colors[attached_]);

    // Draws the animated skeleton.
    success &= _renderer->DrawPosture(skeleton_, make_span(models_),
                                      ozz::math::Float4x4::identity());
    return success;
  }

  virtual bool OnInitialize() {
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
    locals_.resize(num_soa_joints);
    const int num_joints = skeleton_.num_joints();
    models_.resize(num_joints);

    // Allocates a cache that matches animation requirements.
    cache_.Resize(num_joints);

    // Reading track.
    if (!ozz::sample::LoadTrack(OPTIONS_track, &track_)) {
      return false;
    }

    return true;
  }

  virtual void OnDestroy() {}

  virtual bool OnGui(ozz::sample::ImGui* _im_gui) {
    // Exposes sample specific parameters.
    {
      static bool open = true;
      ozz::sample::ImGui::OpenClose oc(_im_gui, "Track access method", &open);
      int method = static_cast<int>(method_);
      bool changed = _im_gui->DoRadioButton(kSampling, "Sampling", &method);
      changed |= _im_gui->DoRadioButton(kTriggering, "Triggering", &method);
      if (changed) {
        method_ = static_cast<Method>(method);
        ResetState();
      }
    }
    // Exposes animation runtime playback controls.
    {
      static bool open = true;
      ozz::sample::ImGui::OpenClose oc(_im_gui, "Animation control", &open);
      if (open) {
        if (controller_.OnGui(animation_, _im_gui, true)) {
          // Triggering method can adapt to "big" time changes (jumps), because
          // all the events will be triggered and processed for the whole time
          // period.
          // No point doing that for sampling method.
          if (method_ == kTriggering) {
            if (!Update_TriggeringMethod()) {
              return false;
            }
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
  // Playback animation controller. This is a utility class that helps with
  // controlling animation playback time.
  ozz::sample::PlaybackController controller_;

  // Runtime skeleton.
  ozz::animation::Skeleton skeleton_;

  // Runtime animation.
  ozz::animation::Animation animation_;

  // Sampling cache.
  ozz::animation::SamplingCache cache_;

  // Buffer of local transforms as sampled from animation_.
  ozz::vector<ozz::math::SoaTransform> locals_;

  // Buffer of model space matrices.
  ozz::vector<ozz::math::Float4x4> models_;

  // Runtime float track.
  // Stores whether the box should be attached to the hand.
  ozz::animation::FloatTrack track_;

  // Track reading method.
  enum Method {
    kSampling,   // Will use TrackSamplingJob
    kTriggering  // Will use TrackTriggeringJob
  } method_;

  // Stores whether the box is currently attached. This flag is computed
  // during update. This is only used for debug display purpose.
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
  return UserChannelSampleApplication().Run(_argc, _argv, "1.0", title);
}
