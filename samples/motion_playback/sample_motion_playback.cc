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
#include "ozz/animation/runtime/local_to_model_job.h"
#include "ozz/animation/runtime/sampling_job.h"
#include "ozz/animation/runtime/skeleton.h"
#include "ozz/base/log.h"
#include "ozz/base/maths/box.h"
#include "ozz/base/maths/simd_math.h"
#include "ozz/base/maths/soa_transform.h"
#include "ozz/base/maths/transform.h"
#include "ozz/options/options.h"

// Skeleton archive can be specified as an option.
OZZ_OPTIONS_DECLARE_STRING(skeleton,
                           "Path to the skeleton (ozz archive format).",
                           "media/skeleton.ozz", false)

// Animation archive can be specified as an option.
OZZ_OPTIONS_DECLARE_STRING(animation,
                           "Path to the animation (ozz archive format).",
                           "media/animation.ozz", false)

// Motion tracks archive can be specified as an option.
OZZ_OPTIONS_DECLARE_STRING(motion,
                           "Path to the motion tracks (ozz archive format).",
                           "media/motion.ozz", false)

class MotionPlaybackSampleApplication : public ozz::sample::Application {
 protected:
  // Updates current animation time and skeleton pose.
  virtual bool OnUpdate(float _dt, float) {
    // Updates current animation time.
    //-------------------------------------------------------------------------
    int loops = controller_.Update(animation_, _dt);

    // Updates motion.
    //-------------------------------------------------------------------------

    // Updates motion accumulator.
    const auto rotation = FrameRotation(_dt * controller_.playback_speed() *
                                        controller_.playing());
    if (!motion_sampler_.Update(motion_track_, controller_.time_ratio(), loops,
                                rotation)) {
      return false;
    }

    // Updates the character transform matrix.
    const auto& transform = motion_sampler_.current;
    transform_ = ozz::math::Float4x4::FromAffine(
        apply_motion_position_ ? transform.translation
                               : ozz::math::Float3::zero(),
        apply_motion_rotation_ ? transform.rotation
                               : ozz::math::Quaternion::identity(),
        transform.scale);

    if (controller_.playing()) {
      trace_.push_back(transform.translation);
      if (static_cast<int>(trace_.size()) > trace_size_) {
        trace_.erase(trace_.begin(), trace_.end() - trace_size_);
      }
    }

    // Updates animation.
    //-------------------------------------------------------------------------

    // Samples optimized animation.
    ozz::animation::SamplingJob sampling_job;
    sampling_job.animation = &animation_;
    sampling_job.context = &context_;
    sampling_job.ratio = controller_.time_ratio();
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

  virtual bool OnDisplay(ozz::sample::Renderer* _renderer) {
    bool success = true;
    success &=
        _renderer->DrawPosture(skeleton_, make_span(models_), transform_);

    if (show_box_) {
      const ozz::math::Box bound(ozz::math::Float3(-.3f, 0.f, -.2f),
                                 ozz::math::Float3(.3f, 1.8f, .2f));
      success &= _renderer->DrawBoxIm(bound, transform_, ozz::sample::kWhite);
    }

    if (show_trace_) {
      success &= _renderer->DrawLineStrip(make_span(trace_), ozz::sample::kRed,
                                          ozz::math::Float4x4::identity());
    }

    if (show_motion_) {
      const float step = 1.f / (animation_.duration() * 60.f);
      const float at = controller_.time_ratio();
      const float from = floating_display_ ? at - floating_before_ : 0.f;
      const float to = floating_display_ ? at + floating_after_ : 1.f;
      success &= ozz::sample::DrawMotion(
          _renderer, motion_track_, from, at, to, step, transform_,
          FrameRotation(step * animation_.duration()));
    }

    return success;
  }

  // Compute rotation to apply for the given _duration
  ozz::math::Quaternion FrameRotation(float _duration) const {
    const float angle = angular_velocity_ * _duration;
    return ozz::math::Quaternion::FromEuler({angle, 0, 0});
  }

  virtual bool OnInitialize() {
    // Reading skeleton.
    if (!ozz::sample::LoadSkeleton(OPTIONS_skeleton, &skeleton_)) {
      return false;
    }

    // Reading animation.
    if (!ozz::sample::LoadAnimation(OPTIONS_animation, &animation_)) {
      return false;
    }

    // Skeleton and animation needs to match.
    if (skeleton_.num_joints() != animation_.num_tracks()) {
      return false;
    }

    // Reading motion tracks.
    if (!ozz::sample::LoadMotionTrack(OPTIONS_motion, &motion_track_)) {
      return false;
    }

    // Allocates runtime buffers.
    const int num_soa_joints = skeleton_.num_soa_joints();
    locals_.resize(num_soa_joints);
    const int num_joints = skeleton_.num_joints();
    models_.resize(num_joints);

    // Allocates a context that matches animation requirements.
    context_.Resize(num_joints);

    return true;
  }

  virtual bool OnGui(ozz::sample::ImGui* _im_gui) {
    char label[64];
    // Exposes animation runtime playback controls.
    {
      static bool open = true;
      ozz::sample::ImGui::OpenClose oc(_im_gui, "Animation control", &open);
      if (open) {
        controller_.OnGui(animation_, _im_gui);
      }
    }

    {
      static bool open = true;
      ozz::sample::ImGui::OpenClose oc(_im_gui, "Motion control", &open);
      if (open) {
        _im_gui->DoCheckBox("Apply motion position", &apply_motion_position_);
        _im_gui->DoCheckBox("Apply motion rotation", &apply_motion_rotation_);
        std::snprintf(label, sizeof(label), "Angular vel: %.0f deg/s",
                      angular_velocity_ * 180.f / ozz::math::kPi);
        _im_gui->DoSlider(label, -ozz::math::kPi_2, ozz::math::kPi_2,
                          &angular_velocity_);
        if (_im_gui->DoButton("Teleport")) {
          motion_sampler_.Teleport(ozz::math::Transform::identity());
          trace_.clear();
        }
      }
    }
    {
      static bool open = true;
      ozz::sample::ImGui::OpenClose oc(_im_gui, "Motion display", &open);
      if (open) {
        _im_gui->DoCheckBox("Show box", &show_box_);

        _im_gui->DoCheckBox("Show trace", &show_trace_);
        std::snprintf(label, sizeof(label), "Trace size: %d", trace_size_);
        _im_gui->DoSlider(label, 100, 2000, &trace_size_, 2.f);

        _im_gui->DoCheckBox("Show motion", &show_motion_);
        _im_gui->DoCheckBox("Floating display", &floating_display_,
                            show_motion_);

        std::snprintf(label, sizeof(label), "Motion before: %.0f%%",
                      floating_before_ * 100.f);
        _im_gui->DoSlider(label, 0.f, 3.f, &floating_before_, 1.f,
                          floating_display_ && show_motion_);
        std::snprintf(label, sizeof(label), "Motion after: %.0f%%",
                      floating_after_ * 100.f);
        _im_gui->DoSlider(label, 0.f, 3.f, &floating_after_, 1.f,
                          floating_display_ && show_motion_);
      }
    }
    return true;
  }

  virtual void GetSceneBounds(ozz::math::Box* _bound) const {
    *_bound = TransformBox(transform_, bounding_);
  }

 private:
  // Playback animation controller. This is a utility class that helps with
  // controlling animation playback time.
  ozz::sample::PlaybackController controller_;

  // Runtime skeleton.
  ozz::animation::Skeleton skeleton_;

  // Runtime animation.
  ozz::animation::Animation animation_;

  // Position and rotation motion tracks
  ozz::sample::MotionTrack motion_track_;

  // Motion accumulator helper
  ozz::sample::MotionSampler motion_sampler_;

  // Character transform
  ozz::math::Float4x4 transform_;

  // Sampling context.
  ozz::animation::SamplingJob::Context context_;

  // Buffer of local transforms as sampled from animation_.
  ozz::vector<ozz::math::SoaTransform> locals_;

  // Buffer of model space matrices.
  ozz::vector<ozz::math::Float4x4> models_;

  // Character bounding box.
  const ozz::math::Box bounding_{{-.3f, 0.f, -.2f}, {.3f, 1.8f, .2f}};

  // GUI options to apply root motion.
  bool apply_motion_position_ = true;
  bool apply_motion_rotation_ = true;

  // Rotation deformation, rad/s
  float angular_velocity_ = ozz::math::kPi_4;

  // Debug display UI options

  // Show box at root transform
  bool show_box_ = true;

  // Trace of last positions
  bool show_trace_ = true;
  int trace_size_ = 500;
  ozz::vector<ozz::math::Float3> trace_;

  // Motion display options
  bool show_motion_ = true;

  // Floatting display means that the motion is displayed around the current
  // time, instead of from begin to end.
  bool floating_display_ = true;
  float floating_before_ = .3f;
  float floating_after_ = 1.f;
};

int main(int _argc, const char** _argv) {
  const char* title = "Ozz-animation sample: Motion root playback";
  return MotionPlaybackSampleApplication().Run(_argc, _argv, "1.0", title);
}
