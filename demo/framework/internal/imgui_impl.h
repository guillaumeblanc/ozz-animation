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

#ifndef OZZ_DEMO_FRAMEWORK_INTERNAL_IMGUI_IMPL_H_
#define OZZ_DEMO_FRAMEWORK_INTERNAL_IMGUI_IMPL_H_

#ifndef OZZ_INCLUDE_PRIVATE_HEADER
#error "This header is private, it cannot be included from public headers."
#endif  // OZZ_INCLUDE_PRIVATE_HEADER

// Implements immediate mode gui.
// See imgui.h for details about function specifications.

#include "framework/imgui.h"

#include "ozz/base/containers/vector.h"
#include "ozz/base/maths/rect.h"

namespace ozz {
namespace demo {
namespace internal {

// Immediate mode gui implementation.
class ImGuiImpl : public ImGui {
 public:

  ImGuiImpl();
  virtual ~ImGuiImpl();

  // Input state.
  struct Inputs {
    // Default input values.
    Inputs()
      : mouse_x(0),
        mouse_y(0),
        lmb_pressed(false) {
    }

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
  void BeginFrame(const Inputs& _inputs, const math::RectInt& _rect);

  // Ends the current imgui frame.
  void EndFrame();

 protected:

  // Starts ImGui interface implementation.
  // See imgui.h for virtual function specifications.

  virtual void BeginContainer(const char* _title,
                              const math::RectInt* _rect,
                              bool* _open);

  virtual void EndContainer();

  virtual bool DoButton(const char* _label, bool _enabled);

  virtual bool DoSlider(const char* _label,
                        float _min, float _max, float* _value,
                        float _pow = 1.f,
                        bool _enabled = true);

  virtual bool DoCheckBox(const char* _label, bool* _state, bool _enabled);

  virtual bool DoRadioButton(int _ref,
                             const char* _label,
                             int* _value,
                             bool _enabled);

  virtual void DoLabel(const char* _label,
                       Justification _justification,
                       bool _single_line);

  virtual void DoGraph(const char* _label,
                       float _min, float _max, float _mean,
                       const float* _value_cursor,
                       const float* _value_begin, const float* _value_end);
 private:

  // Computes the rect of a new widget to add.
  // Returns true if there is enough space for a new widget.
  bool AddWidget(int _height, math::RectInt* _rect);

  // Implements button logic.
  // Returns true if the button was clicked.
  bool ButtonLogic(const math::RectInt& _rect, int _id, bool* _hot, bool* _active);

  // Fills a rectangle with _rect coordinates. Draws rounded angles if _radius
  // is greater than 0.
  // If _texture id is not 0, then texture with id _texture is mapped using a
  // planar projection to the rect.
  void FillRect(const math::RectInt& _rect, int _radius) const;

  // Strokes a rectangle with _rect coordinates. Draws rounded angles if _radius
  // is greater than 0.
  void StrokeRect(const math::RectInt& _rect, int _radius) const;

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
  int Print(const char* _text,
            const math::RectInt& _rect,
            PrintLayout _layout) const;

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
    math::RectInt rect;

    // The y offset of the top of the next widget in the current container.
    int offset_y;
  };

  // Container stack.
  ozz::Vector<Container>::Std containers_;

  // Round rendering.

  // Defines the number of segments used by the precomputed circle.
  // Must be multiple of 4.
  static const int kCircleSegments = 20;

  // The radius of the precomputed circle.
  // Uses a power of 2 value in order for the compiler to optimize the division.
  static const int kCircleRadius = 32;

  // Circle vertices coordinates (cosine/sinus)
  int circle_[kCircleSegments][2];

  // Font rendering.

  // Declares font's structure.
  struct Font {
    int texture_width;  // width of the pow2 texture.
    int texture_height;  // height of the pow2 texture.
    int image_width;  // width of the image area in the texture.
    int image_height;  // height of the image area in the texture.
    int glyph_height;
    int glyph_width;
    int glyph_count;
    unsigned char glyph_start;  // ascii code of the first character.
    const unsigned char* pixels;
    std::size_t pixels_size;
  };

  // Declares the font instance.
  static const Font font_;

  // The base index of the glyph GL display list.
  unsigned int glyph_displaylist_base_;

  // Glyph GL texture
  unsigned int glyph_texture_;
};
}  // internal
}  // demo
}  // ozz
#endif  // OZZ_DEMO_FRAMEWORK_INTERNAL_IMGUI_IMPL_H_
