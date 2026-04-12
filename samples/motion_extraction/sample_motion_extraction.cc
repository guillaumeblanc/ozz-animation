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
#include "ozz/animation/offline/animation_builder.h"
#include "ozz/animation/offline/animation_optimizer.h"
#include "ozz/animation/offline/motion_extractor.h"
#include "ozz/animation/offline/raw_animation.h"
#include "ozz/animation/offline/raw_track.h"
#include "ozz/animation/offline/track_builder.h"
#include "ozz/animation/offline/track_optimizer.h"
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
#include "ozz/base/maths/vec_float.h"
#include "ozz/options/options.h"

// Skeleton archive can be specified as an option.
OZZ_OPTIONS_DECLARE_STRING(skeleton,
                           "Path to the skeleton (ozz archive format).",
                           "media/skeleton.ozz", false)

// Animation archive can be specified as an option.
OZZ_OPTIONS_DECLARE_STRING(animation,
                           "Path to the animation (ozz archive format).",
                           "media/raw_animation.ozz", false)

class MotionExtractionSampleApplication : public ozz::sample::Application {
 public:
  MotionExtractionSampleApplication() {}

 protected:
  // Updates current animation time and skeleton pose.
  virtual bool OnUpdate(float _dt, float) {
    // Updates current animation time.
    controller_.Update(animation_, _dt);

    // Updates motion.
    //-------------------------------------------------------------------------

    // Get position from motion track
    auto position = ozz::math::Float3::zero();
    if (apply_motion_position_) {
      ozz::animation::Float3TrackSamplingJob position_sampler;
      position_sampler.track = &motion_track_.position;
      position_sampler.result = &position;
      position_sampler.ratio = controller_.time_ratio();
      if (!position_sampler.Run()) {
        return false;
      }
    }

    // Get rotation from motion track
    auto rotation = ozz::math::Quaternion::identity();
    if (apply_motion_rotation_) {
      ozz::animation::QuaternionTrackSamplingJob rotation_sampler;
      rotation_sampler.track = &motion_track_.rotation;
      rotation_sampler.result = &rotation;
      rotation_sampler.ratio = controller_.time_ratio();
      if (!rotation_sampler.Run()) {
        return false;
      }
    }

    // Set character transform
    transform_ = ozz::math::Float4x4::FromAffine(
        position + motion_track_.reference.translation,
        rotation * motion_track_.reference.rotation, ozz::math::Float3::one());

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

    // Draw a box at character's root.
    if (show_bounding_box_) {
      const ozz::math::Box box(ozz::math::Float3(-.25f, -.25f, -.8f),
                               ozz::math::Float3(.25f, .25f, .8f));
      success &= _renderer->DrawBoxIm(box, transform_, ozz::sample::kWhite);
      success &= _renderer->DrawAxes(transform_);
    }

    if (show_reference_) {
      success &= _renderer->DrawAxes(
          ozz::math::Float4x4::FromAffine(motion_track_.reference));
      success &= _renderer->DrawAxes(transform_);
    }

    // Draw tracks.
    {
      const float at = controller_.time_ratio();
      const float step = 1.f / (animation_.duration() * 120.f);

      if (show_motion_track_) {  // Motion track
        success &=
            ozz::sample::DrawMotion(_renderer, motion_track_, 0.f, at, 1.f,
                                    step, ozz::math::Float4x4::identity());
      }

      if (show_joint_tracks_) {  // Extra joint tracks (display only)
        for (const auto& joint_track : joint_tracks_) {
          success &=
              ozz::sample::DrawMotion(_renderer, joint_track, 0.f, at, 1.f,
                                      step, ozz::math::Float4x4::identity());
        }
      }
    }

    return success;
  }

  bool ExtractMotion() {
    // Extract root motion track and baked animation
    ozz::animation::offline::RawAnimation baked_animation;
    if (!ExtractJointMotion(motion_extractor_joint_, &motion_track_,
                            &baked_animation)) {
      return false;
    }

    {  // Optimizes and builds runtime animation
      ozz::animation::offline::RawAnimation animation_opt;
      ozz::animation::offline::AnimationOptimizer optimizer;
      if (!optimizer(baked_animation, skeleton_, &animation_opt)) {
        return false;
      }

      ozz::animation::offline::AnimationBuilder builder;
      auto rt_animation = builder(animation_opt);
      if (!rt_animation) {
        return false;
      }
      animation_ = std::move(*rt_animation);

      // Animation was changed, context needs to know.
      context_.Invalidate();
    }

    return true;
  }

