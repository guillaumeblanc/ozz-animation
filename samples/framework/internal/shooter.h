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

#ifndef OZZ_SAMPLES_FRAMEWORK_INTERNAL_SHOOTER_H_
#define OZZ_SAMPLES_FRAMEWORK_INTERNAL_SHOOTER_H_

#include "ozz/base/containers/vector.h"

#include "framework/image.h"

namespace ozz {
namespace sample {
namespace internal {

// Implements GL screen shot and video shooter
class Shooter {
 public:
  Shooter();
  virtual ~Shooter();

  // Resize notification, used to resize memory buffers.
  void Resize(int _width, int _height);

  // Updates shooter (output captured buffers to memory).
  bool Update();

  // Captures current (GL_FRONT or GL_BACK) _buffer to memory.
  bool Capture(int _buffer);

 private:
  // Updates all cooldowns and process terminated shots. Returns false if it
  // fails, true on success or empty stack.
  bool Process();

  // Processes all shots from the stack. Returns false if it fails, true on
  // success or empty stack.
  bool ProcessAll();

  // Defines shot buffer (pbo) and data.
  struct Shot {
    unsigned int pbo;
    int width;
    int height;
    int cooldown;  // Shot is processed when cooldown falls to 0.
    Shot() : pbo(0), width(0), height(0), cooldown(0) {}
  };

  // Array of pre-allocated shots, used to allow asynchronous dma transfers of
  // pbos.
  enum {
    kInitialCountDown = 2,  // Allows to delay pbo mapping 2 rendering frames.
    kNumShots = kInitialCountDown,
  };
  Shot shots_[kNumShots];

  // Format of pixels to use to glReadPixels calls.
  int gl_shot_format_;

  // Image format that matches GL format.
  image::Format::Value image_format_;

  // Shot number, used to name images.
  int shot_number_;

  // Is the shooter functionality supported.
  bool supported_;
};
}  // namespace internal
}  // namespace sample
}  // namespace ozz
#endif  // OZZ_SAMPLES_FRAMEWORK_INTERNAL_SHOOTER_H_
