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

#include "ozz/animation/runtime/animation.h"
#include "ozz/animation/runtime/local_to_model_job.h"
#include "ozz/animation/runtime/sampling_job.h"
#include "ozz/animation/runtime/skeleton.h"

#include "ozz/animation/offline/animation_builder.h"
#include "ozz/animation/offline/animation_optimizer.h"
#include "ozz/animation/offline/raw_animation.h"
#include "ozz/animation/offline/raw_animation_utils.h"

#include "ozz/base/memory/unique_ptr.h"

#include "ozz/base/io/archive.h"
#include "ozz/base/io/stream.h"
#include "ozz/base/log.h"

#include "ozz/base/maths/math_ex.h"
#include "ozz/base/maths/simd_math.h"
#include "ozz/base/maths/soa_transform.h"
#include "ozz/base/maths/vec_float.h"

#include "ozz/options/options.h"

#include "framework/application.h"
#include "framework/imgui.h"
#include "framework/profile.h"
#include "framework/renderer.h"
#include "framework/utils.h"

#include <algorithm>

// Skeleton and animation file can be specified as an option.
OZZ_OPTIONS_DECLARE_STRING(skeleton, "Path to the runtime skeleton file.",
                           "media/skeleton.ozz", false)

OZZ_OPTIONS_DECLARE_STRING(animation, "Path to the raw animation file.",
                           "media/animation_raw.ozz", false)

namespace {

// Loads a raw animation from a file.
bool LoadAnimation(const char* _filename,
                   ozz::animation::offline::RawAnimation* _animation) {
  assert(_filename && _animation);
  ozz::log::Out() << "Loading raw animation archive: " << _filename << "."
                  << std::endl;
  ozz::io::File file(_filename, "rb");
  if (!file.opened()) {
    ozz::log::Err() << "Failed to open animation file " << _filename << "."
                    << std::endl;
    return false;
  }
  ozz::io::IArchive archive(&file);
  if (!archive.TestTag<ozz::animation::offline::RawAnimation>()) {
    ozz::log::Err() << "Failed to load raw animation instance from file "
                    << _filename << "." << std::endl;
    return false;
  }

  // Once the tag is validated, reading cannot fail.
  archive >> *_animation;

  // Ensure animation is valid.
  return _animation->Validate();
}
}  // namespace

class OptimizeSampleApplication : public ozz::sample::Application {
 public:
  OptimizeSampleApplication()
      : selected_display_(eRuntimeAnimation),
        optimize_(true),
        joint_setting_enable_(true),
        joint_(0),
        error_record_med_(64),
        error_record_max_(64),
        joint_error_record_(64) {}

