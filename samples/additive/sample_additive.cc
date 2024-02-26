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

#include <cstring>

#include "framework/application.h"
#include "framework/imgui.h"
#include "framework/renderer.h"
#include "framework/utils.h"
#include "ozz/animation/runtime/animation.h"
#include "ozz/animation/runtime/blending_job.h"
#include "ozz/animation/runtime/local_to_model_job.h"
#include "ozz/animation/runtime/sampling_job.h"
#include "ozz/animation/runtime/skeleton.h"
#include "ozz/animation/runtime/skeleton_utils.h"
#include "ozz/base/containers/vector.h"
#include "ozz/base/log.h"
#include "ozz/base/maths/box.h"
#include "ozz/base/maths/simd_math.h"
#include "ozz/base/maths/soa_transform.h"
#include "ozz/base/maths/vec_float.h"
#include "ozz/options/options.h"

// Skeleton archive can be specified as an option.
OZZ_OPTIONS_DECLARE_STRING(skeleton,
                           "Path to the skeleton (ozz archive format).",
                           "media/skeleton.ozz", false)

// MAin animation archive can be specified as an option.
OZZ_OPTIONS_DECLARE_STRING(animation,
                           "Path to the main animation(ozz archive format).",
                           "media/animation_base.ozz", false)

// Additive animation archive can be specified as an option.
OZZ_OPTIONS_DECLARE_STRING(
    splay_animation,
    "Path to the additive splay animation (ozz archive format).",
    "media/animation_splay_additive.ozz", false)
OZZ_OPTIONS_DECLARE_STRING(
    curl_animation, "Path to the additive curl animation (ozz archive format).",
    "media/animation_curl_additive.ozz", false)

class AdditiveBlendSampleApplication : public ozz::sample::Application {
 public:
  AdditiveBlendSampleApplication()
      : base_weight_(0.f),
        additive_weigths_{.3f, .9f},
        auto_animate_weights_(true) {}

 protected:
  // Updates current animation time and skeleton pose.
  virtual bool OnUpdate(float _dt, float) {
    // For the sample purpose, animates additive weights automatically so the
    // hand moves.
    if (auto_animate_weights_) {
      AnimateWeights(_dt);
    }

    // Updates base animation time for main animation.
    controller_.Update(base_animation_, _dt);

    // Setup sampling job.
    ozz::animation::SamplingJob sampling_job;
    sampling_job.animation = &base_animation_;
    sampling_job.context = &context_;
    sampling_job.ratio = controller_.time_ratio();
    sampling_job.output = make_span(locals_);

    // Samples animation.
    if (!sampling_job.Run()) {
      return false;
    }

    // Setups blending job layers.

    // Main animation is used as-is.
    ozz::animation::BlendingJob::Layer layers[1];
    layers[0].transform = make_span(locals_);
    layers[0].weight = base_weight_;
    layers[0].joint_weights = make_span(base_joint_weights_);

    // The two additive layers (curl and splay) are blended on top of the main
    // layer.
    ozz::animation::BlendingJob::Layer additive_layers[kNumLayers];
    for (size_t i = 0; i < kNumLayers; ++i) {
      additive_layers[i].transform = make_span(additive_locals_[i]);
      additive_layers[i].weight = additive_weigths_[i];
    }

    // Setups blending job.
    ozz::animation::BlendingJob blend_job;
    blend_job.layers = layers;
    blend_job.additive_layers = additive_layers;
    blend_job.rest_pose = skeleton_.joint_rest_poses();
    blend_job.output = make_span(blended_locals_);

    // Blends.
    if (!blend_job.Run()) {
      return false;
    }

    // Gets the output of the blending stage, and converts it to model space.

    // Setup local-to-model conversion job.
    ozz::animation::LocalToModelJob ltm_job;
    ltm_job.skeleton = &skeleton_;
    ltm_job.input = make_span(blended_locals_);
    ltm_job.output = make_span(models_);

    // Run ltm job.
    if (!ltm_job.Run()) {
      return false;
    }

    return true;
  }

  void AnimateWeights(float _dt) {
    static float t = 0.f;
    t += _dt;
    additive_weigths_[0] = .5f + std::cos(t * 1.7f) * .5f;
    additive_weigths_[1] = .5f + std::cos(t * 2.5f) * .5f;
  }

  // Samples animation, transforms to model space and renders.
  virtual bool OnDisplay(ozz::sample::Renderer* _renderer) {
    return _renderer->DrawPosture(skeleton_, make_span(models_),
                                  ozz::math::Float4x4::identity());
  }

  bool SetJointWeights(const char* _name, float _weight) {
    const auto set_joint = [this, _weight](int _joint, int) {
      ozz::math::SimdFloat4& soa_weight = base_joint_weights_[_joint / 4];
      soa_weight = ozz::math::SetI(
          soa_weight, ozz::math::simd_float4::Load1(_weight), _joint % 4);
    };

    const int joint = FindJoint(skeleton_, _name);
    if (joint >= 0) {
      ozz::animation::IterateJointsDF(skeleton_, set_joint, joint);
      return true;
    }
    return false;
  }

