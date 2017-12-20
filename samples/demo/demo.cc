//----------------------------------------------------------------------------//
//                                                                            //
// ozz-animation is hosted at http://github.com/guillaumeblanc/ozz-animation  //
// and distributed under the MIT License (MIT).                               //
//                                                                            //
// Copyright (c) 2015 Guillaume Blanc                                         //
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
#include "ozz/animation/runtime/blending_job.h"
#include "ozz/animation/runtime/local_to_model_job.h"
#include "ozz/animation/runtime/sampling_job.h"
#include "ozz/animation/runtime/skeleton.h"
#include "ozz/animation/runtime/skeleton_utils.h"

#include "ozz/base/log.h"

#include "ozz/base/containers/vector.h"

#include "ozz/base/maths/simd_math.h"
#include "ozz/base/maths/soa_transform.h"
#include "ozz/base/maths/vec_float.h"

#include "ozz/base/memory/allocator.h"

#include "ozz/options/options.h"

#include "framework/application.h"
#include "framework/imgui.h"
#include "framework/mesh.h"
#include "framework/renderer.h"
#include "framework/utils.h"

#include <cstring>

// Skeleton archive can be specified as an option.
OZZ_OPTIONS_DECLARE_STRING(skeleton,
                           "Path to the skeleton (ozz archive format).",
                           "media/skeleton.ozz", false)

// MAin animation archive can be specified as an option.
OZZ_OPTIONS_DECLARE_STRING(animation,
                           "Path to the main animation(ozz archive format).",
                           "media/animation.ozz", false)

// Mesh archive can be specified as an option.
OZZ_OPTIONS_DECLARE_STRING(mesh,
                           "Path to the skinned mesh (ozz archive format).",
                           "media/mesh.ozz", false)

class DemoApplication : public ozz::sample::Application {
 public:
  DemoApplication() : cache_(NULL), camera_index_(-1) {}

 protected:
  // Updates current animation time.
  virtual bool OnUpdate(float _dt) {
    // Updates animations time.
    controller_.Update(animation_, _dt);

    // Setup sampling job.
    ozz::animation::SamplingJob sampling_job;
    sampling_job.animation = &animation_;
    sampling_job.cache = cache_;
    sampling_job.time = controller_.time();
    sampling_job.output = locals_;

    // Samples animation.
    if (!sampling_job.Run()) {
      return false;
    }

    // Setup local-to-model conversion job.
    ozz::animation::LocalToModelJob ltm_job;
    ltm_job.skeleton = &skeleton_;
    ltm_job.input = locals_;
    ltm_job.output = models_;

    // Run ltm job.
    if (!ltm_job.Run()) {
      return false;
    }

    return true;
  }

  // Renders all meshes.
  virtual bool OnDisplay(ozz::sample::Renderer* _renderer) {
    bool success = true;
    for (size_t m = 0; success && m < meshes_.size(); ++m) {
      const ozz::sample::Mesh& mesh = meshes_[m];

      if (mesh.num_joints() > 1) {  // Mesh requires skinning.
        // Mesh must be compatible with animation/skeleton.
        assert(models_.Count() >=
                   static_cast<size_t>(mesh.highest_joint_index()) &&
               skinning_matrices_.Count() >=
                   static_cast<size_t>(mesh.num_joints()));

        // Builds skinning matrices, based on the output of the animation stage.
        // The mesh might not use (aka be skinned by) all skeleton joints. We
        // use the joint remapping table (available from the mesh object) to
        // reorder skinning matrices.
        for (size_t i = 0; i < mesh.joint_remaps.size(); ++i) {
          skinning_matrices_[i] =
              models_[mesh.joint_remaps[i]] * mesh.inverse_bind_poses[i];
        }

        // Renders skin.
        success &= _renderer->DrawSkinnedMesh(mesh, skinning_matrices_,
                                              ozz::math::Float4x4::identity(),
                                              render_options_);
      } else if (mesh.num_joints() == 1) {
        // Every mesh vertex is transformed by the same joint.
        // It can thus be rendered as static mesh.
        // assert(mesh.joint_remaps.size() == 1);
        // Builds the static mesh transformation matrix as it was a skinning
        // matrix.
        const ozz::math::Float4x4 transform =
            models_[mesh.joint_remaps[0]] * mesh.inverse_bind_poses[0];
        success &= _renderer->DrawMesh(mesh, transform, render_options_);
      } else {
        // Not skinned. Renders it as an untransformed static meshes.
        success &= _renderer->DrawMesh(mesh, ozz::math::Float4x4::identity(),
                                       render_options_);
      }
    }

    return success;
  }

