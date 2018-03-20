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

#include "ozz/animation/offline/tools/convert2ozz.h"

// Mocks AnimationConverter so it can be used to dump default and reference
// configurations.
class DumpAnimationConverter
    : public ozz::animation::offline::AnimationConverter {
 public:
  DumpAnimationConverter() {}
  ~DumpAnimationConverter() {}

 private:
  virtual bool Load(const char* _filename) {
    (void)_filename;
    return true;
  }

  virtual bool Import(ozz::animation::offline::RawSkeleton* _skeleton,
                      bool _all_nodes) {
    (void)_skeleton;
    (void)_all_nodes;
    return true;
  }

  virtual AnimationNames GetAnimationNames() {
    AnimationNames names;
    return names;
  }

  virtual bool Import(const char* _animation_name,
                      const ozz::animation::Skeleton& _skeleton,
                      float _sampling_rate,
                      ozz::animation::offline::RawAnimation* _animation) {
    (void)_animation_name;
    (void)_skeleton;
    (void)_sampling_rate;
    (void)_animation;
    return true;
  }

  virtual NodeProperties GetNodeProperties(const char* _node_name) {
    NodeProperties ppts;
    return ppts;
  }

  virtual bool Import(const char* _animation_name, const char* _node_name,
                      const char* _track_name, float _sampling_rate,
                      ozz::animation::offline::RawFloatTrack* _track) {
    (void)_animation_name;
    (void)_node_name;
    (void)_track_name;
    (void)_sampling_rate;
    (void)_track;
    return true;
  }

  virtual bool Import(const char* _animation_name, const char* _node_name,
                      const char* _track_name, float _sampling_rate,
                      ozz::animation::offline::RawFloat2Track* _track) {
    (void)_animation_name;
    (void)_node_name;
    (void)_track_name;
    (void)_sampling_rate;
    (void)_track;
    return true;
  }

  virtual bool Import(const char* _animation_name, const char* _node_name,
                      const char* _track_name, float _sampling_rate,
                      ozz::animation::offline::RawFloat3Track* _track) {
    (void)_animation_name;
    (void)_node_name;
    (void)_track_name;
    (void)_sampling_rate;
    (void)_track;
    return true;
  }
  virtual bool Import(const char* _animation_name, const char* _node_name,
                      const char* _track_name, float _sampling_rate,
                      ozz::animation::offline::RawFloat4Track* _track) {
    (void)_animation_name;
    (void)_node_name;
    (void)_track_name;
    (void)_sampling_rate;
    (void)_track;
    return true;
  }
};

int main(int _argc, const char** _argv) {
  DumpAnimationConverter converter;
  return converter(_argc, _argv);
}