 protected:
  // Updates current animation time and skeleton pose.
  virtual bool OnUpdate(float _dt, float) {
    // Updates current animation time.
    controller_.Update(*animation_rt_, _dt);

    // Prepares sampling job.
    ozz::animation::SamplingJob sampling_job;
    sampling_job.cache = &cache_;
    sampling_job.ratio = controller_.time_ratio();

    // Samples optimized animation (_according to the display mode).
    sampling_job.animation = animation_rt_.get();
    sampling_job.output = make_span(locals_rt_);
    if (!sampling_job.Run()) {
      return false;
    }

    // Also samples non-optimized animation, from the raw animation.
    if (!SampleRawAnimation(raw_animation_,
                            controller_.time_ratio() * raw_animation_.duration,
                            make_span(locals_raw_))) {
      return false;
    }

    // Computes difference between the optimized and non-optimized animations
    // in local space, and rebinds it to the bind pose.
    {
      const ozz::span<const ozz::math::SoaTransform>& bind_poses =
          skeleton_.joint_bind_poses();
      const ozz::math::SoaTransform* bind_pose = bind_poses.begin();
      const ozz::math::SoaTransform* locals_raw = locals_raw_.data();
      const ozz::math::SoaTransform* locals_rt = locals_rt_.data();
      ozz::math::SoaTransform* locals_diff = locals_diff_.data();
      for (; bind_pose < bind_poses.end();
           ++locals_raw, ++locals_rt, ++locals_diff, ++bind_pose) {
        assert(locals_raw < array_end(locals_raw_) &&
               locals_rt < array_end(locals_rt_) &&
               locals_diff < array_end(locals_diff_) &&
               bind_pose < bind_poses.end());

        // Computes difference.
        const ozz::math::SoaTransform diff = {
            locals_rt->translation - locals_raw->translation,
            locals_rt->rotation * Conjugate(locals_raw->rotation),
            locals_rt->scale / locals_raw->scale};

        // Rebinds to the bind pose in the diff buffer.
        locals_diff->translation = bind_pose->translation + diff.translation;
        locals_diff->rotation = bind_pose->rotation * diff.rotation;
        locals_diff->scale = bind_pose->scale * diff.scale;
      }
    }

    // Converts from local space to model space matrices.
    ozz::animation::LocalToModelJob ltm_job;
    ltm_job.skeleton = &skeleton_;

    // Optimized samples.
    ltm_job.input = make_span(locals_rt_);
    ltm_job.output = make_span(models_rt_);
    if (!ltm_job.Run()) {
      return false;
    }

    // Non-optimized samples (from the raw animation).
    ltm_job.input = make_span(locals_raw_);
    ltm_job.output = make_span(models_raw_);
    if (!ltm_job.Run()) {
      return false;
    }

    // Difference between optimized and non-optimized samples.
    ltm_job.input = make_span(locals_diff_);
    ltm_job.output = make_span(models_diff_);
    if (!ltm_job.Run()) {
      return false;
    }

    // Computes the absolute error, aka the difference between the raw and
    // runtime model space transformation.
    const size_t num_joints = models_rt_.size();
    float errors_sq[ozz::animation::Skeleton::kMaxJoints];
    for (size_t i = 0; i < num_joints; ++i) {
      // Computes error based on the translation difference.
      errors_sq[i] = ozz::math::GetX(ozz::math::Length3Sqr(
          models_rt_[i].cols[3] - models_raw_[i].cols[3]));
    }

    std::sort(errors_sq, errors_sq + models_rt_.size());
    error_record_med_.Push(std::sqrt(errors_sq[num_joints / 2]) * 1000.f);
    error_record_max_.Push(std::sqrt(errors_sq[num_joints - 1]) * 1000.f);
    joint_error_record_.Push(std::sqrt(errors_sq[joint_]) * 1000.f);

    return true;
  }

  bool SampleRawAnimation(
      const ozz::animation::offline::RawAnimation& _animation, float _time,
      ozz::span<ozz::math::SoaTransform> _locals) {
    // Ensure output is big enough.
    if (_locals.size() * 4 < _animation.tracks.size() &&
        locals_raw_aos_.size() * 4 < _animation.tracks.size()) {
      return false;
    }

    // Sample raw animation and converts AoS transforms to SoA transform array.
    assert(_animation.Validate() && "Animation should be valid.");
    bool success = true;
    for (int i = 0; success && i < _animation.num_tracks(); i += 4) {
      ozz::math::SimdFloat4 translations[4];
      ozz::math::SimdFloat4 rotations[4];
      ozz::math::SimdFloat4 scales[4];

      // Works on 4 consecutive tracks, or what remains to be processed if it's
      // lower than 4.
      const int jmax = ozz::math::Min(_animation.num_tracks() - i, 4);
      for (int j = 0; success && j < jmax; ++j) {
        // Samples raw animation.
        ozz::math::Transform transform;
        success &= SampleTrack(_animation.tracks[i + j], _time, &transform);

        // Convert transform to AoS SimdFloat4 values.
        translations[j] =
            ozz::math::simd_float4::Load3PtrU(&transform.translation.x);
        rotations[j] = ozz::math::simd_float4::LoadPtrU(&transform.rotation.x);
        scales[j] = ozz::math::simd_float4::Load3PtrU(&transform.scale.x);
      }
      // Fills remaining transforms.
      for (int j = jmax; j < 4; ++j) {
        translations[j] = ozz::math::simd_float4::zero();
        rotations[j] = ozz::math::simd_float4::w_axis();
        scales[j] = ozz::math::simd_float4::one();
      }
      // Stores AoS keyframes to the SoA output.
      ozz::math::SoaTransform& output = _locals[i / 4];
      ozz::math::Transpose4x3(translations, &output.translation.x);
      ozz::math::Transpose4x4(rotations, &output.rotation.x);
      ozz::math::Transpose4x3(scales, &output.scale.x);
    }

    return success;
  }

