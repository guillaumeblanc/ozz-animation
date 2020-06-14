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

#include <algorithm>
#include <atomic>
#include <cstdlib>
#include <future>

#include "ozz/animation/runtime/animation.h"
#include "ozz/animation/runtime/local_to_model_job.h"
#include "ozz/animation/runtime/sampling_job.h"
#include "ozz/animation/runtime/skeleton.h"

#include "ozz/base/log.h"

#include "ozz/base/containers/vector.h"

#include "ozz/base/maths/box.h"
#include "ozz/base/maths/math_ex.h"
#include "ozz/base/maths/simd_math.h"
#include "ozz/base/maths/soa_transform.h"
#include "ozz/base/maths/vec_float.h"

#include "ozz/options/options.h"

#include "framework/application.h"
#include "framework/imgui.h"
#include "framework/renderer.h"
#include "framework/utils.h"

#if EMSCRIPTEN
#include <emscripten.h>
#include <emscripten/threading.h>
#endif  // EMSCRIPTEN

// Skeleton archive can be specified as an option.
OZZ_OPTIONS_DECLARE_STRING(skeleton,
                           "Path to the skeleton (ozz archive format).",
                           "media/skeleton.ozz", false)

// Animation archive can be specified as an option.
OZZ_OPTIONS_DECLARE_STRING(animation,
                           "Path to the first animation (ozz archive format).",
                           "media/animation.ozz", false)

// Interval between each character.
const float kInterval = 2.f;

// Width and depth of characters repartition.
const int kWidth = 16;
const int kDepth = 16;

// The maximum number of characters.
const int kMaxCharacters = 4096;

// The minimum number of characters per task.
const int kMinGrainSize = 32;

// Checks if platform has threading support.
bool HasThreadingSupport() {
#ifdef EMSCRIPTEN
  return emscripten_has_threading_support();
#else   // EMSCRIPTEN
  return true;
#endif  // EMSCRIPTEN
}

class MultithreadSampleApplication : public ozz::sample::Application {
 public:
  MultithreadSampleApplication()
      : characters_(kMaxCharacters),
        num_characters_(kMaxCharacters / 4),
        has_threading_support_(HasThreadingSupport()),
        enable_theading_(has_threading_support_),
        grain_size_(128) {
    if (has_threading_support_) {
      ozz::log::Out() << "Platform has threading support." << std::endl;
    } else {
      ozz::log::Out() << "Platform doesn't have threading support, "
                      << "multithreading is disabled." << std::endl;
    }
  }

 private:
  // Nested Character struct forward declaration.
  struct Character;

  static bool UpdateCharacter(const ozz::animation::Animation& _animation,
                              const ozz::animation::Skeleton& _skeleton,
                              float _dt, Character* _character) {
    // Samples animation.
    _character->controller.Update(_animation, _dt);

    // Setup sampling job.
    ozz::animation::SamplingJob sampling_job;
    sampling_job.animation = &_animation;
    sampling_job.cache = &_character->cache;
    sampling_job.ratio = _character->controller.time_ratio();
    sampling_job.output = make_span(_character->locals);

    // Samples animation.
    if (!sampling_job.Run()) {
      return false;
    }

    // Converts from local space to model space matrices.
    ozz::animation::LocalToModelJob ltm_job;
    ltm_job.skeleton = &_skeleton;
    ltm_job.input = make_span(_character->locals);
    ltm_job.output = make_span(_character->models);
    if (!ltm_job.Run()) {
      return false;
    }
    return true;
  }

  // Data structure used to pass const arguments to parallel tasks.
  struct ParallelArgs {
    const ozz::animation::Animation* animation;
    const ozz::animation::Skeleton* skeleton;
    float dt;
    int grain_size;  // Maximum number of characters that can be processed by a
                     // task.
  };

  // Data used to monitor and analyze threading.
  // Every task will push its thread id to an array. We can then process it to
  // find how many threads were used.
  struct ParallelMonitor {
    ParallelMonitor() {
      // Finds the maximum possible number of tasks considering kMinGrain grain
      // size for kMaxCharacters characters.
      int max_tasks = 1;
      for (int processed = kMinGrainSize; processed < kMaxCharacters;
           processed *= 2, max_tasks *= 2) {
      }
      thread_ids_.resize(max_tasks);
      num_async_tasks.store(0);
    }
    ozz::vector<std::thread::id> thread_ids_;
    std::atomic_uint num_async_tasks;
  };

  static bool ParallelUpdate(const ParallelArgs& _args, Character* _characters,
                             int _num, ParallelMonitor* _monitor) {
    bool success = true;
    if (_num <= _args.grain_size) {
      // Stores this thread identifier to a new task slot.
      _monitor->thread_ids_[_monitor->num_async_tasks++] =
          std::this_thread::get_id();

      for (int i = 0; i < _num; ++i) {
        success &= UpdateCharacter(*_args.animation, *_args.skeleton, _args.dt,
                                   &_characters[i]);
      }
    } else {
      // Run half the job on an async task, possibly another thread.
      const int half = _num / 2;
      auto handle = std::async(std::launch::async, ParallelUpdate, _args,
                               _characters + half, _num - half, _monitor);

      // The other half is processed by this thread.
      success &= ParallelUpdate(_args, _characters, half, _monitor);

      // Get result of the async task or process it if it's not started yet.
      success &= handle.get();
    }
    return success;
  }

  // Updates current animation time.
  virtual bool OnUpdate(float _dt, float) {
    bool success = true;
    if (enable_theading_) {
      // Initialize task counter. It's only used to monitor threading behavior.
      monitor_.num_async_tasks.store(0);

      const ParallelArgs args = {&animation_, &skeleton_, _dt, grain_size_};
      success = ParallelUpdate(args, array_begin(characters_), num_characters_,
                               &monitor_);
    } else {
      for (int i = 0; i < num_characters_; ++i) {
        success &= UpdateCharacter(animation_, skeleton_, _dt,
                                   array_begin(characters_) + i);
      }
    }
    return success;
  }

