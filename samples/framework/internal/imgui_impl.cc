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

#include "imgui_impl.h"

#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstring>

#include "ozz/base/maths/math_constant.h"
#include "ozz/base/maths/math_ex.h"
#include "ozz/base/memory/allocator.h"

#include "immediate.h"
#include "renderer_impl.h"

namespace ozz {
namespace sample {
namespace internal {

const GLubyte kPanelBackgroundColor[4] = {0x30, 0x30, 0x30, 0x80};
const GLubyte kPanelBorderColor[4] = {0x20, 0x20, 0x20, 0xff};
const GLubyte kPanelTitleColor[4] = {0x20, 0x20, 0x20, 0xf0};
const GLubyte kPanelTitleTextColor[4] = {0xa0, 0xa0, 0xa0, 0xff};

const GLubyte kWidgetBackgroundColor[4] = {0x20, 0x20, 0x20, 0xff};
const GLubyte kWidgetBorderColor[4] = {0x70, 0x70, 0x70, 0xff};

const GLubyte kWidgetDisabledBackgroundColor[4] = {0x30, 0x30, 0x30, 0xff};
const GLubyte kWidgetDisabledBorderColor[4] = {0x50, 0x50, 0x50, 0xff};

const GLubyte kWidgetHotBackgroundColor[4] = {0x40, 0x40, 0x40, 0xff};
const GLubyte kWidgetHotBorderColor[4] = {0xc7, 0x9a, 0x40, 0xff};

const GLubyte kWidgetActiveBackgroundColor[4] = {0xc7, 0x9a, 0x40, 0xff};
const GLubyte kWidgetActiveBorderColor[4] = {0x30, 0x30, 0x30, 0xff};

const GLubyte kWidgetTextColor[4] = {0xa0, 0xa0, 0xa0, 0xff};
const GLubyte kWidgetDisabledTextColor[4] = {0x60, 0x60, 0x60, 0xff};

const GLubyte kGraphBackgroundColor[4] = {0x20, 0x20, 0x20, 0xff};
const GLubyte kGraphPlotColor[4] = {0xc7, 0x9a, 0x40, 0xff};

const GLubyte kSliderBackgroundColor[4] = {0x20, 0x20, 0x20, 0xff};
const GLubyte kSliderCursorColor[4] = {0x70, 0x70, 0x70, 0xff};
const GLubyte kSliderCursorHotColor[4] = {0x80, 0x80, 0x80, 0xff};
const GLubyte kSliderDisabledCursorColor[4] = {0x70, 0x70, 0x70, 0xff};

const GLubyte kCursorColor[4] = {0xf0, 0xf0, 0xf0, 0xff};
const float kCursorSize = 16.f;

const float kGraphHeightFactor = 3.f;
const int kGraphLabelDigits = 5;

const float kTextMarginX = 2.f;
const float kWidgetRoundRectRadius = 2.f;
const float kWidgetCursorWidth = 8.f;
const float kWidgetHeight = 13.f;
const float kWidgetMarginX = 6.f;
const float kWidgetMarginY = 4.f;
const float kSliderRoundRectRadius = 1.f;
const float kButtonRoundRectRadius = 4.f;
const float kPanelRoundRectRadius = 1.f;
const float kPanelMarginX = 2.f;
const float kPanelTitleMarginY = 1.f;

// The radius of the precomputed circle.
const float kCircleRadius = 32.f;

bool FormatFloat(float _value, char* _string, const char* _string_end) {
  // Requires 8 fixed characters count + digits of precision + \0.
  static const int precision = 2;
  if (!_string || _string_end - _string < 8 + precision + 1) {
    return false;
  }
  std::sprintf(_string, "%.2g\n", _value);

  // Removes unnecessary '0' digits in the exponent.
  char* exponent = strchr(_string, 'e');
  char* last_zero = strrchr(_string, '0');
  if (exponent && last_zero > exponent) {
    memmove(exponent + 2,   // After exponent and exponent's sign.
            last_zero + 1,  // From the first character after the exponent.
            strlen(last_zero + 1) + 1);  // The remaining characters count.
  }
  return true;
}

ImGuiImpl::ImGuiImpl()
    : hot_item_(0),
      active_item_(0),
      auto_gen_id_(0),
      glyph_texture_(0),
      renderer_(nullptr) {
  InitializeCircle();
  InitalizeFont();
}

ImGuiImpl::~ImGuiImpl() { DestroyFont(); }

void ImGuiImpl::BeginFrame(const Inputs& _inputs, const math::RectInt& _rect,
                           RendererImpl* _renderer) {
  if (!containers_.empty()) {
    return;
  }

  // Stores renderer
  renderer_ = _renderer;

  // Store inputs.
  inputs_ = _inputs;

  // Sets no widget to hot for the current frame.
  hot_item_ = 0;

  // Regenerates widgets ids from scratch.
  auto_gen_id_ = 0;

  // Reset container stack info.
  const math::RectFloat rect(
      static_cast<float>(_rect.left), static_cast<float>(_rect.bottom),
      static_cast<float>(_rect.width), static_cast<float>(_rect.height));
  Container container = {rect, rect.height - kWidgetHeight, false};
  containers_.push_back(container);

  // Setup GL context in order to render layered the widgets.
  GL(Clear(GL_DEPTH_BUFFER_BIT));

  // Enables blending
  GL(Enable(GL_BLEND));
  GL(BlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));
}

void ImGuiImpl::EndFrame() {
  // Pops the frame container.
  containers_.pop_back();

  if (inputs_.lmb_pressed == false) {
    // Clear the active item if the mouse isn't down.
    active_item_ = 0;
  } else {
    // If the mouse is clicked, but no widget is active, we need to mark the
    // active item unavailable so that we won't activate the next widget we drag
    // the cursor onto.
    if (active_item_ == 0) {
      active_item_ = -1;
    }
  }

  // Renders mouse cursor.
  {
    GlImmediatePC im(renderer_->immediate_renderer(), GL_LINE_LOOP,
                     ozz::math::Float4x4::identity());
    GlImmediatePC::Vertex v = {
        {0.f, 0.f, 0.f},
        {kCursorColor[0], kCursorColor[1], kCursorColor[2], kCursorColor[3]}};

    v.pos[0] = static_cast<float>(inputs_.mouse_x);
    v.pos[1] = static_cast<float>(inputs_.mouse_y);
    im.PushVertex(v);
    v.pos[0] = static_cast<float>(inputs_.mouse_x) + kCursorSize;
    v.pos[1] = static_cast<float>(inputs_.mouse_y) - kCursorSize / 2.f;
    im.PushVertex(v);
    v.pos[0] = static_cast<float>(inputs_.mouse_x) + kCursorSize / 2.f;
    v.pos[1] = static_cast<float>(inputs_.mouse_y) - kCursorSize / 2.f;
    im.PushVertex(v);
    v.pos[0] = static_cast<float>(inputs_.mouse_x) + kCursorSize / 2.f;
    v.pos[1] = static_cast<float>(inputs_.mouse_y) - kCursorSize;
    im.PushVertex(v);
  }

  // Restores blending
  GL(Disable(GL_BLEND));

  // Release renderer.
  renderer_ = nullptr;
}

bool ImGuiImpl::AddWidget(float _height, math::RectFloat* _rect) {
  if (containers_.empty()) {
    return false;
  }

  // Get current container.
  Container& container = containers_.back();

  // Early out if outside of the container.
  // But don't modify current container's state.
  if (container.offset_y < kWidgetMarginY + _height) {
    return false;
  }

  // Computes widget rect and update internal offset in the container.
  container.offset_y -= _height;

  _rect->left = container.rect.left + kWidgetMarginX;
  _rect->bottom = container.rect.bottom + container.offset_y;
  _rect->width = container.rect.width - kWidgetMarginX * 2.f;
  _rect->height = _height;

  // Includes margin for the next widget.
  container.offset_y -= kWidgetMarginY;

  return true;
}

bool ImGuiImpl::ButtonLogic(const math::RectFloat& _rect, int _id, bool* _hot,
                            bool* _active) {
  // Checks whether the button should be hot.
  if (_rect.is_inside(static_cast<float>(inputs_.mouse_x),
                      static_cast<float>(inputs_.mouse_y))) {
    // Becomes hot if no other item is active.
    hot_item_ = active_item_ <= 0 || active_item_ == _id ? _id : 0;

    // If the mouse button is down and no other widget is active,
    // then this one becomes active.
    if (active_item_ == 0 && inputs_.lmb_pressed) {
      active_item_ = _id;
    }
  }

  // Update button state.
  const bool hot = hot_item_ == _id;
  *_hot = hot;
  const bool active = active_item_ == _id;
  *_active = active;

  // Finally checks if the button has been triggered. If button is hot and
  // active, but mouse button is not down, the user must have clicked the
  // button.
  return !inputs_.lmb_pressed && hot && active;
}

void ImGuiImpl::BeginContainer(const char* _title, const math::RectFloat* _rect,
                               bool* _open, bool _constrain) {
  if (containers_.empty()) {
    return;
  }

  // Adds the new container to the stack.
  containers_.resize(containers_.size() + 1);
  const Container& parent = containers_[containers_.size() - 2];
  Container& container = containers_[containers_.size() - 1];

  // Does the container have a header to render ?
  const bool header = _title || _open;
  const float header_height = header ? kWidgetHeight + kPanelTitleMarginY : 0.f;

  if (_rect) {
    // Crops _rect in parent's rect.
    container.rect.left = parent.rect.left + _rect->left;
    container.rect.bottom = parent.rect.bottom + _rect->bottom;
    container.rect.width = math::Max(
        0.f, math::Min(parent.rect.width - _rect->left, _rect->width));
    container.rect.height = math::Max(
        0.f, math::Min(parent.rect.height - _rect->bottom, _rect->height));
  } else {
    // Concatenate to the parent's rect.
    container.rect.left = parent.rect.left + kPanelMarginX;
    container.rect.bottom = parent.rect.bottom;
    container.rect.width =
        math::Max(0.f, parent.rect.width - kPanelMarginX * 2.f);
    container.rect.height = parent.offset_y;
  }

  // Shrinks rect if the container is closed.
  if (_open && !*_open) {
    const float closed_height = header_height;
    container.rect.bottom = container.rect.top() - closed_height;
    container.rect.height = closed_height;
  }

  container.offset_y = container.rect.height;
  container.constrain = _constrain;

  // Early out if there's not enough space for a container.
  // Includes top panel margins.
  if (container.offset_y < header_height) {
    container.offset_y = 0;
    return;
  }

  // Early out if there's no container title.
  if (!header) {
    // Adds margin before the new widget.
    container.offset_y -= kWidgetMarginY;
    return;
  }

  ++auto_gen_id_;

  // Inserts header.
  container.offset_y -= header_height;

  const math::RectFloat title_rect(container.rect.left,
                                   container.rect.bottom + container.offset_y,
                                   container.rect.width, header_height);

  // Don't display any arrow if _open is nullptr.
  const float arrow_size = _open != nullptr ? kWidgetHeight : 0;
  const math::RectFloat open_close_rect(title_rect.left, title_rect.bottom,
                                        arrow_size, kWidgetHeight);

  const math::RectFloat label_rect(
      title_rect.left + arrow_size + kTextMarginX, title_rect.bottom,
      title_rect.width - arrow_size - kTextMarginX, kWidgetHeight);

  // Adds a margin before the next widget only if it is opened.
  if (!_open || *_open) {
    container.offset_y -= kWidgetMarginY;
  }

  // Renders title background.
  FillRect(title_rect, kPanelRoundRectRadius, kPanelTitleColor);
  if (_title) {
    Print(_title, label_rect, kWest, kPanelTitleTextColor);
  }

  // Handles open close button.
  bool hot = false, active = false;
  if (_open) {
    if (ButtonLogic(title_rect, auto_gen_id_, &hot, &active)) {
      // Swap open/close state if button was clicked.
      *_open = !*_open;
    }

    // Renders arrow.
    {
      GlImmediatePC im(renderer_->immediate_renderer(), GL_TRIANGLES,
                       ozz::math::Float4x4::identity());
      GlImmediatePC::Vertex v = {
          {0.f, 0.f, 0.f},
          {kPanelTitleTextColor[0], kPanelTitleTextColor[1],
           kPanelTitleTextColor[2], kPanelTitleTextColor[3]}};

      if (*_open) {
        v.pos[0] = open_close_rect.left + 3.f;
        v.pos[1] = open_close_rect.bottom + 3.f;
        im.PushVertex(v);
        v.pos[0] = open_close_rect.left + 11.f;
        v.pos[1] = open_close_rect.bottom + 7.f;
        im.PushVertex(v);
        v.pos[0] = open_close_rect.left + 3.f;
        v.pos[1] = open_close_rect.bottom + 11.f;
        im.PushVertex(v);
      } else {
        v.pos[0] = open_close_rect.left + 3.f;
        v.pos[1] = open_close_rect.bottom + 11.f;
        im.PushVertex(v);
        v.pos[0] = open_close_rect.left + 7.f;
        v.pos[1] = open_close_rect.bottom + 3.f;
        im.PushVertex(v);
        v.pos[0] = open_close_rect.left + 11.f;
        v.pos[1] = open_close_rect.bottom + 11.f;
        im.PushVertex(v);
      }
    }
  }
}

void ImGuiImpl::EndContainer() {
  // Get current container. Copy it as it's going to be remove from the vector.
  Container container = containers_.back();
  containers_.pop_back();
  Container& parent = containers_.back();

  // Shrinks container rect.
  if (container.constrain) {
    float final_height =
        math::Max(0.f, container.rect.height - container.offset_y);
    container.rect.bottom += container.offset_y;
    container.rect.height = final_height;
  }

  // Renders container background.
  if (container.rect.height > 0.f) {
    const ozz::math::SimdFloat4 translation =
        ozz::math::simd_float4::Load(0.f, 0.f, -.1f, 0.f);
    const ozz::math::Float4x4 transform =
        ozz::math::Float4x4::Translation(translation);
    FillRect(container.rect, kPanelRoundRectRadius, kPanelBackgroundColor,
             transform);
    StrokeRect(container.rect, kPanelRoundRectRadius, kPanelBorderColor);
  }

  // Applies changes to the parent container.
  parent.offset_y -= container.rect.height + kWidgetMarginY;
}

bool ImGuiImpl::DoButton(const char* _label, bool _enabled, bool* _state) {
  math::RectFloat rect;
  if (!AddWidget(kWidgetHeight, &rect)) {
    return false;
  }

  ++auto_gen_id_;

  // Do button logic.
  bool hot = false, active = false, clicked = false;
  if (_enabled) {
    clicked = ButtonLogic(rect, auto_gen_id_, &hot, &active);
  }

  // Changes state if it was clicked.
  if (_state) {
    if (clicked) {
      *_state = !*_state;
    }
    active |= *_state;  // Considered active if state is true.
  }

  // Renders the button.
  const GLubyte* background_color = kWidgetBackgroundColor;
  const GLubyte* border_color = kWidgetBorderColor;
  const GLubyte* text_color = kWidgetTextColor;

  float active_offset = 0.f;
  if (!_enabled) {
    background_color = kWidgetDisabledBackgroundColor;
    border_color = kWidgetDisabledBorderColor;
    text_color = kWidgetDisabledTextColor;
  } else if (active) {  // Button is 'active'.
    background_color = kWidgetActiveBackgroundColor;
    border_color = kWidgetActiveBorderColor;
    active_offset = 1.f;
  } else if (hot) {  // Hot but not 'active'.
    // Button is merely 'hot'.
    background_color = kWidgetHotBackgroundColor;
    border_color = kWidgetHotBorderColor;
  } else {
    // button is not hot, but it may be active. Use default colors.
  }

  FillRect(rect, kButtonRoundRectRadius, background_color);
  StrokeRect(rect, kButtonRoundRectRadius, border_color);

  // Renders button label.
  const math::RectFloat text_rect(
      rect.left + kButtonRoundRectRadius, rect.bottom - active_offset,
      rect.width - kButtonRoundRectRadius * 2.f, rect.height - active_offset);
  Print(_label, text_rect, kMiddle, text_color);

  return clicked;
}

bool ImGuiImpl::DoCheckBox(const char* _label, bool* _state, bool _enabled) {
  math::RectFloat widget_rect;
  if (!AddWidget(kWidgetHeight, &widget_rect)) {
    return false;
  }

  math::RectFloat check_rect(widget_rect.left, widget_rect.bottom,
                             kWidgetHeight,  // The check box is square.
                             widget_rect.height);

  ++auto_gen_id_;

  // Do button logic.
  bool hot = false, active = false, clicked = false;
  if (_enabled) {
    clicked = ButtonLogic(widget_rect, auto_gen_id_, &hot, &active);
  }

  // Changes check box state if it was clicked.
  if (clicked) {
    *_state = !*_state;
  }

  // Renders the check box.
  const GLubyte* background_color = kWidgetBackgroundColor;
  const GLubyte* border_color = kWidgetBorderColor;
  const GLubyte* check_color = kWidgetTextColor;
  const GLubyte* text_color = kWidgetTextColor;

  if (!_enabled) {
    background_color = kWidgetDisabledBackgroundColor;
    border_color = kWidgetDisabledBorderColor;
    check_color = kWidgetDisabledBorderColor;
    text_color = kWidgetDisabledTextColor;
  } else if (hot) {
    if (active) {
      // Button is both 'hot' and 'active'.
      background_color = kWidgetActiveBackgroundColor;
      border_color = kWidgetActiveBorderColor;
      check_color = kWidgetActiveBorderColor;
    } else {
      // Button is merely 'hot'.
      background_color = kWidgetHotBackgroundColor;
      border_color = kWidgetHotBorderColor;
      check_color = kWidgetHotBorderColor;
    }
  } else {
    // button is not hot, but it may be active. Use default colors.
  }

  FillRect(check_rect, 0, background_color);
  StrokeRect(check_rect, 0, border_color);

  // Renders the "check" mark.
  if (*_state) {
    GlImmediatePC im(renderer_->immediate_renderer(), GL_TRIANGLES,
                     ozz::math::Float4x4::identity());
    GlImmediatePC::Vertex v = {
        {0.f, 0.f, 0.f},
        {check_color[0], check_color[1], check_color[2], check_color[3]}};
    v.pos[0] = check_rect.left + check_rect.width / 8.f;
    v.pos[1] = check_rect.bottom + check_rect.height / 2.f;
    im.PushVertex(v);
    v.pos[0] = check_rect.left + check_rect.width / 2.f;
    v.pos[1] = check_rect.bottom + check_rect.height / 8.f;
    im.PushVertex(v);
    v.pos[0] = check_rect.left + check_rect.width / 2.f;
    v.pos[1] = check_rect.bottom + check_rect.height / 2.f;
    im.PushVertex(v);

    v.pos[0] = check_rect.left + check_rect.width / 3.f;
    v.pos[1] = check_rect.bottom + check_rect.height / 2.f;
    im.PushVertex(v);
    v.pos[0] = check_rect.left + check_rect.width / 2.f;
    v.pos[1] = check_rect.bottom + check_rect.height / 8.f;
    im.PushVertex(v);
    v.pos[0] = check_rect.right() - check_rect.width / 6.f;
    v.pos[1] = check_rect.top() - check_rect.height / 6.f;
    im.PushVertex(v);
  }

  // Renders button label.
  const math::RectFloat text_rect(
      check_rect.right() + kTextMarginX, widget_rect.bottom,
      widget_rect.width - check_rect.width - kTextMarginX, widget_rect.height);
  Print(_label, text_rect, kWest, text_color);

  return clicked;
}

bool ImGuiImpl::DoRadioButton(int _ref, const char* _label, int* _value,
                              bool _enabled) {
  math::RectFloat widget_rect;
  if (!AddWidget(kWidgetHeight, &widget_rect)) {
    return false;
  }

  math::RectFloat radio_rect(widget_rect.left, widget_rect.bottom,
                             kWidgetHeight,  // The check box is square.
                             widget_rect.height);

  ++auto_gen_id_;

  // Do button logic.
  bool hot = false, active = false, clicked = false;
  if (_enabled) {
    clicked = ButtonLogic(widget_rect, auto_gen_id_, &hot, &active);
  }

  // Changes check box state if it was clicked.
  if (clicked) {
    *_value = _ref;
  }

  // Renders the check box.
  const GLubyte* background_color = kWidgetBackgroundColor;
  const GLubyte* border_color = kWidgetBorderColor;
  const GLubyte* check_color = kWidgetBorderColor;
  const GLubyte* text_color = kWidgetTextColor;

  if (!_enabled) {
    background_color = kWidgetDisabledBackgroundColor;
    border_color = kWidgetDisabledBorderColor;
    check_color = kWidgetDisabledBorderColor;
    text_color = kWidgetDisabledBackgroundColor;
  } else if (hot) {
    if (active) {
      // Button is both 'hot' and 'active'.
      background_color = kWidgetActiveBackgroundColor;
      border_color = kWidgetActiveBorderColor;
      check_color = kWidgetActiveBorderColor;
    } else {
      // Button is merely 'hot'.
      background_color = kWidgetHotBackgroundColor;
      border_color = kWidgetHotBorderColor;
      check_color = kWidgetHotBorderColor;
    }
  } else {
    // button is not hot, but it may be active. Use default colors.
  }

  FillRect(radio_rect, kWidgetRoundRectRadius, background_color);
  StrokeRect(radio_rect, kWidgetRoundRectRadius, border_color);

  // Renders the "checked" button.
  if (*_value == _ref) {
    const math::RectFloat checked_rect(
        radio_rect.left + 1.f, radio_rect.bottom + 1.f, radio_rect.width - 3.f,
        radio_rect.height - 3.f);
    FillRect(checked_rect, kWidgetRoundRectRadius, check_color);
  }

  // Renders button label.
  const math::RectFloat text_rect(
      radio_rect.right() + kTextMarginX, widget_rect.bottom,
      widget_rect.width - radio_rect.width - kTextMarginX, widget_rect.height);
  Print(_label, text_rect, kWest, text_color);

  return clicked;
}

namespace {
float FindMax(float _value) {
  if (_value == 0.f) {
    return 0.f;
  }
  const float mexp = floor(log10(_value));
  const float mpow = pow(10.f, mexp);
  return ceil(_value / mpow) * 1.5f * mpow;
}
}  // namespace

void ImGuiImpl::DoGraph(const char* _label, float _min, float _max, float _mean,
                        const float* _value_cursor, const float* _value_begin,
                        const float* _value_end) {
  // Computes widget rect.
  math::RectFloat widget_rect;
  const float label_height =
      _label ? (kWidgetMarginY + font_.glyph_height) : 0.f;
  const float height = kWidgetHeight * kGraphHeightFactor + label_height;
  if (!AddWidget(height, &widget_rect)) {
    return;
  }

  const float label_width =
      static_cast<float>(kGraphLabelDigits * font_.glyph_width);

  const math::RectFloat graph_rect(
      widget_rect.left, widget_rect.bottom,
      widget_rect.width - label_width - kTextMarginX,
      kWidgetHeight * kGraphHeightFactor);

  // Renders background and borders.
  FillRect(graph_rect, 0, kGraphBackgroundColor);
  StrokeRect(graph_rect, 0, kWidgetBorderColor);

  // Render labels.
  char sz[16];
  const char* sz_end = sz + OZZ_ARRAY_SIZE(sz);
  const math::RectFloat max_rect(
      widget_rect.left, graph_rect.top() - font_.glyph_height,
      widget_rect.width, static_cast<float>(font_.glyph_height));
  if (FormatFloat(_max, sz, sz_end)) {
    Print(sz, max_rect, kEst, kWidgetTextColor);
  }

  const math::RectFloat mean_rect(
      widget_rect.left,
      graph_rect.bottom + graph_rect.height / 2.f - font_.glyph_height / 2.f,
      widget_rect.width, static_cast<float>(font_.glyph_height));
  if (FormatFloat(_mean, sz, sz_end)) {
    Print(sz, mean_rect, kEst, kWidgetTextColor);
  }

  const math::RectFloat min_rect(widget_rect.left, graph_rect.bottom,
                                 widget_rect.width,
                                 static_cast<float>(font_.glyph_height));
  if (FormatFloat(_min, sz, sz_end)) {
    Print(sz, min_rect, kEst, kWidgetTextColor);
  }

  // Prints title on the left.
  if (_label) {
    const math::RectFloat label_rect(
        widget_rect.left, widget_rect.top() - font_.glyph_height,
        widget_rect.width, static_cast<float>(font_.glyph_height));
    Print(_label, label_rect, kNorthWest, kWidgetTextColor);
  }

  // Renders the graph.
  if (_value_end - _value_begin >= 2) {  // Reject invalid or to small inputs.
    const float abscissa_min = graph_rect.bottom + 1.f;
    const float abscissa_max = graph_rect.top() - 1.f;
    // Computes a new max value, rounded up to be more stable.
    const float graph_max = FindMax(_max);
    const float abscissa_scale =
        (abscissa_max - abscissa_min) / (graph_max - _min);
    const float abscissa_begin = graph_rect.bottom + 1.f;
    const float ordinate_inc =
        -(graph_rect.width - 2.f) / (_value_end - _value_begin - 1.f);
    float ordinate_current = graph_rect.right() - 1.f;

    GlImmediatePC im(renderer_->immediate_renderer(), GL_LINE_STRIP,
                     ozz::math::Float4x4::identity());
    GlImmediatePC::Vertex v = {{0.f, 0.f, 0.f},
                               {kGraphPlotColor[0], kGraphPlotColor[1],
                                kGraphPlotColor[2], kGraphPlotColor[3]}};

    const float* current = _value_cursor;
    const float* end = _value_end;
    while (current < end) {
      const float abscissa =
          abscissa_begin + abscissa_scale * (*current - _min);
      const float clamped_abscissa =
          math::Clamp(abscissa_min, abscissa, abscissa_max);

      v.pos[0] = ordinate_current;
      v.pos[1] = clamped_abscissa;
      im.PushVertex(v);
      ordinate_current += ordinate_inc;

      ++current;
      if (current == _value_end) {  // Looping...
        end = _value_cursor;
        current = _value_begin;
      }
    }
  }
}

bool ImGuiImpl::DoSlider(const char* _label, float _min, float _max,
                         float* _value, float _pow, bool _enabled) {
  math::RectFloat rect;
  if (!AddWidget(kWidgetHeight, &rect)) {
    return false;
  }

  auto_gen_id_++;

  // Calculate mouse cursor's relative y offset.
  const float initial_value = *_value;
  const float clamped_value = ozz::math::Clamp(_min, initial_value, _max);

  // Check for hotness.
  bool hot = false, active = false;
  if (_enabled) {
    // Includes the cursor size in the pick region.
    math::RectFloat pick_rect = rect;
    pick_rect.left -= kWidgetCursorWidth / 2.f;
    pick_rect.width += kWidgetCursorWidth;
    ButtonLogic(pick_rect, auto_gen_id_, &hot, &active);

    // A slider is active on lmb pressed, not released. It's different to the
    // usual button behavior.
    active &= inputs_.lmb_pressed;
  }

  // Render the scrollbar
  const GLubyte* background_color = kSliderBackgroundColor;
  const GLubyte* border_color = kWidgetBorderColor;
  const GLubyte* slider_color = kSliderCursorColor;
  const GLubyte* slider_border_color = kWidgetBorderColor;
  const GLubyte* text_color = kWidgetTextColor;

  if (!_enabled) {
    background_color = kWidgetDisabledBackgroundColor;
    border_color = kWidgetDisabledBorderColor;
    slider_color = kSliderDisabledCursorColor;
    slider_border_color = kWidgetDisabledBorderColor;
    text_color = kWidgetDisabledTextColor;
  } else if (hot) {
    if (active) {
      // Button is both 'hot' and 'active'.
      slider_color = kWidgetActiveBackgroundColor;
      slider_border_color = kWidgetActiveBorderColor;
    } else {
      // Button is merely 'hot'.
      slider_color = kSliderCursorHotColor;
      slider_border_color = kWidgetHotBorderColor;
    }
  } else {
    // button is not hot, but it may be active. Use default colors.
  }

  // Update widget value.
  const float pow_min = powf(_min, _pow);
  const float pow_max = powf(_max, _pow);
  float pow_value = powf(clamped_value, _pow);
  if (_enabled) {
    if (active) {
      int mousepos = inputs_.mouse_x - static_cast<int>(rect.left);
      if (mousepos < 0) {
        mousepos = 0;
      }
      if (mousepos > rect.width) {
        mousepos = static_cast<int>(rect.width);
      }
      pow_value = (mousepos * (pow_max - pow_min)) / rect.width + pow_min;
      *_value = ozz::math::Clamp(_min, powf(pow_value, 1.f / _pow), _max);
    } else {
      // Clamping is only applied if the widget is enabled.
      *_value = clamped_value;
    }
  }

  // Renders slider's rail.
  const math::RectFloat rail_rect(rect.left, rect.bottom, rect.width,
                                  rect.height);
  FillRect(rail_rect, kSliderRoundRectRadius, background_color);
  StrokeRect(rail_rect, kSliderRoundRectRadius, border_color);

  // Finds cursor position and rect.
  const float cursor =
      floorf((rect.width * (pow_value - pow_min)) / (pow_max - pow_min));
  const math::RectFloat cursor_rect(
      rect.left + cursor - kWidgetCursorWidth / 2.f, rect.bottom - 1.f,
      kWidgetCursorWidth, rect.height + 2.f);
  FillRect(cursor_rect, kSliderRoundRectRadius, slider_color);
  StrokeRect(cursor_rect, kSliderRoundRectRadius, slider_border_color);

  const math::RectFloat text_rect(
      rail_rect.left + kSliderRoundRectRadius, rail_rect.bottom,
      rail_rect.width - kSliderRoundRectRadius * 2.f, rail_rect.height);
  Print(_label, text_rect, kMiddle, text_color);

  // Returns true if the value has changed or if it was clamped in _min / _max
  // bounds.
  return initial_value != *_value;
}

bool ImGuiImpl::DoSlider(const char* _label, int _min, int _max, int* _value,
                         float _pow, bool _enabled) {
  const float fmin = static_cast<float>(_min);
  const float fmax = static_cast<float>(_max);
  float fvalue = static_cast<float>(*_value);
  bool changed = DoSlider(_label, fmin, fmax, &fvalue, _pow, _enabled);
  *_value = static_cast<int>(fvalue);
  return changed;
}

void ImGuiImpl::DoLabel(const char* _label, Justification _justification,
                        bool _single_line) {
  if (containers_.empty()) {
    return;
  }
  // Get current container.
  Container& container = containers_.back();

  // Computes widget rect and update internal offset in the panel.
  math::RectFloat rect;
  if (!AddWidget(_single_line
                     ? font_.glyph_height
                     : math::Max(0.f, container.offset_y - kWidgetMarginY),
                 &rect)) {
    return;
  }

  PrintLayout layout[] = {kNorthWest, kNorth, kNorthEst};
  const float offset =
      Print(_label, rect, layout[_justification], kWidgetTextColor);

  if (!_single_line) {  // Resume following widgets below the label.
    container.offset_y = offset - kWidgetMarginY;
  }
}

void ImGuiImpl::FillRect(const math::RectFloat& _rect, float _radius,
                         const GLubyte _rgba[4]) const {
  FillRect(_rect, _radius, _rgba, ozz::math::Float4x4::identity());
}

void ImGuiImpl::FillRect(const math::RectFloat& _rect, float _radius,
                         const GLubyte _rgba[4],
                         const ozz::math::Float4x4& _transform) const {
  if (_radius <= 0.f) {
    GlImmediatePC im(renderer_->immediate_renderer(), GL_TRIANGLE_STRIP,
                     _transform);
    GlImmediatePC::Vertex v = {{0.f, 0.f, 0.f},
                               {_rgba[0], _rgba[1], _rgba[2], _rgba[3]}};
    v.pos[0] = _rect.left;
    v.pos[1] = _rect.top();
    im.PushVertex(v);
    v.pos[1] = _rect.bottom;
    im.PushVertex(v);
    v.pos[0] = _rect.left + _rect.width;
    v.pos[1] = _rect.top();
    im.PushVertex(v);
    v.pos[1] = _rect.bottom;
    im.PushVertex(v);
  } else {
    const float x = _rect.left + _radius;
    const float y = _rect.bottom + _radius;
    const float w = _rect.width - _radius * 2.f;
    const float h = _rect.height - _radius * 2.f;
    const float radius = _radius / kCircleRadius;

    GlImmediatePC im(renderer_->immediate_renderer(), GL_TRIANGLES, _transform);
    GlImmediatePC::Vertex v = {{0.f, 0.f, 0.f},
                               {_rgba[0], _rgba[1], _rgba[2], _rgba[3]}};
    for (int i = 1, j = i - 1; i <= kCircleSegments / 4; j = i++) {
      v.pos[0] = x + w;
      v.pos[1] = y + h;
      im.PushVertex(v);
      v.pos[0] = x + w + circle_[j][0] * radius;
      v.pos[1] = y + h + circle_[j][1] * radius;
      im.PushVertex(v);
      v.pos[0] = x + w + circle_[i][0] * radius;
      v.pos[1] = y + h + circle_[i][1] * radius;
      im.PushVertex(v);
    }
    for (int i = 1 + kCircleSegments / 4, j = i - 1;
         i <= 2 * kCircleSegments / 4; j = i++) {
      v.pos[0] = x;
      v.pos[1] = y + h;
      im.PushVertex(v);
      v.pos[0] = x + circle_[j][0] * radius;
      v.pos[1] = y + h + circle_[j][1] * radius;
      im.PushVertex(v);
      v.pos[0] = x + circle_[i][0] * radius;
      v.pos[1] = y + h + circle_[i][1] * radius;
      im.PushVertex(v);
    }
    for (int i = 1 + 2 * kCircleSegments / 4, j = i - 1;
         i <= 3 * kCircleSegments / 4; j = i++) {
      v.pos[0] = x;
      v.pos[1] = y;
      im.PushVertex(v);
      v.pos[0] = x + circle_[j][0] * radius;
      v.pos[1] = y + circle_[j][1] * radius;
      im.PushVertex(v);
      v.pos[0] = x + circle_[i][0] * radius;
      v.pos[1] = y + circle_[i][1] * radius;
      im.PushVertex(v);
    }
    for (int i = 1 + 3 * kCircleSegments / 4, j = i - 1;
         i <= 4 * kCircleSegments / 4; j = i++) {
      const int index = i % kCircleSegments;
      v.pos[0] = x + w;
      v.pos[1] = y;
      im.PushVertex(v);
      v.pos[0] = x + w + circle_[j][0] * radius;
      v.pos[1] = y + circle_[j][1] * radius;
      im.PushVertex(v);
      v.pos[0] = x + w + circle_[index][0] * radius;
      v.pos[1] = y + circle_[index][1] * radius;
      im.PushVertex(v);
    }

    v.pos[0] = _rect.left;
    v.pos[1] = y;
    im.PushVertex(v);
    v.pos[0] = _rect.right();
    v.pos[1] = y;
    im.PushVertex(v);
    v.pos[0] = _rect.right();
    v.pos[1] = y + h;
    im.PushVertex(v);

    v.pos[0] = _rect.right();
    v.pos[1] = y + h;
    im.PushVertex(v);
    v.pos[0] = _rect.left;
    v.pos[1] = y + h;
    im.PushVertex(v);
    v.pos[0] = _rect.left;
    v.pos[1] = y;
    im.PushVertex(v);

    v.pos[0] = x;
    v.pos[1] = _rect.bottom;
    im.PushVertex(v);
    v.pos[0] = x + w;
    v.pos[1] = _rect.bottom;
    im.PushVertex(v);
    v.pos[0] = x + w;
    v.pos[1] = y;
    im.PushVertex(v);

    v.pos[0] = x + w;
    v.pos[1] = y;
    im.PushVertex(v);
    v.pos[0] = x;
    v.pos[1] = y;
    im.PushVertex(v);
    v.pos[0] = x;
    v.pos[1] = _rect.bottom;
    im.PushVertex(v);

    v.pos[0] = x;
    v.pos[1] = _rect.top() - _radius;
    im.PushVertex(v);
    v.pos[0] = x + w;
    v.pos[1] = _rect.top() - _radius;
    im.PushVertex(v);
    v.pos[0] = x + w;
    v.pos[1] = _rect.top();
    im.PushVertex(v);

    v.pos[0] = x + w;
    v.pos[1] = _rect.top();
    im.PushVertex(v);
    v.pos[0] = x;
    v.pos[1] = _rect.top();
    im.PushVertex(v);
    v.pos[0] = x;
    v.pos[1] = _rect.top() - _radius;
    im.PushVertex(v);
  }
}

void ImGuiImpl::StrokeRect(const math::RectFloat& _rect, float _radius,
                           const GLubyte _rgba[4]) const {
  StrokeRect(_rect, _radius, _rgba, ozz::math::Float4x4::identity());
}

void ImGuiImpl::StrokeRect(const math::RectFloat& _rect, float _radius,
                           const GLubyte _rgba[4],
                           const ozz::math::Float4x4& _transform) const {
  // Lines rendering requires to coordinate to be in the pixel center.
  const ozz::math::SimdFloat4 translation =
      ozz::math::simd_float4::Load(-.5f, -.5f, 0.f, 0.f);
  const ozz::math::Float4x4 transform = Translate(_transform, translation);

  if (_radius <= 0.f) {
    GlImmediatePC im(renderer_->immediate_renderer(), GL_LINE_LOOP, transform);
    GlImmediatePC::Vertex v = {{0.f, 0.f, 0.f},
                               {_rgba[0], _rgba[1], _rgba[2], _rgba[3]}};
    v.pos[0] = _rect.left;
    v.pos[1] = _rect.bottom;
    im.PushVertex(v);
    v.pos[0] = _rect.left + _rect.width;
    im.PushVertex(v);
    v.pos[1] = _rect.top();
    im.PushVertex(v);
    v.pos[0] = _rect.left;
    im.PushVertex(v);
  } else {
    const float x = _rect.left + _radius;
    const float y = _rect.bottom + _radius;
    const float w = _rect.width - _radius * 2;
    const float h = _rect.height - _radius * 2;
    const float radius = _radius / kCircleRadius;

    GlImmediatePC im(renderer_->immediate_renderer(), GL_LINE_LOOP, transform);
    GlImmediatePC::Vertex v = {{0.f, 0.f, 0.f},
                               {_rgba[0], _rgba[1], _rgba[2], _rgba[3]}};
    for (int i = 0; i <= kCircleSegments / 4; ++i) {
      v.pos[0] = x + w + circle_[i][0] * radius;
      v.pos[1] = y + h + circle_[i][1] * radius;
      im.PushVertex(v);
    }
    for (int i = kCircleSegments / 4; i <= 2 * kCircleSegments / 4; ++i) {
      v.pos[0] = x + circle_[i][0] * radius;
      v.pos[1] = y + h + circle_[i][1] * radius;
      im.PushVertex(v);
    }
    for (int i = 2 * kCircleSegments / 4; i <= 3 * kCircleSegments / 4; ++i) {
      v.pos[0] = x + circle_[i][0] * radius;
      v.pos[1] = y + circle_[i][1] * radius;
      im.PushVertex(v);
    }
    for (int i = 3 * kCircleSegments / 4; i < 4 * kCircleSegments / 4; ++i) {
      v.pos[0] = x + w + circle_[i][0] * radius;
      v.pos[1] = y + circle_[i][1] * radius;
      im.PushVertex(v);
    }
    v.pos[0] = x + w + circle_[0][0] * radius;
    v.pos[1] = y + circle_[0][1] * radius;
    im.PushVertex(v);
  }
}

void ImGuiImpl::InitializeCircle() {
  assert((kCircleSegments & 3) == 0);  // kCircleSegments must be multiple of 4.

  for (int i = 0; i < kCircleSegments; ++i) {
    const float angle = i * math::k2Pi / kCircleSegments;
    const float cos = std::cos(angle) * kCircleRadius;
    const float sin = std::sin(angle) * kCircleRadius;
    circle_[i][0] = cos + (cos >= 0.f ? 0.5f : -0.5f);
    circle_[i][1] = sin + (sin >= 0.f ? 0.5f : -0.5f);
  }
}

static const unsigned char kFontPixelRawData[] = {
    0x00, 0x21, 0xb0, 0xa1, 0x04, 0x00, 0x08, 0x08, 0x40, 0x40, 0x00, 0x00,
    0x00, 0x02, 0x38, 0x60, 0xe1, 0xc0, 0xc7, 0x87, 0x3e, 0x38, 0x70, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x38, 0x63, 0xe1, 0xef, 0x1f, 0x9f, 0x9e, 0xee,
    0xf8, 0xf7, 0x77, 0x1d, 0xfb, 0x9c, 0x78, 0x73, 0xe1, 0xaf, 0xfd, 0xfb,
    0xf7, 0xc7, 0xdd, 0xf1, 0xc4, 0x07, 0x04, 0x00, 0x10, 0x03, 0x00, 0x00,
    0xc0, 0x07, 0x00, 0xc0, 0x20, 0x46, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x41, 0x04, 0x00, 0x00,
    0x00, 0x21, 0x20, 0xa3, 0x8a, 0x00, 0x08, 0x08, 0x41, 0xf0, 0x80, 0x00,
    0x00, 0x02, 0x44, 0x21, 0x12, 0x21, 0x44, 0x08, 0x22, 0x44, 0x88, 0x00,
    0x00, 0xc0, 0x30, 0x0c, 0x44, 0x21, 0x12, 0x24, 0x88, 0x88, 0xa2, 0x44,
    0x20, 0x22, 0x22, 0x0d, 0x99, 0x22, 0x24, 0x89, 0x12, 0x69, 0x28, 0x91,
    0x22, 0x44, 0x89, 0x11, 0x02, 0x01, 0x04, 0x00, 0x08, 0x01, 0x00, 0x00,
    0x40, 0x08, 0x00, 0x40, 0x00, 0x02, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x81, 0x02, 0x00, 0x00,
    0x00, 0x21, 0x21, 0x44, 0x04, 0x06, 0x08, 0x10, 0x20, 0x40, 0x80, 0x00,
    0x00, 0x04, 0x44, 0x20, 0x10, 0x21, 0x44, 0x10, 0x02, 0x44, 0x88, 0xc0,
    0xc1, 0x00, 0x08, 0x12, 0x4c, 0x51, 0x12, 0x04, 0x4a, 0x0a, 0x20, 0x44,
    0x20, 0x22, 0x42, 0x0d, 0x99, 0x22, 0x24, 0x89, 0x12, 0x01, 0x08, 0x91,
    0x22, 0x28, 0x50, 0x21, 0x02, 0x01, 0x0a, 0x00, 0x00, 0x71, 0x61, 0xe3,
    0x47, 0x1f, 0x1b, 0x58, 0xe1, 0xe2, 0xe1, 0x1d, 0x36, 0x1c, 0xd8, 0x6d,
    0xb1, 0xe7, 0xd9, 0xbb, 0xf7, 0xcd, 0xdd, 0xf0, 0x81, 0x02, 0x00, 0x00,
    0x00, 0x20, 0x03, 0xe4, 0x01, 0x88, 0x08, 0x10, 0x20, 0xa0, 0x80, 0x00,
    0x00, 0x04, 0x44, 0x20, 0x20, 0xc2, 0x47, 0x1e, 0x04, 0x38, 0x88, 0xc0,
    0xc6, 0x0f, 0x86, 0x02, 0x54, 0x51, 0xe2, 0x04, 0x4e, 0x0e, 0x20, 0x7c,
    0x20, 0x22, 0x82, 0x0a, 0x95, 0x22, 0x24, 0x89, 0x11, 0xc1, 0x08, 0x8a,
    0x2a, 0x10, 0x50, 0x41, 0x02, 0x01, 0x11, 0x00, 0x00, 0x89, 0x92, 0x24,
    0xc8, 0x88, 0x26, 0x64, 0x20, 0x22, 0x41, 0x0a, 0x99, 0x22, 0x64, 0x98,
    0xc2, 0x22, 0x08, 0x91, 0x22, 0x48, 0x89, 0x20, 0x81, 0x02, 0x00, 0x00,
    0x00, 0x20, 0x01, 0x43, 0x8e, 0x08, 0x00, 0x10, 0x20, 0xa7, 0xf0, 0x0f,
    0x80, 0x08, 0x44, 0x20, 0x40, 0x24, 0x40, 0x91, 0x04, 0x44, 0x78, 0x00,
    0x08, 0x00, 0x01, 0x04, 0x54, 0x51, 0x12, 0x04, 0x4a, 0x0a, 0x27, 0x44,
    0x21, 0x23, 0x82, 0x0a, 0x95, 0x22, 0x38, 0x89, 0xe0, 0x21, 0x08, 0x8a,
    0x2a, 0x10, 0x20, 0x41, 0x01, 0x01, 0x00, 0x00, 0x00, 0x79, 0x12, 0x04,
    0x4f, 0x88, 0x22, 0x44, 0x20, 0x23, 0x81, 0x0a, 0x91, 0x22, 0x44, 0x88,
    0x81, 0xc2, 0x08, 0x91, 0x2a, 0x30, 0x48, 0x40, 0x81, 0x02, 0x09, 0x00,
    0x00, 0x00, 0x03, 0xe4, 0x81, 0x15, 0x00, 0x10, 0x20, 0x00, 0x80, 0x00,
    0x00, 0x08, 0x44, 0x20, 0x80, 0x27, 0xe0, 0x91, 0x04, 0x44, 0x08, 0x00,
    0x06, 0x0f, 0x86, 0x08, 0x4c, 0xf9, 0x12, 0x04, 0x48, 0x08, 0x22, 0x44,
    0x21, 0x22, 0x42, 0x48, 0x95, 0x22, 0x20, 0x89, 0x20, 0x21, 0x08, 0x8a,
    0x2a, 0x28, 0x20, 0x81, 0x01, 0x01, 0x00, 0x00, 0x00, 0x89, 0x12, 0x04,
    0x48, 0x08, 0x22, 0x44, 0x20, 0x22, 0x81, 0x0a, 0x91, 0x22, 0x44, 0x88,
    0x80, 0x22, 0x08, 0x8a, 0x2a, 0x30, 0x50, 0x81, 0x01, 0x01, 0x16, 0x00,
    0x00, 0x00, 0x01, 0x47, 0x02, 0x92, 0x00, 0x10, 0x20, 0x00, 0x81, 0x80,
    0x0c, 0x10, 0x44, 0x21, 0x12, 0x20, 0x48, 0x91, 0x08, 0x44, 0x10, 0xc0,
    0xc1, 0x00, 0x08, 0x00, 0x40, 0x89, 0x12, 0x24, 0x88, 0x88, 0x22, 0x44,
    0x21, 0x22, 0x22, 0x48, 0x93, 0x22, 0x20, 0x89, 0x13, 0x21, 0x08, 0x84,
    0x2a, 0x44, 0x21, 0x11, 0x00, 0x81, 0x00, 0x00, 0x00, 0x89, 0x12, 0x24,
    0x48, 0x08, 0x22, 0x44, 0x20, 0x22, 0x41, 0x0a, 0x91, 0x22, 0x44, 0x88,
    0x82, 0x22, 0x29, 0x8a, 0x2a, 0x48, 0x31, 0x10, 0x81, 0x02, 0x00, 0x00,
    0x00, 0x20, 0x02, 0x81, 0x01, 0x0d, 0x00, 0x10, 0x20, 0x00, 0x81, 0x00,
    0x0c, 0x10, 0x38, 0xf9, 0xf1, 0xc0, 0xe7, 0x0e, 0x08, 0x38, 0xe0, 0xc1,
    0x80, 0xc0, 0x30, 0x18, 0x45, 0xdf, 0xe1, 0xcf, 0x1f, 0x9c, 0x1c, 0xee,
    0xf8, 0xc7, 0x37, 0xdd, 0xfb, 0x1c, 0x70, 0x73, 0x8a, 0xc3, 0x87, 0x04,
    0x14, 0xc6, 0x71, 0xf1, 0x00, 0x81, 0x00, 0x00, 0x00, 0x7f, 0xe1, 0xc3,
    0xe7, 0x9f, 0x1e, 0xee, 0xf8, 0x26, 0xe7, 0xdf, 0xfb, 0x9c, 0x78, 0x79,
    0xf3, 0xc1, 0xc6, 0xc4, 0x14, 0xcc, 0x21, 0xf0, 0x81, 0x02, 0x00, 0x00,
    0x00, 0x00, 0x02, 0x81, 0x00, 0x00, 0x00, 0x08, 0x40, 0x00, 0x03, 0x00,
    0x00, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01,
    0x00, 0x00, 0x00, 0x00, 0x38, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x38, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x01, 0x00, 0x81, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x02, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00, 0x00, 0x40, 0x08,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x00, 0x81, 0x02, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08, 0x40, 0x00, 0x02, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x01, 0xc0, 0x07, 0x00, 0x7f, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x1c, 0x00, 0x01, 0xc0, 0x00, 0x00, 0x00, 0x00, 0xe0, 0x1c,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xf0, 0x00, 0x40, 0x04, 0x00, 0x00};

// Declares internal font raw data.
const ImGuiImpl::Font ImGuiImpl::font_ = {
    1024,                      //  texture_width
    16,                        //  texture_height
    672,                       //  image_width
    10,                        //  image_height
    10,                        //  glyph_height
    7,                         //  glyph_width
    95,                        //  glyph_count
    32,                        //  glyph_start
    kFontPixelRawData,         // pixels
    sizeof(kFontPixelRawData)  // pixels_size
};

void ImGuiImpl::InitalizeFont() {
  // Builds font texture.
  assert(static_cast<size_t>(font_.texture_width * font_.texture_height) >=
         font_.pixels_size * 8);

  const size_t buffer_size = 4 * font_.texture_width * font_.texture_height;
  uint8_t* pixels = reinterpret_cast<uint8_t*>(
      memory::default_allocator()->Allocate(buffer_size, 4));
  memset(pixels, 0, buffer_size);

  // Unpack font data font 1 bit per pixel to 8.
  for (int i = 0; i < font_.image_width * font_.image_height; i += 8) {
    for (int j = 0; j < 8; ++j) {
      const int pixel = (i + j) / font_.image_width * font_.texture_width +
                        (i + j) % font_.image_width;
      const int bit = 7 - j;
      const uint8_t cpnt = ((font_.pixels[i / 8] >> bit) & 1) * 255;
      pixels[4 * pixel + 0] = cpnt;
      pixels[4 * pixel + 1] = cpnt;
      pixels[4 * pixel + 2] = cpnt;
      pixels[4 * pixel + 3] = cpnt;
    }
  }

  GL(GenTextures(1, &glyph_texture_));
  GL(BindTexture(GL_TEXTURE_2D, glyph_texture_));
  GL(TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST));
  GL(TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST));
  GL(TexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, font_.texture_width,
                font_.texture_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels));
  GL(PixelStorei(GL_UNPACK_ALIGNMENT, 1));
  GL(BindTexture(GL_TEXTURE_2D, 0));
  memory::default_allocator()->Deallocate(pixels);

  // Pre-computes glyphes texture and vertex coordinates.
  const float glyph_uv_width =
      static_cast<float>(font_.glyph_width) / font_.texture_width;
  const float glyph_uv_height =
      static_cast<float>(font_.glyph_height) / font_.texture_height;

  const int num_glyphes = static_cast<int>(OZZ_ARRAY_SIZE(glyphes_));
  assert(num_glyphes >= font_.glyph_start + font_.glyph_count);
  for (int i = 0; i < num_glyphes; ++i) {
    if (i >= font_.glyph_start && i < font_.glyph_start + font_.glyph_count) {
      Glyph& glyph = glyphes_[i];
      const int index = i - font_.glyph_start;
      glyph.uv[0][0] = index * glyph_uv_width;
      glyph.uv[0][1] = 0.f;
      glyph.pos[0][0] = 0.f;
      glyph.pos[0][1] = static_cast<float>(font_.glyph_height);

      glyph.uv[1][0] = index * glyph_uv_width;
      glyph.uv[1][1] = glyph_uv_height;
      glyph.pos[1][0] = 0.f;
      glyph.pos[1][1] = 0.f;

      glyph.uv[2][0] = (index + 1) * glyph_uv_width;
      glyph.uv[2][1] = 0.f;
      glyph.pos[2][0] = static_cast<float>(font_.glyph_width);
      glyph.pos[2][1] = static_cast<float>(font_.glyph_height);

      glyph.uv[3][0] = (index + 1) * glyph_uv_width;
      glyph.uv[3][1] = glyph_uv_height;
      glyph.pos[3][0] = static_cast<float>(font_.glyph_width);
      glyph.pos[3][1] = 0.f;
    } else {
      memset(&glyphes_[i], 0, sizeof(glyphes_[i]));
    }
  }
}

