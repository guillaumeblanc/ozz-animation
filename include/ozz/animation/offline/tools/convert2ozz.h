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

#ifndef OZZ_OZZ_ANIMATION_OFFLINE_TOOLS_CONVERT2OZZ_H_
#define OZZ_OZZ_ANIMATION_OFFLINE_TOOLS_CONVERT2OZZ_H_

#include "ozz/base/containers/vector.h"

#include "ozz/animation/offline/raw_animation.h"
#include "ozz/animation/offline/raw_skeleton.h"
#include "ozz/animation/offline/raw_track.h"

namespace ozz {
namespace animation {

class Skeleton;

namespace offline {

class OzzConverter {
 public:
  int operator()(int _argc, const char** _argv);

  // Skeleton management.

  struct NodeType {
    bool skeleton : 1;  // Uses skeleton nodes as skeleton joints.
    bool marker : 1;    // Uses marker nodes as skeleton joints.
    bool camera : 1;    // Uses camera nodes as skeleton joints.
    bool geometry : 1;  // Uses geometry nodes as skeleton joints.
    bool light : 1;     // Uses light nodes as skeleton joints.
    bool any : 1;  // Uses any node type as skeleton joints, including those
                   // listed above and any other.
  };

  virtual bool Import(ozz::animation::offline::RawSkeleton* _skeleton,
                      const NodeType& _types) = 0;

  // Animations management.

  typedef ozz::Vector<ozz::String::Std>::Std AnimationNames;

  virtual AnimationNames GetAnimationNames() = 0;

  virtual bool Import(const char* _animation_name,
                      const ozz::animation::Skeleton& _skeleton,
                      float _sampling_rate, RawAnimation* _animation) = 0;

  // Tracks / properties management.

  struct NodeProperty {
    ozz::String::Std name;
    enum Type { kFloat1 = 1, kFloat2 = 2, kFloat3 = 3, kFloat4 = 4 };
    Type type;
  };
  typedef ozz::Vector<NodeProperty>::Std NodeProperties;

  virtual NodeProperties GetNodeProperties(const char* _node_name) = 0;

  virtual bool Import(const char* _animation_name, const char* _node_name,
                      const char* _track_name, float _sampling_rate,
                      RawFloatTrack* _track) = 0;
  virtual bool Import(const char* _animation_name, const char* _node_name,
                      const char* _track_name, float _sampling_rate,
                      RawFloat2Track* _track) = 0;
  virtual bool Import(const char* _animation_name, const char* _node_name,
                      const char* _track_name, float _sampling_rate,
                      RawFloat3Track* _track) = 0;
  virtual bool Import(const char* _animation_name, const char* _node_name,
                      const char* _track_name, float _sampling_rate,
                      RawFloat4Track* _track) = 0;

 private:
  virtual bool Load(const char* _filename) = 0;
};
}  // namespace offline
}  // namespace animation
}  // namespace ozz
#endif  // OZZ_OZZ_ANIMATION_OFFLINE_TOOLS_CONVERT2OZZ_H_
