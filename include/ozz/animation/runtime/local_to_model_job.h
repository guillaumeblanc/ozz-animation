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

#ifndef OZZ_OZZ_ANIMATION_RUNTIME_LOCAL_TO_MODEL_JOB_H_
#define OZZ_OZZ_ANIMATION_RUNTIME_LOCAL_TO_MODEL_JOB_H_

#include "ozz/base/platform.h"

namespace ozz {

// Forward declaration math structures.
namespace math { struct SoaTransform; }
namespace math { struct Float4x4; }

namespace animation {

// Forward declares the Skeleton object used to describe joint hierarchy.
class Skeleton;

// Computes model-space joint matrices from local-space SoaTransform.
// This job uses the skeleton to define joints parent-child hierarchy. The job
// iterates through all joints to compute their transform relatively to the
// skeleton root.
// Job inputs is an array of SoaTransform objects (in local-space), ordered like
// skeleton's joints. Job output is an array of matrices (in model-space),
// ordered like skeleton's joints. Output are matrices, because the combination
// of affine transformations can contain shearing or complex transformation
// that cannot be represented as Transform object.
struct LocalToModelJob {
  // Default constructor, initializes default values.
  LocalToModelJob() :
    skeleton(NULL) {
  }

  // Validates job parameters. Returns true for a valid job, or false otherwise:
  // -if any input pointer, including ranges, is NULL.
  // -if the size of the input is smaller than the skeleton's number of joints.
  // Note that this input has a SoA format.
  // -if the size of of the output is smaller than the skeleton's number of
  // joints.
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
  // The input range that store local transforms.
  Range<const ozz::math::SoaTransform> input;

  // Job output.
  // The output range to be filled with model matrices.
  Range<ozz::math::Float4x4> output;
};
}  // animation
}  // ozz
#endif  // OZZ_OZZ_ANIMATION_RUNTIME_LOCAL_TO_MODEL_JOB_H_
