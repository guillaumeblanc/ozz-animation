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

#include "ozz/animation/offline/tools/convert2skel.h"

#include "ozz/animation/offline/fbx/fbx.h"
#include "ozz/animation/offline/fbx/fbx_skeleton.h"

#include "ozz/animation/offline/raw_skeleton.h"

#include "ozz/base/log.h"
#include "ozz/base/memory/allocator.h"

// fbx2skel is a command line tool that converts a skeleton imported from a
// Fbx document to ozz runtime format.
//
// fbx2skel extracts the skeleton from the Fbx document. It then builds an
// ozz runtime skeleton, from the Fbx skeleton, and serializes it to a ozz
// binary archive.
//
// Use fbx2skel integrated help command (fbx2skel --help) for more details
// about available arguments.
class FbxSkeletonConverter : public ozz::animation::offline::SkeletonConverter {
 public:
  FbxSkeletonConverter() : settings_(fbx_manager_), scene_loader_(NULL) {}
  ~FbxSkeletonConverter() {
    ozz::memory::default_allocator()->Delete(scene_loader_);
  }

 private:
  virtual bool Load(const char* _filename) {
    ozz::memory::default_allocator()->Delete(scene_loader_);
    scene_loader_ = ozz::memory::default_allocator()
                        ->New<ozz::animation::offline::fbx::FbxSceneLoader>(
                            _filename, "", fbx_manager_, settings_);

    if (!scene_loader_->scene()) {
      ozz::log::Err() << "Failed to import file " << _filename << "."
                      << std::endl;
      ozz::memory::default_allocator()->Delete(scene_loader_);
      scene_loader_ = NULL;
      return false;
    }
    return true;
  }

  virtual bool Import(ozz::animation::offline::RawSkeleton* _skeleton,
                      bool _all_nodes) {
    if (!_skeleton) {
      return false;
    }

    // Reset skeleton.
    *_skeleton = ozz::animation::offline::RawSkeleton();

    if (!scene_loader_) {
      return false;
    }

    if (!ozz::animation::offline::fbx::ExtractSkeleton(*scene_loader_,
                                                       _all_nodes, _skeleton)) {
      ozz::log::Err() << "Fbx skeleton extraction failed." << std::endl;
      return false;
    }

    return true;
  }

  ozz::animation::offline::fbx::FbxManagerInstance fbx_manager_;
  ozz::animation::offline::fbx::FbxSkeletonIOSettings settings_;
  ozz::animation::offline::fbx::FbxSceneLoader* scene_loader_;
};

int main(int _argc, const char** _argv) {
  FbxSkeletonConverter converter;
  return converter(_argc, _argv);
}
