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

#include "ozz/base/maths/box.h"

#include <limits>

#include "ozz/base/maths/math_ex.h"

namespace ozz {
namespace math {

Box::Box()
  : min(std::numeric_limits<float>::max()),
    max(-std::numeric_limits<float>::max()) {
}

Box::Box(const Float3* _points, std::size_t _stride, std::size_t _count) {
  Float3 local_min(std::numeric_limits<float>::max());
  Float3 local_max(-std::numeric_limits<float>::max());

  const Float3* end = Stride(_points, _stride * _count);
  for (; _points < end; _points = Stride(_points, _stride)) {
    local_min = Min(local_min, *_points);
    local_max = Max(local_max, *_points);
  }

  min = local_min;
  max = local_max;
}
}  // ozz
}  // math
