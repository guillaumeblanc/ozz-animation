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

#ifndef OZZ_DEMO_FRAMEWORK_UTILS_H_
#define OZZ_DEMO_FRAMEWORK_UTILS_H_

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
namespace demo {
class ImGui;

// Utility class that helps with controlling animation playback time. Time is
// computed every update according to the dt given by the caller, playback speed
// and "play" state. OnGui function allows to tweak controller parameters
// through the application Gui.
class PlaybackController {
 public:
  // Constructor.
  PlaybackController();

  // Gets animation current time.
  float time() const { return time_; }

  // Updates animation time if in "play" state, according to playback speed and
  // given frame time _dt.
  void Update(const animation::Animation& _animation, float _dt);

  // Do controller Gui.
  void OnGui(const animation::Animation& _animation, ImGui* _im_gui);

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

// Computes the bounding box of posture defines be matrices in range
// [_begin,_end[.
// _bound must be a valid math::Box instance.
// Return true on success and fills _bound. Returns false if any pointer
// is NULL, or if _begin and _end doesn't define a valid range.
bool ComputePostureBounds(const ozz::math::Float4x4* _begin,
                          const ozz::math::Float4x4* _end,
                          math::Box* _bound);
}  // demo
}  // ozz
#endif  // OZZ_DEMO_FRAMEWORK_UTILS_H_
