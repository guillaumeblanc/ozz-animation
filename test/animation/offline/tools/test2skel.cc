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

#include "ozz/animation/offline/tools/convert2skel.h"

#include "ozz/base/io/stream.h"

class TestSkeletonConverter
    : public ozz::animation::offline::SkeletonConverter {
 private:
  // Implement SkeletonConverter::Import function.
  virtual bool Import(const char* _filename,
                      ozz::animation::offline::RawSkeleton* _skeleton) {
    (void)_skeleton;
    ozz::io::File file(_filename, "rb");
    if (file.opened()) {
      char buffer[256];
      const char good_content[] = "good content 1";
      if (file.Read(buffer, sizeof(buffer)) >= sizeof(good_content) - 1 &&
          memcmp(buffer, good_content, sizeof(good_content) - 1) == 0) {
        return true;
      }
    }
    return false;
  }
};

int main(int _argc, const char** _argv) {
  TestSkeletonConverter converter;
  return converter(_argc, _argv);
}
