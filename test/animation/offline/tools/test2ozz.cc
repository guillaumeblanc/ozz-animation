//----------------------------------------------------------------------------//
//                                                                            //
// ozz-animation is hosted at http://github.com/guillaumeblanc/ozz-animation  //
// and distributed under the MIT License (MIT).                               //
//                                                                            //
// Copyright (c) Guillaume Blanc                                              //
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

#include "ozz/animation/offline/tools/import2ozz.h"
#include "ozz/animation/runtime/skeleton.h"
#include "ozz/base/io/stream.h"
#include "ozz/base/memory/unique_ptr.h"

class TestConverter : public ozz::animation::offline::OzzImporter {
 public:
  TestConverter() {}
  ~TestConverter() {}

 private:
  virtual bool Load(const char* _filename) {
    file_ = ozz::make_unique<ozz::io::File>(_filename, "rb");
    if (!file_->opened()) {
      file_.reset(nullptr);
      return false;
    }

    const char good_content[] = "good content";
    char buffer[256];
    bool valid =
        file_->Read(buffer, sizeof(buffer)) >= sizeof(good_content) - 1 &&
        memcmp(buffer, good_content, sizeof(good_content) - 1) == 0;
    file_->Seek(0, ozz::io::File::kSet);
    return valid;
  }

  virtual bool Import(ozz::animation::offline::RawSkeleton* _skeleton,
                      const NodeType& _types) {
    (void)_types;
    if (file_ && file_->opened()) {
      char buffer[256];

      {
        file_->Seek(0, ozz::io::File::kSet);
        const char content[] = "good content 1";
        if (file_->Read(buffer, sizeof(buffer)) >= sizeof(content) - 1 &&
            memcmp(buffer, content, sizeof(content) - 1) == 0) {
          _skeleton->roots.resize(1);
          ozz::animation::offline::RawSkeleton::Joint& root =
              _skeleton->roots[0];
          root.name = "root";

          root.children.resize(3);
          ozz::animation::offline::RawSkeleton::Joint& joint0 =
              root.children[0];
          joint0.name = "joint0";
          ozz::animation::offline::RawSkeleton::Joint& joint1 =
              root.children[1];
          joint1.name = "joint1";
          ozz::animation::offline::RawSkeleton::Joint& joint2 =
              root.children[2];
          joint2.name = "joint2";

          return true;
        }
      }
      {
        file_->Seek(0, ozz::io::File::kSet);
        const char content[] = "good content renamed";
        if (file_->Read(buffer, sizeof(buffer)) >= sizeof(content) - 1 &&
            memcmp(buffer, content, sizeof(content) - 1) == 0) {
          _skeleton->roots.resize(1);
          ozz::animation::offline::RawSkeleton::Joint& root =
              _skeleton->roots[0];
          root.name = "root";

          return true;
        }
      }
      {
        file_->Seek(0, ozz::io::File::kSet);
        const char content[] =
            "good content but not unique joint names";
        if (file_->Read(buffer, sizeof(buffer)) >= sizeof(content) - 1 &&
            memcmp(buffer, content, sizeof(content) - 1) == 0) {
          _skeleton->roots.resize(1);
          ozz::animation::offline::RawSkeleton::Joint& root =
              _skeleton->roots[0];
          root.name = "jointx";

          root.children.resize(3);
          ozz::animation::offline::RawSkeleton::Joint& joint0 =
              root.children[0];
          joint0.name = "joint0";
          ozz::animation::offline::RawSkeleton::Joint& joint1 =
              root.children[1];
          joint1.name = "joint1";
          ozz::animation::offline::RawSkeleton::Joint& joint2 =
              root.children[2];
          joint2.name = "jointx";

          return true;
        }
      }
    }
    return false;
  }

