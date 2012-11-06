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

#ifndef OZZ_OZZ_BASE_MATHS_BOX_H_
#define OZZ_OZZ_BASE_MATHS_BOX_H_

#include <cstddef>

#include "ozz/base/maths/vec_float.h"

namespace ozz {
namespace math {

// Defines an axis aligned box.
struct Box {

  // Constructs an invalid box.
  Box();

  // Constructs a box with the specified _min and _max bounds.
  Box(const Float3& _min, const Float3& _max)
    : min(_min),
      max(_max){
  }

  // Constructs the smallest box that contains the _count points _points.
  // _stride is the number of bytes points.
  Box(const Float3* _points, std::size_t _stride, std::size_t _count);

  // Tests whether *this is a valid box.
  bool is_valid() const {
    return min <= max;
  }

  // Tests whether _p is within box bounds.
  bool is_inside(const Float3& _p) const {
    return _p >= min && _p <= max;
  }

  // Box's min and max bounds.
  Float3 min;
  Float3 max;
};

// Merges two boxes _a and _b.
// Both _a and _b can be invalid.
OZZ_INLINE Box Merge(const Box& _a, const Box& _b) {
  if (!_a.is_valid()) {
    return _b;
  } else if(!_b.is_valid()) {
    return _a;
  }
  return Box(Min(_a.min, _b.min), Max(_a.max, _b.max));
}
}  // ozz
}  // math
#endif  // OZZ_OZZ_BASE_MATHS_BOX_H_
