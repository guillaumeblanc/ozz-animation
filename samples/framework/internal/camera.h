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

#ifndef OZZ_SAMPLES_FRAMEWORK_INTERNAL_CAMERA_H_
#define OZZ_SAMPLES_FRAMEWORK_INTERNAL_CAMERA_H_

#ifndef OZZ_INCLUDE_PRIVATE_HEADER
#error "This header is private, it cannot be included from public headers."
#endif  // OZZ_INCLUDE_PRIVATE_HEADER

#include "ozz/base/maths/vec_float.h"
#include "ozz/base/maths/simd_math.h"

namespace ozz {
namespace math { struct Box; }
namespace sample {
class ImGui;
namespace internal {

// Framework internal implementation of an OpenGL/glfw camera system that can be
// manipulated with the mouse and some shortcuts.
class Camera {
 public:

  // Initializes camera to its default framing.
  Camera();

  // Destructor.
  ~Camera();

  // Updates camera framing: mouse manipulation, timed transitions...
  // Returns actions that the user applied to the camera during the frame.
  void Update(const math::Box& _box, float _delta_time, bool _first_frame);

  // Provides immediate mode gui display event.
  void OnGui(ImGui* _im_gui);

  // Binds 3d projection and view matrices to the current matrix.
  void Bind3D();

  // Binds 2d projection and view matrices to the current matrix.
  void Bind2D();

  // Resize notification, used to rebuild projection matrix.
  void Resize(int _width, int _height);

  // Get the current projection matrix.
  const math::Float4x4& projection() {
    return projection_;
  }

  // Get the current model-view matrix.
  const math::Float4x4& view() {
    return view_;
  }

  // Get the current model-view-projection matrix.
  const math::Float4x4& view_proj() {
    return view_proj_;
  }

  // Set to true to automatically frame the camera on the whole scene.
  void set_auto_framing(bool _auto) {
    auto_framing_ = _auto;
  }
  // Get auto framing state.
  bool auto_framing() const {
    return auto_framing_;
  }

 private:
  // The current projection matrix.
  math::Float4x4 projection_;

  // The current projection matrix.
  math::Float4x4 projection_2d_;

  // The current model-view matrix.
  math::Float4x4 view_;

  // The current model-view-projection matrix.
  math::Float4x4 view_proj_;

  // The angles in degree of the camera rotation around x and y axes.
  math::Float2 angles_;

  // The center of the rotation.
  math::Float3 center_;

  // The view distance, from the center of rotation.
  float distance_;

  // The current animated angles.
  math::Float2 animated_angles_;

  // True if angles should be animated.
  bool smooth_angles_;

  // The current animated center position.
  math::Float3 animated_center_;

  // True if angles should be animated.
  bool smooth_center_;

  // The current animated distance for the center.
  float animated_distance_;

  // True if angles should be animated.
  bool smooth_distance_;

  // Fix (don't animate) camera distance to the target.
  bool fix_distance_;

  // The remaining time for the  animated center to reach the center.
  float remaining_animation_time_;

  // The position of the mouse, the last time it has been seen.
  int mouse_last_x_;
  int mouse_last_y_;
  int mouse_last_wheel_;

  // Set to true to automatically frame the camera on the whole scene.
  bool auto_framing_;
};
}  // internal
}  // sample
}  // ozz
#endif  // OZZ_SAMPLES_FRAMEWORK_INTERNAL_CAMERA_H_