  // Selects model space matrices according to the display mode.
  ozz::span<const ozz::math::Float4x4> models() const {
    switch (selected_display_) {
      case eRuntimeAnimation:
        return make_span(models_rt_);
      case eRawAnimation:
        return make_span(models_raw_);
      case eAbsoluteError:
        return make_span(models_diff_);
      default: {
        assert(false && "Invalid display mode");
        return make_span(models_rt_);
      }
    }
  }

  // Samples animation, transforms to model space and renders.
  virtual bool OnDisplay(ozz::sample::Renderer* _renderer) {
    bool success = true;

    const ozz::span<const ozz::math::Float4x4> transforms = models();

    // Renders posture.
    success &= _renderer->DrawPosture(skeleton_, transforms,
                                      ozz::math::Float4x4::identity());

    if (joint_setting_enable_) {
      // Renders an axes with targetted joint transform.
      const ozz::math::Float4x4 axes_scale = ozz::math::Float4x4::Scaling(
          ozz::math::simd_float4::Load1(joint_setting_.distance));
      success &= _renderer->DrawAxes(transforms[joint_] * axes_scale);
    }

    return success;
  }

  virtual bool OnInitialize() {
    // Imports offline skeleton from a binary file.
    if (!ozz::sample::LoadSkeleton(OPTIONS_skeleton, &skeleton_)) {
      return false;
    }

    // Imports offline animation from a binary file.
    // Invalid animations are rejected by the load function.
    if (!LoadAnimation(OPTIONS_animation, &raw_animation_)) {
      return false;
    }

    const int num_joints = skeleton_.num_joints();
    const int num_soa_joints = skeleton_.num_soa_joints();

    // Finds the joint where the object should be attached.
    for (int i = 0; i < num_joints; i++) {
      if (std::strstr(skeleton_.joint_names()[i], "L Finger2Nub")) {
        joint_ = i;
        break;
      }
    }

    // Builds the runtime animation from the raw one.
    if (!BuildAnimations()) {
      return false;
    }

    // Allocates runtime buffers.
    locals_rt_.resize(num_soa_joints);
    models_rt_.resize(num_joints);
    locals_raw_aos_.resize(num_joints);
    locals_raw_.resize(num_soa_joints);
    models_raw_.resize(num_joints);
    locals_diff_.resize(num_soa_joints);
    models_diff_.resize(num_joints);

    // Allocates a cache that matches animation requirements.
    cache_.Resize(num_joints);

    return true;
  }

