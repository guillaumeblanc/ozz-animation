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

#include "ozz/animation/runtime/animation_utils.h"

// Internal include file
#define OZZ_INCLUDE_PRIVATE_HEADER  // Allows to include private headers.
#include "animation/runtime/animation_keyframe.h"

namespace ozz {
namespace animation {

inline int CountKeyframesImpl(const Animation::KeyframesCtrlConst& _ctrl,
                              int _track) {
  if (_track < 0) {
    return static_cast<int>(_ctrl.previouses.size());
  }

  int count = 1;
  size_t previous = static_cast<size_t>(_track);
  for (size_t i = previous + 1; i < _ctrl.previouses.size(); ++i) {
    if (i - _ctrl.previouses[i] == previous) {
      ++count;
      previous = i;
    }
  }
  return count;
}

int CountTranslationKeyframes(const Animation& _animation, int _track) {
  return CountKeyframesImpl(_animation.translations_ctrl(), _track);
}
int CountRotationKeyframes(const Animation& _animation, int _track) {
  return CountKeyframesImpl(_animation.rotations_ctrl(), _track);
}
int CountScaleKeyframes(const Animation& _animation, int _track) {
  return CountKeyframesImpl(_animation.scales_ctrl(), _track);
}
}  // namespace animation
}  // namespace ozz