  bool ExtractJointMotion(
      int _joint, ozz::sample::MotionTrack* _motion_track,
      ozz::animation::offline::RawAnimation* _baked_animation) {
    ozz::animation::offline::MotionExtractor motion_extractor;
    motion_extractor.position_settings = motion_extractor_position_;
    motion_extractor.rotation_settings = motion_extractor_rotation_;
    motion_extractor.joint = _joint;

    // Raw motion tracks extraction
    ozz::animation::offline::RawFloat3Track raw_motion_position;
    ozz::animation::offline::RawQuaternionTrack raw_motion_rotation;
    ozz::animation::offline::RawAnimation baked_animation;
    if (!motion_extractor(raw_animation_, skeleton_, &raw_motion_position,
                          &raw_motion_rotation, _baked_animation,
                          &_motion_track->reference)) {
      return false;
    }

    {  // Track optimization and runtime building
      ozz::animation::offline::TrackOptimizer optimizer;
      ozz::animation::offline::RawFloat3Track raw_track_position_opt;
      if (!optimizer(raw_motion_position, &raw_track_position_opt)) {
        return false;
      }

      ozz::animation::offline::RawQuaternionTrack raw_track_rotation_opt;
      if (!optimizer(raw_motion_rotation, &raw_track_rotation_opt)) {
        return false;
      }

      // Build runtime tracks
      ozz::animation::offline::TrackBuilder track_builder;
      auto position_track = track_builder(raw_track_position_opt);
      auto rotation_track = track_builder(raw_track_rotation_opt);
      if (!position_track || !rotation_track) {
        return false;
      }
      _motion_track->position = std::move(*position_track);
      _motion_track->rotation = std::move(*rotation_track);
    }

    return true;
  }

  bool ExtractDisplayJointTracks() {
    joint_tracks_.clear();

    if (!show_joint_tracks_) {  // Don't extract if not displayed
      return true;
    }

    for (int i = 0; i < skeleton_.num_joints(); ++i) {
      ozz::animation::offline::RawAnimation baked_animation;
      ozz::sample::MotionTrack motion_track;
      if (!ExtractJointMotion(i, &motion_track, &baked_animation)) {
        return false;
      }
      joint_tracks_.push_back(std::move(motion_track));
    }
    return true;
  }

