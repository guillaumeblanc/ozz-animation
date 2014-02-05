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

#include "ozz/animation/runtime/local_to_model_job.h"

#include <cassert>

#include "ozz/base/maths/soa_transform.h"
#include "ozz/base/maths/soa_float4x4.h"
#include "ozz/base/maths/simd_math.h"
#include "ozz/base/maths/math_ex.h"

#include "ozz/animation/runtime/skeleton.h"

namespace ozz {
namespace animation {

bool LocalToModelJob::Validate() const {
  // Don't need any early out, as jobs are valid in most of the performance
  // critical cases.
  // Tests are written in multiple lines in order to avoid branches.
  bool valid = true;

  // Test for NULL begin pointers.
  if (!skeleton) {
    return false;
  }
  valid &= input.begin != NULL;
  valid &= output.begin != NULL;

  const int num_joints = skeleton->num_joints();
  const int num_soa_joints = (num_joints + 3) / 4;

  // Test input and output ranges, implicitly tests for NULL end pointers.
  valid &= input.end - input.begin >= num_soa_joints;
  valid &= output.end - output.begin >= num_joints;

  return valid;
}

bool LocalToModelJob::Run() const {
  using math::SoaTransform;
  using math::SoaFloat4x4;
  using math::Float4x4;

  if (!Validate()) {
    return false;
  }

  // Early out if no joint.
  const int num_joints = skeleton->num_joints();
  if (num_joints == 0) {
    return true;
  }

  // Fetch joint's properties.
  Range<const Skeleton::JointProperties> properties =
    skeleton->joint_properties();

  // Output.
  Float4x4* const model_matrices = output.begin;

  // Initializes an identity matrix that will be used to compute roots model
  // matrices without requiring a branch.
  const Float4x4 identity = Float4x4::identity();

  // Converts to matrices and applies hierarchical transformation.
  for (int joint = 0; joint < num_joints;) {
    // Builds soa matrices from soa transforms.
    const SoaTransform& transform = input.begin[joint / 4];
    const SoaFloat4x4 local_soa_matrices =
      SoaFloat4x4::FromAffine(transform.translation,
                              transform.rotation,
                              transform.scale);
    // Converts to aos matrices.
    math::SimdFloat4 local_aos_matrices[16];
    math::Transpose16x16(&local_soa_matrices.cols[0].x, local_aos_matrices);

    // Applies hierarchical transformation.
    const int proceed_up_to = joint + math::Min(4, num_joints - joint);
    const math::SimdFloat4* local_aos_matrix = local_aos_matrices;
    for (; joint < proceed_up_to; joint++, local_aos_matrix += 4) {
      const int parent = properties.begin[joint].parent;
      const Float4x4* parent_matrix =
        math::Select(parent == Skeleton::kNoParentIndex,
                     &identity,
                     &model_matrices[parent]);
      const Float4x4 local_matrix = {{local_aos_matrix[0],
                                      local_aos_matrix[1],
                                      local_aos_matrix[2],
                                      local_aos_matrix[3]}};
      model_matrices[joint] = (*parent_matrix) * local_matrix;
    }
  }
  return true;
}
}  // animation
}  // ozz
