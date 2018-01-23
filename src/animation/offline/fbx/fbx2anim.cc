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

#include "ozz/animation/offline/tools/convert2anim.h"

#include "ozz/animation/offline/fbx/fbx.h"
#include "ozz/animation/offline/fbx/fbx_animation.h"
#include "ozz/animation/offline/fbx/fbx_skeleton.h"

#include "ozz/animation/offline/raw_animation.h"
#include "ozz/animation/offline/raw_skeleton.h"

#include "ozz/base/log.h"

// fbx2anim is a command line tool that converts an animation imported from a
// fbx document to ozz runtime format.
//
// fbx2anim extracts animated joints from a fbx document. Only the animated
// joints whose names match those of the ozz runtime skeleton given as argument
// are selected. Keyframes are then optimized, based on command line settings,
// and serialized as a runtime animation to an ozz binary archive.
//
// Use fbx2anim integrated help command (fbx2anim --help) for more details
// about available arguments.

class FbxAnimationConverter
    : public ozz::animation::offline::AnimationConverter {
 public:
  FbxAnimationConverter() : settings_(fbx_manager_), scene_loader_(NULL) {}
  ~FbxAnimationConverter() {
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

  virtual AnimationNames GetAnimationNames() {
    if (!scene_loader_) {
      return AnimationNames();
    }
    return ozz::animation::offline::fbx::GetAnimationNames(*scene_loader_);
  }

  virtual bool Import(const char* _animation_name,
                      const ozz::animation::Skeleton& _skeleton,
                      float _sampling_rate,
                      ozz::animation::offline::RawAnimation* _animation) {
    if (!_animation) {
      return false;
    }

    *_animation = ozz::animation::offline::RawAnimation();

    if (!scene_loader_) {
      return false;
    }

    return ozz::animation::offline::fbx::ExtractAnimation(
        _animation_name, *scene_loader_, _skeleton, _sampling_rate, _animation);
  }

  virtual NodeProperties GetNodeProperties(const char* _node_name) {
    if (!scene_loader_) {
      return NodeProperties();
    }

    return ozz::animation::offline::fbx::GetNodeProperties(*scene_loader_, _node_name);
  }

  template<typename _RawTrack>
  bool Import(const char* _animation_name, const char* _node_name,
              const char* _track_name, float _sampling_rate,
              _RawTrack* _track) {
    if (!scene_loader_) {
      return false;
    }
    return ozz::animation::offline::fbx::ExtractTrack(
        _animation_name, _node_name, _track_name, *scene_loader_,
        _sampling_rate, _track);

  }

  virtual bool Import(const char* _animation_name, const char* _node_name,
                      const char* _track_name, float _sampling_rate,
                      ozz::animation::offline::RawFloatTrack* _track) {
    return Import(_animation_name, _node_name, _track_name, _sampling_rate, _track);
  }

  virtual bool Import(const char* _animation_name, const char* _node_name,
                      const char* _track_name, float _sampling_rate,
                      ozz::animation::offline::RawFloat2Track* _track) {
    return Import(_animation_name, _node_name, _track_name, _sampling_rate, _track);
  }

  virtual bool Import(const char* _animation_name, const char* _node_name,
                      const char* _track_name, float _sampling_rate,
                      ozz::animation::offline::RawFloat3Track* _track) {
    return Import(_animation_name, _node_name, _track_name, _sampling_rate, _track);
  }

  ozz::animation::offline::fbx::FbxManagerInstance fbx_manager_;
  ozz::animation::offline::fbx::FbxAnimationIOSettings settings_;
  ozz::animation::offline::fbx::FbxSceneLoader* scene_loader_;
};

int main(int _argc, const char** _argv) {
  FbxAnimationConverter converter;
  return converter(_argc, _argv);
}
