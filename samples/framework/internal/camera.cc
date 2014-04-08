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

#define OZZ_INCLUDE_PRIVATE_HEADER  // Allows to include private headers.

#include "framework/internal/camera.h"

#include "ozz/base/platform.h"
#include "ozz/base/log.h"
#include "ozz/base/maths/box.h"
#include "ozz/base/maths/math_ex.h"
#include "ozz/base/maths/math_constant.h"

#include "framework/internal/renderer_impl.h"

using ozz::math::Float2;
using ozz::math::Float3;
using ozz::math::Float4x4;

namespace ozz {
namespace sample {
namespace internal {

// Declares camera navigation constants.
static const float kDefaultDistance = 8.f;
static const Float3 kDefaultCenter = Float3(0.f, .5f, 0.f);
static const Float2 kDefaultAngle = Float2(-ozz::math::kPi * 1.f / 12.f,
                                           ozz::math::kPi * 1.f / 5.f);
static const float kAngleFactor = .002f;
static const float kDistanceFactor = .1f;
static const float kScrollFactor = .02f;
static const float kPanFactor = .01f;
static const float kKeyboardFactor = 500.f;
static const float kAnimationTime = .15f;
static const float kNear = .01f;
static const float kFar = 1000.f;
static const float kFovY = ozz::math::kPi / 3.f;
static const float kFrameAllZoomOut = 1.1f;  // 10% bigger than the scene.

// Setups initial values.
Camera::Camera()
    : projection_(Float4x4::identity()),
      model_view_(Float4x4::identity()),
      angles_(kDefaultAngle),
      center_(kDefaultCenter),
      distance_(kDefaultDistance),
      animated_angles_(kDefaultAngle),
      animated_center_(kDefaultCenter),
      animated_distance_(kDefaultDistance),
      remaining_animation_time_(0.f),
      mouse_last_x_(0),
      mouse_last_y_(0),
      mouse_last_mb_(GLFW_RELEASE),
      mouse_last_wheel_(0) {
}

Camera::~Camera() {
}

Camera::Actions Camera::Update(float _delta_time) {
  Actions actions = {};  // Default to false.

    // Mouse wheel activates Zoom.
  const int w = glfwGetMouseWheel();
  const int dw = w - mouse_last_wheel_;
  mouse_last_wheel_ = w;
  if (dw != 0) {
    actions.zoomed = true;  // Camera zoomed manually.
    distance_ += -dw * kScrollFactor * distance_;
    remaining_animation_time_= 0.f;
  }

  // Fetches current mouse position and compute its movement since last frame.
  int x, y;
  glfwGetMousePos(&x, &y);
  const int mdx = x - mouse_last_x_;
  const int mdy = y - mouse_last_y_;
  mouse_last_x_ = x;
  mouse_last_y_ = y;

  // Finds keyboard relative dx and dy commmands.
  const int timed_factor =
    ozz::math::Max(1, static_cast<int>(kKeyboardFactor * _delta_time));
  const int kdx =
    timed_factor * (glfwGetKey(GLFW_KEY_RIGHT) - glfwGetKey(GLFW_KEY_LEFT));
  const int kdy =
    timed_factor * (glfwGetKey(GLFW_KEY_DOWN) - glfwGetKey(GLFW_KEY_UP));
  const bool keyboard_interact = kdx || kdy;

  // Computes composed keyboard and mouse dx and dy.
  const int dx = mdx + kdx;
  const int dy = mdy + kdy;

  // Mouse right button activates Zoom, Pan and Orbit modes.
  if (keyboard_interact || glfwGetMouseButton(GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS) {
    if (glfwGetKey(GLFW_KEY_LCTRL) == GLFW_PRESS) {  // Zoom mode.
      actions.zoomed = true;  // Camera zoomed manually.
      distance_ += dy * kDistanceFactor;
      remaining_animation_time_= 0.f;
    } else if (glfwGetKey(GLFW_KEY_LSHIFT) == GLFW_PRESS) {  // Pan mode.
      actions.panned = true;  // Camera panned manually.
      const float dx_pan = -dx * kPanFactor;
      const float dy_pan = dy * kPanFactor;

      // Moves along camera axes.
      math::Float4x4 transpose = Transpose(model_view_);
      math::Float3 right_transpose, up_transpose;
      math::Store3PtrU(transpose.cols[0], &right_transpose.x);
      math::Store3PtrU(transpose.cols[1], &up_transpose.x);
      center_ = center_ + right_transpose * dx_pan + up_transpose * dy_pan;
    } else {  // Orbit mode.
      actions.rotated = true;  // Camera zoomed manually.
      angles_.x = fmodf(angles_.x - dy * kAngleFactor, ozz::math::k2Pi);
      angles_.y = fmodf(angles_.y - dx * kAngleFactor, ozz::math::k2Pi);
      remaining_animation_time_= 0.f;
    }
  }

  // The middle button set view center.
  const int mouse_mb = glfwGetMouseButton(GLFW_MOUSE_BUTTON_MIDDLE);
  const int mouse_mb_released = mouse_last_mb_ == GLFW_PRESS &&
                                mouse_mb == GLFW_RELEASE;
  mouse_last_mb_ = mouse_mb;
  if (mouse_mb_released) {
    actions.centered = true;  // Camera centered manually.
    // Find picking ray: un-project mouse position on the near and far planes.
    int viewport[4];
    GL(GetIntegerv(GL_VIEWPORT, viewport));

    const Float2 mouse_norm(
      (x - viewport[0]) / static_cast<float>(viewport[2]) * 2.f - 1.f,
      (viewport[3] - y - viewport[1]) / static_cast<float>(viewport[3]) * 2.f - 1.f);

    const math::SimdFloat4 in_near =
      math::simd_float4::Load(mouse_norm.x, mouse_norm.y, -1.f, 1.f);
    const math::SimdFloat4 in_far =
      math::simd_float4::Load(mouse_norm.x, mouse_norm.y, 1.f, 1.f);

    const Float4x4 inv_mvp = Invert(projection_ * model_view_);
    const math::SimdFloat4 proj_near = inv_mvp * in_near;
    const math::SimdFloat4 proj_far = inv_mvp * in_far;
    const math::SimdFloat4 near = proj_near / math::SplatW(proj_near);
    const math::SimdFloat4 far =  proj_far / math::SplatW(proj_far);

    // The new center is at the intersection of the ray and the floor plane.
    const math::SimdFloat4 ray = math::Normalize3(far - near);
    const math::SimdFloat4 ray_y = math::SplatY(ray);
    const math::SimdFloat4 center =
      math::Select(math::CmpNe(ray_y, math::simd_float4::zero()),
                   near + ray * (-math::SplatY(near) / ray_y),
                   math::simd_float4::zero());
    math::Store3PtrU(center, &center_.x);

    // Starts animation.
    remaining_animation_time_ = kAnimationTime;
  }

  // An animation (to the new center position) is in progress.
  if (remaining_animation_time_ > 0.001f) {
    const float clamped_delta_time =
      ozz::math::Min(_delta_time, remaining_animation_time_);
    const float ratio = clamped_delta_time / remaining_animation_time_;
    animated_angles_ = animated_angles_ + (angles_ - animated_angles_) * ratio;
    animated_center_ = animated_center_ + (center_ - animated_center_) * ratio;
    animated_distance_ += (distance_ - animated_distance_) * ratio;
    remaining_animation_time_ -= clamped_delta_time;
  } else {
    animated_angles_ = angles_;
    animated_center_ = center_;
    animated_distance_ = distance_;
  }

  // Build the model view matrix components.
  const Float4x4 center = Float4x4::Translation(
    math::simd_float4::Load(
      animated_center_.x, animated_center_.y, animated_center_.z, 1.f));
  const Float4x4 y_rotation = Float4x4::FromAxisAngle(
    math::simd_float4::Load(0.f, 1.f, 0.f, animated_angles_.y));
  const Float4x4 x_rotation = Float4x4::FromAxisAngle(
    math::simd_float4::Load(1.f, 0.f, 0.f, animated_angles_.x));
  const Float4x4 distance = Float4x4::Translation(
    math::simd_float4::Load(0.f, 0.f, animated_distance_, 1.f));

  // Concatenate model_view matrix components.
  model_view_ = Invert(center * y_rotation * x_rotation * distance);

  return actions;
}

void Camera::Bind() {
  OZZ_ALIGN(16) float values[16];

  // Sets the projection.
  GL(MatrixMode(GL_PROJECTION));
  math::StorePtr(projection_.cols[0], values + 0);
  math::StorePtr(projection_.cols[1], values + 4);
  math::StorePtr(projection_.cols[2], values + 8);
  math::StorePtr(projection_.cols[3], values + 12);
  GL(LoadMatrixf(values));

  // Set the model-view.
  GL(MatrixMode(GL_MODELVIEW));
  math::StorePtr(model_view_.cols[0], values + 0);
  math::StorePtr(model_view_.cols[1], values + 4);
  math::StorePtr(model_view_.cols[2], values + 8);
  math::StorePtr(model_view_.cols[3], values + 12);
  GL(LoadMatrixf(values));
}

void Camera::Resize(int _width, int _height) {

  // Compute the projection matrix.
  const float ratio = _height != 0 ? 1.f * _width / _height : 1.f;
  const float h = tan( (kFovY * .5f)) * kNear;
  const float w = h * ratio;

  projection_.cols[0] = math::simd_float4::Load(
    kNear / w, 0.f, 0.f, 0.f); 
  projection_.cols[1] = math::simd_float4::Load(
    0.f, kNear / h, 0.f, 0.f);
  projection_.cols[2] = math::simd_float4::Load(
    0.f, 0.f, -(kFar + kNear) / (kFar - kNear), -1.f);
  projection_.cols[3] = math::simd_float4::Load(
    0.f, 0.f, -(2.f * kFar * kNear) / (kFar - kNear), 0.f);
}

bool Camera::FrameAll(const math::Box& _box, bool _immediate) {
  if (!_box.is_valid()) {
    return false;
  }

  center_ = (_box.max + _box.min) * .5f;
  const float radius = Length(_box.max - _box.min) * .5f;
  distance_ = (radius * kFrameAllZoomOut) / tanf(kFovY * .5f);
  remaining_animation_time_ = _immediate? 0.f : kAnimationTime;

  return true;
}
}  // internal
}  // sample
}  // ozz
