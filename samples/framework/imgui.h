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

#ifndef OZZ_SAMPLES_FRAMEWORK_IMGUI_H_
#define OZZ_SAMPLES_FRAMEWORK_IMGUI_H_

#include <cstdio>

namespace ozz {
namespace math { struct RectFloat; }
namespace sample {

// Interface for immediate mode graphical user interface rendering.
class ImGui {
 public:

  // Declares a virtual destructor to allow proper destruction.
  virtual ~ImGui() {
  }

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
  // Providing a non NULL _title argument displays a title on top of the form.
  // Providing a non NULL _open argument enables the open/close mechanism.
  class Form {
   public:
    Form(ImGui* _im_gui,
         const char* _title,
         const math::RectFloat& _rect,
         bool* _open,
         bool _constrain)
      : im_gui_(_im_gui){
      im_gui_->BeginContainer(_title, &_rect, _open, _constrain);
    }
    ~Form() {
      im_gui_->EndContainer();
    }
   private:
    Form(const Form&);  // Forbids copy.
    void operator = (const Form&);  // Forbids assignment.
    ImGui* im_gui_;
  };

  // Begins a new open-close widget in the parent's rect, ie: a form or an other
  // open-close.
  // This object uses the RAII mechanism to ensure open/close symmetry.
  // Providing a non NULL _title argument displays a title on top of the
  // open-close.
  // Providing a non NULL _open argument enables the open/close mechanism.
  class OpenClose {
   public:
    OpenClose(ImGui* _im_gui, const char* _title, bool* _open)
      : im_gui_(_im_gui) {
      im_gui_->BeginContainer(_title, NULL, _open, true);
    }
    ~OpenClose() {
      im_gui_->EndContainer();
    }
   private:
    OpenClose(const OpenClose&);  // Forbids copy.
    void operator = (const OpenClose&);  // Forbids assignment.
    ImGui* im_gui_;
  };

  // Adds a button to the current context and returns true if it was clicked.
  // If _enabled is false then interactions with the button are disabled, and
  // rendering is grayed out.
  // If _state is not NULL, then it is used as an in-out parameter to set and
  // store button's state. The button can then behave like a check box, with
  // a button rendering style.
  // It allows for example to 
  // Returns true is button was clicked.
  virtual bool DoButton(const char* _label,
                        bool _enabled = true,
                        bool* _state = NULL) = 0;

  // Adds a float slider to the current context and returns true if _value was
  // modified.
  // _value is the in-out parameter that stores slider value. It's clamped
  // between _min and _max bounds.
  // _pow is used to modify slider's scale. It can be used to give a higher
  // precision to low or high values according to _pow.
  // If _enabled is false then interactions with the slider are disabled, and
  // rendering is grayed out.
  // Returns true if _value _value has changed.
  virtual bool DoSlider(const char* _label,
                        float _min, float _max, float* _value,
                        float _pow = 1.f,
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
  virtual bool DoSlider(const char* _label,
                        int _min, int _max, int* _value,
                        float _pow = 1.f,
                        bool _enabled = true) = 0;

  // Adds a check box to the current context and returns true if it has been
  // modified. Used to represent boolean value.
  // _state is the in-out parameter that stores check box state.
  // If _enabled is false then interactions with the slider are disabled, and
  // rendering is grayed out.
  // Returns true if _value _state has changed.
  virtual bool DoCheckBox(const char* _label,
                          bool* _state,
                          bool _enabled = true) = 0;

  // Adds a radio button to the current context and returns true if it has been
  // modified. Used to represent a possible reference _ref value taken by the
  // current value _value.
  // Displays a "checked" radio button if _ref si equal to th selected _value.
  // Returns true if _value _value has changed.
  virtual bool DoRadioButton(int _ref,
                             const char* _label,
                             int* _value,
                             bool _enabled = true) = 0;

  // Adds a text label to the current context. Its height depends on the number
  // of lines.
  // _justification selects the text alignment in the current container.
  // if _single_line is true then _label text is cut at the end of the first
  // line.
  virtual void DoLabel(const char* _label,
                       Justification _justification = kLeft,
                       bool _single_line = true) = 0;

  // Adds a graph widget to the current context.
  // Displays values from the right (newest value) to the left (latest).
  // The range of value is described by _value_begin, _value_end. _value_cursor
  // allows using a linear or circular buffer of values. Set _value_cursor to
  // _value_begin to use a linear buffer of range [_value_begin,_value_end[. Or
  // set _value_cursor of a circular buffer such that range [_value_cursor,
  // _value_end[ and [_value_begin,_value_cursor[ are used.
  // All values outside of _min and _max range are clamped.
  // If _label is not NULL then a text is displayed on top of the graph.
  virtual void DoGraph(const char* _label,
                       float _min, float _max, float _mean,
                       const float* _value_cursor,
                       const float* _value_begin, const float* _value_end) = 0;

 private:

  // Begins a new container of size _rect.
  // Widgets (buttons, sliders...) can only be displayed in a container.
  // The rectangles height is the maximum height that the container can use. The
  // container automatically shrinks to fit the size of the widgets it contains.
  // Providing a non NULL _title argument displays a title on top of the container.
  // Providing a NULL _rect argument means that the container will use all its
  // parent size.
  // Providing a non NULL _open argument enables the open/close mechanism.
  virtual void BeginContainer(const char* _title,
                              const math::RectFloat* _rect,
                              bool* _open,
                              bool _constrain) = 0;

  // Ends the current container.
  virtual void EndContainer() = 0;
};
}  // sample
}  // ozz
#endif  // OZZ_SAMPLES_FRAMEWORK_IMGUI_H_
