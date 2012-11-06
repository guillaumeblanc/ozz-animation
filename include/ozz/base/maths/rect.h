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

#ifndef OZZ_OZZ_BASE_MATHS_RECT_H_
#define OZZ_OZZ_BASE_MATHS_RECT_H_

namespace ozz {
namespace math {

// Defines a rectangle by the integer coordinates of its lower-left and
// width-height.
struct RectInt {

  // Constructs a uninitialized rectangle.
  RectInt() {
  }

  // Constructs a rectangle with the specified arguments.
  RectInt(int _left, int _bottom, int _width, int _height)
    : left(_left),
      bottom(_bottom),
      width(_width),
      height(_height) {
  }

  // Tests whether _x and _y coordinates are within rectangle bounds.
  bool is_inside(int _x, int _y) const {
    return _x >= left && _x < left + width &&
           _y >= bottom && _y < bottom + height;
  }

  // Gets the rectangle x coordinate of the right rectangle side.
  int right() const {
    return left + width;
  }

  // Gets the rectangle y coordinate of the top rectangle side.
  int top() const {
    return bottom + height;
  }

  // Specifies the x-coordinate of the lower side. 
  int left;
  // Specifies the x-coordinate of the left side.
  int bottom;
  // Specifies the width of the rectangle.
  int width;
  // Specifies the height of the rectangle..
  int height;
};

// Defines a rectangle by the floating point coordinates of its lower-left
// and width-height.
struct RectFloat {

  // Constructs a uninitialized rectangle.
  RectFloat() {
  }

  // Constructs a rectangle with the specified arguments.
  RectFloat(float _left, float _bottom, float _width, float _height) :
    left(_left),
    bottom(_bottom),
    width(_width),
    height(_height) {
  }

  // Tests whether _x and _y coordinates are within rectangle bounds
  bool is_inside(float _x, float _y) const {
    return _x >= left && _x < left + width &&
           _y >= bottom && _y < bottom + height;
  }

  // Gets the rectangle x coordinate of the right rectangle side.
  float right() const {
    return left + width;
  }

  // Gets the rectangle y coordinate of the top rectangle side.
  float top() const {
    return bottom + height;
  }

  // Specifies the x-coordinate of the lower side. 
  float left;
  // Specifies the x-coordinate of the left side.
  float bottom;
  // Specifies the width of the rectangle.
  float width;
  // Specifies the height of the rectangle.
  float height;
};
}  // math
}  // ozz
#endif  // OZZ_OZZ_BASE_MATHS_RECT_H_
