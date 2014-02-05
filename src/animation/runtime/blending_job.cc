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

#include "ozz/animation/runtime/blending_job.h"

#include <cstddef>
#include <cassert>

#include "ozz/animation/runtime/skeleton.h"

#include "ozz/base/maths/math_ex.h"
#include "ozz/base/maths/soa_transform.h"

namespace ozz {
namespace animation {

BlendingJob::Layer::Layer()
    : weight(0.f) {
}

BlendingJob::BlendingJob()
    : threshold(.1f) {
}

bool BlendingJob::Validate() const {
  // Don't need any early out, as jobs are valid in most of the performance
  // critical cases.
  // Tests are written in multiple lines in order to avoid branches.
  bool valid = true;

  // Test for valid threshold).
  valid &= threshold > 0.f;

  // Test for NULL begin pointers.
  valid &= layers.begin != NULL;
  valid &= bind_pose.begin != NULL;
  valid &= output.begin != NULL;

  // Test ranges are valid (implicitly test for NULL end pointers).
  valid &= layers.end >= layers.begin;
  valid &= bind_pose.end >= bind_pose.begin;
  valid &= output.end >= output.begin;

  // The bind pose size defines the ranges of transforms to blend, so all
  // other buffers should be bigger.
  const std::ptrdiff_t min_range = bind_pose.end - bind_pose.begin;
  valid &= output.end - output.begin >= min_range;

  // Validates layers.
  for (const Layer* layer = layers.begin;
       layers.begin && layer < layers.end;  // Handles NULL pointers.
       layer++) {
    // Tests transforms validity.
    valid &= layer->transform.begin != NULL;
    valid &= layer->transform.end >= layer->transform.begin;
    valid &= layer->transform.end - layer->transform.begin >= min_range;

    // Joint weights are optional.
    if (layer->joint_weights.begin != NULL) {
      valid &= layer->joint_weights.end >= layer->joint_weights.begin;
      valid &= layer->joint_weights.end - layer->joint_weights.begin >= min_range;
    } else {
      valid &= layer->joint_weights.end == NULL;
    }
  }

  return valid;
}

namespace {

// Macro that defines the process of blending the 1st pass.
#define OZZ_BLEND_1ST_PASS(_in, _simd_weight, _out) {\
  _out->translation = _in.translation * _simd_weight;\
  _out->rotation = _in.rotation * _simd_weight;\
  _out->scale = _in.scale * _simd_weight;\
}

// Macro that defines the process of blending any pass but the first.
#define OZZ_BLEND_N_PASS(_in, _simd_weight, _out) {\
  /* Blends translation. */\
  _out->translation = _out->translation + _in.translation * _simd_weight;\
  /* Blends rotations, negates opposed quaternions to be sure to choose*/\
  /* the shortest path between the two.*/\
  const math::SimdFloat4 dot = _out->rotation.x * _in.rotation.x +\
                               _out->rotation.y * _in.rotation.y +\
                               _out->rotation.z * _in.rotation.z +\
                               _out->rotation.w * _in.rotation.w;\
  const math::SimdInt4 sign = math::Sign(dot);\
  const math::SoaQuaternion rotation = {math::Xor(_in.rotation.x, sign),\
                                        math::Xor(_in.rotation.y, sign),\
                                        math::Xor(_in.rotation.z, sign),\
                                        math::Xor(_in.rotation.w, sign)};\
  _out->rotation = _out->rotation + rotation * _simd_weight;\
  /* Blends scales.*/\
  _out->scale = _out->scale + _in.scale * _simd_weight;\
}

// Defines parameters that are exchanged accross blending stages.
struct ProcessArgs {
  ProcessArgs(const BlendingJob& _job)
    : job(_job),
      num_soa_joints(_job.bind_pose.end - _job.bind_pose.begin),
      num_passes(0),
      num_partial_passes(0),
      accumulated_weight(0.f) {
    // The range of all buffers has already been validated.
    assert(job.output.end >= job.output.begin + num_soa_joints);
    assert(OZZ_ARRAY_SIZE(accumulated_weights) >= num_soa_joints);
  }

