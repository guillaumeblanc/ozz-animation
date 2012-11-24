//============================================================================//
// Copyright (c) 2012 Guillaume Blanc                                         //
//                                                                            //
// This software is provided 'as-is', without any express or implied warranty.//
// In no event will the authors be held liable for any damages arising from   //
// the use of this software.                                                  //
//                                                                            //
// Permission is granted to anyone to use this software for any purpose,      //
// including commercial applications, and to alter it and redistribute it     //
// freely, subject to the following restrictions:                             //
// Copyright (c) 2012 Guillaume Blanc                                         //
// 1. The origin of this software must not be misrepresented; you must not    //
// claim that you wrote the original software. If you use this software in a  //
// product, an acknowledgment in the product documentation would be           //
// appreciated but is not required.                                           //
//                                                                            //
// 2. Altered source versions must be plainly marked as such, and must not be //
// misrepresented as being the original software.                             //
//                                                                            //
// 3. This notice may not be removed or altered from any source distribution. //
//============================================================================//

#include "ozz/animation/blending_job.h"

#include <cstddef>
#include <cassert>

#include "ozz/base/maths/math_ex.h"
#include "ozz/base/maths/soa_transform.h"

namespace ozz {
namespace animation {

BlendingJob::Layer::Layer()
    : weight(0.f),
      begin(NULL),
      end(NULL) {
}

BlendingJob::BlendingJob()
    : thresold(.1f),
      layers_begin(NULL),
      layers_end(NULL),
      bind_pose_begin(NULL),
      bind_pose_end(NULL),
      output_begin(NULL),
      output_end(NULL) {
}

bool BlendingJob::Validate() const {
  // Don't need any early out, as jobs are valid in most of the performance
  // critical cases.
  // Tests are written in multiple lines in order to avoid branches.
  bool valid = true;

  // Test for valid thresold).
  valid &= thresold > 0.f;

  // Test for NULL begin pointers.
  valid &= layers_begin != NULL;
  valid &= bind_pose_begin != NULL;
  valid &= output_begin != NULL;

  // Test ranges are valid (implicitly test for NULL end pointers).
  valid &= layers_end >= layers_begin;
  valid &= bind_pose_end >= bind_pose_begin;
  valid &= output_end >= output_begin;

  // The bind pose size defines the ranges of transforms to blend, so all
  // other buffers should be bigger.
  const std::ptrdiff_t min_range = bind_pose_end - bind_pose_begin;
  valid &= output_end - output_begin >= min_range;

  // Validates layers.
  for (const Layer* layer = layers_begin;
       layers_begin && layer < layers_end;  // Handles NULL pointers.
       layer++) {
    valid &= layer->begin != NULL;
    valid &= layer->end >= layer->begin;
    valid &= layer->end - layer->begin >= min_range;
  }

  return valid;
}

namespace {
void Blend(const math::SoaTransform* _src, const math::SoaTransform* _src_end,
           math::SoaTransform* _dest, const math::SoaTransform* _dest_end,
           float _weight,
           bool _first_pass) {
  (void)_dest_end;
  assert(_weight > 0.f);

  // Load weight to a simd value, any weight less than 0 is already rejected.
  math::SimdFloat4 weight = math::simd_float4::Load1(_weight);

  // During the first pass the weighted layer is copied to output, then others
  // are accumulated.
  if (_first_pass) {
    for (; _src < _src_end; _src++, _dest++) {
      _dest->translation = _src->translation * weight;
      _dest->rotation = _src->rotation * weight;
      _dest->scale = _src->scale * weight;
    }
  } else {
    for (; _src < _src_end; _src++, _dest++) {
      // Blends translation.
      _dest->translation = _dest->translation + _src->translation * weight;

      // Blends rotations, negates opposed quaternions to be sure to choose
      // the shortest path between the two.
      const math::SimdFloat4 dot = _dest->rotation.x * _src->rotation.x +
                                   _dest->rotation.y * _src->rotation.y +
                                   _dest->rotation.z * _src->rotation.z +
                                   _dest->rotation.w * _src->rotation.w;
      const math::SimdInt4 sign = math::Sign(dot);
      const math::SoaQuaternion rotation = { math::Xor(_src->rotation.x, sign),
                                             math::Xor(_src->rotation.y, sign),
                                             math::Xor(_src->rotation.z, sign),
                                             math::Xor(_src->rotation.w, sign)};
      _dest->rotation = _dest->rotation + rotation * weight;

      // Blends scales.
      _dest->scale = _dest->scale + _src->scale * weight;
    }
  }
  assert(_dest <= _dest_end);
}
}  // namespace

bool BlendingJob::Run() const {
  if (!Validate()) {
    return false;
  }

  // Determines the accumulated weight in order to pre-compute normalization.
  float accumulated_weight = 0.f;
  for (const Layer* layer = layers_begin; layer < layers_end; layer++) {
    // Negative values must be considered as 0.
    accumulated_weight += math::Max(0.f, layer->weight);
  }
  assert(thresold > 0.f);
  const float normalization_ratio =
    1.f / math::Max(thresold, accumulated_weight);

  // The number of transforms to process is defind by the size of the bind pose.
  // The range of all buffers has already been validated.
  const std::ptrdiff_t loops = bind_pose_end - bind_pose_begin;
  assert(output_end >= output_begin + loops);

  // Iterate throught all layers and accumulate blending values.
  bool first_pass = true;
  for (const Layer* layer = layers_begin; layer < layers_end; layer++) {
    // Skip irrelevant layers.
    if (layer->weight <= 0.f) {
      continue;
    }

    // Blends this layer to the output.
    assert(layer->end >= layer->begin + loops);
    Blend(layer->begin, layer->begin + loops,
          output_begin, output_begin + loops,
          layer->weight * normalization_ratio,
          first_pass);
    first_pass = false;
  }

  // Blends in bind pose if accumulated weight in under the thresold.
  if (accumulated_weight < thresold) {
    // Compute the weigh given to the bind pose and update accumulated weight.
    float bind_pose_weight = thresold - accumulated_weight;

    // Blends in bind pose.
    Blend(bind_pose_begin, bind_pose_end,
          output_begin, output_begin + loops,
          bind_pose_weight * normalization_ratio,
          first_pass);
  }

  // Normalizes output rotations. Quanternion length can not be zero as opposed
  // quaternions have been fixed up.
  // Translations and scales are already normalized because weights were
  // pre-multiplied by the normalization ratio.
  for (math::SoaTransform* output = output_begin;
       output < output_begin + loops;
       output++) {
    output->rotation = NormalizeEst(output->rotation);
  }

  return true;
}
}  // animation
}  // ozz
