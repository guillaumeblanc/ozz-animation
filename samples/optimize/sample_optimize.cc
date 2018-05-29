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

#include <algorithm>

#include "ozz/animation/runtime/animation.h"
#include "ozz/animation/runtime/local_to_model_job.h"
#include "ozz/animation/runtime/sampling_job.h"
#include "ozz/animation/runtime/skeleton.h"

#include "ozz/animation/offline/animation_builder.h"
#include "ozz/animation/offline/animation_optimizer.h"
#include "ozz/animation/offline/raw_animation.h"
#include "ozz/animation/offline/raw_animation_utils.h"

#include "ozz/base/memory/allocator.h"

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

// Skeleton and animation file can be specified as an option.
OZZ_OPTIONS_DECLARE_STRING(skeleton, "Path to the runtime skeleton file.",
                           "media/skeleton.ozz", false)

OZZ_OPTIONS_DECLARE_STRING(animation, "Path to the raw animation file.",
                           "media/animation_raw.ozz", false)

namespace {

// The next functions are used to sample a RawAnimation. This feature is not
// part of ozz sdk, as RawAnimation is a intermediate format used to build the
// runtime animation.

// Less comparator, used by search algorithm to walk through track sorted
// keyframes
template <typename _Key>
bool Less(const _Key& _left, const _Key& _right) {
  return _left.time < _right.time;
}

// Samples a component (translation, rotation or scale) of a track.
template <typename _Track, typename _Lerp>
typename _Track::value_type::Value SampleComponent(const _Track& _track,
                                                   const _Lerp& _lerp,
                                                   float _time) {
  if (_track.size() == 0) {
    // Return identity if there's no key for this track.
    return _Track::value_type::identity();
  } else if (_time <= _track.front().time) {
    // Returns the first keyframe if _time is before the first keyframe.
    return _track.front().value;
  } else if (_time >= _track.back().time) {
    // Returns the last keyframe if _time is before the last keyframe.
    return _track.back().value;
  } else {
    // Needs to interpolate the 2 keyframes before and after _time.
    assert(_track.size() >= 2);
    // First find the 2 keys.
    const typename _Track::value_type cmp = {_time,
                                             _Track::value_type::identity()};
    typename _Track::const_pointer it =
        std::lower_bound(array_begin(_track), array_end(_track), cmp,
                         Less<typename _Track::value_type>);
    assert(it > array_begin(_track) && it < array_end(_track));

    // Then interpolate them at t = _time.
    const typename _Track::const_reference right = it[0];
    const typename _Track::const_reference left = it[-1];
    const float alpha = (_time - left.time) / (right.time - left.time);
    return _lerp(left.value, right.value, alpha);
  }
}

// Samples all 3 components of a track.
ozz::math::Transform SampleTrack(
    const ozz::animation::offline::RawAnimation::JointTrack& _track,
    float _time) {
  // Samples each track's component.
  ozz::math::Transform transform;
  transform.translation = SampleComponent(
      _track.translations, ozz::animation::offline::LerpTranslation, _time);
  transform.rotation = SampleComponent(
      _track.rotations, ozz::animation::offline::LerpRotation, _time);
  transform.scale =
      SampleComponent(_track.scales, ozz::animation::offline::LerpScale, _time);
  return transform;
}

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
        cache_(NULL),
        animation_rt_(NULL),
        error_record_(64) {}

