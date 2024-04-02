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

// Position and rotation motion tracks
struct MotionTrack {
  ozz::animation::Float3Track position;
  ozz::animation::QuaternionTrack rotation;
};

bool SampleMotion(const MotionTrack& _tracks, float _ratio,
                  ozz::math::Transform* _transform) {
  // Get position from motion track
  ozz::animation::Float3TrackSamplingJob position_sampler;
  position_sampler.track = &_tracks.position;
  position_sampler.result = &_transform->translation;
  position_sampler.ratio = _ratio;
  if (!position_sampler.Run()) {
    return false;
  }

  // Get rotation from motion track
  ozz::animation::QuaternionTrackSamplingJob rotation_sampler;
  rotation_sampler.track = &_tracks.rotation;
  rotation_sampler.result = &_transform->rotation;
  rotation_sampler.ratio = _ratio;
  if (!rotation_sampler.Run()) {
    return false;
  }

  _transform->scale = ozz::math::Float3::one();

  return true;
}

struct MotionAccumulator {
  // Accumulates motion deltas (new transform - last).
  void Update(const ozz::math::Transform& _new) {
    const ozz::math::Float3 delta_p = _new.translation - last.translation;
    current.translation = current.translation + delta_p;

    const ozz::math::Quaternion delta_r =
        Conjugate(last.rotation) * _new.rotation;
    current.rotation = Normalize(
        current.rotation * delta_r);  // Avoid accumulating error that leads to
                                      // non-normalized quaternions.
    last = _new;
  }

  // Updates the accumulator with a new motion sample.
  bool Update(const MotionTrack& _motion, float _ratio, int _loops) {
    ozz::math::Transform motion_transform;
    for (; _loops; _loops > 0 ? --_loops : ++_loops) {
      // Samples motion at the loop end (begin or end of animation depending on
      // playback/loop direction).
      if (!SampleMotion(_motion, _loops > 0 ? 1.f : 0.f, &motion_transform)) {
        return false;
      }
      Update(motion_transform);

      // Samples motion at the new origin (begin or end of animation depending
      // on playback/loop direction).
      if (!SampleMotion(_motion, _loops > 0 ? 0.f : 1.f, &motion_transform)) {
        return false;
      }
      ResetOrigin(motion_transform);
    }

    // Samples motion at the current ratio (from last known origin, or the
    // reset one).
    if (!SampleMotion(_motion, _ratio, &motion_transform)) {
      return false;
    }
    Update(motion_transform);

    return true;
  }

  // Tells the accumulator that the _new transform is the new origin.
  // This is useful when animation loops, so next delta is computed from the new
  // origin.
  void ResetOrigin(const ozz::math::Transform& _new) { last = _new; }

  // Teleports accumulator to a new transform. This also resets the origin, so
  // next delta is computed from the new origin.
  void Teleport(const ozz::math::Transform& _new) { current = last = _new; }

  ozz::math::Transform last = ozz::math::Transform::identity();
  ozz::math::Transform current = ozz::math::Transform::identity();
};

class MotionPlaybackSampleApplication : public ozz::sample::Application {
 protected:
  // Updates current animation time and skeleton pose.
  virtual bool OnUpdate(float _dt, float) {
    // Updates current animation time.
    int loops = controller_.Update(animation_, _dt);

    // Updates motion.
    //-------------------------------------------------------------------------

    // Updates motion accumulator.
    if (!motion_accumulator_.Update(motion_track_, controller_.time_ratio(),
                                    loops)) {
      return false;
    }

    // Updates the character transform matrix.
    const auto& transform = motion_accumulator_.current;
    transform_ = ozz::math::Float4x4::identity();
    if (apply_motion_position_) {
      transform_ =
          transform_ *
          ozz::math::Float4x4::Translation(
              ozz::math::simd_float4::Load3PtrU(&transform.translation.x));
    }
    if (apply_motion_rotation_) {
      transform_ = transform_ *
                   ozz::math::Float4x4::FromQuaternion(
                       ozz::math::simd_float4::LoadPtrU(&transform.rotation.x));
    }

    // Updates animation.
    //-------------------------------------------------------------------------

    // Samples optimized animation at t = animation_time_.
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
      const ozz::math::Box bound(ozz::math::Float3(-.3f, 0, -.2f),
                                 ozz::math::Float3(.3f, 1.8f, .2f));
      success &= _renderer->DrawBoxIm(bound, transform_, ozz::sample::kWhite);
    }

    return success;
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

    // Reading motion tracks.
    if (!ozz::sample::LoadMotionTrack(OPTIONS_motion, &motion_track_.position,
                                      &motion_track_.rotation)) {
      return false;
    }

    // Skeleton and animation needs to match.
    if (skeleton_.num_joints() != animation_.num_tracks()) {
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

  virtual void OnDestroy() {}

  virtual bool OnGui(ozz::sample::ImGui* _im_gui) {
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
        _im_gui->DoCheckBox("Use motion position", &apply_motion_position_);
        _im_gui->DoCheckBox("Use motion rotation", &apply_motion_rotation_);
        _im_gui->DoCheckBox("Show box", &show_box_);

        if (_im_gui->DoButton("Reset accumulator")) {
          motion_accumulator_.Teleport(ozz::math::Transform::identity());
        }
      }
    }
    return true;
  }

  virtual void GetSceneBounds(ozz::math::Box* _bound) const {
    ozz::sample::ComputePostureBounds(make_span(models_), transform_, _bound);
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
  MotionTrack motion_track_;

  // Motion accumulator helper
  MotionAccumulator motion_accumulator_;

  // Character transform
  ozz::math::Float4x4 transform_;

  // Sampling context.
  ozz::animation::SamplingJob::Context context_;

  // Buffer of local transforms as sampled from animation_.
  ozz::vector<ozz::math::SoaTransform> locals_;

  // Buffer of model space matrices.
  ozz::vector<ozz::math::Float4x4> models_;

  // Show box at root transform
  bool show_box_ = true;

  // GUI options to apply root motion.
  bool apply_motion_position_ = true;
  bool apply_motion_rotation_ = true;
};

int main(int _argc, const char** _argv) {
  const char* title = "Ozz-animation sample: Motion root playback";
  return MotionPlaybackSampleApplication().Run(_argc, _argv, "1.0", title);
}
