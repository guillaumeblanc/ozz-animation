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
#include "ozz/base/memory/allocator.h"

class TestAnimationConverter
    : public ozz::animation::offline::AnimationConverter {
 public:
  TestAnimationConverter() : file_(NULL) {}
  ~TestAnimationConverter() { ozz::memory::default_allocator()->Delete(file_); }

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

  virtual AnimationNames GetAnimationNames() {
    AnimationNames names;

    if (file_ && file_->opened()) {
      char buffer[256];

      file_->Seek(0, ozz::io::File::kSet);
      const char good_content1[] = "good content 1";
      if (file_->Read(buffer, sizeof(buffer)) >= sizeof(good_content1) - 1 &&
          memcmp(buffer, good_content1, sizeof(good_content1) - 1) == 0) {
        names.push_back("one");
        return names;
      }

      // Handles more than one animation per file.
      file_->Seek(0, ozz::io::File::kSet);
      const char good_content2[] = "good content 2";
      if (file_->Read(buffer, sizeof(buffer)) >= sizeof(good_content2) - 1 &&
          memcmp(buffer, good_content2, sizeof(good_content2) - 1) == 0) {
        names.push_back("one");
        names.push_back("TWO");
        return names;
      }
    }

    return names;
  }

  virtual bool Import(const char* _animation_name,
                      const ozz::animation::Skeleton& _skeleton,
                      float _sampling_rate,
                      ozz::animation::offline::RawAnimation* _animation) {
    (void)_sampling_rate;
    (void)_skeleton;

    if (file_ && file_->opened()) {
      char buffer[256];
      // Handles a single animation per file.
      file_->Seek(0, ozz::io::File::kSet);
      const char good_content1[] = "good content 1";
      if (file_->Read(buffer, sizeof(buffer)) >= sizeof(good_content1) - 1 &&
          memcmp(buffer, good_content1, sizeof(good_content1) - 1) == 0) {
        _animation->name = _animation_name;
        return true;
      }
      // Handles more than one animation per file.
      file_->Seek(0, ozz::io::File::kSet);
      const char good_content2[] = "good content 2";
      if (file_->Read(buffer, sizeof(buffer)) >= sizeof(good_content2) - 1 &&
          memcmp(buffer, good_content2, sizeof(good_content2) - 1) == 0) {
        _animation->name = _animation_name;
        return true;
      }
    }
    return false;
  }

  virtual bool Import(const char* _animation_name, const char* _node_name,
                      const char* _track_name, float _sampling_rate,
                      ozz::animation::offline::RawFloatTrack* _track) {
    (void)_animation_name;
    (void)_sampling_rate;
    (void)_track;

    return strcmp(_node_name, "node_name") == 0 &&
           strcmp(_track_name, "track_name") == 0;
  }

  ozz::io::File* file_;
};

int main(int _argc, const char** _argv) {
  TestAnimationConverter converter;
  return converter(_argc, _argv);
}
