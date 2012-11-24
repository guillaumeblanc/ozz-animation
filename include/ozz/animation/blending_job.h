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

#ifndef OZZ_OZZ_ANIMATION_BLENDING_JOB_H_
#define OZZ_OZZ_ANIMATION_BLENDING_JOB_H_

namespace ozz {

// Forward declaration of math structures.
namespace math { struct SoaTransform; }

namespace animation {

// Blends multiple input layer/postures to a single output. The number of
// transforms/joints blended by the job is defined by the number of transfoms of
// the bind pose (note that this is a SoA format). This means that all buffers
// must be at least as big as the bind pose buffer.
// The job does not owned the buffers (input/output) and will thus not delete
// them during job's destruction.
struct BlendingJob {
  // Default constructor, initializes default values.
  BlendingJob();

  // Validates job parameters. Returns true for a valid job, or false otherwise:
  // -if any layer pointer is NULL, or layer_end is less than layer_begin.
  // -if any layer is not valid, ie any layer pointer is NULL, or layer pointer
  // end is less than begin.
  // -if any ouput pointer is NULL, or output_end is less than output_begin.
  // -if any buffer is smaller than the bind pose buffer.
  // -if the thresold value is less than or equal to 0.f.
  bool Validate() const;

  // Runs job's sampling task.
  // The job is validated before any operation is performed, see Validate() for
  // more details.
  // Returns false if *this job is not valid.
  bool Run() const;

  struct Layer {
    // Default constructor, initializes default values.
    Layer();

    // Blending weight of this layer. Negative values are considered as 0.
    // Normalization is performed at the end of the blending stage, so weight
    // can be in any range, even though range [0:1] is optimal.
    float weight;

    // The range [begin,end[ of input layer posture. Must be at least as big as
    // the bind pose buffer, but only the number of transforms defined by the
    // bind pose buffer will be processed.
    const ozz::math::SoaTransform* begin;
    const ozz::math::SoaTransform* end;
  };

  // The job blends the bind pose to the output when the accumulated weight of
  // all layers is less than this thresold value.
  // Must be greater than 0.f.
  float thresold;

  // Job input layers.
  // The range [layers_begin,layers_end[ of layers that must be blended.
  const Layer* layers_begin;
  const Layer* layers_end;

  // The skeleton bind pose. The size of this buffer defines the number of
  // transforms to blend. This is the reference because this buffer is defined
  // by the skeleton that all the animations belongs to.
  // It is used when the accumulated weight for a bone on all layers is
  // less than the thresold value, in order to fall back on valid transforms.
  const ozz::math::SoaTransform* bind_pose_begin;
  const ozz::math::SoaTransform* bind_pose_end;

  // Job output.
  // The range [output_begin,output_end[ to be filled with blended layer
  // transforms during job execution.
  // Must be at least as big as the bind pose buffer, but only the number of
  // transforms defined by the bind pose buffer size will be processed.
  // output_end is const as it is never dereferenced.
  ozz::math::SoaTransform* output_begin;
  const ozz::math::SoaTransform* output_end;
};
}  // animation
}  // ozz
#endif  // OZZ_OZZ_ANIMATION_BLENDING_JOB_H_