  virtual bool OnGui(ozz::sample::ImGui* _im_gui) {
    char label[64];
    // Exposes animation runtime playback controls.
    {
      static bool open = true;
      ozz::sample::ImGui::OpenClose occ(_im_gui, "Animation control", &open);
      if (open) {
        controller_.OnGui(*animation_rt_, _im_gui);
      }
    }

    // Exposes optimizer's tolerances.
    {
      static bool open = true;
      ozz::sample::ImGui::OpenClose ocb(_im_gui, "Optimization tolerances",
                                        &open);
      if (open) {
        bool rebuild = false;

        rebuild |= _im_gui->DoCheckBox("Enable optimizations", &optimize_);

        std::sprintf(label, "Tolerance: %0.2f mm", setting_.tolerance * 1000);
        rebuild |= _im_gui->DoSlider(label, 0.f, .1f, &setting_.tolerance, .5f,
                                     optimize_);

        std::sprintf(label, "Distance: %0.2f mm", setting_.distance * 1000);
        rebuild |= _im_gui->DoSlider(label, 0.f, 1.f, &setting_.distance, .5f,
                                     optimize_);

        rebuild |= _im_gui->DoCheckBox("Enable joint setting",
                                       &joint_setting_enable_, optimize_);

        std::sprintf(label, "%s (%d)", skeleton_.joint_names()[joint_], joint_);
        rebuild |=
            _im_gui->DoSlider(label, 0, skeleton_.num_joints() - 1, &joint_,
                              1.f, joint_setting_enable_ && optimize_);

        std::sprintf(label, "Tolerance: %0.2f mm",
                     joint_setting_.tolerance * 1000);
        rebuild |= _im_gui->DoSlider(label, 0.f, .1f, &joint_setting_.tolerance,
                                     .5f, joint_setting_enable_ && optimize_);

        std::sprintf(label, "Distance: %0.2f mm",
                     joint_setting_.distance * 1000);
        rebuild |= _im_gui->DoSlider(label, 0.f, 1.f, &joint_setting_.distance,
                                     .5f, joint_setting_enable_ && optimize_);

        if (rebuild) {
          // Invalidates the cache in case the new animation has the same
          // address as the previous one. Other cases (like changing animation)
          // are automatic handled by the cache. See SamplingCache::Invalidate
          // for more details.
          cache_.Invalidate();

          // Rebuilds a new runtime animation.
          if (!BuildAnimations()) {
            return false;
          }
        }
      }
    }
    {
      static bool open = true;
      ozz::sample::ImGui::OpenClose ocb(_im_gui, "Memory size", &open);
      if (open) {
        std::sprintf(label, "Original: %dKB",
                     static_cast<int>(raw_animation_.size() >> 10));
        _im_gui->DoLabel(label);

        std::sprintf(label, "Optimized: %dKB (%.1f:1)",
                     static_cast<int>(raw_optimized_animation_.size() >> 10),
                     static_cast<float>(raw_animation_.size()) /
                         raw_optimized_animation_.size());
        _im_gui->DoLabel(label);

        std::sprintf(
            label, "Compressed: %dKB (%.1f:1)",
            static_cast<int>(animation_rt_->size() >> 10),
            static_cast<float>(raw_animation_.size()) / animation_rt_->size());
        _im_gui->DoLabel(label);
      }
    }

    // Selects display mode.
    static bool open_mode = true;
    ozz::sample::ImGui::OpenClose mode(_im_gui, "Display mode", &open_mode);
    if (open_mode) {
      _im_gui->DoRadioButton(eRuntimeAnimation, "Runtime animation",
                             &selected_display_);
      _im_gui->DoRadioButton(eRawAnimation, "Raw animation",
                             &selected_display_);
      _im_gui->DoRadioButton(eAbsoluteError, "Absolute error",
                             &selected_display_);
    }

    // Show absolute error.
    {
      char szLabel[64];
      static bool error_open = true;
      ozz::sample::ImGui::OpenClose oc_stats(_im_gui, "Absolute error",
                                             &error_open);
      if (error_open) {
        {
          std::sprintf(szLabel, "Median error: %.2fmm",
                       *error_record_med_.cursor());
          const ozz::sample::Record::Statistics error_stats =
              error_record_med_.GetStatistics();
          _im_gui->DoGraph(szLabel, 0.f, error_stats.max, error_stats.latest,
                           error_record_med_.cursor(),
                           error_record_med_.record_begin(),
                           error_record_med_.record_end());
        }
        {
          std::sprintf(szLabel, "Maximum error: %.2fmm",
                       *error_record_max_.cursor());
          const ozz::sample::Record::Statistics error_stats =
              error_record_max_.GetStatistics();
          _im_gui->DoGraph(szLabel, 0.f, error_stats.max, error_stats.latest,
                           error_record_max_.cursor(),
                           error_record_max_.record_begin(),
                           error_record_max_.record_end());
        }
        {
          std::sprintf(szLabel, "Joint %d error: %.2fmm", joint_,
                       *joint_error_record_.cursor());
          const ozz::sample::Record::Statistics error_stats =
              joint_error_record_.GetStatistics();
          _im_gui->DoGraph(szLabel, 0.f, error_stats.max, error_stats.latest,
                           joint_error_record_.cursor(),
                           joint_error_record_.record_begin(),
                           joint_error_record_.record_end());
        }
      }
    }

    return true;
  }

