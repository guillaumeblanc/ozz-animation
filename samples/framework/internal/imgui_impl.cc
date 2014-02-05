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

#include "framework/internal/imgui_impl.h"

#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstring>

#include "ozz/base/maths/math_constant.h"
#include "ozz/base/maths/math_ex.h"
#include "ozz/base/memory/allocator.h"

#include "framework/internal/renderer_impl.h"

#include "GL/glfw.h"

namespace ozz {
namespace sample {
namespace internal {

const GLubyte panel_background_color[4] = {0xa0, 0xa0, 0xa0, 0xc0};
const GLubyte panel_border_color[4] = {0x30, 0x30, 0x30, 0xff};
const GLubyte panel_title_color[4] = {0x30, 0x30, 0x30, 0xd0};
const GLubyte panel_title_text_color[4] = {0x80, 0x80, 0x80, 0xff};

const GLubyte widget_background_color[4] = {0xb0, 0xb0, 0xb0, 0xff};
const GLubyte widget_border_color[4] = {0x30, 0x30, 0x30, 0xff};

const GLubyte widget_disabled_background_color[4] = {0xb0, 0xb0, 0xb0, 0xff};
const GLubyte widget_disabled_border_color[4] = {0x90, 0x90, 0x90, 0xff};

const GLubyte widget_hot_background_color[4] = {0xb0, 0xb0, 0xb0, 0xff};
const GLubyte widget_hot_border_color[4] = {0xc7, 0x9a, 0x40, 0xff};

const GLubyte widget_active_background_color[4] = {0xc7, 0x9a, 0x40, 0xff};
const GLubyte widget_active_border_color[4] = {0x30, 0x30, 0x30, 0xff};

const GLubyte widget_text_color[4] = {0x20, 0x20, 0x20, 0xff};
const GLubyte widget_disabled_text_color[4] = {0x50, 0x50, 0x50, 0xff};

const GLubyte graph_background_color[4] = {0x30, 0x40, 0x9a, 0xff};
const GLubyte graph_plot_color[4] = {0, 0xe0, 0, 0xff};
const GLubyte graph_text_color[4] = {0xff, 0xff, 0xff, 0xff};

const GLubyte slider_background_color[4] = {0xb0, 0xb0, 0xb0, 0xff};
const GLubyte slider_cursor_color[4] = {0x90, 0x90, 0x90, 0xff};
const GLubyte slider_cursor_hot_color[4] = {0x80, 0x80, 0x80, 0xff};
const GLubyte slider_disabled_cursor_color[4] = {0x80, 0x80, 0x80, 0xff};

const GLubyte cursor_color[4] = {0xf0, 0xf0, 0xf0, 0xff};
const int cursor_size = 16;

const int graph_height_factor = 3;
const int graph_label_digits = 5;

const int text_margin_x = 2;

const int widget_round_rect_radius = 2;
const int widget_cursor_width = 8;
const int widget_height = 12;
const int widget_margin_x = 6;
const int widget_margin_y = 4;

const int slider_round_rect_radius = 1;

const int button_round_rect_radius = widget_height / 3;

const int panel_round_rect_radius = 1;
const int panel_margin_x = 2;
const int panel_title_margin_y = 1;

bool FormatFloat(float _value, char* _string, const char* _string_end) {
  // Requires 8 fixed characters count + digits of precision + \0.
  static const int precision = 3;
  if (!_string || _string_end - _string < 8 + precision + 1) {
    return false;
  }
  std::sprintf(_string, "%.3g\n", _value);

  // Removes unnecessary '0' digits in the exponent.
  char* exponent = strchr(_string, 'e');
  char* last_zero = strrchr(_string, '0');
  if (exponent && last_zero > exponent) {
    memmove(exponent + 2,  // After exponent and exponent's sign.
            last_zero + 1,  // From the first character after the exponent.
            strlen(last_zero + 1) + 1);  // The remaining characters count.
  }
  return true;
}

ImGuiImpl::ImGuiImpl()
    : hot_item_(0),
      active_item_(0),
      auto_gen_id_(0),
      glyph_displaylist_base_(0),
      glyph_texture_(0) {
  InitializeCircle();
  InitalizeFont();
}

ImGuiImpl::~ImGuiImpl() {
  DestroyFont();
}

void ImGuiImpl::BeginFrame(const Inputs& _inputs, const math::RectInt& _rect) {
  if (!containers_.empty()) {
    return;
  }

  // Store inputs.
  inputs_ = _inputs;

  // Sets no widget to hot for the current frame.
  hot_item_ = 0;

  // Regenerates widgets ids from scratch.
  auto_gen_id_ = 0;

  // Reset container stack info.
  Container container = {_rect, _rect.height - widget_height};
  containers_.push_back(container);

  // Setup GL context in order to render layered the widgets.
  GL(Clear(GL_DEPTH_BUFFER_BIT));

  // Setup orthographic projection.
  GL(MatrixMode(GL_PROJECTION));
  GL(PushMatrix());
  GL(LoadIdentity());
  GL(Ortho(0, math::Max(_rect.width, 1), 0, math::Max(_rect.height, 1), -1, 1));
  GL(MatrixMode(GL_MODELVIEW));
  GL(PushMatrix());
  GL(LoadIdentity());

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
  GL(Color4ubv(cursor_color));
  glBegin(GL_LINE_LOOP);
    glVertex2i(inputs_.mouse_x, inputs_.mouse_y);
    glVertex2i(inputs_.mouse_x + cursor_size,
               inputs_.mouse_y - cursor_size / 2);
    glVertex2i(inputs_.mouse_x + cursor_size / 2,
               inputs_.mouse_y - cursor_size / 2);
    glVertex2i(inputs_.mouse_x + cursor_size / 2,
               inputs_.mouse_y - cursor_size);
  GL(End());

  // Restores projection.
  GL(MatrixMode(GL_PROJECTION));
  GL(PopMatrix());
  GL(MatrixMode(GL_MODELVIEW));
  GL(PopMatrix());

  // Restores blending
  GL(Disable(GL_BLEND));
}

bool ImGuiImpl::AddWidget(int _height, math::RectInt* _rect) {
  if (containers_.empty()) {
    return false;
  }

  // Get current container.
  Container& container = containers_.back();

  // Early out if outside of the container.
  // But don't modify current container's state.
  if (container.offset_y < widget_margin_y + _height) {
    return false;
  }

  // Computes widget rect and update internal offset in the container.
  container.offset_y -= _height;

  _rect->left = container.rect.left + widget_margin_x;
  _rect->bottom = container.rect.bottom + container.offset_y;
  _rect->width = container.rect.width - widget_margin_x * 2;
  _rect->height = _height;

  // Includes margin for the next widget.
  container.offset_y -= widget_margin_y;

  return true;
}

bool ImGuiImpl::ButtonLogic(const math::RectInt& _rect, int _id,
                            bool* _hot, bool* _active) {
  // Checks whether the button should be hot.
  if (_rect.is_inside(inputs_.mouse_x, inputs_.mouse_y)) {
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

void ImGuiImpl::BeginContainer(const char* _title,
                               const math::RectInt* _rect,
                               bool* _open,
                               bool _constrain) {
  if (containers_.empty()) {
    return;
  }

  // Adds the new container to the stack.
  containers_.resize(containers_.size() + 1);
  const Container& parent = containers_[containers_.size() - 2];
  Container& container = containers_[containers_.size() - 1];

  // Does the container have a header to render ?
  const bool header = _title || _open;
  const int header_height = header ? widget_height + panel_title_margin_y : 0;

  if (_rect) {
    // Crops _rect in parent's rect.
    container.rect.left = parent.rect.left + _rect->left;
    container.rect.bottom = parent.rect.bottom + _rect->bottom;
    container.rect.width = math::Max(
      0, math::Min(parent.rect.width -  _rect->left, _rect->width));
    container.rect.height = math::Max(
      0, math::Min(parent.rect.height - _rect->bottom, _rect->height));
  } else {
    // Concatenate to the parent's rect.
    container.rect.left = parent.rect.left + panel_margin_x;
    container.rect.bottom = parent.rect.bottom;
    container.rect.width =
      math::Max(0, parent.rect.width - panel_margin_x * 2);
    container.rect.height = parent.offset_y;
  }

  // Shrinks rect if the container is closed.
  if (_open && !*_open) {
    const int closed_height = header_height;
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
    container.offset_y -= widget_margin_y;
    return;
  }

  auto_gen_id_++;

  // Inserts header.
  container.offset_y -= header_height;

  const math::RectInt title_rect(container.rect.left ,
                                 container.rect.bottom + container.offset_y,
                                 container.rect.width,
                                 header_height);

  const math::RectInt open_close_rect(title_rect.left,
                                      title_rect.bottom,
                                      widget_height,
                                      widget_height);

  const math::RectInt label_rect(
    title_rect.left + widget_height + text_margin_x,
    title_rect.bottom,
    title_rect.width - widget_height - text_margin_x,
    widget_height);

  // Adds a margin before the next widget only if it is opened.
  if (!_open || *_open) {
    container.offset_y -= widget_margin_y;
  }

  // Renders title background.
  GL(Color4ubv(panel_title_color));
  FillRect(title_rect, panel_round_rect_radius);

  GL(Color4ubv(panel_title_text_color));
  if (_title) {
    Print(_title, label_rect, kWest);
  }

  // Handles open close button.
  bool hot = false, active = false;
  if (_open) {
    if (ButtonLogic(title_rect, auto_gen_id_, &hot, &active)) {
      // Swap open/close state if button was clicked.
      *_open = !*_open;
    }

    GL(Color4ubv(panel_title_text_color));
    glBegin(GL_TRIANGLES);
    if (*_open) {
      glVertex2i(open_close_rect.left + open_close_rect.width / 8,
                 open_close_rect.top() - open_close_rect.height / 4);
      glVertex2i(open_close_rect.left + open_close_rect.width / 2,
                 open_close_rect.bottom + open_close_rect.height / 4);
      glVertex2i(open_close_rect.right() - open_close_rect.width / 8,
                 open_close_rect.top() - open_close_rect.height / 4);
    } else {
      glVertex2i(open_close_rect.left + open_close_rect.width / 4,
                 open_close_rect.top() - open_close_rect.height / 8);
      glVertex2i(open_close_rect.left + open_close_rect.width / 4,
                 open_close_rect.bottom + open_close_rect.height / 8);
      glVertex2i(open_close_rect.right() - open_close_rect.width / 4,
                 open_close_rect.bottom + open_close_rect.height / 2);
    }
    GL(End());
  }
}

void ImGuiImpl::EndContainer() {
  // Get current container. Copy it as it's going to be remove from the vector.
  Container container = containers_.back();
  containers_.pop_back();
  Container& parent = containers_.back();

  // Shrinks container rect.
  if (container.constrain) {
    int final_height =
      math::Max(0, container.rect.height - container.offset_y);
    container.rect.bottom += container.offset_y;
    container.rect.height = final_height;
  }

  // Renders container background.
  if (container.rect.height > 0) {
    glPushMatrix();
    glTranslatef(0, 0, -.1f);  // Renders the background in the depth.
    GL(Color4ubv(panel_background_color));
    FillRect(container.rect, panel_round_rect_radius);
    glPopMatrix();

    GL(Color4ubv(panel_border_color));
    StrokeRect(container.rect, panel_round_rect_radius);
  }

  // Applies changes to the parent container.
  parent.offset_y -= container.rect.height + widget_margin_y;
}

bool ImGuiImpl::DoButton(const char* _label, bool _enabled, bool* _state) {
  math::RectInt rect;
  if (!AddWidget(widget_height, &rect)) {
    return false;
  }

  auto_gen_id_++;

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
  const GLubyte* background_color = widget_background_color;
  const GLubyte* border_color = widget_border_color;
  const GLubyte* text_color = widget_text_color;

  int active_offset = 0;
  if (!_enabled) {
    background_color = widget_disabled_background_color;
    border_color = widget_disabled_border_color;
    text_color = widget_disabled_text_color;
  } else if (active) {  // Button is 'active'.
    background_color = widget_active_background_color;
    border_color = widget_active_border_color;
    active_offset = 1;
  } else if (hot) {  // Hot but not 'active'.
      // Button is merely 'hot'.
      background_color = widget_hot_background_color;
      border_color = widget_hot_border_color;
  } else {
    // button is not hot, but it may be active. Use default colors.
  }

  GL(Color4ubv(background_color));
  FillRect(rect, button_round_rect_radius);
  GL(Color4ubv(border_color));
  StrokeRect(rect, button_round_rect_radius);

  // Renders button label.
  const math::RectInt text_rect(rect.left + button_round_rect_radius,
                                rect.bottom - active_offset,
                                rect.width - button_round_rect_radius * 2,
                                rect.height - active_offset);
  GL(Color4ubv(text_color));
  Print(_label, text_rect, kMiddle);

  return clicked;
}

bool ImGuiImpl::DoCheckBox(const char* _label,
                           bool* _state,
                           bool _enabled) {
  math::RectInt widget_rect;
  if (!AddWidget(widget_height, &widget_rect)) {
    return false;
  }

  math::RectInt check_rect(widget_rect.left,
                           widget_rect.bottom,
                           widget_height,  // The check box is square.
                           widget_rect.height);

  auto_gen_id_++;

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
  const GLubyte* background_color = widget_background_color;
  const GLubyte* border_color = widget_border_color;
  const GLubyte* check_color = widget_border_color;
  const GLubyte* text_color = widget_text_color;

  if (!_enabled) {
    background_color = widget_disabled_background_color;
    border_color = widget_disabled_border_color;
    check_color = widget_disabled_border_color;
    text_color = widget_disabled_text_color;
  } else if (hot) {
    if (active) {
      // Button is both 'hot' and 'active'.
      background_color = widget_active_background_color;
      border_color = widget_active_border_color;
      check_color = widget_active_border_color;
    } else {
      // Button is merely 'hot'.
      background_color = widget_hot_background_color;
      border_color = widget_hot_border_color;
      check_color = widget_hot_border_color;
    }
  } else {
    // button is not hot, but it may be active. Use default colors.
  }

  GL(Color4ubv(background_color));
  FillRect(check_rect, 0);
  GL(Color4ubv(border_color));
  StrokeRect(check_rect, 0);

  // Renders the "check" mark.
  if (*_state) {
    glBegin(GL_TRIANGLES);
    glColor4ubv(check_color);
    glVertex2i(check_rect.left + check_rect.width / 8,
               check_rect.bottom + check_rect.height / 2);
    glVertex2i(check_rect.left + check_rect.width / 2,
               check_rect.bottom + check_rect.height / 8);
    glVertex2i(check_rect.left + check_rect.width / 2,
               check_rect.bottom + check_rect.height / 2);

    glVertex2i(check_rect.left + check_rect.width / 3,
               check_rect.bottom + check_rect.height / 2);
    glVertex2i(check_rect.left + check_rect.width / 2,
               check_rect.bottom + check_rect.height / 8);
    glVertex2i(check_rect.right() - check_rect.width / 6,
               check_rect.top() - check_rect.height / 6);
    GL(End());
  }

  // Renders button label.
  const math::RectInt text_rect(
    check_rect.right() + text_margin_x,
    widget_rect.bottom,
    widget_rect.width - check_rect.width - text_margin_x,
    widget_rect.height);
  GL(Color4ubv(text_color));
  Print(_label, text_rect, kWest);

  return clicked;
}

bool ImGuiImpl::DoRadioButton(int _ref,
                              const char* _label,
                              int* _value,
                              bool _enabled) {
  math::RectInt widget_rect;
  if (!AddWidget(widget_height, &widget_rect)) {
    return false;
  }

  math::RectInt radio_rect(widget_rect.left,
                           widget_rect.bottom,
                           widget_height,  // The check box is square.
                           widget_rect.height);

  auto_gen_id_++;

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
  const GLubyte* background_color = widget_background_color;
  const GLubyte* border_color = widget_border_color;
  const GLubyte* check_color = widget_border_color;
  const GLubyte* text_color = widget_text_color;

  if (!_enabled) {
    background_color = widget_disabled_background_color;
    border_color = widget_disabled_border_color;
    check_color = widget_disabled_border_color;
    text_color = widget_disabled_background_color;
  } else if (hot) {
    if (active) {
      // Button is both 'hot' and 'active'.
      background_color = widget_active_background_color;
      border_color = widget_active_border_color;
      check_color = widget_active_border_color;
    } else {
      // Button is merely 'hot'.
      background_color = widget_hot_background_color;
      border_color = widget_hot_border_color;
      check_color = widget_hot_border_color;
    }
  } else {
    // button is not hot, but it may be active. Use default colors.
  }

  GL(Color4ubv(background_color));
  FillRect(radio_rect, widget_round_rect_radius);
  GL(Color4ubv(border_color));
  StrokeRect(radio_rect, widget_round_rect_radius);

  // Renders the "checked" button.
  if (*_value == _ref) {
    GL(Color4ubv(check_color));
    const math::RectInt checked_rect(radio_rect.left + 1,
                                     radio_rect.bottom + 1,
                                     radio_rect.width - 3,
                                     radio_rect.height - 3);
    FillRect(checked_rect, widget_round_rect_radius);
  }

  // Renders button label.
  const math::RectInt text_rect(
    radio_rect.right() + text_margin_x,
    widget_rect.bottom,
    widget_rect.width - radio_rect.width - text_margin_x,
    widget_rect.height);
  GL(Color4ubv(text_color));
  Print(_label, text_rect, kWest);

  return clicked;
}

void ImGuiImpl::DoGraph(const char* _label,
                        float _min, float _max, float _mean,
                        const float* _value_cursor,
                        const float* _value_begin, const float* _value_end) {
  // Computes widget rect.
  math::RectInt widget_rect;
  const int label_height = _label ? (widget_margin_y + font_.glyph_height) : 0;
  const int height = widget_height * graph_height_factor + label_height;
  if (!AddWidget(height, &widget_rect)) {
    return;
  }

  const int label_width = graph_label_digits * font_.glyph_width;

  const math::RectInt graph_rect(
    widget_rect.left,
    widget_rect.bottom,
    widget_rect.width - label_width - text_margin_x,
    widget_height * graph_height_factor);

  // Renders background and borders.
  GL(Color4ubv(graph_background_color));
  FillRect(graph_rect, 0);

  GL(Color4ubv(widget_text_color));
  StrokeRect(graph_rect, 0);

  glBegin(GL_LINES);
  glVertex2i(graph_rect.left, graph_rect.bottom + graph_rect.height / 2);
  glVertex2i(graph_rect.right(), graph_rect.bottom + graph_rect.height / 2);
  GL(End());

  // Render labels.
  char sz[16];
  const char* sz_end = sz + OZZ_ARRAY_SIZE(sz);
  GL(Color4ubv(widget_text_color));
  const math::RectInt max_rect(widget_rect.left,
                               graph_rect.top() - font_.glyph_height,
                               widget_rect.width,
                               font_.glyph_height);
  if (FormatFloat(_max, sz, sz_end)) {
    Print(sz, max_rect, kEst);
  }

  const math::RectInt mean_rect(
    widget_rect.left,
    graph_rect.bottom + graph_rect.height / 2 - font_.glyph_height / 2,
    widget_rect.width,
    font_.glyph_height);
  if (FormatFloat(_mean, sz, sz_end)) {
    Print(sz, mean_rect, kEst);
  }

  const math::RectInt min_rect(widget_rect.left,
                               graph_rect.bottom,
                               widget_rect.width,
                               font_.glyph_height);
  if (FormatFloat(_min, sz, sz_end)) {
    Print(sz, min_rect, kEst);
  }

  // Prints title on the left.
  if (_label) {
    const math::RectInt label_rect(widget_rect.left,
                                   widget_rect.top() - font_.glyph_height,
                                   widget_rect.width,
                                   font_.glyph_height);
    Print(_label, label_rect, kNorthWest);
  }

  // Renders the graph.
  if (_value_end - _value_begin >= 2) {  // Reject invalid or to small inputs.
    const float abscissa_min = static_cast<float>(graph_rect.bottom + 1);
    const float abscissa_max = static_cast<float>(graph_rect.top() - 1);
    const float abscissa_scale = (abscissa_max - abscissa_min) / (_max - _min);
    const float abscissa_begin = static_cast<float>(graph_rect.bottom + 1);
    const float ordinate_inc = -static_cast<float>(graph_rect.width - 2) /
      (_value_end - _value_begin - 1);
    float ordinate_current = static_cast<float>(graph_rect.right() - 1);
    GL(Color4ubv(graph_plot_color));
    glBegin(GL_LINE_STRIP);

    const float* current = _value_cursor;
    const float* end = _value_end;
    while (current < end) {
      const float abscissa =
        abscissa_begin + abscissa_scale * (*current - _min);
      const float clamped_abscissa =
        math::Clamp(abscissa_min, abscissa, abscissa_max);
      glVertex2f(ordinate_current, clamped_abscissa);
      ordinate_current += ordinate_inc;

      current++;
      if (current == _value_end) {  // Looping...
        end = _value_cursor;
        current = _value_begin;
      }
    }
    GL(End());
  }
}

bool ImGuiImpl::DoSlider(const char* _label,
                         float _min, float _max, float* _value,
                         float _pow,
                         bool _enabled) {
  math::RectInt rect;
  if (!AddWidget(widget_height, &rect)) {
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
    math::RectInt pick_rect = rect;
    pick_rect.left -= widget_cursor_width / 2;
    pick_rect.width += widget_cursor_width;
    ButtonLogic(pick_rect, auto_gen_id_, &hot, &active);
  }

  // Render the scrollbar
  const GLubyte* background_color = slider_background_color;
  const GLubyte* border_color = widget_border_color;
  const GLubyte* slider_color = slider_cursor_color;
  const GLubyte* slider_border_color = widget_border_color;
  const GLubyte* text_color = widget_text_color;

  if (!_enabled) {
    background_color = widget_disabled_background_color;
    border_color = widget_disabled_border_color;
    slider_color = slider_disabled_cursor_color;
    slider_border_color = widget_disabled_border_color;
    text_color = widget_disabled_text_color;
  } else if (hot) {
    if (active) {
      // Button is both 'hot' and 'active'.
      slider_color = widget_active_background_color;
      slider_border_color = widget_active_border_color;
    } else {
      // Button is merely 'hot'.
      slider_color = slider_cursor_hot_color;
      slider_border_color = widget_hot_border_color;
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
      int mousepos = inputs_.mouse_x - rect.left;
      if (mousepos < 0) {
        mousepos = 0;
      }
      if (mousepos > rect.width) {
        mousepos = rect.width;
      }
      pow_value = (mousepos * (pow_max - pow_min)) / rect.width + pow_min;
      *_value = ozz::math::Clamp(_min, powf(pow_value, 1.f / _pow), _max);
    } else {
      // Clamping is only applied if the widget is enabled.
      *_value = clamped_value;
    }
  }

  // Renders slider's rail.
  const math::RectInt rail_rect(rect.left,
                                rect.bottom,
                                rect.width,
                                rect.height);
  GL(Color4ubv(background_color));
  FillRect(rail_rect, slider_round_rect_radius);
  GL(Color4ubv(border_color));
  StrokeRect(rail_rect, slider_round_rect_radius);

  // Finds cursor position and rect.
  const int cursor = static_cast<int>(
    (rect.width * (pow_value - pow_min)) / (pow_max - pow_min));
  const math::RectInt cursor_rect(rect.left + cursor - widget_cursor_width / 2,
                                  rect.bottom - 1,
                                  widget_cursor_width,
                                  rect.height + 2);
  GL(Color4ubv(slider_color));
  FillRect(cursor_rect, slider_round_rect_radius);
  GL(Color4ubv(slider_border_color));
  StrokeRect(cursor_rect, slider_round_rect_radius);

  const math::RectInt text_rect(rail_rect.left + slider_round_rect_radius,
                                rail_rect.bottom,
                                rail_rect.width - slider_round_rect_radius * 2,
                                rail_rect.height);
  GL(Color4ubv(text_color));
  Print(_label, text_rect, kMiddle);

  // Returns true if the value has changed or if it was clamped in _min / _max
  // bounds.
  return initial_value != *_value;
}

bool ImGuiImpl::DoSlider(const char* _label,
                         int _min, int _max, int* _value,
                         float _pow,
                         bool _enabled) {
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
  math::RectInt rect;
  if (!AddWidget(
    _single_line ?
      font_.glyph_height :
      math::Max(0, container.offset_y - widget_margin_y),
    &rect)) {
    return;
  }

  GL(Color4ubv(widget_text_color));
  PrintLayout layout[] = {kNorthWest, kNorth, kNorthEst};
  const int offset = Print(_label, rect, layout[_justification]);

  if (!_single_line) {  // Resume following widgets below the label.
    container.offset_y = offset - widget_margin_y;
  }
}

void ImGuiImpl::FillRect(const math::RectInt& _rect, int _radius) const {
  if (_radius <= 0) {
    glBegin(GL_QUADS);
    glVertex2i(_rect.left, _rect.bottom);
    glVertex2i(_rect.left + _rect.width, _rect.bottom);
    glVertex2i(_rect.left + _rect.width, _rect.top());
    glVertex2i(_rect.left, _rect.top());
    GL(End());
  } else {
    const int x = _rect.left + _radius;
    const int y = _rect.bottom + _radius;
    const int w = _rect.width - _radius * 2;
    const int h = _rect.height - _radius * 2;

    glBegin(GL_TRIANGLES);
    if (_radius > 0) {
      for (int i = 1, j = i - 1; i <= kCircleSegments / 4; j = i++) {
        glVertex2i(x + w, y + h);
        glVertex2i(x + w + circle_[j][0] * _radius / kCircleRadius,
                   y + h + circle_[j][1] * _radius / kCircleRadius);
        glVertex2i(x + w + circle_[i][0] * _radius / kCircleRadius,
                   y + h + circle_[i][1] * _radius / kCircleRadius);
      }
      for (int i = 1 + kCircleSegments / 4, j = i - 1;
           i <= 2 * kCircleSegments / 4;
           j = i++) {
        glVertex2i(x, y + h);
        glVertex2i(x + circle_[j][0] * _radius / kCircleRadius,
                   y + h + circle_[j][1] * _radius / kCircleRadius);
        glVertex2i(x + circle_[i][0] * _radius / kCircleRadius,
                   y + h + circle_[i][1] * _radius / kCircleRadius);
      }
      for (int i = 1 + 2 * kCircleSegments / 4, j = i - 1;
           i <= 3 * kCircleSegments / 4;
           j = i++) {
        glVertex2i(x, y);
        glVertex2i(x + circle_[j][0] * _radius / kCircleRadius,
                   y + circle_[j][1] * _radius / kCircleRadius);
        glVertex2i(x + circle_[i][0] * _radius / kCircleRadius,
                   y + circle_[i][1] * _radius / kCircleRadius);
      }
      for (int i = 1 + 3 * kCircleSegments / 4, j = i - 1;
           i <= 4 * kCircleSegments / 4;
           j = i++) {
        const int index = i % kCircleSegments;
        glVertex2i(x + w, y);
        glVertex2i(x + w + circle_[j][0] * _radius / kCircleRadius,
                   y + circle_[j][1] * _radius / kCircleRadius);
        glVertex2i(x + w + circle_[index][0] * _radius / kCircleRadius,
                   y + circle_[index][1] * _radius / kCircleRadius);
      }
    }
    glVertex2i(_rect.left, y);
    glVertex2i(_rect.right(), y);
    glVertex2i(_rect.right(), y + h);

    glVertex2i(_rect.right(), y + h);
    glVertex2i(_rect.left, y + h);
    glVertex2i(_rect.left, y);

    glVertex2i(x, _rect.bottom);
    glVertex2i(x + w, _rect.bottom);
    glVertex2i(x + w, y);

    glVertex2i(x + w, y);
    glVertex2i(x, y);
    glVertex2i(x, _rect.bottom);

    glVertex2i(x, _rect.top() - _radius);
    glVertex2i(x + w, _rect.top() - _radius);
    glVertex2i(x + w, _rect.top());

    glVertex2i(x + w, _rect.top());
    glVertex2i(x, _rect.top());
    glVertex2i(x, _rect.top() - _radius);
    GL(End());
  }
}

void ImGuiImpl::StrokeRect(const math::RectInt& _rect, int _radius) const {
  // OpenGL line rendering requires
  GL(PushMatrix());
  GL(Translatef(-.5f, -.5f, 0.f));

  if (_radius <= 0) {
    glBegin(GL_LINE_LOOP);
    glVertex2i(_rect.left, _rect.bottom);
    glVertex2i(_rect.left + _rect.width, _rect.bottom);
    glVertex2i(_rect.left + _rect.width, _rect.top());
    glVertex2i(_rect.left, _rect.top());
    GL(End());
  } else {
    const int x = _rect.left + _radius;
    const int y = _rect.bottom + _radius;
    const int w = _rect.width - _radius * 2;
    const int h = _rect.height - _radius * 2;

    glBegin(GL_LINE_LOOP);
    for (int i = 0; i <= kCircleSegments / 4; i++) {
      glVertex2i(x + w + circle_[i][0] * _radius / kCircleRadius,
                 y + h + circle_[i][1] * _radius / kCircleRadius);
    }
    for (int i = kCircleSegments / 4; i <= 2 * kCircleSegments / 4; i++) {
      glVertex2i(x + circle_[i][0] * _radius / kCircleRadius,
                 y + h + circle_[i][1] * _radius / kCircleRadius);
    }
    for (int i = 2 * kCircleSegments / 4; i <= 3 * kCircleSegments / 4; i++) {
      glVertex2i(x + circle_[i][0] * _radius / kCircleRadius,
                 y + circle_[i][1] * _radius / kCircleRadius);
    }
    for (int i = 3 * kCircleSegments / 4; i < 4 * kCircleSegments / 4; i++) {
      glVertex2i(x + w + circle_[i][0] * _radius / kCircleRadius,
                 y + circle_[i][1] * _radius / kCircleRadius);
    }
    glVertex2i(x + w + circle_[0][0] * _radius / kCircleRadius,
               y + circle_[0][1] * _radius / kCircleRadius);
    GL(End());
  }
  GL(PopMatrix());
}

void ImGuiImpl::InitializeCircle() {
  assert((kCircleSegments & 3) == 0);  // kCircleSegments must be multiple of 4.

  for (int i = 0; i < kCircleSegments; i++) {
    const float angle = i * math::k2Pi / kCircleSegments;
    const float cos = std::cos(angle) * kCircleRadius;
    const float sin = std::sin(angle) * kCircleRadius;
    circle_[i][0] = static_cast<int>(cos + (cos >= 0.f?0.5f:-0.5f));
    circle_[i][1] = static_cast<int>(sin + (sin >= 0.f?0.5f:-0.5f));
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
  1024,  //  texture_width
  16,  //  texture_height
  672,  //  image_width
  10,  //  image_height
  10,  //  glyph_height
  7,  //  glyph_width
  95,  //  glyph_count
  32,  //  glyph_start
  kFontPixelRawData,  // pixels
  sizeof(kFontPixelRawData)  // pixels_size
};

void ImGuiImpl::InitalizeFont() {
  // Builds font texture.
  assert(static_cast<std::size_t>(font_.texture_width * font_.texture_height) >=
         font_.pixels_size * 8);

  const std::size_t pixel_count = font_.texture_width * font_.texture_height;
  unsigned char* pixels =
    memory::default_allocator()->Allocate<unsigned char>(pixel_count);
  memset(pixels, 0, pixel_count);

  // Unpack font data font 1 bit per pixel to 8.
  for (int i = 0; i < font_.image_width * font_.image_height; i += 8) {
    for (int j = 0; j < 8; j++) {
      const int pixel = (i + j) / font_.image_width * font_.texture_width +
                        (i + j) % font_.image_width;
      const int bit = 7 - j;
      pixels[pixel] = ((font_.pixels[i / 8] >> bit) & 1) * 255;
    }
  }

  GL(GenTextures(1, &glyph_texture_));
  GL(BindTexture(GL_TEXTURE_2D, glyph_texture_));
  GL(TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST));
  GL(TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST));
  GL(TexImage2D(GL_TEXTURE_2D,
                0,
                GL_ALPHA8,
                font_.texture_width,
                font_.texture_height,
                0,
                GL_ALPHA,
                GL_UNSIGNED_BYTE,
                pixels));
  GL(PixelStorei(GL_UNPACK_ALIGNMENT, 1));
  GL(BindTexture(GL_TEXTURE_2D, 0));
  memory::default_allocator()->Deallocate(pixels);

  // Builds glyph display lists.
  const float glyph_uv_width = static_cast<float>(font_.glyph_width) /
                               font_.texture_width;
  const float glyph_uv_height = static_cast<float>(font_.glyph_height) /
                                font_.texture_height;

  glyph_displaylist_base_ = glGenLists(256);
  for (int i = 0; i < 256; i++) {
    GL(NewList(glyph_displaylist_base_ + i, GL_COMPILE));
    if (i >= font_.glyph_start && i < font_.glyph_start + font_.glyph_count) {
      const int index = i - font_.glyph_start;
      glBegin(GL_TRIANGLE_STRIP);
      glTexCoord2f(index * glyph_uv_width, 0);
      glVertex2i(0, font_.glyph_height);
      glTexCoord2f(index * glyph_uv_width, glyph_uv_height);
      glVertex2i(0, 0);
      glTexCoord2f((index + 1) * glyph_uv_width, 0);
      glVertex2i(font_.glyph_width, font_.glyph_height);
      glTexCoord2f((index + 1) * glyph_uv_width, glyph_uv_height);
      glVertex2i(font_.glyph_width, 0);
      GL(End());
    }
    // Move to the right of the character inside the display list.
    glTranslatef(static_cast<float>(font_.glyph_width), 0, 0);
    GL(EndList());
  }
}

void ImGuiImpl::DestroyFont() {
  GL(DeleteLists(glyph_displaylist_base_, 256));
  glyph_displaylist_base_ = 0;
  GL(DeleteTextures(1, &glyph_texture_));
  glyph_texture_ = 0;
}

namespace {
inline bool IsDivisible(char _c) {
  return _c == ' ' || _c == '\t';
}

struct LineSpec {
  const char* begin;
  const char* end;
};
}

int ImGuiImpl::Print(const char* _text,
                     const math::RectInt& _rect,
                     PrintLayout _layout) const {
  LineSpec line_specs[64];
  int interlign = font_.glyph_height / 4;
  int max_lines = (_rect.height + interlign) / (font_.glyph_height + interlign);
  if (max_lines == 0) {
    return _rect.height;  // No offset from the bottom.
  }
  const int max_line_specs = OZZ_ARRAY_SIZE(line_specs);
  if (max_lines > max_line_specs) {
    max_lines = max_line_specs;
  }
  int line_count = 0;

  const int chars_per_line = _rect.width / font_.glyph_width;
  {
    const char* last_div = NULL;
    LineSpec spec = {_text, _text};
    while (*spec.end) {
      if (IsDivisible(*spec.end)) {  // Found a divisible character.
        last_div = spec.end;
      }
      spec.end++;

      // Is this the last character of the line.
      if (*spec.end == '\n' || spec.end + 1 > spec.begin + chars_per_line) {
        if (*spec.end != '\n' && last_div != NULL) {
          // Breaks the line after the last divisible character.
          spec.end = last_div;
          last_div = NULL;
        }

        // Trims ' ' left if it is the end of the new line.
        while ((spec.end - 1) >= spec.begin && IsDivisible(*(spec.end - 1))) {
          spec.end--;
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
          spec.begin++;
        }
        // Trims '\n' right if this line was stopped because of a cr.
        if (*spec.begin == '\n') {
          spec.begin++;
        }

        spec.end = spec.begin;
      }
    }

    if (line_count < max_lines) {
      // Trims ' ' left as it is the end of the new line.
      while ((spec.end - 1) >= spec.begin && IsDivisible(*(spec.end - 1))) {
        spec.end--;
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
  int ly = _rect.bottom + _rect.height - font_.glyph_height;
  switch (_layout) {
    case kWest:
    case kMiddle:
    case kEst: {
      ly = _rect.bottom - font_.glyph_height
           + (_rect.height - 1 +  // -1 rounds on the pixel below.
              line_count * font_.glyph_height +
              (line_count - 1) * interlign) / 2;
      break;
    }
    case kSouthWest:
    case kSouth:
    case kSouthEst: {
      ly = _rect.bottom + (line_count - 1) * (font_.glyph_height + interlign);
      break;
    }
    default: {
      break;
    }
  }

  GL(PushMatrix());
  GL(ListBase(glyph_displaylist_base_));
  GL(Enable(GL_ALPHA_TEST));
  GL(AlphaFunc(GL_GREATER, .5f));
  GL(Enable(GL_TEXTURE_2D));
  GL(BindTexture(GL_TEXTURE_2D, glyph_texture_));

  for (int l = 0; l < line_count; l++) {
    const int line_char_count =
      static_cast<int>(line_specs[l].end - line_specs[l].begin);
    int lx = _rect.left;  // Default value is kWest*.
    switch (_layout) {
      case kNorth:
      case kMiddle:
      case kSouth: {
        lx = _rect.left +
             ((_rect.width - (line_char_count * font_.glyph_width)) / 2);
        break;
      }
      case kNorthEst:
      case kEst:
      case kSouthEst: {
        lx = _rect.right() - (line_char_count * font_.glyph_width);
        break;
      }
      default: {
        break;
      }
    }

    glPushMatrix();
    glTranslatef(static_cast<float>(lx), static_cast<float>(ly), 0.f);
    GL(CallLists(line_char_count, GL_UNSIGNED_BYTE, line_specs[l].begin));
    glPopMatrix();

    // Computes next line height.
    ly -= font_.glyph_height + interlign;
  }
  GL(Disable(GL_TEXTURE_2D));
  GL(BindTexture(GL_TEXTURE_2D, 0));
  GL(Disable(GL_ALPHA_TEST));
  GL(ListBase(0));
  GL(PopMatrix());

  // Returns the bottom of the last line.
  return ly + font_.glyph_height + interlign - _rect.bottom;
}
}  // internal
}  // sample
}  // ozz
