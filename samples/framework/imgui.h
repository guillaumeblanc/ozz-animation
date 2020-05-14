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

#ifndef OZZ_SAMPLES_FRAMEWORK_IMGUI_H_
#define OZZ_SAMPLES_FRAMEWORK_IMGUI_H_

#include <cstdio>

namespace ozz {
namespace math {
struct RectFloat;
}
namespace sample {

// Interface for immediate mode graphical user interface rendering.
class ImGui {
 public:
  // Declares a virtual destructor to allow proper destruction.
  virtual ~ImGui() {}

  // Text justification types.
  enum Justification {
    kLeft,
    kCenter,
    kRight,
  };

  // Begins a new form of size _rect.
  // This object uses the RAII mechanism to ensure begin/end symmetry.
  // A form is a root in the frame's container stack.
  // The _rect argument is relative to the parent's rect and is automatically
  // shrunk to fit inside parent's rect and to the size of its widgets.
  // Providing a non nullptr _title argument displays a title on top of the form.
  // Providing a non nullptr _open argument enables the open/close mechanism.
  class Form {
   public:
    Form(ImGui* _im_gui, const char* _title, const math::RectFloat& _rect,
         bool* _open, bool _constrain)
        : im_gui_(_im_gui) {
      im_gui_->BeginContainer(_title, &_rect, _open, _constrain);
    }
    ~Form() { im_gui_->EndContainer(); }

   private:
    Form(const Form&);            // Forbids copy.
    void operator=(const Form&);  // Forbids assignment.
    ImGui* im_gui_;
  };

  // Begins a new open-close widget in the parent's rect, ie: a form or an other
  // open-close.
  // This object uses the RAII mechanism to ensure open/close symmetry.
  // Providing a non nullptr _title argument displays a title on top of the
  // open-close.
  // Providing a non nullptr _open argument enables the open/close mechanism.
  class OpenClose {
   public:
    OpenClose(ImGui* _im_gui, const char* _title, bool* _open)
        : im_gui_(_im_gui) {
      im_gui_->BeginContainer(_title, nullptr, _open, true);
    }
    ~OpenClose() { im_gui_->EndContainer(); }

   private:
    OpenClose(const OpenClose&);       // Forbids copy.
    void operator=(const OpenClose&);  // Forbids assignment.
    ImGui* im_gui_;
  };

  // Adds a button to the current context and returns true if it was clicked.
  // If _enabled is false then interactions with the button are disabled, and
  // rendering is grayed out.
  // If _state is not nullptr, then it is used as an in-out parameter to set and
  // store button's state. The button can then behave like a check box, with
  // a button rendering style.
  // It allows for example to
  // Returns true is button was clicked.
  virtual bool DoButton(const char* _label, bool _enabled = true,
                        bool* _state = nullptr) = 0;

  // Adds a float slider to the current context and returns true if _value was
  // modified.
  // _value is the in-out parameter that stores slider value. It's clamped
  // between _min and _max bounds.
  // _pow is used to modify slider's scale. It can be used to give a higher
  // precision to low or high values according to _pow.
  // If _enabled is false then interactions with the slider are disabled, and
  // rendering is grayed out.
  // Returns true if _value _value has changed.
  virtual bool DoSlider(const char* _label, float _min, float _max,
                        float* _value, float _pow = 1.f,
                        bool _enabled = true) = 0;

  // Adds an integer slider to the current context and returns true if _value
  //  was modified.
  // _value is the in-out parameter that stores slider value. It's clamped
  // between _min and _max bounds.
  // _pow is used to modify slider's scale. It can be used to give a higher
  // precision to low or high values according to _pow.
  // If _enabled is false then interactions with the slider are disabled, and
  // rendering is grayed out.
  // Returns true if _value _value has changed.
  virtual bool DoSlider(const char* _label, int _min, int _max, int* _value,
                        float _pow = 1.f, bool _enabled = true) = 0;

  // Adds a check box to the current context and returns true if it has been
  // modified. Used to represent boolean value.
  // _state is the in-out parameter that stores check box state.
  // If _enabled is false then interactions with the slider are disabled, and
  // rendering is grayed out.
  // Returns true if _value _state has changed.
  virtual bool DoCheckBox(const char* _label, bool* _state,
                          bool _enabled = true) = 0;

  // Adds a radio button to the current context and returns true if it has been
  // modified. Used to represent a possible reference _ref value taken by the
  // current value _value.
  // Displays a "checked" radio button if _ref si equal to th selected _value.
  // Returns true if _value _value has changed.
  virtual bool DoRadioButton(int _ref, const char* _label, int* _value,
                             bool _enabled = true) = 0;

  // Adds a text label to the current context. Its height depends on the number
  // of lines.
  // _justification selects the text alignment in the current container.
  // if _single_line is true then _label text is cut at the end of the first
  // line.
  virtual void DoLabel(const char* _label, Justification _justification = kLeft,
                       bool _single_line = true) = 0;

  // Adds a graph widget to the current context.
  // Displays values from the right (newest value) to the left (latest).
  // The range of value is described by _value_begin, _value_end. _value_cursor
  // allows using a linear or circular buffer of values. Set _value_cursor to
  // _value_begin to use a linear buffer of range [_value_begin,_value_end[. Or
  // set _value_cursor of a circular buffer such that range [_value_cursor,
  // _value_end[ and [_value_begin,_value_cursor[ are used.
  // All values outside of _min and _max range are clamped.
  // If _label is not nullptr then a text is displayed on top of the graph.
  virtual void DoGraph(const char* _label, float _min, float _max, float _mean,
                       const float* _value_cursor, const float* _value_begin,
                       const float* _value_end) = 0;

 private:
  // Begins a new container of size _rect.
  // Widgets (buttons, sliders...) can only be displayed in a container.
  // The rectangles height is the maximum height that the container can use. The
  // container automatically shrinks to fit the size of the widgets it contains.
  // Providing a non nullptr _title argument displays a title on top of the
  // container.
  // Providing a nullptr _rect argument means that the container will use all its
  // parent size.
  // Providing a non nullptr _open argument enables the open/close mechanism.
  virtual void BeginContainer(const char* _title, const math::RectFloat* _rect,
                              bool* _open, bool _constrain) = 0;

  // Ends the current container.
  virtual void EndContainer() = 0;
};
}  // namespace sample
}  // namespace ozz
#endif  // OZZ_SAMPLES_FRAMEWORK_IMGUI_H_
