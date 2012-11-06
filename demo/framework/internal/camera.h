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

#ifndef OZZ_DEMO_FRAMEWORK_INTERNAL_CAMERA_H_
#define OZZ_DEMO_FRAMEWORK_INTERNAL_CAMERA_H_

#ifndef OZZ_INCLUDE_PRIVATE_HEADER
#error "This header is private, it cannot be included from public headers."
#endif  // OZZ_INCLUDE_PRIVATE_HEADER

#include "ozz/base/maths/vec_float.h"
#include "ozz/base/maths/simd_math.h"

namespace ozz {
namespace math { struct Box; }
namespace demo {
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
  void Update(float _delta_time);

  // Binds projection and model-view matrices to OpenGL.
  void Bind();

  // Resize notification, used to rebuild projection matrix.
  void Resize(int _width, int _height);

  // Frame all _box, while maintaining camera orientation.
  // If _animate is true, the framing is animated/smoothed.
  // Returns true on success.
  bool FrameAll(const math::Box& _box, bool _animate);

 private:
  // The current projection matrix.
  math::Float4x4 projection_;

  // The current model-view matrix.
  math::Float4x4 model_view_;

  // The angles in degree (OpenGL) of the camera rotation around x and y axes.
  math::Float2 angles_;

  // The center of the rotation.
  math::Float3 center_;

  // The view distance, from the center of rotation.
  float distance_;

  // The current animated angles.
  math::Float2 animated_angles_;

  // The current animated center position.
  math::Float3 animated_center_;

  // The current animated distance for the center.
  float animated_distance_;

  // The remaining time for the  animated center to reach the center.
  float remaining_animation_time_;

  // The position of the mouse, the last time it has been seen.
  int mouse_last_x_;
  int mouse_last_y_;
  int mouse_last_mb_;
};
}  // internal
}  // demo
}  // ozz
#endif  // OZZ_DEMO_FRAMEWORK_INTERNAL_CAMERA_H_
