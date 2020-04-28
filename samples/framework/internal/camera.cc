//----------------------------------------------------------------------------//
//                                                                            //
// ozz-animation is hosted at http://github.com/guillaumeblanc/ozz-animation  //
// and distributed under the MIT License (MIT).                               //
//                                                                            //
// Copyright (c) Guillaume Blanc                                              //
//                                                                            //
// Permission is hereby granted, free of charge, to any person obtaining a    //
// copy of this software and associated documentation files (the "Software"), //
// to deal in the Software without restriction, including without limitation  //
// the rights to use, copy, modify, merge, publish, distribute, sublicense,   //
// and/or sell copies of the Software, and to permit persons to whom the      //
// Software is furnished to do so, subject to the following conditions:       //
//                                                                            //
// The above copyright notice and this permission notice shall be included in //
// all copies or substantial portions of the Software.                        //
//                                                                            //
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR //
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,   //
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL    //
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER //
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING    //
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER        //
// DEALINGS IN THE SOFTWARE.                                                  //
//                                                                            //
//----------------------------------------------------------------------------//

#define OZZ_INCLUDE_PRIVATE_HEADER  // Allows to include private headers.

#include "camera.h"

#include "ozz/base/log.h"
#include "ozz/base/maths/box.h"
#include "ozz/base/maths/math_constant.h"
#include "ozz/base/maths/math_ex.h"
#include "ozz/base/platform.h"

#include "framework/application.h"
#include "framework/imgui.h"

#include "renderer_impl.h"

using ozz::math::Float2;
using ozz::math::Float3;
using ozz::math::Float4x4;