  virtual void OnDestroy() {}

  bool BuildAnimations() {
    // Instantiate an animation builder.
    ozz::animation::offline::AnimationBuilder animation_builder;

    // Builds the optimized animation.
    if (optimize_) {
      ozz::animation::offline::AnimationOptimizer optimizer;

      // Setup global optimization settings.
      optimizer.setting = setting_;

      // Setup joint specific optimization settings.
      if (joint_setting_enable_) {
        optimizer.joints_setting_override[joint_] = joint_setting_;
      } else {
        optimizer.joints_setting_override.clear();
      }

      if (!optimizer(raw_animation_, skeleton_, &raw_optimized_animation_)) {
        return false;
      }
    } else {
      // Builds runtime animation from the brute one.
      raw_optimized_animation_ = raw_animation_;
    }

    // Builds runtime animation from the optimized one.
    animation_rt_ = animation_builder(raw_optimized_animation_);

    // Check if building runtime animation was successful.
    if (!animation_rt_) {
      return false;
    }

    return true;
  }

  virtual void GetSceneBounds(ozz::math::Box* _bound) const {
    ozz::sample::ComputePostureBounds(models(), _bound);
  }

 private:
  // Selects which animation is displayed.
  enum DisplayMode {
    eRuntimeAnimation,
    eRawAnimation,
    eAbsoluteError,
  };
  int selected_display_;

  // Select whether optimization should be performed.
  bool optimize_;

  // Imported non-optimized animation.
  ozz::animation::offline::RawAnimation raw_animation_;

  // Optimized raw animation.
  ozz::animation::offline::RawAnimation raw_optimized_animation_;

  // Optimizer global settings.
  ozz::animation::offline::AnimationOptimizer::Setting setting_;

  // Optimizer joint specific settings.
  bool joint_setting_enable_;
  int joint_;
  ozz::animation::offline::AnimationOptimizer::Setting joint_setting_;

  // Playback animation controller. This is a utility class that helps with
  // controlling animation playback time.
  ozz::sample::PlaybackController controller_;

  // Runtime skeleton.
  ozz::animation::Skeleton skeleton_;

  // Sampling cache, shared across optimized and non-optimized animations. This
  // is not optimal, but it's not an issue either.
  ozz::animation::SamplingCache cache_;

  // Runtime optimized animation.
  ozz::unique_ptr<ozz::animation::Animation> animation_rt_;

  // Buffer of local and model space transformations as sampled from the
  // rutime (optimized and compressed) animation.
  ozz::vector<ozz::math::SoaTransform> locals_rt_;
  ozz::vector<ozz::math::Float4x4> models_rt_;

  // Buffer of local and model space transformations as sampled from the
  // non-optimized (raw) animation.
  // Sampling the raw animation results in AoS data, meaning we have to
  // allocate AoS data and do the SoA conversion by hand.
  ozz::vector<ozz::math::Transform> locals_raw_aos_;
  ozz::vector<ozz::math::SoaTransform> locals_raw_;
  ozz::vector<ozz::math::Float4x4> models_raw_;

  // Buffer of local and model space transformations storing samples from the
  // difference between optimized and non-optimized animations.
  ozz::vector<ozz::math::SoaTransform> locals_diff_;
  ozz::vector<ozz::math::Float4x4> models_diff_;

  // Record of accuracy errors produced by animation compression and
  // optimization.
  ozz::sample::Record error_record_med_;
  ozz::sample::Record error_record_max_;
  ozz::sample::Record joint_error_record_;
};

int main(int _argc, const char** _argv) {
  const char* title = "Ozz-animation sample: Animation keyframe optimization";
  return OptimizeSampleApplication().Run(_argc, _argv, "1.0", title);
}