  virtual AnimationNames GetAnimationNames() {
    AnimationNames names;

    if (file_ && file_->opened()) {
      char buffer[256];

      {
        file_->Seek(0, ozz::io::File::kSet);
        const char content[] = "good content 0";
        if (file_->Read(buffer, sizeof(buffer)) >= sizeof(content) - 1 &&
            memcmp(buffer, content, sizeof(content) - 1) == 0) {
          return names;  // No animations
        }
      }

      {
        file_->Seek(0, ozz::io::File::kSet);
        const char content[] = "good content 1";
        if (file_->Read(buffer, sizeof(buffer)) >= sizeof(content) - 1 &&
            memcmp(buffer, content, sizeof(content) - 1) == 0) {
          names.push_back("one");
          return names;
        }
      }

      {
        file_->Seek(0, ozz::io::File::kSet);
        const char content[] = "good content renamed";
        if (file_->Read(buffer, sizeof(buffer)) >= sizeof(content) - 1 &&
            memcmp(buffer, content, sizeof(content) - 1) == 0) {
          names.push_back("renamed?");
          return names;
        }
      }

      // Handles more than one animation per file.
      {
        file_->Seek(0, ozz::io::File::kSet);
        const char content[] = "good content 2";
        if (file_->Read(buffer, sizeof(buffer)) >= sizeof(content) - 1 &&
            memcmp(buffer, content, sizeof(content) - 1) == 0) {
          names.push_back("one");
          names.push_back("TWO");
          return names;
        }
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
      {  // Handles a single animation per file.
        file_->Seek(0, ozz::io::File::kSet);
        const char content[] = "good content 1";
        if (file_->Read(buffer, sizeof(buffer)) >= sizeof(content) - 1 &&
            memcmp(buffer, content, sizeof(content) - 1) == 0) {
          if (strcmp(_animation_name, "one") != 0) {
            return false;
          }
          _animation->tracks.resize(_skeleton.num_joints());
          return true;
        }
      }
      {  // Handles a single animation per file that needs renaming.
        file_->Seek(0, ozz::io::File::kSet);
        const char content[] = "good content renamed";
        if (file_->Read(buffer, sizeof(buffer)) >= sizeof(content) - 1 &&
            memcmp(buffer, content, sizeof(content) - 1) == 0) {
          if (strcmp(_animation_name, "renamed?") != 0) {
            return false;
          }
          _animation->tracks.resize(_skeleton.num_joints());
          return true;
        }
      }
      {  // Handles more than one animation per file.
        file_->Seek(0, ozz::io::File::kSet);
        const char content[] = "good content 2";
        if (file_->Read(buffer, sizeof(buffer)) >= sizeof(content) - 1 &&
            memcmp(buffer, content, sizeof(content) - 1) == 0) {
          if (strcmp(_animation_name, "one") != 0 &&
              strcmp(_animation_name, "TWO") != 0) {
            return false;
          }
          _animation->tracks.resize(_skeleton.num_joints());
          return true;
        }
      }
    }
    return false;
  }

  virtual NodeProperties GetNodeProperties(const char* _node_name) {
    NodeProperties ppts;
    bool found =
        strcmp(_node_name, "joint0") == 0 || strcmp(_node_name, "joint1") == 0;

    if (found) {
      const NodeProperty ppt0 = {"property0", NodeProperty::kFloat1};
      ppts.push_back(ppt0);
      const NodeProperty ppt1 = {"property1", NodeProperty::kFloat1};
      ppts.push_back(ppt1);
      const NodeProperty ppt2 = {"property2", NodeProperty::kFloat2};
      ppts.push_back(ppt2);
      const NodeProperty ppt3 = {"property3", NodeProperty::kFloat3};
      ppts.push_back(ppt3);
    }
    return ppts;
  }

  virtual bool Import(const char* _animation_name, const char* _node_name,
                      const char* _track_name, NodeProperty::Type _track_type,
                      float _sampling_rate,
                      ozz::animation::offline::RawFloatTrack* _track) {
    (void)_animation_name;
    (void)_track_type;
    (void)_sampling_rate;
    (void)_track;

    // joint2 doesn't have the property
    bool found = (strcmp(_node_name, "joint0") == 0 ||
                  strcmp(_node_name, "joint1") == 0) &&
                 (strcmp(_track_name, "property0") == 0 ||
                  strcmp(_track_name, "property1") == 0);
    return found;
  }

  virtual bool Import(const char* _animation_name, const char* _node_name,
                      const char* _track_name, NodeProperty::Type _track_type,
                      float _sampling_rate,
                      ozz::animation::offline::RawFloat2Track* _track) {
    (void)_animation_name;
    (void)_track_type;
    (void)_sampling_rate;
    (void)_track;

    // joint2 doesn't have the property
    bool found = (strcmp(_node_name, "joint0") == 0 ||
                  strcmp(_node_name, "joint1") == 0) &&
                 strcmp(_track_name, "property2") == 0;
    return found;
  }

  virtual bool Import(const char* _animation_name, const char* _node_name,
                      const char* _track_name, NodeProperty::Type _track_type,
                      float _sampling_rate,
                      ozz::animation::offline::RawFloat3Track* _track) {
    (void)_animation_name;
    (void)_track_type;
    (void)_sampling_rate;
    (void)_track;

    // joint2 doesn't have the property
    bool found = (strcmp(_node_name, "joint0") == 0 ||
                  strcmp(_node_name, "joint1") == 0) &&
                 strcmp(_track_name, "property3") == 0;
    return found;
  }

  virtual bool Import(const char* _animation_name, const char* _node_name,
                      const char* _track_name, NodeProperty::Type _track_type,
                      float _sampling_rate,
                      ozz::animation::offline::RawFloat4Track* _track) {
    (void)_animation_name;
    (void)_track_type;
    (void)_sampling_rate;
    (void)_track;

    // joint2 doesn't have the property
    bool found = (strcmp(_node_name, "joint0") == 0 ||
                  strcmp(_node_name, "joint1") == 0) &&
                 strcmp(_track_name, "property4") == 0;
    return found;
  }

  ozz::unique_ptr<ozz::io::File> file_;
};

int main(int _argc, const char** _argv) {
  TestConverter converter;
  return converter(_argc, _argv);
}
