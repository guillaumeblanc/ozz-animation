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

#include "framework/application.h"
#include "framework/imgui.h"
#include "framework/mesh.h"
#include "framework/renderer.h"
#include "framework/utils.h"
#include "ozz/animation/runtime/animation.h"
#include "ozz/animation/runtime/local_to_model_job.h"
#include "ozz/animation/runtime/sampling_job.h"
#include "ozz/animation/runtime/skeleton.h"
#include "ozz/base/log.h"
#include "ozz/base/maths/math_ex.h"
#include "ozz/base/maths/simd_math.h"
#include "ozz/base/maths/soa_transform.h"
#include "ozz/base/maths/vec_float.h"
#include "ozz/options/options.h"

// Skeleton archive can be specified as an option.
OZZ_OPTIONS_DECLARE_STRING(skeleton,
                           "Path to the skeleton (ozz archive format).",
                           "media/skeleton.ozz", false)

// Animation archive can be specified as an option.
OZZ_OPTIONS_DECLARE_STRING(animation,
                           "Path to the animation (ozz archive format).",
                           "media/animation.ozz", false)

// Mesh archive can be specified as an option.
OZZ_OPTIONS_DECLARE_STRING(mesh,
                           "Path to the skinned mesh (ozz archive format).",
                           "media/mesh.ozz", false)

class SkinningSampleApplication : public ozz::sample::Application {
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
    bool success = true;

    if (draw_skeleton_) {
      success &= _renderer->DrawPosture(skeleton_, make_span(models_),
                                        ozz::math::Float4x4::identity());
    }

    if (draw_mesh_) {
      // Builds skinning matrices, based on the output of the animation stage.
      // The mesh might not use (aka be skinned by) all skeleton joints. We use
      // the joint remapping table (available from the mesh object) to reorder
      // model-space matrices and build skinning ones.
      for (const ozz::sample::Mesh& mesh : meshes_) {
        for (size_t i = 0; i < mesh.joint_remaps.size(); ++i) {
          skinning_matrices_[i] =
              models_[mesh.joint_remaps[i]] * mesh.inverse_bind_poses[i];
        }

        // Renders skin.
        success &= _renderer->DrawSkinnedMesh(
            mesh, make_span(skinning_matrices_),
            ozz::math::Float4x4::identity(), render_options_);
      }
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

    // Skeleton and animation needs to match.
    if (skeleton_.num_joints() != animation_.num_tracks()) {
      ozz::log::Err() << "The provided animation doesn't match skeleton "
                         "(joint count mismatch)."
                      << std::endl;
      return false;
    }

    // Allocates runtime buffers.
    const int num_soa_joints = skeleton_.num_soa_joints();
    locals_.resize(num_soa_joints);
    const int num_joints = skeleton_.num_joints();
    models_.resize(num_joints);

    // Allocates a cache that matches animation requirements.
    cache_.Resize(num_joints);

    // Reading skinned meshes.
    if (!ozz::sample::LoadMeshes(OPTIONS_mesh, &meshes_)) {
      return false;
    }

    // Computes the number of skinning matrices required to skin all meshes.
    // A mesh is skinned by only a subset of joints, so the number of skinning
    // matrices might be less that the number of skeleton joints.
    // Mesh::joint_remaps is used to know how to order skinning matrices. So the
    // number of matrices required is the size of joint_remaps.
    size_t num_skinning_matrices = 0;
    for (const ozz::sample::Mesh& mesh : meshes_) {
      num_skinning_matrices =
          ozz::math::Max(num_skinning_matrices, mesh.joint_remaps.size());
    }

    // Allocates skinning matrices.
    skinning_matrices_.resize(num_skinning_matrices);

    // Check the skeleton matches with the mesh, especially that the mesh
    // doesn't expect more joints than the skeleton has.
    for (const ozz::sample::Mesh& mesh : meshes_) {
      if (num_joints < mesh.highest_joint_index()) {
        ozz::log::Err() << "The provided mesh doesn't match skeleton "
                           "(joint count mismatch)."
                        << std::endl;
        return false;
      }
    }

    return true;
  }

  virtual void OnDestroy() {}

  virtual bool OnGui(ozz::sample::ImGui* _im_gui) {
    // Exposes model informations.
    {
      static bool open = true;
      ozz::sample::ImGui::OpenClose oc(_im_gui, "Model statisitics", &open);
      if (open) {
        char label[255];
        sprintf(label, "%d animated joints", skeleton_.num_joints());
        _im_gui->DoLabel(label);

        int influences = 0;
        for (const auto& mesh : meshes_) {
          influences = ozz::math::Max(influences, mesh.max_influences_count());
        }
        sprintf(label, "%d influences (max)", influences);
        _im_gui->DoLabel(label);

        int vertices = 0;
        for (const auto& mesh : meshes_) {
          vertices += mesh.vertex_count();
        }
        sprintf(label, "%.1fK vertices", vertices / 1000.f);
        _im_gui->DoLabel(label);

        int indices = 0;
        for (const auto& mesh : meshes_) {
          indices += mesh.triangle_index_count();
        }
        sprintf(label, "%.1fK triangles", indices / 3000.f);
        _im_gui->DoLabel(label);
      }
    }

    // Exposes animation runtime playback controls.
    {
      static bool open = true;
      ozz::sample::ImGui::OpenClose oc(_im_gui, "Animation control", &open);
      if (open) {
        controller_.OnGui(animation_, _im_gui);
      }
    }

    // Expose mesh rendering options
    {
      // Rendering options.
      static bool oc_open = false;
      ozz::sample::ImGui::OpenClose oc(_im_gui, "Rendering options", &oc_open);
      if (oc_open) {
        _im_gui->DoCheckBox("Draw skeleton", &draw_skeleton_);
        _im_gui->DoCheckBox("Draw mesh", &draw_mesh_);

        _im_gui->DoCheckBox("Show texture", &render_options_.texture);
        _im_gui->DoCheckBox("Show normals", &render_options_.normals);
        _im_gui->DoCheckBox("Show tangents", &render_options_.tangents);
        _im_gui->DoCheckBox("Show binormals", &render_options_.binormals);
        _im_gui->DoCheckBox("Show colors", &render_options_.colors);
        _im_gui->DoCheckBox("Wireframe", &render_options_.wireframe);
        _im_gui->DoCheckBox("Skip skinning", &render_options_.skip_skinning);
      }
    }
    return true;
  }

  virtual void GetSceneBounds(ozz::math::Box* _bound) const {
    ozz::sample::ComputeSkeletonBounds(skeleton_, _bound);
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

  // Buffer of skinning matrices, result of the joint multiplication of the
  // inverse bind pose with the model space matrix.
  ozz::vector<ozz::math::Float4x4> skinning_matrices_;

  // The mesh used by the sample.
  ozz::vector<ozz::sample::Mesh> meshes_;

  // Redering options.
  bool draw_skeleton_ = false;
  bool draw_mesh_ = true;

  // Mesh rendering options.
  ozz::sample::Renderer::Options render_options_;
};

int main(int _argc, const char** _argv) {
  const char* title = "Ozz-animation sample: Skinning";
  return SkinningSampleApplication().Run(_argc, _argv, "1.0", title);
}
