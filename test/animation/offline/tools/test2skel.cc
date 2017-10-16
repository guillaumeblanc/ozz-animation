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

#include "ozz/animation/offline/raw_skeleton.h"

#include "ozz/base/io/stream.h"
#include "ozz/base/memory/allocator.h"

class TestSkeletonConverter
    : public ozz::animation::offline::SkeletonConverter {
 public:
  TestSkeletonConverter() : file_(NULL) {}
  ~TestSkeletonConverter() { ozz::memory::default_allocator()->Delete(file_); }

 private:
  virtual bool Load(const char* _filename) {
    ozz::memory::default_allocator()->Delete(file_);
    file_ =
        ozz::memory::default_allocator()->New<ozz::io::File>(_filename, "rb");
    if (!file_->opened()) {
      ozz::memory::default_allocator()->Delete(file_);
      file_ = NULL;
      return false;
    }
    return true;
  }

  virtual bool Import(ozz::animation::offline::RawSkeleton* _skeleton) {
    (void)_skeleton;
    if (file_ && file_->opened()) {
      char buffer[256];
      const char good_content[] = "good content 1";
      if (file_->Read(buffer, sizeof(buffer)) >= sizeof(good_content) - 1 &&
          memcmp(buffer, good_content, sizeof(good_content) - 1) == 0) {

        _skeleton->roots.resize(1);
        ozz::animation::offline::RawSkeleton::Joint& root = _skeleton->roots[0];
        root.name = "root";

        root.children.resize(3);
        ozz::animation::offline::RawSkeleton::Joint& joint0 = root.children[0];
        joint0.name = "joint0";
        ozz::animation::offline::RawSkeleton::Joint& joint1 = root.children[1];
        joint1.name = "joint1";
        ozz::animation::offline::RawSkeleton::Joint& joint2 = root.children[2];
        joint2.name = "joint2";

        return true;
      }
    }
    return false;
  }

  ozz::io::File* file_;
};

int main(int _argc, const char** _argv) {
  TestSkeletonConverter converter;
  return converter(_argc, _argv);
}