 protected:
  // Updates current animation time and skeleton pose.
  virtual bool OnUpdate(float _dt) {
    // Updates current animation time.
    controller_.Update(*animation_rt_, _dt);

    // Prepares sampling job.
    ozz::animation::SamplingJob sampling_job;
    sampling_job.cache = cache_;
    sampling_job.ratio = controller_.time_ratio();

    // Samples optimized animation (_according to the display mode).
    sampling_job.animation = animation_rt_;
    sampling_job.output = locals_rt_;
    if (!sampling_job.Run()) {
      return false;
    }

    // Also samples non-optimized animation, from the raw animation.
    if (!SampleRawAnimation(raw_animation_,
                            controller_.time_ratio() * raw_animation_.duration,
                            locals_raw_)) {
      return false;
    }

    // Computes difference between the optimized and non-optimized animations
    // in local space, and rebinds it to the bind pose.
    {
      const ozz::math::SoaTransform* locals_raw = locals_raw_.begin;
      const ozz::math::SoaTransform* locals_rt = locals_rt_.begin;
      ozz::math::SoaTransform* locals_diff = locals_diff_.begin;
      ozz::Range<const ozz::math::SoaTransform> bind_poses =
          skeleton_.bind_pose();
      const ozz::math::SoaTransform* bind_pose = bind_poses.begin;
      for (; bind_pose < bind_poses.end;
           ++locals_raw, ++locals_rt, ++locals_diff, ++bind_pose) {
        assert(locals_raw < locals_raw_.end && locals_rt < locals_rt_.end &&
               locals_diff < locals_diff_.end && bind_pose < bind_poses.end);

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
    ltm_job.input = locals_rt_;
    ltm_job.output = models_rt_;
    if (!ltm_job.Run()) {
      return false;
    }

    // Non-optimized samples (from the raw animation).
    ltm_job.input = locals_raw_;
    ltm_job.output = models_raw_;
    if (!ltm_job.Run()) {
      return false;
    }

    // Difference between optimized and non-optimized samples.
    ltm_job.input = locals_diff_;
    ltm_job.output = models_diff_;
    if (!ltm_job.Run()) {
      return false;
    }

    // Computes the absolute error, aka the difference between the raw and
    // runtime model space transformation.
    float error = 0.f;
    const ozz::math::Float4x4* models_rt = models_rt_.begin;
    const ozz::math::Float4x4* models_raw = models_raw_.begin;
    for (; models_rt < models_rt_.end; ++models_rt, ++models_raw) {
      // Computes the translation difference.
      const ozz::math::SimdFloat4 diff =
          models_rt->cols[3] - models_raw->cols[3];

      // Stores maximum error.
      error = ozz::math::Max(error, ozz::math::GetX(ozz::math::Length3(diff)));
    }
    error_record_.Push(error * 1000.f);  // Error is in millimeters.

    return true;
  }

  bool SampleRawAnimation(
      const ozz::animation::offline::RawAnimation& _animation, float _time,
      ozz::Range<ozz::math::SoaTransform> _locals) {
    // Ensure output is big enough.
    if (_locals.count() * 4 < _animation.tracks.size() &&
        locals_raw_aos_.count() * 4 < _animation.tracks.size()) {
      return false;
    }

    // Sample raw animation and converts AoS transforms to SoA transform array.
    assert(_animation.Validate() && "Animation should be valid.");
    for (int i = 0; i < _animation.num_tracks(); i += 4) {
      ozz::math::SimdFloat4 translations[4];
      ozz::math::SimdFloat4 rotations[4];
      ozz::math::SimdFloat4 scales[4];

      // Works on 4 consecutive tracks, or what remains to be processed if it's
      // lower than 4.
      const int jmax = ozz::math::Min(_animation.num_tracks() - i, 4);
      for (int j = 0; j < jmax; ++j) {
        // Samples raw animation.
        const ozz::math::Transform transform =
            SampleTrack(_animation.tracks[i + j], _time);

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

    return true;
  }

  // Selects model space matrices according to the display mode.
  ozz::Range<const ozz::math::Float4x4> models() const {
    switch (selected_display_) {
      case eRuntimeAnimation:
        return models_rt_;
      case eRawAnimation:
        return models_raw_;
      case eAbsoluteError:
        return models_diff_;
      default: {
        assert(false && "Invalid display mode");
        return models_rt_;
      }
    }
  }

  // Samples animation, transforms to model space and renders.
  virtual bool OnDisplay(ozz::sample::Renderer* _renderer) {
    // Renders posture.
    return _renderer->DrawPosture(skeleton_, models(),
                                  ozz::math::Float4x4::identity());
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

    // Builds the runtime animation from the raw one.
    if (!BuildAnimations()) {
      return false;
    }

    // Allocates runtime buffers.
    ozz::memory::Allocator* allocator = ozz::memory::default_allocator();
    const int num_joints = skeleton_.num_joints();
    const int num_soa_joints = skeleton_.num_soa_joints();

    locals_rt_ =
        allocator->AllocateRange<ozz::math::SoaTransform>(num_soa_joints);
    models_rt_ = allocator->AllocateRange<ozz::math::Float4x4>(num_joints);
    locals_raw_aos_ =
        allocator->AllocateRange<ozz::math::Transform>(num_joints);
    locals_raw_ =
        allocator->AllocateRange<ozz::math::SoaTransform>(num_soa_joints);
    models_raw_ = allocator->AllocateRange<ozz::math::Float4x4>(num_joints);
    locals_diff_ =
        allocator->AllocateRange<ozz::math::SoaTransform>(num_soa_joints);
    models_diff_ = allocator->AllocateRange<ozz::math::Float4x4>(num_joints);

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
        controller_.OnGui(*animation_rt_, _im_gui);
      }
    }

    // Exposes optimizer's tolerances.
    {
      static bool open_tol = true;
      ozz::sample::ImGui::OpenClose ocb(_im_gui, "Optimization tolerances",
                                        &open_tol);
      if (open_tol) {
        bool rebuild = false;
        char label[64];

        rebuild |= _im_gui->DoCheckBox("Enable optimizations", &optimize_);

        std::sprintf(label, "Translation : %0.2f mm",
                     optimizer_.translation_tolerance * 1000);
        rebuild |= _im_gui->DoSlider(
            label, 0.f, .1f, &optimizer_.translation_tolerance, .3f, optimize_);

        std::sprintf(label, "Rotation : %0.3f degree",
                     optimizer_.rotation_tolerance * 180.f / ozz::math::kPi);

        rebuild |=
            _im_gui->DoSlider(label, 0.f, 10.f * ozz::math::kPi / 180.f,
                              &optimizer_.rotation_tolerance, .3f, optimize_);

        std::sprintf(label, "Scale : %0.3f %%",
                     optimizer_.scale_tolerance * 100.f);
        rebuild |= _im_gui->DoSlider(
            label, 0.f, .1f, &optimizer_.scale_tolerance, .3f, optimize_);

        std::sprintf(label, "Hierarchical : %0.2f mm",
                     optimizer_.hierarchical_tolerance * 1000);
        rebuild |= _im_gui->DoSlider(label, 0.f, .1f,
                                     &optimizer_.hierarchical_tolerance, .3f,
                                     optimize_);

        std::sprintf(label, "Animation size : %dKB",
                     static_cast<int>(animation_rt_->size() >> 10));

        _im_gui->DoLabel(label);

        if (rebuild) {
          // Delete current animation and rebuild one with the new tolerances.
          ozz::memory::default_allocator()->Delete(animation_rt_);
          animation_rt_ = NULL;

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
      {  // FPS
        char szLabel[64];
        ozz::sample::Record::Statistics stats = error_record_.GetStatistics();
        static bool error_open = true;
        ozz::sample::ImGui::OpenClose oc_stats(_im_gui, "Absolute error",
                                               &error_open);
        if (error_open) {
          std::sprintf(szLabel, "Absolute error: %.2f mm",
                       *error_record_.cursor());
          _im_gui->DoGraph(szLabel, 0.f, stats.max, stats.latest,
                           error_record_.cursor(), error_record_.record_begin(),
                           error_record_.record_end());
        }
      }
    }
    return true;
  }

  virtual void OnDestroy() {
    ozz::memory::Allocator* allocator = ozz::memory::default_allocator();
    allocator->Delete(animation_rt_);
    allocator->Deallocate(locals_rt_);
    allocator->Deallocate(models_rt_);
    allocator->Deallocate(locals_raw_aos_);
    allocator->Deallocate(locals_raw_);
    allocator->Deallocate(models_raw_);
    allocator->Deallocate(locals_diff_);
    allocator->Deallocate(models_diff_);
    allocator->Delete(cache_);
  }

  bool BuildAnimations() {
    assert(!animation_rt_);

    // Instantiate an animation builder.
    ozz::animation::offline::AnimationBuilder animation_builder;

    // Builds the optimized animation.
    if (optimize_) {
      // Optimizes the raw animation.
      ozz::animation::offline::RawAnimation optimized_animation;
      if (!optimizer_(raw_animation_, skeleton_, &optimized_animation)) {
        return false;
      }
      // Builds runtime animation from the optimized one.
      animation_rt_ = animation_builder(optimized_animation);
    } else {
      // Builds runtime animation from the brute one.
      animation_rt_ = animation_builder(raw_animation_);
    }

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

  // Stores the optimizer in order to expose its parameters.
  ozz::animation::offline::AnimationOptimizer optimizer_;

  // Playback animation controller. This is a utility class that helps with
  // controlling animation playback time.
  ozz::sample::PlaybackController controller_;

  // Runtime skeleton.
  ozz::animation::Skeleton skeleton_;

  // Sampling cache, shared across optimized and non-optimized animations. This
  // is not optimal, but it's not an issue either.
  ozz::animation::SamplingCache* cache_;

  // Runtime optimized animation.
  ozz::animation::Animation* animation_rt_;

  // Buffer of local and model space transformations as sampled from the
  // rutime (optimized and compressed) animation.
  ozz::Range<ozz::math::SoaTransform> locals_rt_;
  ozz::Range<ozz::math::Float4x4> models_rt_;

  // Buffer of local and model space transformations as sampled from the
  // non-optimized (raw) animation.
  // Sampling the raw animation results in AoS data, meaning we have to
  // allocate AoS data and do the SoA conversion by hand.
  ozz::Range<ozz::math::Transform> locals_raw_aos_;
  ozz::Range<ozz::math::SoaTransform> locals_raw_;
  ozz::Range<ozz::math::Float4x4> models_raw_;

  // Buffer of local and model space transformations storing samples from the
  // difference between optimized and non-optimized animations.
  ozz::Range<ozz::math::SoaTransform> locals_diff_;
  ozz::Range<ozz::math::Float4x4> models_diff_;

  // Record of accuracy errors produced by animation compression and
  // optimization.
  ozz::sample::Record error_record_;
};

int main(int _argc, const char** _argv) {
  const char* title = "Ozz-animation sample: Animation keyframe optimization";
  return OptimizeSampleApplication().Run(_argc, _argv, "1.0", title);
}
