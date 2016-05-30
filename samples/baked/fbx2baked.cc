//----------------------------------------------------------------------------//
//                                                                            //
// ozz-animation is hosted at http://github.com/guillaumeblanc/ozz-animation  //
// and distributed under the MIT License (MIT).                               //
//                                                                            //
// Copyright (c) 2015 Guillaume Blanc                                         //
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

#include "ozz/animation/offline/raw_skeleton.h"

#include "ozz/animation/offline/fbx/fbx.h"
#include "ozz/animation/offline/fbx/fbx_base.h"

#include "ozz/base/log.h"

using namespace ozz::animation::offline;
using namespace ozz::animation::offline::fbx;

namespace {
  bool RecurseNode(FbxNode* _node,
                   FbxSystemConverter* _converter,
                   RawSkeleton* _skeleton,
                   RawSkeleton::Joint* _parent) {
    bool skeleton_found = false;
    RawSkeleton::Joint* this_joint = NULL;

    bool process_node = false;

    // Push this node if it's below a skeleton root (aka has a parent).
    process_node |= _parent != NULL;

    // Push this node as a new joint if it has a joint compatible attribute.
    FbxNodeAttribute* node_attribute = _node->GetNodeAttribute();
    process_node |=
      node_attribute &&
      node_attribute->GetAttributeType() == FbxNodeAttribute::eMesh;

    // Process node if required.
    if (process_node) {

      skeleton_found = true;

      RawSkeleton::Joint::Children* sibling = NULL;
      if (_parent) {
        sibling = &_parent->children;
      }
      else {
        sibling = &_skeleton->roots;
      }

      // Adds a new child.
      sibling->resize(sibling->size() + 1);
      this_joint = &sibling->back();  // Will not be resized inside recursion.
      this_joint->name = _node->GetName();

      // Outputs hierarchy on verbose stream.
      ozz::log::LogV() << this_joint->name.c_str() << std::endl;

      // Extract bind pose.
      this_joint->transform =
        _converter->ConvertTransform(_parent ? _node->EvaluateLocalTransform() :
          _node->EvaluateGlobalTransform());
    }

    // Iterate node's children.
    for (int i = 0; i < _node->GetChildCount(); i++) {
      FbxNode* child = _node->GetChild(i);
      skeleton_found |= RecurseNode(child, _converter, _skeleton, this_joint);
    }

    return skeleton_found;
  }

  bool ExtractSkeleton(FbxSceneLoader& _loader, RawSkeleton* _skeleton) {
    if (!RecurseNode(_loader.scene()->GetRootNode(), _loader.converter(), _skeleton, NULL)) {
      ozz::log::Err() << "No skeleton found in Fbx scene." << std::endl;
      return false;
    }
    return true;
  }
}

class FbxBakedSkeletonConverter :
  public SkeletonConverter {
private:
  // Implement SkeletonConverter::Import function.
  virtual bool Import(const char* _filename,
                      RawSkeleton* _skeleton) {

    if (!_skeleton) {
      return false;
    }
    // Reset skeleton.
    *_skeleton = RawSkeleton();

    // Import Fbx content.
    FbxManagerInstance fbx_manager;
    FbxSkeletonIOSettings settings(fbx_manager);
    FbxSceneLoader scene_loader(_filename, "", fbx_manager, settings);
    if (!scene_loader.scene()) {
      ozz::log::Err() << "Failed to import file " << _filename << "." <<
        std::endl;
      return false;
    }

    if (!ExtractSkeleton(scene_loader, _skeleton)) {
      ozz::log::Err() << "Fbx skeleton extraction failed." << std::endl;
      return false;
    }

    return true;
  }
};

int main(int _argc, const char** _argv) {
  FbxBakedSkeletonConverter converter;
  return converter(_argc, _argv);
}
