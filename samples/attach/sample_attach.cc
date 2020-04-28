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

#include "ozz/base/log.h"

#include "ozz/base/maths/box.h"
#include "ozz/base/maths/simd_math.h"
#include "ozz/base/maths/soa_transform.h"
#include "ozz/base/maths/vec_float.h"

#include "ozz/options/options.h"

#include "framework/application.h"
#include "framework/imgui.h"
#include "framework/renderer.h"
#include "framework/utils.h"

// Skeleton archive can be specified as an option.
OZZ_OPTIONS_DECLARE_STRING(skeleton,
                           "Path to the skeleton (ozz archive format).",
                           "media/skeleton.ozz", false)

// Animation archive can be specified as an option.
OZZ_OPTIONS_DECLARE_STRING(animation,
                           "Path to the animation (ozz archive format).",
                           "media/animation.ozz", false)

class AttachSampleApplication : public ozz::sample::Application {
 public:
  AttachSampleApplication() : attachment_(0), offset_(-.02f, .03f, .05f) {}

 protected:
  // Updates current animation time and skeleton pose.
  virtual bool OnUpdate(float _dt, float) {
    // Updates current animation time.
    controller_.Update(animation_, _dt);

    // Samples optimized animation at t = animation_time_.
    ozz::animation::SamplingJob sampling_job;
    sampling_job.animation = &animation_;
    sampling_job.cache = &cache_;
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

  // Samples animation, transforms to model space and renders.
  virtual bool OnDisplay(ozz::sample::Renderer* _renderer) {
    if (!_renderer->DrawPosture(skeleton_, make_span(models_),
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
    const ozz::math::Box box(ozz::math::Float3(-thickness, -thickness, -length),
                             ozz::math::Float3(thickness, thickness, 0.f));
    const ozz::sample::Color colors[2] = {ozz::sample::kRed,
                                          ozz::sample::kGreen};

    return _renderer->DrawBoxIm(box, transform, colors);
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

    // Allocates runtime buffers.
    const int num_joints = skeleton_.num_joints();
    const int num_soa_joints = skeleton_.num_soa_joints();
    locals_.resize(num_soa_joints);
    models_.resize(num_joints);

    // Allocates a cache that matches animation requirements.
    cache_.Resize(num_joints);

    // Finds the joint where the object should be attached.
    for (int i = 0; i < num_joints; i++) {
      if (std::strstr(skeleton_.joint_names()[i], "LeftHandMiddle")) {
        attachment_ = i;
        break;
      }
    }

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

    // Exposes selection of the attachment joint.
    {
      static bool open = true;
      ozz::sample::ImGui::OpenClose oc(_im_gui, "Attachment joint", &open);
      if (open && skeleton_.num_joints() != 0) {
        _im_gui->DoLabel("Select joint:");
        char label[64];
        std::sprintf(label, "%s (%d)", skeleton_.joint_names()[attachment_],
                     attachment_);
        _im_gui->DoSlider(label, 0, skeleton_.num_joints() - 1, &attachment_);

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

  // Joint where the object is attached.
  int attachment_;

  // Offset, translation of the attached object from the joint.
  ozz::math::Float3 offset_;
};

int main(int _argc, const char** _argv) {
  const char* title =
      "Ozz-animation sample: Attachment to animated skeleton joints";
  return AttachSampleApplication().Run(_argc, _argv, "1.0", title);
}
