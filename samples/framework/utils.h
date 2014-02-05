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

#ifndef OZZ_SAMPLES_FRAMEWORK_UTILS_H_
#define OZZ_SAMPLES_FRAMEWORK_UTILS_H_

#include "ozz/base/platform.h"

namespace ozz {
// Forward declarations.
namespace math {
struct Box;
struct Float4x4;
}
namespace animation {
class Animation;
class Skeleton;
} // animation
namespace sample {
class ImGui;

// Utility class that helps with controlling animation playback time. Time is
// computed every update according to the dt given by the caller, playback speed
// and "play" state. OnGui function allows to tweak controller parameters
// through the application Gui.
class PlaybackController {
 public:
  // Constructor.
  PlaybackController();

  // Sets animation current time.
  void set_time(float _time) {
    time_ = _time;
  }

  // Gets animation current time.
  float time() const {
    return time_;
  }

  // Sets playback speed.
  void set_playback_speed(float _speed) {
    playback_speed_ = _speed;
  }

  // Gets playback speed.
  float playback_speed() const {
    return playback_speed_;
  }

  // Updates animation time if in "play" state, according to playback speed and
  // given frame time _dt.
  void Update(const animation::Animation& _animation, float _dt);

  // Resets all parameters to their default value.
  void Reset();

  // Do controller Gui.
  void OnGui(const animation::Animation& _animation,
             ImGui* _im_gui,
             bool _enabled = true);

 private:
  // Current animation time.
  float time_;

  // Playback speed, can be negative in order to play the animation backward.
  float playback_speed_;

  // Animation play mode state: play/pause.
  bool play_;
};

// Computes the bounding box of _skeleton. This is the box that encloses all
// skeleton's joints in model space.
// _bound must be a valid math::Box instance.
// Return true on success and fills _bound.
bool ComputeSkeletonBounds(const animation::Skeleton& _skeleton,
                           math::Box* _bound);

// Computes the bounding box of posture defines be _matrices range.
// _bound must be a valid math::Box instance.
// Return true on success and fills _bound. Returns false if any pointer
// is NULL, or if _matrices doesn't define a valid range.
bool ComputePostureBounds(ozz::Range<const ozz::math::Float4x4> _matrices,
                          math::Box* _bound);

// Loads a skeleton from an ozz archive file named _filename.
// This function will fail and return false if the file cannot be opened or if
// it is not a valid ozz skeleton archive. A valid skeleton archive can be
// produced with ozz tools (dae2skel) or using ozz skeleton serialization API.
// _filename and _skeleton must be non-NULL.
bool LoadSkeleton(const char* _filename,
                  ozz::animation::Skeleton* _skeleton);

// Loads an animation from an ozz archive file named _filename.
// This function will fail and return false if the file cannot be opened or if
// it is not a valid ozz animation archive. A valid animation archive can be
// produced with ozz tools (dae2anim) or using ozz animation serialization API.
// _filename and _animation must be non-NULL.
bool LoadAnimation(const char* _filename,
                   ozz::animation::Animation* _animation);
}  // sample
}  // ozz
#endif  // OZZ_SAMPLES_FRAMEWORK_UTILS_H_
