//============================================================================//
// Copyright (c) <2012> <Guillaume Blanc>                                     //
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
//============================================================================//

#ifndef OZZ_OZZ_ANIMATION_LOCAL_TO_MODEL_JOB_H_
#define OZZ_OZZ_ANIMATION_LOCAL_TO_MODEL_JOB_H_

#include <cstddef>

namespace ozz {

// Forward declaration math structures.
namespace math { struct SoaTransform; }
namespace math { struct Float4x4; }

namespace animation {

// Forward declares the Skeleton object used to describe joint hierarchy.
class Skeleton;

// Computes model-space joint matrices from local-space SoaTransform.
struct LocalToModelJob {
  // Default constructor, initializes default values.
  LocalToModelJob() :
    skeleton(NULL),
    input_begin(NULL),
    input_end(NULL),
    output_begin(NULL),
    output_end(NULL) {
  }

  // Validates job parameters. Returns true for a valid job, or false otherwise:
  // -if any input pointer is NULL.
  // -if the size of (input_end - input_begin) not bigger or equal to the
  // skeleton's number of joints. Note that this input has a soa format.
  // -if the size of (output_end - output_begin) is not bigger or equal to the
  // skeleton's number of joints.
  bool Validate() const;

  // Runs job's local-to-model task.
  // The job is validated before any operation is performed, see Validate() for
  // more details.
  // Returns false if job is not valid. See Validate() function.
  bool Run() const;

  // The Skeleton object describing the joint hierarchy used for local to
  // model space conversion.
  const Skeleton* skeleton;

  // Job input.
  // The range [input_begin,input_end[ that store local transforms.
  const ozz::math::SoaTransform* input_begin;
  const ozz::math::SoaTransform* input_end;

  // Job output.
  // The range [output_begin,output_end[ to be filled with model matrices.
  ozz::math::Float4x4* output_begin;
  const ozz::math::Float4x4* output_end;
};
}  // animation
}  // ozz
#endif  // OZZ_OZZ_ANIMATION_LOCAL_TO_MODEL_JOB_H_