void ImGuiImpl::DestroyFont() {
  GL(DeleteTextures(1, &glyph_texture_));
  glyph_texture_ = 0;
}

namespace {
inline bool IsDivisible(char _c) { return _c == ' ' || _c == '\t'; }

struct LineSpec {
  const char* begin;
  const char* end;
};
}  // namespace

float ImGuiImpl::Print(const char* _text, const math::RectFloat& _rect,
                       PrintLayout _layout, const GLubyte _rgba[4]) const {
  LineSpec line_specs[64];
  int interlign = font_.glyph_height / 4;
  int max_lines = (static_cast<int>(_rect.height) + interlign) /
                  (font_.glyph_height + interlign);
  if (max_lines == 0) {
    return _rect.height;  // No offset from the bottom.
  }
  const int max_line_specs = OZZ_ARRAY_SIZE(line_specs);
  if (max_lines > max_line_specs) {
    max_lines = max_line_specs;
  }
  int line_count = 0;

  const int chars_per_line = static_cast<int>(_rect.width) / font_.glyph_width;
  {
    const char* last_div = nullptr;
    LineSpec spec = {_text, _text};
    while (*spec.end) {
      if (IsDivisible(*spec.end)) {  // Found a divisible character.
        last_div = spec.end;
      }

      // Is this the last character of the line.
      if (*spec.end == '\n' || spec.end + 1 > spec.begin + chars_per_line) {
        if (*spec.end != '\n' && last_div != nullptr) {
          // Breaks the line after the last divisible character.
          spec.end = last_div;
          last_div = nullptr;
        }

        // Trims ' ' left if it is the end of the new line.
        while ((spec.end - 1) >= spec.begin && IsDivisible(*(spec.end - 1))) {
          --spec.end;
        }

        // Pushes the new line specs.
        line_specs[line_count++] = spec;
        if (line_count == max_lines) {
          break;
        }

        // Sets specs for the new line.
        spec.begin = spec.end;

        // Trims ' ' right if it is the beginning of a new line.
        while (IsDivisible(*spec.begin)) {
          ++spec.begin;
        }
        // Trims '\n' right if this line was stopped because of a cr.
        if (*spec.begin == '\n') {
          ++spec.begin;
        }

        spec.end = spec.begin;
      } else {
        ++spec.end;
      }
    }

    if (line_count < max_lines) {
      // Trims ' ' left as it is the end of the new line.
      while ((spec.end - 1) >= spec.begin && IsDivisible(*(spec.end - 1))) {
        --spec.end;
      }
      // Pushes the remaining text.
      if (spec.begin != spec.end) {
        line_specs[line_count++] = spec;
      }
    }
  }

  if (line_count == 0) {
    return _rect.height;  // No offset from the bottom.
  }

  // Default is kNorth*.
  float ly = _rect.bottom + _rect.height - font_.glyph_height;
  switch (_layout) {
    case kWest:
    case kMiddle:
    case kEst: {
      ly = _rect.bottom - font_.glyph_height +
           (_rect.height - 1 +  // -1 rounds on the pixel below.
            line_count * font_.glyph_height + (line_count - 1) * interlign) /
               2.f;
      ly = floorf(ly);
      break;
    }
    case kSouthWest:
    case kSouth:
    case kSouthEst: {
      ly = _rect.bottom + (line_count - 1) * (font_.glyph_height + interlign);
      break;
    }
    default: { break; }
  }

  GL(BindTexture(GL_TEXTURE_2D, glyph_texture_));

  for (int l = 0; l < line_count; ++l) {
    const int line_char_count =
        static_cast<int>(line_specs[l].end - line_specs[l].begin);
    float lx = _rect.left;  // Default value is kWest*.
    switch (_layout) {
      case kNorth:
      case kMiddle:
      case kSouth: {
        lx = _rect.left +
             ((_rect.width - (line_char_count * font_.glyph_width)) / 2.f);
        lx = floorf(lx);
        break;
      }
      case kNorthEst:
      case kEst:
      case kSouthEst: {
        lx = _rect.right() - (line_char_count * font_.glyph_width);
        break;
      }
      default: { break; }
    }

    // Loops through all characters of the current line, and renders them using
    // precomputed texture and vertex coordinates.
    {
      GlImmediatePTC im(renderer_->immediate_renderer(), GL_TRIANGLES,
                        ozz::math::Float4x4::identity());
      GlImmediatePTC::Vertex v = {{0.f, 0.f, 0.f},
                                  {0.f, 0.f},
                                  {_rgba[0], _rgba[1], _rgba[2], _rgba[3]}};

      float offset = 0.f;
      for (int i = 0; i < line_char_count; i++, offset += font_.glyph_width) {
        const int char_index = line_specs[l].begin[i];
        const int num_glyphes = static_cast<int>(OZZ_ARRAY_SIZE(glyphes_));
        if (char_index >= 0 && char_index < num_glyphes) {
          const Glyph& glyph = glyphes_[char_index];
          const int indices[] = {0, 1, 2, 2, 1, 3};
          for (size_t j = 0; j < OZZ_ARRAY_SIZE(indices); j++) {
            const int index = indices[j];
            v.uv[0] = glyph.uv[index][0];
            v.uv[1] = glyph.uv[index][1];
            v.pos[0] = lx + glyph.pos[index][0] + offset;
            v.pos[1] = ly + glyph.pos[index][1];
            im.PushVertex(v);
          }
        }
      }
    }

    // Computes next line height.
    ly -= font_.glyph_height + interlign;
  }
  GL(BindTexture(GL_TEXTURE_2D, 0));

  // Returns the bottom of the last line.
  return ly + font_.glyph_height + interlign - _rect.bottom;
}
}  // namespace internal
}  // namespace sample
}  // namespace ozz
