//----------------------------------------------------------------------------//
//                                                                            //
// ozz-animation is hosted at http://github.com/guillaumeblanc/ozz-animation  //
// and distributed under the MIT License (MIT).                               //
//                                                                            //
// Copyright (c) 2017 Guillaume Blanc                                         //
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

#include <string.h>

#include "ozz/animation/offline/tools/convert2anim.h"

#include "ozz/animation/runtime/skeleton.h"

#include "ozz/base/io/stream.h"

class TestAnimationConverter
    : public ozz::animation::offline::AnimationConverter {
 private:
  // Implement SkeletonConverter::Import function.
  virtual bool Import(const char* _filename,
                      const ozz::animation::Skeleton& _skeleton,
                      float _sampling_rate, Animations* _animations) {
    (void)_sampling_rate;
    (void)_skeleton;

    // Start with a clean output.
    _animations->clear();

    ozz::io::File file(_filename, "rb");
    if (file.opened()) {
      char buffer[256];
      // Hanldes a single animation per file.
      const char good_content1[] = "good content 1";
      file.Seek(0, ozz::io::File::kSet);
      if (file.Read(buffer, sizeof(buffer)) >= sizeof(good_content1) - 1 &&
          memcmp(buffer, good_content1, sizeof(good_content1) - 1) == 0) {
        _animations->resize(1);
        _animations->at(0).name = "one";
        return true;
      }
      // Hanldes more than one animation per file.
      const char good_content2[] = "good content 2";
      file.Seek(0, ozz::io::File::kSet);
      if (file.Read(buffer, sizeof(buffer)) >= sizeof(good_content2) - 1 &&
          memcmp(buffer, good_content2, sizeof(good_content2) - 1) == 0) {
        _animations->resize(2);
        _animations->at(0).name = "one";
        _animations->at(1).name = "TWO";
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
