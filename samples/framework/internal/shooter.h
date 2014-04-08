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
};
}  // internal
}  // sample
}  // ozz
#endif  // OZZ_SAMPLES_FRAMEWORK_INTERNAL_SHOOTER_H_
