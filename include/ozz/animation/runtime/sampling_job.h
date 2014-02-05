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

#ifndef OZZ_OZZ_ANIMATION_RUNTIME_SAMPLING_JOB_H_
#define OZZ_OZZ_ANIMATION_RUNTIME_SAMPLING_JOB_H_

#include "ozz/base/platform.h"

namespace ozz {

// Forward declaration of math structures.
namespace math { struct SoaTransform; }

namespace animation {

// Forward declares the animation type to sample.
class Animation;

namespace internal {
  // Soa hot data to interpolate.
  struct InterpSoaTranslation;
  struct InterpSoaRotation;
  struct InterpSoaScale;
}  // internal

// Declares the cache object used by the workload to take advantage of the
// frame coherency of animation sampling.
class SamplingCache {
 public:
  // Construct a cache that can be used to sample any animation with at most
  // _max_tracks tracks. _num_tracks is internally aligned to a multiple of
  // soa size.
  SamplingCache(int _max_tracks);

  // Deallocate cache.
  ~SamplingCache();

  // Invalidate the cache.
  // The SamplingJob automatically invalidates a cache when required
  // during sampling. This automatic mechanism is based on the animation
  // address and sampling time. The weak point is that it can result in a
  // crash if ever the address of an animation is used again with another
  // animation (could be the result of successive call to delete / new).
  // Therefore it is recommended to manually invalidate a cache when it is
  // known that this cache will not be used for with an animation again.
  void Invalidate();

  // The maximum number of tracks that the cache can handle.
  int max_tracks() const { return max_soa_tracks_ * 4; }
  int max_soa_tracks() const { return max_soa_tracks_; }

 private:

  // Disables copy and assignation.
  SamplingCache(SamplingCache const&);
  void operator=(SamplingCache const&);

  friend struct SamplingJob;

  // Steps the cache in order to use it for a potentially new animation and
  // time. If the _animation is different from the animation currently cached,
  // or if the _time shows that the animation is played backward, then the
  // cache is invalidated and reseted for the new _animation and _time.
  void Step(const Animation& _animation, float _time);

  // The animation this cache refers to. NULL means that the cache is invalid.
  const Animation* animation_;

  // The current time in the animation.
  float time_;

  // The number of soa tracks that can store this cache.
  int max_soa_tracks_;

  // Soa hot data to interpolate.
  internal::InterpSoaTranslation* soa_translations_;
  internal::InterpSoaRotation* soa_rotations_;
  internal::InterpSoaScale* soa_scales_;

  // Points to the keys in the animation that are valid for the current time.
  int* translation_keys_;
  int* rotation_keys_;
  int* scale_keys_;

  // Current cursors in the animation. 0 means that the cache is invalid.
  int translation_cursor_;
  int rotation_cursor_;
  int scale_cursor_;

  // Outdated soa entries. One bit per soa entry (32 joints per byte).
  unsigned char* outdated_translations_;
  unsigned char* outdated_rotations_;
  unsigned char* outdated_scales_;
};

// Samples animation to output postures in local space.
// The job does not owned the buffers (in/output) and will thus not delete them
// during job's destruction.
struct SamplingJob {
  // Default constructor, initializes default values.
  SamplingJob();

  // Validates job parameters. Returns true for a valid job, or false otherwise:
  // -if any input pointer is NULL
  // -if output range is invalid.
  bool Validate() const;

  // Runs job's sampling task.
  // The job is validated before any operation is performed, see Validate() for
  // more details.
  // Returns false if *this job is not valid.
  bool Run() const;

  // Time used to sample animation, clamped in range [0,duration] before
  // job execution. This resolves approximations issues on range bounds.
  float time;

  // The animation to sample.
  const Animation* animation;

  // A cache object that must be big enough to sample *this animation.
  SamplingCache* cache;

  // Job output.
  // The output range to be filled with sampled joints during job execution.
  // If there are less joints in the animation compared to the output range,
  // then remaining SoaTransform are left unchanged.
  // If there are more joints in the animation, then the last joints are not
  // sampled.
  Range<ozz::math::SoaTransform> output;
};
}  // animation
}  // ozz
#endif  // OZZ_OZZ_ANIMATION_RUNTIME_SAMPLING_JOB_H_
