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

#include <string.h>

#include "ozz/animation/offline/tools/convert2anim.h"

#include "ozz/animation/runtime/skeleton.h"

#include "ozz/base/io/stream.h"

class TestAnimationConverter :
  public ozz::animation::offline::AnimationConverter {
private:
  // Implement SkeletonConverter::Import function.
  virtual bool Import(const char* _filename,
                      const ozz::animation::Skeleton& _skeleton,
                      float _sampling_rate,
                      ozz::animation::offline::RawAnimation* _animation) {
    (void)_sampling_rate;
    (void)_skeleton;
    (void)_animation;

    ozz::io::File file(_filename, "rb");
    if (file.opened()) {
      char buffer[256];
      const char good_content[] = "good content";
      file.Read(buffer, sizeof(buffer));
      if (memcmp(buffer, good_content, sizeof(good_content) - 1) == 0) {
        return true;
      }
    }
    return false;
  }
};

int main(int _argc, const char** _argv) {
  TestAnimationConverter converter;
  return converter(_argc, _argv);
}