namespace ozz {
namespace sample {
namespace internal {

// Declares camera navigation constants.
const float kDefaultDistance = 8.f;
const Float3 kDefaultCenter = Float3(0.f, .5f, 0.f);
const Float2 kDefaultAngle =
    Float2(-ozz::math::kPi * 1.f / 12.f, ozz::math::kPi * 1.f / 5.f);
const float kAngleFactor = .01f;
const float kDistanceFactor = .1f;
const float kScrollFactor = .03f;
const float kPanFactor = .05f;
const float kKeyboardFactor = 100.f;
const float kNear = .01f;
const float kFar = 1000.f;
const float kFovY = ozz::math::kPi / 3.f;
const float kFrameAllZoomOut = 1.3f;  // 30% bigger than the scene.

// Setups initial values.
Camera::Camera()
    : projection_(Float4x4::identity()),
      projection_2d_(Float4x4::identity()),
      view_(Float4x4::identity()),
      view_proj_(Float4x4::identity()),
      angles_(kDefaultAngle),
      center_(kDefaultCenter),
      distance_(kDefaultDistance),
      mouse_last_x_(0),
      mouse_last_y_(0),
      mouse_last_wheel_(0),
      auto_framing_(true) {}

Camera::~Camera() {}

void Camera::Update(const math::Box& _box, float _delta_time,
                    bool _first_frame) {
  // Frame the scene according to the provided box.
  if (_box.is_valid()) {
    if (auto_framing_ || _first_frame) {
      center_ = (_box.max + _box.min) * .5f;
      if (_first_frame) {
        const float radius = Length(_box.max - _box.min) * .5f;
        distance_ = radius * kFrameAllZoomOut / tanf(kFovY * .5f);
      }
    }
  }

  // Update manual controls.
  const Controls controls = UpdateControls(_delta_time);

  // Disable autoframing according to inputs.
  auto_framing_ &=
      !controls.panning && !controls.zooming && !controls.zooming_wheel;
}

void Camera::Update(const math::Float4x4& _transform, const math::Box& _box,
                    float _delta_time, bool _first_frame) {
  // Extract distance and angles such that theu are coherent when switching out
  // of auto_framing_.
  if (_box.is_valid()) {
    if (auto_framing_ || _first_frame) {
      // Extract components from the view martrix.
      ozz::math::Float3 camera_dir;
      ozz::math::Store3PtrU(-ozz::math::Normalize3(_transform.cols[2]),
                            &camera_dir.x);
      ozz::math::Float3 camera_pos;
      ozz::math::Store3PtrU(_transform.cols[3], &camera_pos.x);

      // Arbitrary decides that distance (focus point) is from camera to scene
      // center.
      const ozz::math::Float3 box_center_ = (_box.max + _box.min) * .5f;
      distance_ = Length(box_center_ - camera_pos);
      center_ = camera_pos + camera_dir * distance_;
      angles_.x = asinf(camera_dir.y);
      angles_.y = atan2(-camera_dir.x, -camera_dir.z);
    }
  }

  // Update manual controls.
  const Controls controls = UpdateControls(_delta_time);

  // Disable autoframing according to inputs.
  auto_framing_ &= !controls.panning && !controls.rotating &&
                   !controls.zooming && !controls.zooming_wheel;

  if (auto_framing_) {
    view_ = Invert(_transform);
  }
}

Camera::Controls Camera::UpdateControls(float _delta_time) {
  Controls controls;
  controls.zooming = false;
  controls.zooming_wheel = false;
  controls.rotating = false;
  controls.panning = false;

  // Mouse wheel + SHIFT activates Zoom.
  if (glfwGetKey(GLFW_KEY_LSHIFT) == GLFW_PRESS) {
    const int w = glfwGetMouseWheel();
    const int dw = w - mouse_last_wheel_;
    mouse_last_wheel_ = w;
    if (dw != 0) {
      controls.zooming_wheel = true;
      distance_ *= 1.f + -dw * kScrollFactor;
    }
  } else {
    mouse_last_wheel_ = glfwGetMouseWheel();
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
      timed_factor * (glfwGetKey(GLFW_KEY_LEFT) - glfwGetKey(GLFW_KEY_RIGHT));
  const int kdy =
      timed_factor * (glfwGetKey(GLFW_KEY_DOWN) - glfwGetKey(GLFW_KEY_UP));
  const bool keyboard_interact = kdx || kdy;

  // Computes composed keyboard and mouse dx and dy.
  const int dx = mdx + kdx;
  const int dy = mdy + kdy;

  // Mouse right button activates Zoom, Pan and Orbit modes.
  if (keyboard_interact ||
      glfwGetMouseButton(GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS) {
    if (glfwGetKey(GLFW_KEY_LSHIFT) == GLFW_PRESS) {  // Zoom mode.
      controls.zooming = true;

      distance_ += dy * kDistanceFactor;
    } else if (glfwGetKey(GLFW_KEY_LALT) == GLFW_PRESS) {  // Pan mode.
      controls.panning = true;

      const float dx_pan = -dx * kPanFactor;
      const float dy_pan = -dy * kPanFactor;

      // Moves along camera axes.
      math::Float4x4 transpose = Transpose(view_);
      math::Float3 right_transpose, up_transpose;
      math::Store3PtrU(transpose.cols[0], &right_transpose.x);
      math::Store3PtrU(transpose.cols[1], &up_transpose.x);
      center_ = center_ + right_transpose * dx_pan + up_transpose * dy_pan;
    } else {  // Orbit mode.
      controls.rotating = true;

      angles_.x = fmodf(angles_.x - dy * kAngleFactor, ozz::math::k2Pi);
      angles_.y = fmodf(angles_.y - dx * kAngleFactor, ozz::math::k2Pi);
    }
  }

  // Build the model view matrix components.
  const Float4x4 center = Float4x4::Translation(
      math::simd_float4::Load(center_.x, center_.y, center_.z, 1.f));
  const Float4x4 y_rotation = Float4x4::FromAxisAngle(
      math::simd_float4::y_axis(), math::simd_float4::Load1(angles_.y));
  const Float4x4 x_rotation = Float4x4::FromAxisAngle(
      math::simd_float4::x_axis(), math::simd_float4::Load1(angles_.x));
  const Float4x4 distance =
      Float4x4::Translation(math::simd_float4::Load(0.f, 0.f, distance_, 1.f));

  // Concatenate view matrix components.
  view_ = Invert(center * y_rotation * x_rotation * distance);

  return controls;
}

void Camera::Reset(const math::Float3& _center, const math::Float2& _angles,
                   float _distance) {
  center_ = _center;
  angles_ = _angles;
  distance_ = _distance;
}

void Camera::OnGui(ImGui* _im_gui) {
  const char* controls_label =
      "-RMB: Rotate\n"
      "-Shift + Wheel: Zoom\n"
      "-Shift + RMB: Zoom\n"
      "-Alt + RMB: Pan\n";
  _im_gui->DoLabel(controls_label, ImGui::kLeft, false);

  _im_gui->DoCheckBox("Automatic", &auto_framing_);
}

void Camera::Bind3D() {
  // Updates internal vp matrix.
  view_proj_ = projection_ * view_;
}

void Camera::Bind2D() {
  // Updates internal vp matrix. View matrix is identity.
  view_proj_ = projection_2d_;
}

void Camera::Resize(int _width, int _height) {
  // Handle empty windows.
  if (_width <= 0 || _height <= 0) {
    projection_ = ozz::math::Float4x4::identity();
    projection_2d_ = ozz::math::Float4x4::identity();
    return;
  }

  // Compute the 3D projection matrix.
  const float ratio = 1.f * _width / _height;
  const float h = tan(kFovY * .5f) * kNear;
  const float w = h * ratio;

  projection_.cols[0] = math::simd_float4::Load(kNear / w, 0.f, 0.f, 0.f);
  projection_.cols[1] = math::simd_float4::Load(0.f, kNear / h, 0.f, 0.f);
  projection_.cols[2] =
      math::simd_float4::Load(0.f, 0.f, -(kFar + kNear) / (kFar - kNear), -1.f);
  projection_.cols[3] = math::simd_float4::Load(
      0.f, 0.f, -(2.f * kFar * kNear) / (kFar - kNear), 0.f);

  // Computes the 2D projection matrix.
  projection_2d_.cols[0] = math::simd_float4::Load(2.f / _width, 0.f, 0.f, 0.f);
  projection_2d_.cols[1] =
      math::simd_float4::Load(0.f, 2.f / _height, 0.f, 0.f);
  projection_2d_.cols[2] = math::simd_float4::Load(0.f, 0.f, -2.0f, 0.f);
  projection_2d_.cols[3] = math::simd_float4::Load(-1.f, -1.f, 0.f, 1.f);
}
}  // namespace internal
}  // namespace sample
}  // namespace ozz
