//============================================================================//
//                                                                            //
// ozz-animation, 3d skeletal animation libraries and tools.                  //
// https://code.google.com/p/ozz-animation/                                   //
//                                                                            //
//----------------------------------------------------------------------------//
//                                                                            //
// Copyright (c) 2012-2015 Guillaume Blanc                                    //
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

#include "ozz/animation/offline/raw_animation.h"
#include "ozz/animation/runtime/skeleton.h"

namespace ozz {
namespace animation {
namespace offline {

RawAnimation::RawAnimation()
  : duration(1.f) {
}

RawAnimation::~RawAnimation() {
}

namespace {

// Implements key frames' time range and ordering checks.
// See AnimationBuilder::Create for more details.
template <typename _Key>
static bool ValidateTrack(const typename ozz::Vector<_Key>::Std& _track,
                          float _duration) {
  float previous_time = -1.f;
  for (size_t k = 0; k < _track.size(); ++k) {
    const float frame_time = _track[k].time;
    // Tests frame's time is in range [0:duration].
    if (frame_time < 0.f || frame_time > _duration) {
      return false;
    }
    // Tests that frames are sorted.
    if (frame_time <= previous_time) {
      return false;
    }
    previous_time = frame_time;
  }
  return true;  // Validated.
}
}  // namespace

bool RawAnimation::Validate() const {
  if (duration <= 0.f) {  // Tests duration is valid.
    return false;
  }
  if (tracks.size() > Skeleton::kMaxJoints) {  // Tests number of tracks.
    return false;
  }
  // Ensures that all key frames' time are valid, ie: in a strict ascending
  // order and within range [0:duration].
  for (size_t j = 0; j < tracks.size(); ++j) {
    const RawAnimation::JointTrack& track = tracks[j];
    if (!ValidateTrack<TranslationKey>(track.translations, duration) ||
        !ValidateTrack<RotationKey>(track.rotations, duration) || 
        !ValidateTrack<ScaleKey>(track.scales, duration)) {
      return false;
    }
  }

  return true;  // *this is valid.
}
}  // offline
}  // animation
}  // ozz