  virtual bool OnInitialize() {
    // Reading skeleton.
    if (!ozz::sample::LoadSkeleton(OPTIONS_skeleton, &skeleton_)) {
      return false;
    }

    // Reading animation.
    if (!ozz::sample::LoadRawAnimation(OPTIONS_animation, &raw_animation_)) {
      return false;
    }

    motion_extractor_joint_ = 1;

    // Setup default extraction for the sample.
    motion_extractor_position_ = {
        true, true, true,  // Components
        ozz::animation::offline::MotionExtractor::Reference::kSkeleton,
        true  // Bake
    };
    motion_extractor_rotation_ = {
        false, false, true,  // Components
        ozz::animation::offline::MotionExtractor::Reference::kSkeleton,
        true  // Bake
    };

    if (!ExtractMotion()) {
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

    if (!ExtractDisplayJointTracks()) {
      return false;
    }

    return true;
  }

  virtual bool OnGui(ozz::sample::ImGui* _im_gui) {
    // Exposes animation runtime playback controls.
    {
      static bool open = true;
      ozz::sample::ImGui::OpenClose oc(_im_gui, "Animation control", &open);
      if (open) {
        controller_.OnGui(animation_, _im_gui);
      }
    }

    bool rebuild = false;
    {
      static bool open = true;
      ozz::sample::ImGui::OpenClose oc(_im_gui, "Motion extraction", &open);
      {
        _im_gui->DoLabel("Select joint:");
        char label[64];
        std::snprintf(label, sizeof(label), "%s (%d)",
                      skeleton_.joint_names()[motion_extractor_joint_],
                      motion_extractor_joint_);
        rebuild |= _im_gui->DoSlider(label, 0, skeleton_.num_joints() - 1,
                                     &motion_extractor_joint_);

        static bool position = true;
        ozz::sample::ImGui::OpenClose ocp(_im_gui, "Position", &position);
        {
          ozz::sample::ImGui::OpenClose occ(_im_gui, "Components", nullptr);
          rebuild |= _im_gui->DoCheckBox("x", &motion_extractor_position_.x);
          rebuild |= _im_gui->DoCheckBox("y", &motion_extractor_position_.y);
          rebuild |= _im_gui->DoCheckBox("z", &motion_extractor_position_.z);
        }

        {
          ozz::sample::ImGui::OpenClose ocr(_im_gui, "Reference", nullptr);
          int ref = static_cast<int>(motion_extractor_position_.reference);
          rebuild |= _im_gui->DoRadioButton(0, "Identity", &ref);
          rebuild |= _im_gui->DoRadioButton(1, "Skeleton", &ref);
          rebuild |= _im_gui->DoRadioButton(2, "Animation", &ref);
          motion_extractor_position_.reference =
              static_cast<ozz::animation::offline::MotionExtractor::Reference>(
                  ref);
        }
        rebuild |=
            _im_gui->DoCheckBox("Bake", &motion_extractor_position_.bake);
        rebuild |=
            _im_gui->DoCheckBox("Loop", &motion_extractor_position_.loop);
      }

      {
        static bool rotation = true;
        ozz::sample::ImGui::OpenClose ocp(_im_gui, "Rotation", &rotation);
        {
          ozz::sample::ImGui::OpenClose occ(_im_gui, "Components", nullptr);
          rebuild |=
              _im_gui->DoCheckBox("x / pitch", &motion_extractor_rotation_.x);
          rebuild |=
              _im_gui->DoCheckBox("y / yaw", &motion_extractor_rotation_.y);
          rebuild |=
              _im_gui->DoCheckBox("z / roll", &motion_extractor_rotation_.z);
        }

        {
          ozz::sample::ImGui::OpenClose ocr(_im_gui, "Reference", nullptr);
          int ref = static_cast<int>(motion_extractor_rotation_.reference);
          rebuild |= _im_gui->DoRadioButton(0, "Identity", &ref);
          rebuild |= _im_gui->DoRadioButton(1, "Skeleton", &ref);
          rebuild |= _im_gui->DoRadioButton(2, "Animation", &ref);
          motion_extractor_rotation_.reference =
              static_cast<ozz::animation::offline::MotionExtractor::Reference>(
                  ref);
        }
        rebuild |=
            _im_gui->DoCheckBox("Bake", &motion_extractor_rotation_.bake);
        rebuild |=
            _im_gui->DoCheckBox("Loop", &motion_extractor_rotation_.loop);
      }
    }

    {
      static bool open = true;
      ozz::sample::ImGui::OpenClose oc(_im_gui, "Display options", &open);
      if (open) {
        _im_gui->DoCheckBox("Show motion track", &show_motion_track_);
        rebuild |=
            _im_gui->DoCheckBox("Show joint tracks", &show_joint_tracks_);
        _im_gui->DoCheckBox("Show reference", &show_reference_);
        _im_gui->DoCheckBox("Show bounding box", &show_bounding_box_);
      }
    }

    {
      static bool open = false;
      ozz::sample::ImGui::OpenClose oc(_im_gui, "Debug options", &open);
      if (open) {
        _im_gui->DoCheckBox("Use motion position", &apply_motion_position_);
        _im_gui->DoCheckBox("Use motion rotation", &apply_motion_rotation_);
      }
    }

    if (rebuild) {
      if (!ExtractMotion() || !ExtractDisplayJointTracks()) {
        return false;
      }
    }

    return true;
  }

  virtual ozz::math::Box GetSceneBounds() const {
    return ozz::sample::ComputePostureBounds(make_span(models_), transform_);
  }

 private:
  // Playback animation controller. This is a utility class that helps with
  // controlling animation playback time.
  ozz::sample::PlaybackController controller_;

  // Store extractor settings to expose them to GUI.

  // Joint to use for motion extraction.
  int motion_extractor_joint_ = 0;

  // Position and rotation motion extractor settings.
  ozz::animation::offline::MotionExtractor::Settings motion_extractor_position_;
  ozz::animation::offline::MotionExtractor::Settings motion_extractor_rotation_;

  // Runtime skeleton.
  ozz::animation::Skeleton skeleton_;

  // Original animation.
  ozz::animation::offline::RawAnimation raw_animation_;

  // Runtime animation.
  ozz::animation::Animation animation_;

  // Runtime motion tracks.
  ozz::sample::MotionTrack motion_track_;

  // Runtime joint tracks tracks, not part of the motion extraction, but
  // useful to display the original joint motion alongside the extracted
  // motion.
  ozz::vector<ozz::sample::MotionTrack> joint_tracks_;

  // Sampling context.
  ozz::animation::SamplingJob::Context context_;

  // Character transform.
  ozz::math::Float4x4 transform_;

  // Buffer of local transforms as sampled from animation_.
  ozz::vector<ozz::math::SoaTransform> locals_;

  // Buffer of model space matrices.
  ozz::vector<ozz::math::Float4x4> models_;

  // Display options

  // Option to display motion track
  bool show_motion_track_ = true;

  // Option to display extra joints tracks
  bool show_joint_tracks_ = true;

  // Option to show motion extraction reference axis
  bool show_reference_ = false;

  // Option character bounding box on extracted motion
  bool show_bounding_box_ = true;

  // Debug options

  // Apply motion position and rotation to character
  bool apply_motion_position_ = true;
  bool apply_motion_rotation_ = true;
};

int main(int _argc, const char** _argv) {
  const char* title = "Ozz-animation sample: Root motion extraction";
  return MotionExtractionSampleApplication().Run(_argc, _argv, "1.0", title);
}