  virtual bool OnInitialize() {
    // Reading skeleton.
    if (!ozz::sample::LoadSkeleton(OPTIONS_skeleton, &skeleton_)) {
      return false;
    }
    const int num_soa_joints = skeleton_.num_soa_joints();
    const int num_joints = skeleton_.num_joints();

    // Reads base animation.
    if (!ozz::sample::LoadAnimation(OPTIONS_animation, &base_animation_)) {
      return false;
    }

    if (num_joints != base_animation_.num_tracks()) {
      return false;
    }

    // Allocates sampling context.
    context_.Resize(num_joints);

    // Allocates local space runtime buffers for base animation.
    locals_.resize(num_soa_joints);

    // Allocates model space runtime buffers of blended data.
    models_.resize(num_joints);

    // Storage for blending stage output.
    blended_locals_.resize(num_soa_joints);

    // Allocates and sets base animation mask weights to one.
    base_joint_weights_.resize(num_soa_joints, ozz::math::simd_float4::one());
    SetJointWeights("Lefthand", 0.f);
    SetJointWeights("RightHand", 0.f);

    // Reads and extract additive animations pose.
    const char* filenames[] = {OPTIONS_splay_animation, OPTIONS_curl_animation};
    for (int i = 0; i < kNumLayers; ++i) {
      // Reads animation on the stack as it won't need to be maintained in
      // memory. Only the pose is needed.
      ozz::animation::Animation animation;
      if (!ozz::sample::LoadAnimation(filenames[i], &animation)) {
        return false;
      }

      if (num_joints != animation.num_tracks()) {
        return false;
      }

      // Allocates additive poses, aka buffers of Soa tranforms.
      additive_locals_[i].resize(num_soa_joints);

      // Samples the first frame pose.
      ozz::animation::SamplingJob sampling_job;
      sampling_job.animation = &animation;
      sampling_job.context = &context_;
      sampling_job.ratio = 0.f;  // Only needs the first frame pose
      sampling_job.output = make_span(additive_locals_[i]);

      // Samples animation.
      if (!sampling_job.Run()) {
        return false;
      }

      // Invalidates context which will be re-used for another animation.
      // This is usually not needed, animation address on the stack is the same
      // each loop, hence creating an issue as animation content is changing.
      context_.Invalidate();
    }

    return true;
  }

  virtual void OnDestroy() {}

  virtual bool OnGui(ozz::sample::ImGui* _im_gui) {
    char label[64];

    // Exposes blending parameters.
    {
      static bool open = true;
      ozz::sample::ImGui::OpenClose oc(_im_gui, "Blending parameters", &open);
      if (open) {
        _im_gui->DoLabel("Main layer:");
        std::snprintf(label, sizeof(label), "Layer weight: %.2f", base_weight_);
        _im_gui->DoSlider(label, 0.f, 1.f, &base_weight_, 1.f);

        _im_gui->DoLabel("Additive layer:");
        _im_gui->DoCheckBox("Animates weights", &auto_animate_weights_);
        ozz::array<float, OZZ_ARRAY_SIZE(additive_weigths_)> weights;
        std::memcpy(weights.data(), additive_weigths_,
                    sizeof(additive_weigths_));

        std::snprintf(label, sizeof(label), "Weights\nCurl: %.2f\nSplay: %.2f",
                      additive_weigths_[kCurl], additive_weigths_[kSplay]);
        if (_im_gui->DoSlider2D(label, {{0.f, 0.f}}, {{1.f, 1.f}}, &weights)) {
          auto_animate_weights_ = false;  // User interacted.
          std::memcpy(additive_weigths_, weights.data(),
                      sizeof(additive_weigths_));
        }
      }
    }

    // Exposes base animation runtime playback controls.
    {
      static bool oc_open = true;
      ozz::sample::ImGui::OpenClose oc(_im_gui, "Animation control", &oc_open);
      if (oc_open) {
        controller_.OnGui(base_animation_, _im_gui);
      }
    }

    return true;
  }

  virtual void GetSceneBounds(ozz::math::Box* _bound) const {
    // Finds the "hand" joint in the joint hierarchy.
    const int hand = FindJoint(skeleton_, "Lefthand");

    // Creates a bounding volume around the hand.
    if (hand != -1) {
      const ozz::math::Float4x4& hand_matrix = models_[hand];
      ozz::math::Float3 hand_position;
      ozz::math::Store3PtrU(hand_matrix.cols[3], &hand_position.x);
      const ozz::math::Float3 extent(.15f);
      _bound->min = hand_position - extent;
      _bound->max = hand_position + extent;
    } else {
      ozz::sample::ComputePostureBounds(make_span(models_), _bound);
    }
  }

 private:
  // Runtime skeleton.
  ozz::animation::Skeleton skeleton_;

  // The number of additive layers to blend.
  enum { kSplay, kCurl, kNumLayers };

  // Runtime animation.
  ozz::animation::Animation base_animation_;

  // Per-joint weights used to define the base animation mask. Allows to remove
  // hands from base animations.
  ozz::vector<ozz::math::SimdFloat4> base_joint_weights_;

  // Main animation controller. This is a utility class that helps with
  // controlling animation playback time.
  ozz::sample::PlaybackController controller_;

  // Sampling context.
  ozz::animation::SamplingJob::Context context_;

  // Buffer of local transforms as sampled from main animation_.
  ozz::vector<ozz::math::SoaTransform> locals_;

  // Blending weight of the base animation layer.
  float base_weight_;

  // Poses of local transforms as sampled from curl and splay animations.
  // They are sampled during initialization, as a single pose is used.
  ozz::vector<ozz::math::SoaTransform> additive_locals_[kNumLayers];

  // Blending weight of the additive animation layer.
  float additive_weigths_[kNumLayers];

  // Buffer of local transforms which stores the blending result.
  ozz::vector<ozz::math::SoaTransform> blended_locals_;

  // Buffer of model space matrices. These are computed by the local-to-model
  // job after the blending stage.
  ozz::vector<ozz::math::Float4x4> models_;

  // Automatically animates additive weights.
  bool auto_animate_weights_;
};

int main(int _argc, const char** _argv) {
  const char* title = "Ozz-animation sample: Additive animations blending";
  return AdditiveBlendSampleApplication().Run(_argc, _argv, "1.0", title);
}