  virtual bool OnInitialize() {
    ozz::memory::Allocator* allocator = ozz::memory::default_allocator();

    // Reading skeleton.
    if (!ozz::sample::LoadSkeleton(OPTIONS_skeleton, &skeleton_)) {
      return false;
    }
    const int num_soa_joints = skeleton_.num_soa_joints();
    const int num_joints = skeleton_.num_joints();

    // Reading skinned mesh.
    if (!ozz::sample::LoadMeshes(OPTIONS_mesh, &meshes_)) {
      return false;
    }

    // The number of joints of the mesh needs to match skeleton.
    for (size_t m = 0; m < meshes_.size(); ++m) {
      const ozz::sample::Mesh& mesh = meshes_[m];

      if (num_joints < mesh.highest_joint_index()) {
        ozz::log::Err() << "The provided mesh doesn't match skeleton "
                           "(joint count mismatch)."
                        << std::endl;
        return false;
      }
    }

    // Reading animation.
    if (!ozz::sample::LoadAnimation(OPTIONS_animation, &animation_)) {
      return false;
    }

    // The number of tracks of the animation needs to match the number of joints
    // of the skeleton.
    if (animation_.num_tracks() < skeleton_.num_joints()) {
      ozz::log::Err() << "The provided animation doesn't match skeleton "
                         "(tracks/joint count mismatch)."
                      << std::endl;
      return false;
    }

    // Allocates runtime buffers.
    locals_ = allocator->AllocateRange<ozz::math::SoaTransform>(num_soa_joints);

    // Allocates a cache that matches animation requirements.
    cache_ = allocator->New<ozz::animation::SamplingCache>(num_joints);

    // Allocates model space runtime buffers .
    models_ = allocator->AllocateRange<ozz::math::Float4x4>(num_joints);
    skinning_matrices_ =
        allocator->AllocateRange<ozz::math::Float4x4>(num_joints);

    // Look for a "camera" joint.
    for (int i = 0; i < num_joints; i++) {
      if (std::strstr(skeleton_.joint_names()[i], "Cam_Joint")) {
        camera_index_ = i;
        break;
      }
    }
    return true;
  }

  virtual void OnDestroy() {
    ozz::memory::Allocator* allocator = ozz::memory::default_allocator();

    allocator->Deallocate(locals_);
    allocator->Delete(cache_);
    allocator->Deallocate(models_);
    allocator->Deallocate(skinning_matrices_);
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
    {
      static bool oc_open = false;
      ozz::sample::ImGui::OpenClose oc(_im_gui, "Rendering options", &oc_open);
      if (oc_open) {
        _im_gui->DoCheckBox("Show texture", &render_options_.texture);
        _im_gui->DoCheckBox("Show normals", &render_options_.normals);
        _im_gui->DoCheckBox("Show tangents", &render_options_.tangents);
        _im_gui->DoCheckBox("Show binormals", &render_options_.binormals);
        _im_gui->DoCheckBox("Show colors", &render_options_.colors);
        _im_gui->DoCheckBox("Skip skinning", &render_options_.skip_skinning);
      }
    }

    return true;
  }

  virtual bool GetCameraOverride(ozz::math::Float4x4* _transform) const {
    // Early out if no camera joint was found.
    if (camera_index_ == -1) {
      return false;
    }

    *_transform = models_[camera_index_];
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

  // Buffer of skinning matrices, result of the joint multiplication of the
  // inverse bind pose with the model space matrix.
  ozz::Range<ozz::math::Float4x4> skinning_matrices_;

  // The mesh used by the sample.
  ozz::Vector<ozz::sample::Mesh>::Std meshes_;

  // Mesh rendering options.
  ozz::sample::Renderer::Options render_options_;

  // Camera joint index. -1 if not found.
  int camera_index_;
};

int main(int _argc, const char** _argv) {
  const char* title = "Ozz-animation demo";
  return DemoApplication().Run(_argc, _argv, "1.0", title);
}