  // Renders all skeletons.
  virtual bool OnDisplay(ozz::sample::Renderer* _renderer) {
    bool success = true;
    for (int c = 0; success && c < num_characters_; ++c) {
      const ozz::math::Float4 position(
          ((c % kWidth) - kWidth / 2) * kInterval,
          ((c / kWidth) / kDepth) * kInterval,
          (((c / kWidth) % kDepth) - kDepth / 2) * kInterval, 1.f);
      const ozz::math::Float4x4 transform = ozz::math::Float4x4::Translation(
          ozz::math::simd_float4::LoadPtrU(&position.x));
      success &= _renderer->DrawPosture(
          skeleton_, make_span(characters_[c].models), transform, false);
    }

    return true;
  }

  virtual bool OnInitialize() {
    // Reading skeleton.
    if (!ozz::sample::LoadSkeleton(OPTIONS_skeleton, &skeleton_)) {
      return false;
    }

    // Reading animations.
    if (!ozz::sample::LoadAnimation(OPTIONS_animation, &animation_)) {
      return false;
    }

    // Allocate a default number of characters.
    AllocateCharaters();

    return true;
  }

  bool AllocateCharaters() {
    // Reallocate all characters.
    for (size_t c = 0; c < kMaxCharacters; ++c) {
      Character& character = characters_[c];

      // Initializes each controller start time to a different value.
      character.controller.set_time_ratio(animation_.duration() * kWidth * c /
                                          kMaxCharacters);

      character.locals.resize(skeleton_.num_soa_joints());
      character.models.resize(skeleton_.num_joints());
      character.cache.Resize(animation_.num_tracks());
    }

    return true;
  }

  virtual void OnDestroy() {}

  virtual bool OnGui(ozz::sample::ImGui* _im_gui) {
    // Exposes number of characters.
    {
      static bool oc_open = true;
      ozz::sample::ImGui::OpenClose oc(_im_gui, "Sample control", &oc_open);
      if (oc_open) {
        char label[64];
        std::sprintf(label, "Number of entities: %d", num_characters_);
        _im_gui->DoSlider(label, 1, kMaxCharacters, &num_characters_, .7f);
        const int num_joints = num_characters_ * skeleton_.num_joints();
        std::sprintf(label, "Number of joints: %d", num_joints);
        _im_gui->DoLabel(label);
      }
    }
    // Exposes multi-threading parameters.
    {
      static bool oc_open = true;
      ozz::sample::ImGui::OpenClose oc(_im_gui, "Threading control", &oc_open);
      if (oc_open) {
        _im_gui->DoCheckBox("Enables threading", &enable_theading_,
                            has_threading_support_);
        if (enable_theading_) {
          char label[64];
          std::sprintf(label, "Grain size: %d", grain_size_);
          _im_gui->DoSlider(label, kMinGrainSize, kMaxCharacters, &grain_size_,
                            .2f);

          // Finds number of threads, which is the number of unique thread ids
          // found.
          std::sort(monitor_.thread_ids_.begin(),
                    monitor_.thread_ids_.begin() + monitor_.num_async_tasks);
          auto end = std::unique(
              monitor_.thread_ids_.begin(),
              monitor_.thread_ids_.begin() + monitor_.num_async_tasks);
          const int num_threads =
              static_cast<int>(end - monitor_.thread_ids_.begin());
          std::sprintf(label, "Thread/task count: %d/%d", num_threads,
                       monitor_.num_async_tasks.load());
          _im_gui->DoLabel(label);
        }
      }
    }
    return true;
  }

  virtual void GetSceneBounds(ozz::math::Box* _bound) const {
    _bound->min.x = -(kWidth / 2) * kInterval;
    _bound->max.x =
        _bound->min.x + ozz::math::Min(num_characters_, kWidth) * kInterval;
    _bound->min.y = 0.f;
    _bound->max.y = ((num_characters_ / kWidth / kDepth) + 1) * kInterval;
    _bound->min.z = -(kDepth / 2) * kInterval;
    _bound->max.z =
        _bound->min.z +
        ozz::math::Min(num_characters_ / kWidth, kDepth) * kInterval;
  }

  // Runtime skeleton.
  ozz::animation::Skeleton skeleton_;

  // Runtime animation.
  ozz::animation::Animation animation_;

  // Character structure contains all the data required to sample and blend a
  // character.
  struct Character {
    // Playback animation controller. This is a utility class that helps with
    // controlling animation playback time.
    ozz::sample::PlaybackController controller;

    // Sampling cache.
    ozz::animation::SamplingCache cache;

    // Buffer of local transforms which stores the blending result.
    ozz::vector<ozz::math::SoaTransform> locals;

    // Buffer of model space matrices. These are computed by the local-to-model
    // job after the blending stage.
    ozz::vector<ozz::math::Float4x4> models;
  };

  // Array of characters of the sample.
  ozz::vector<Character> characters_;

  // Number of used characters.
  int num_characters_;

  // Does the current plateform actually has threading support.
  bool has_threading_support_;

  // Enable or disable threading.
  bool enable_theading_;

  // Define the number of characters that a task can handle.
  int grain_size_;

  // Data used to monitor and analyze threading.
  ParallelMonitor monitor_;
};

int main(int _argc, const char** _argv) {
  const char* title = "Ozz-animation sample: Multi-threading";
  return MultithreadSampleApplication().Run(_argc, _argv, "1.0", title);
}
