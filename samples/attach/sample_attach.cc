//============================================================================//
//                                                                            //
// ozz-animation, 3d skeletal animation libraries and tools.                  //
// https://code.google.com/p/ozz-animation/                                   //
//                                                                            //
//----------------------------------------------------------------------------//
//                                                                            //
// Copyright (c) 2012-2014 Guillaume Blanc                                    //
//                                                                            //
// This software is provided 'as-is', without any express or implied          //
// warranty. In no event will the authors be held liable for any damages      //
// arising from the use of this software.                                     //
//                                                                            //
// Permission is granted to anyone to use this software for any purpose,      //
// including commercial applications, and to alter it and redistribute it     //
// freely, subject to the following restrictions:                             //
//                                                                            //
// 1. The origin of this software must not be misrepresented; you must not    //
// claim that you wrote the original software. If you use this software       //
// in a product, an acknowledgment in the product documentation would be      //
// appreciated but is not required.                                           //
//                                                                            //
// 2. Altered source versions must be plainly marked as such, and must not be //
// misrepresented as being the original software.                             //
//                                                                            //
// 3. This notice may not be removed or altered from any source               //
// distribution.                                                              //
//                                                                            //
//============================================================================//

#include "ozz/animation/runtime/skeleton.h"
#include "ozz/animation/runtime/animation.h"
#include "ozz/animation/runtime/sampling_job.h"
#include "ozz/animation/runtime/local_to_model_job.h"

#include "ozz/base/log.h"

#include "ozz/base/memory/allocator.h"

#include "ozz/base/maths/vec_float.h"
#include "ozz/base/maths/simd_math.h"
#include "ozz/base/maths/box.h"
#include "ozz/base/maths/soa_transform.h"

#include "ozz/options/options.h"

#include "framework/application.h"
#include "framework/renderer.h"
#include "framework/imgui.h"
#include "framework/utils.h"

// Skeleton archive can be specified as an option.
OZZ_OPTIONS_DECLARE_STRING(
  skeleton,
  "Path to the skeleton (ozz archive format).",
  "media/skeleton.ozz",
  false)

// Animation archive can be specified as an option.
OZZ_OPTIONS_DECLARE_STRING(
  animation,
  "Path to the animation (ozz archive format).",
  "media/animation.ozz",
  false)

class LoadAttachApplication : public ozz::sample::Application {
 public:
  LoadAttachApplication()
    : cache_(NULL),
      attachment_(0),
      offset_(.12f, .01f, -.445f) {
    set_auto_framing(true);
  }

 protected:
  // Updates current animation time.
  virtual bool OnUpdate(float _dt) {
    // Updates current animation time.
    controller_.Update(animation_, _dt);

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

    return true;
  }

  // Samples animation, transforms to model space and renders.
  virtual bool OnDisplay(ozz::sample::Renderer* _renderer) {
    if (!_renderer->DrawPosture(skeleton_,
                                models_,
                                ozz::math::Float4x4::identity())) {
      return false;
    }

    // Prepares attached object transformation.
    // Gets model space transformation of the joint.
    const ozz::math::Float4x4& joint = models_[attachment_];

    // Builds offset transformation matrix.
    const ozz::math::SimdFloat4 translation =
      ozz::math::simd_float4::Load3PtrU(&offset_.x);

    // Concatenates joint and offset transformations.
    const ozz::math::Float4x4 transform =
      joint * ozz::math::Float4x4::Translation(translation);

    // Prepare rendering.
    const float thickness = .01f;
    const float length = .5f;
    const ozz::math::Box box(ozz::math::Float3(-thickness, -thickness, 0.f),
                             ozz::math::Float3(thickness, thickness, length));
    const ozz::sample::Renderer::Color colors[2] = {
      {0xff, 0, 0, 0xff}, {0, 0xff, 0, 0xff}};

    return _renderer->DrawBox(box, transform, colors);
  }

  virtual bool OnInitialize() {
    ozz::memory::Allocator* allocator = ozz::memory::default_allocator();

    // Reading skeleton.
    if (!ozz::sample::LoadSkeleton(OPTIONS_skeleton, &skeleton_)) {
        return false;
    }

    // Reading animation.
    if (!ozz::sample::LoadAnimation(OPTIONS_animation, &animation_)) {
        return false;
    }

    // Allocates runtime buffers.
    const int num_joints = skeleton_.num_joints();
    const int num_soa_joints = skeleton_.num_soa_joints();
    locals_ = allocator->AllocateRange<ozz::math::SoaTransform>(num_soa_joints);
    models_ = allocator->AllocateRange<ozz::math::Float4x4>(num_joints);

    // Allocates a cache that matches animation requirements.
    cache_ = allocator->New<ozz::animation::SamplingCache>(num_joints);

    // Finds the joint where the object should be attached.
    for (int i = 0; i < num_joints; ++i) {
      if (std::strstr(skeleton_.joint_names()[i], "Hand")) {
        attachment_ = i;
        break;
      }
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
    // Exposes animation runtime playback controls.
    {
      static bool open = true;
      ozz::sample::ImGui::OpenClose oc(_im_gui, "Animation control", &open);
      if (open) {
        controller_.OnGui(animation_, _im_gui);
      }
    }

    // Exposes selection of the attachment joint.
    {
      static bool open = true;
      ozz::sample::ImGui::OpenClose oc(_im_gui, "Attachment joint", &open);
      if (open && skeleton_.num_joints() != 0) {
        _im_gui->DoLabel("Select joint:");
        char label[64];
        std::sprintf(label, "%s (%d)",
                     skeleton_.joint_names()[attachment_],
                     attachment_);
        _im_gui->DoSlider(label,
                          0, skeleton_.num_joints() - 1,
                          &attachment_);

        _im_gui->DoLabel("Attachment offset:");
        sprintf(label, "x: %02f", offset_.x);
        _im_gui->DoSlider(label, -1.f, 1.f, &offset_.x);
        sprintf(label, "y: %02f", offset_.y);
        _im_gui->DoSlider(label, -1.f, 1.f, &offset_.y);
        sprintf(label, "z: %02f", offset_.z);
        _im_gui->DoSlider(label, -1.f, 1.f, &offset_.z);
      }
    }

    return true;
  }

  virtual bool GetSceneBounds(ozz::math::Box* _bound) const {
    return ozz::sample::ComputePostureBounds(models_, _bound);
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

  // Joint where the object is attached.
  int attachment_;

  // Offset, translation of the attached object from the joint.
  ozz::math::Float3 offset_;
};

int main(int _argc, const char** _argv) {
  const char* title =
    "Ozz-animation sample: Attachment to animated skeleton joints";
  return LoadAttachApplication().Run(_argc, _argv, "1.0", title);
}
