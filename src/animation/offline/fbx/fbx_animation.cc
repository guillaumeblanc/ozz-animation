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

#define OZZ_INCLUDE_PRIVATE_HEADER  // Allows to include private headers.

#include "animation/offline/fbx/fbx_animation.h"

#include "ozz/animation/runtime/skeleton.h"
#include "ozz/animation/offline/animation_builder.h"

#include "ozz/base/log.h"

#include "ozz/base/maths/transform.h"

namespace ozz {
namespace animation {
namespace offline {
namespace fbx {

namespace {
bool ExtractAnimation(FbxScene* _scene,
                      FbxAnimStack* anim_stack,
                      const Skeleton& _skeleton,
                      float _sampling_rate,
                      RawAnimation* _animation) {
  // Setup Fbx animation evaluator.
  _scene->SetCurrentAnimationStack(anim_stack);
  FbxAnimEvaluator* evaluator = _scene->GetAnimationEvaluator();

  // Initialize animation duration.
  const float start = static_cast<float>(
    anim_stack->GetLocalTimeSpan().GetStart().GetSecondDouble());
  const float end = static_cast<float>(
    anim_stack->GetLocalTimeSpan().GetStop().GetSecondDouble());
  _animation->duration = end - start;

  // Allocates all tracks with the same number of tracks as the skeleton.
  // Tracks that would not be found will remain empty (identity transformation).
  _animation->tracks.resize(_skeleton.num_joints());

  // Iterate all skeleton joints and fills there track with key frames.
  for (int i = 0; i < _skeleton.num_joints(); i++) {
    RawAnimation::JointTrack& track = _animation->tracks[i];

    // Find a node that matches skeleton joint.
    FbxNode* node = NULL;
    const char* joint_name = _skeleton.joint_names()[i];
    for (int j = 0; j < _scene->GetNodeCount(); j++) {
      FbxNode* node_j = _scene->GetNode(j);
      if (strcmp(node_j->GetName(), joint_name) == 0) {
        node = node_j;
        break;
      }
    }

    if (!node) {
      // Empty joint track.
      ozz::log::LogV() << "No animation track found for joint \"" << joint_name
        << "\"." << std::endl;
      continue;
    }

    // Reserve keys in animation tracks (allocation strategy optimization
    // purpose).
    const float sampling_period = 1.f / _sampling_rate;
    const int max_keys =
      static_cast<int>(1.f + (end - start) / sampling_period);
    track.translations.reserve(max_keys);
    track.rotations.reserve(max_keys);
    track.scales.reserve(max_keys);

    // Evaluate joint transformation at the specified time.
    // Make sure to include "end" time.
    bool loop_again = true;
    for (float t = start; loop_again; t += sampling_period) {
      if (t >= end) {
        t = end;
        loop_again = false;
      }

      // Evaluate local transform at fbx_time.
      const FbxAMatrix matrix =
        evaluator->GetNodeLocalTransform(node, FbxTimeSeconds(t));
      ozz::math::Transform transform;

      // Converts to ozz transformation format.
      FbxAMatrixToTransform(matrix, &transform);

      // Fills corresponding track.
      const float local_time = t - start;
      const RawAnimation::TranslationKey translation = {
        local_time, transform.translation};
      track.translations.push_back(translation);
      const RawAnimation::RotationKey rotation = {
        local_time, transform.rotation};
      track.rotations.push_back(rotation);
      const RawAnimation::ScaleKey scale = {
        local_time, transform.scale};
      track.scales.push_back(scale);
    }
  }

  return true;
}
}

bool ExtractAnimation(FbxScene* _scene,
                      const Skeleton& _skeleton,
                      float _sampling_rate,
                      RawAnimation* _animation) {
  int anim_stacks_count = _scene->GetSrcObjectCount<FbxAnimStack>();

  // Early out if no animation's found.
  if(anim_stacks_count == 0) {
    ozz::log::Log() << "No animation found." << std::endl;
    return false;
  }
  
  if (anim_stacks_count > 1) {
    ozz::log::Log() << anim_stacks_count <<
      " animations found. Only the first one will be exported." << std::endl;
    return false;
  }

  // Arbitrarily take the first animation of the stack.
  FbxAnimStack* anim_stack = _scene->GetSrcObject<FbxAnimStack>(0);
  ozz::log::Log() << "Extracting animation \"" << anim_stack->GetName() << "\""
    << std::endl;
  return ExtractAnimation(_scene,
                          anim_stack,
                          _skeleton,
                          _sampling_rate,
                          _animation);
}
}  // fbx
}  // offline
}  // animation
}  // ozz
