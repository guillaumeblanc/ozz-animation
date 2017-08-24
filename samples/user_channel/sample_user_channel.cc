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

// Some atlas (the ball) constants.
const float kAtlasRadius = .3f;
const ozz::math::SimdFloat4 kAtlasOffsetToBone =
    ozz::math::simd_float4::Load(0.f, kAtlasRadius, 0.f, 0.f);
const ozz::math::SimdFloat4 kAtlasInitialPosition =
    ozz::math::simd_float4::Load(0.f, kAtlasRadius, .1f, 0.f);
const ozz::math::SimdFloat4 kAtlasFinalPosition =
    ozz::math::simd_float4::Load(0.f, 1.f + kAtlasRadius, -2.f, 0.f);
const ozz::math::Box kPedestalBox(ozz::math::Float3(0.f, 0.f, 0.f),
                                  ozz::math::Float3(1.f, 1.f, 1.f));
const ozz::sample::Renderer::Color kAtlasColor = {0x80, 0x80, 0x80, 0xff};

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
        attached_(false),
        hand_joint_(0),
        atlas_transform_(
            ozz::math::Float4x4::Translation(kAtlasInitialPosition)) {}

 protected:
  virtual bool OnUpdate(float _dt) {
    // Updates current animation time.
    bool looped = controller_.Update(animation_, _dt);

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

    // Reset atlas position to its initial location. Note that it depends on
    // animation playback direction.
    if (looped) {
      if (_dt * controller_.playback_speed() >= 0.f) {
        atlas_transform_ =
            ozz::math::Float4x4::Translation(kAtlasInitialPosition);
      } else {
        atlas_transform_ =
            ozz::math::Float4x4::Translation(kAtlasFinalPosition);
      }
    }

    // Samples the track in order to know if the atlas ball should be attached
    // to the skeleton joint (hand).
    ozz::animation::FloatTrackSamplingJob track_sampling_job;

    // Tracks have a unit length duration. They are thus sampled with a ratio
    // (rather than a time), which is computed based on the duration of the
    // animation  they refer to.
    track_sampling_job.time = controller_.time() / animation_.duration();
    track_sampling_job.track = &track_;
    float attached;
    track_sampling_job.result = &attached;
    if (!track_sampling_job.Run()) {
      return false;
    }

    // Sampling the FloatTrack returns a float, which can be interpreted to a
    // bool.
    attached_ = attached != 0.f;

    // Updates the atlas ball transform if it is attached to the hand joint.
    // Otherwise let it where it is.
    if (attached_) {
      atlas_transform_ = Translate(models_[hand_joint_], kAtlasOffsetToBone);
    }

    return true;
  }

  // Samples animation, transforms to model space and renders.
  virtual bool OnDisplay(ozz::sample::Renderer* _renderer) {
    bool success = true;

    // Draw atlas ball at the position computed during update.
    success &= _renderer->DrawSphereShaded(kAtlasRadius, atlas_transform_,
                                           kAtlasColor);

    // Draws a sphere at hand position, which shows "attached" flag status.
    const ozz::sample::Renderer::Color colors[]{{0xff, 0, 0, 0xff},
                                                {0, 0xff, 0, 0xff}};
    _renderer->DrawSphereIm(.02f, models_[hand_joint_], colors[attached_]);

    // Draws the background pedestal.
    success &= _renderer->DrawBoxShaded(
        kPedestalBox, ozz::math::Float4x4::identity(), kAtlasColor);

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

    // Finds the hand joint where the atlas ball should be attached.
    // If not found, let it be 0.
    for (int i = 0; i < skeleton_.num_joints(); i++) {
      if (std::strstr(skeleton_.joint_names()[i], "LeftHandMiddle")) {
        hand_joint_ = i;
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
    //     if (!ozz::sample::LoadTrack(OPTIONS_track, &track_)) {
    //       return false;
    //     }

    return true;
  }

  virtual void OnDestroy() {
    ozz::memory::Allocator* allocator = ozz::memory::default_allocator();
    allocator->Deallocate(locals_);
    allocator->Deallocate(models_);
    allocator->Delete(cache_);
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
  // Stores whether the atlas ball should be attached to the hand.
  ozz::animation::FloatTrack track_;

  // Stores whether the atlas ball is attached to the hand. This flag is
  // computed during update. This is only used for debug display purpose.
  bool attached_;

  // Index of the hand joint, where the atlas ball must be attached.
  int hand_joint_;

  // Atlas (the ball) current transformation.
  ozz::math::Float4x4 atlas_transform_;
};

int main(int _argc, const char** _argv) {
  const char* title = "Ozz-animation sample: User channels";
  return LoadSampleApplication().Run(_argc, _argv, "1.0", title);
}
