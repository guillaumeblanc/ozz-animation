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

#ifndef OZZ_SAMPLES_FRAMEWORK_INTERNAL_IMGUI_IMPL_H_
#define OZZ_SAMPLES_FRAMEWORK_INTERNAL_IMGUI_IMPL_H_

#ifndef OZZ_INCLUDE_PRIVATE_HEADER
#error "This header is private, it cannot be included from public headers."
#endif  // OZZ_INCLUDE_PRIVATE_HEADER

// Implements immediate mode gui.
// See imgui.h for details about function specifications.

#include "framework/imgui.h"

#include "ozz/base/containers/vector.h"
#include "ozz/base/maths/rect.h"

#include "renderer_impl.h"

namespace ozz {
namespace sample {
namespace internal {

class RendererImpl;

// Immediate mode gui implementation.
class ImGuiImpl : public ImGui {
 public:
  ImGuiImpl();
  virtual ~ImGuiImpl();

  // Input state.
  struct Inputs {
    // Default input values.
    Inputs() : mouse_x(0), mouse_y(0), lmb_pressed(false) {}

    // Cursor x position. 0 indicates the screen left border.
    int mouse_x;
    // Cursor y position. 0 indicates the screen bottom border.
    int mouse_y;
    // Left mouse button state. true when the left mouse button is pressed.
    bool lmb_pressed;
  };

  // Starts an imgui frame.
  // _inputs describes next frame inputs. It is internally copied.
  // _rect is the windows size.
  void BeginFrame(const Inputs& _inputs, const math::RectInt& _rect,
                  RendererImpl* _renderer);

  // Ends the current imgui frame.
  void EndFrame();

 protected:
  // Starts ImGui interface implementation.
  // See imgui.h for virtual function specifications.

  virtual void BeginContainer(const char* _title, const math::RectFloat* _rect,
                              bool* _open, bool _constrain);

  virtual void EndContainer();

  virtual bool DoButton(const char* _label, bool _enabled, bool* _state);

  virtual bool DoSlider(const char* _label, float _min, float _max,
                        float* _value, float _pow, bool _enabled);

  virtual bool DoSlider(const char* _label, int _min, int _max, int* _value,
                        float _pow, bool _enabled);

  virtual bool DoCheckBox(const char* _label, bool* _state, bool _enabled);

  virtual bool DoRadioButton(int _ref, const char* _label, int* _value,
                             bool _enabled);

  virtual void DoLabel(const char* _label, Justification _justification,
                       bool _single_line);

  virtual void DoGraph(const char* _label, float _min, float _max, float _mean,
                       const float* _value_cursor, const float* _value_begin,
                       const float* _value_end);

 private:
  // Computes the rect of a new widget to add.
  // Returns true if there is enough space for a new widget.
  bool AddWidget(float _height, math::RectFloat* _rect);

  // Implements button logic.
  // Returns true if the button was clicked.
  bool ButtonLogic(const math::RectFloat& _rect, int _id, bool* _hot,
                   bool* _active);

  // Fills a rectangle with _rect coordinates. Draws rounded angles if _radius
  // is greater than 0.
  // If _texture id is not 0, then texture with id _texture is mapped using a
  // planar projection to the rect.
  void FillRect(const math::RectFloat& _rect, float _radius,
                const GLubyte _rgba[4]) const;
  void FillRect(const math::RectFloat& _rect, float _radius,
                const GLubyte _rgba[4],
                const ozz::math::Float4x4& _transform) const;

  // Strokes a rectangle with _rect coordinates. Draws rounded angles if _radius
  // is greater than 0.
  void StrokeRect(const math::RectFloat& _rect, float _radius,
                  const GLubyte _rgba[4]) const;
  void StrokeRect(const math::RectFloat& _rect, float _radius,
                  const GLubyte _rgba[4],
                  const ozz::math::Float4x4& _transform) const;

  enum PrintLayout {
    kNorthWest,
    kNorth,
    kNorthEst,
    kWest,
    kMiddle,
    kEst,
    kSouthWest,
    kSouth,
    kSouthEst,
  };

  // Print _text in _rect.
  float Print(const char* _text, const math::RectFloat& _rect,
              PrintLayout _layout, const GLubyte _rgba[4]) const;

  // Initialize circle vertices.
  void InitializeCircle();

  // Initializes and destroys the internal font rendering system.
  void InitalizeFont();
  void DestroyFont();

  // ImGui state.

  // Current frame inputs.
  Inputs inputs_;

  // Internal states.
  // The hot item is the one that's below the mouse cursor.
  int hot_item_;

  // The active item is the one being currently interacted with.
  int active_item_;

  // The automatically generated widget identifier.
  int auto_gen_id_;

  struct Container {
    // The current rect.
    math::RectFloat rect;

    // The y offset of the top of the next widget in the current container.
    float offset_y;

    // Constrains container height to the size used by controls in the
    // container.
    bool constrain;
  };

  // Container stack.
  ozz::vector<Container> containers_;

  // Round rendering.

  // Defines the number of segments used by the precomputed circle.
  // Must be multiple of 4.
  static const int kCircleSegments = 8;

  // Circle vertices coordinates (cosine/sinus)
  float circle_[kCircleSegments][2];

  // Font rendering.

  // Declares font's structure.
  struct Font {
    int texture_width;   // width of the pow2 texture.
    int texture_height;  // height of the pow2 texture.
    int image_width;     // width of the image area in the texture.
    int image_height;    // height of the image area in the texture.
    int glyph_height;
    int glyph_width;
    int glyph_count;
    unsigned char glyph_start;  // ascii code of the first character.
    const unsigned char* pixels;
    size_t pixels_size;
  };

  // Declares the font instance.
  static const Font font_;

  // Pre-cooked glyphes uv and vertex coordinates.
  struct Glyph {
    float uv[4][2];
    float pos[4][2];
  };
  Glyph glyphes_[256];

  // Glyph GL texture
  unsigned int glyph_texture_;

  // Renderer, available between Begin/EndFrame
  RendererImpl* renderer_;
};
}  // namespace internal
}  // namespace sample
}  // namespace ozz
#endif  // OZZ_SAMPLES_FRAMEWORK_INTERNAL_IMGUI_IMPL_H_