  // Allocates enough space to store a accumulated weights per-joint.
  // It will be initialized by the first pass processed, if any.
  // This is quite big for a stack allocation (16 byte * maximum number of
  // joints). This is one of the reasons why the number of joints is limited
  // by the API.
  // Note that this array is used with SoA data.
  // This is the first argument in order to avoid wasting too much space with
  // alignment padding.
  math::SimdFloat4 accumulated_weights[Skeleton::kMaxSoAJoints];

  // The job to process.
  const BlendingJob& job;

  // The number of transforms to process as defind by the size of the bind pose.
  std::size_t num_soa_joints;

  // Number of processed blended passes (excluding passes with a weight <= 0.f),
  // including partial passes.
  int num_passes;

  // Number of processed partial blending passes (aka with a weight per-joint).
  int num_partial_passes;

  // The accumulated weight of all layers.
  float accumulated_weight;

 private:
   // Disables assignment operators.
   ProcessArgs(const ProcessArgs&);
   void operator = (const ProcessArgs&);
};

// Blends all layers of the job to its output.
void BlendLayers(ProcessArgs* _args) {
  assert(_args);

  // Iterates through all layers and blend them to the output.
  for (const BlendingJob::Layer* layer = _args->job.layers.begin;
       layer < _args->job.layers.end;
       layer++) {

    // Asserts buffer sizes, which must never fail as it has been validated.
    assert(layer->transform.end >=
           layer->transform.begin + _args->num_soa_joints);
    assert(!layer->joint_weights.begin ||
           (layer->joint_weights.end >=
            layer->joint_weights.begin + _args->num_soa_joints));

    // Skip irrelevant layers.
    if (layer->weight <= 0.f) {
      continue;
    }

    // Accumulates global weights.
    _args->accumulated_weight += layer->weight;
    const math::SimdFloat4 layer_weight = math::simd_float4::Load1(layer->weight);

    if (layer->joint_weights.begin) {
      // This layer has per-joint weights.
      _args->num_partial_passes++;

      if (_args->num_passes == 0) {
        for (std::size_t i = 0; i < _args->num_soa_joints; i++) {
          const math::SoaTransform& src = layer->transform.begin[i];
          math::SoaTransform* dest = _args->job.output.begin + i;
          const math::SimdFloat4 weight =
            layer_weight * math::Max0(layer->joint_weights.begin[i]);
          _args->accumulated_weights[i] = weight;
          OZZ_BLEND_1ST_PASS(src, weight, dest);
        }
      } else {
        for (std::size_t i = 0; i < _args->num_soa_joints; i++) {
          const math::SoaTransform& src = layer->transform.begin[i];
          math::SoaTransform* dest = _args->job.output.begin + i;
          const math::SimdFloat4 weight =
            layer_weight * math::Max0(layer->joint_weights.begin[i]);
          _args->accumulated_weights[i] =
            _args->accumulated_weights[i] + weight;
          OZZ_BLEND_N_PASS(src, weight, dest);
        }
      }
    } else {
      // This is a full layer.
      if (_args->num_passes == 0) {
        for (std::size_t i = 0; i < _args->num_soa_joints; i++) {
          const math::SoaTransform& src = layer->transform.begin[i];
          math::SoaTransform* dest = _args->job.output.begin + i;
          _args->accumulated_weights[i] = layer_weight;
          OZZ_BLEND_1ST_PASS(src, layer_weight, dest);
        }
      } else {
        for (std::size_t i = 0; i < _args->num_soa_joints; i++) {
          const math::SoaTransform& src = layer->transform.begin[i];
          math::SoaTransform* dest = _args->job.output.begin + i;
          _args->accumulated_weights[i] =
            _args->accumulated_weights[i] + layer_weight;
          OZZ_BLEND_N_PASS(src, layer_weight, dest);
        }
      }
    }
    // One more pass blended.
    _args->num_passes++;
  }
}

// Blends bind pose to the output if accumulated weight is less than the
// threshold value.
void BlendBindPose(ProcessArgs* _args) {
  assert(_args);

  // Asserts buffer sizes, which must never fail as it has been validated.
  assert(_args->job.bind_pose.end >=
         _args->job.bind_pose.begin + _args->num_soa_joints);

  if (_args->num_partial_passes == 0) {
    // No partial blending pass detected, threshold can be tested globaly.
    const float bp_weight =
      _args->job.threshold - _args->accumulated_weight;

    if (bp_weight > 0.f) {  // The bind pose is needed if it has a weight.
      const math::SimdFloat4 simd_bp_weight =
        math::simd_float4::Load1(bp_weight);

      // Updates global accumulated weight, but not per-joint weight any more
      // because normalization stage will be global also.
      _args->accumulated_weight = _args->job.threshold;
      if (_args->num_passes == 0) {
        for (std::size_t i = 0; i < _args->num_soa_joints; i++) {
          const math::SoaTransform& src = _args->job.bind_pose.begin[i];
          math::SoaTransform* dest = _args->job.output.begin + i;
          OZZ_BLEND_1ST_PASS(src, simd_bp_weight, dest);
        }
      } else {
        for (std::size_t i = 0; i < _args->num_soa_joints; i++) {
          const math::SoaTransform& src = _args->job.bind_pose.begin[i];
          math::SoaTransform* dest = _args->job.output.begin + i;
          OZZ_BLEND_N_PASS(src, simd_bp_weight, dest);
        }
      }
    }
  } else {
    // Blending passes contain partial blending, threshold must be tested for
    // each joint.
    const math::SimdFloat4 threshold = 
      math::simd_float4::Load1(_args->job.threshold);

    // There's been at least 1 pass as num_partial_passes != 0.
    assert(_args->num_passes != 0);

    for (std::size_t i = 0; i < _args->num_soa_joints; i++) {
      const math::SoaTransform& src = _args->job.bind_pose.begin[i];
      math::SoaTransform* dest = _args->job.output.begin + i;
      const math::SimdFloat4 bp_weight =
        math::Max0(threshold - _args->accumulated_weights[i]);
      _args->accumulated_weights[i] =
        math::Max(threshold, _args->accumulated_weights[i]);
      OZZ_BLEND_N_PASS(src, bp_weight, dest);
    }
  }
}

// Normalizes output rotations. Quanternion length can not be zero as opposed
// quaternions have been fixed up during blending passes.
// Translations and scales are already normalized because weights were
// pre-multiplied by the normalization ratio.
void Normalize(ProcessArgs* _args) {
  assert(_args);

  if (_args->num_partial_passes == 0) {
    // Normalization of a non-partial blending requires to apply the same
    // division to all joints.
    const math::SimdFloat4 ratio =
      math::simd_float4::Load1(1.f / _args->accumulated_weight);
    for (std::size_t i = 0; i < _args->num_soa_joints; i++) {
      math::SoaTransform& dest = _args->job.output.begin[i];
      dest.translation = dest.translation * ratio;
      dest.rotation = NormalizeEst(dest.rotation);
      dest.scale = dest.scale * ratio;
    }
  } else {
    // Partial blending normalization requires to compute the divider per-joint.
    const math::SimdFloat4 one = math::simd_float4::one();
    for (std::size_t i = 0; i < _args->num_soa_joints; i++) {
      const math::SimdFloat4 ratio = one / _args->accumulated_weights[i];
      math::SoaTransform& dest = _args->job.output.begin[i];
      dest.translation = dest.translation * ratio;
      dest.rotation = NormalizeEst(dest.rotation);
      dest.scale = dest.scale * ratio;
    }
  }
}
}  // namespace

bool BlendingJob::Run() const {
  if (!Validate()) {
    return false;
  }

  // Initializes blended parameters that are exchanged accross blend stages.
  ProcessArgs process_args(*this);

  // Blends all layers to the job output buffers.
  BlendLayers(&process_args);

  // Applies bind pose.
  BlendBindPose(&process_args);

  // Normalizes output.
  Normalize(&process_args);

  return true;
}
}  // animation
}  // ozz
