//============================================================================//
//                                                                            //
// ozz-animation, 3d skeletal animation libraries and tools.                  //
// https://code.google.com/p/ozz-animation/                                   //
//                                                                            //
//----------------------------------------------------------------------------//
//                                                                            //
// Copyright (c) 2012-2015 Guillaume Blanc                                    //
//                                                                            //
// This software is provided 'as-is', without any express or implied          //
// warranty. In no event will the authors be held liable for any damages      //
// arising from the use of this software.                                     //
//                                                                            //
// Permission is granted to anyone to use this software for any purpose,      //
// including commercial applications, and to alter it and redistribute it     //
// freely, subject to the following restrictions:                             //
//                                                                            //
// 1. The origin of this software must not be misrepresented; you must not    //
// claim that you wrote the original software. If you use this software       //
// in a product, an acknowledgment in the product documentation would be      //
// appreciated but is not required.                                           //
//                                                                            //
// 2. Altered source versions must be plainly marked as such, and must not be //
// misrepresented as being the original software.                             //
//                                                                            //
// 3. This notice may not be removed or altered from any source               //
// distribution.                                                              //
//                                                                            //
//============================================================================//

#define OZZ_INCLUDE_PRIVATE_HEADER  // Allows to include private headers.

#include "animation/offline/fbx/fbx_skeleton.h"

#include "ozz/animation/offline/raw_skeleton.h"

#include "ozz/base/log.h"

namespace ozz {
namespace animation {
namespace offline {
namespace fbx {

namespace {
bool RecurseNode(FbxNode* _node,
                 FbxSystemConverter* _converter,
                 RawSkeleton* _skeleton,
                 RawSkeleton::Joint*
                 _parent,
                 int _depth) {
  bool skeleton_found = false;
  RawSkeleton::Joint* this_joint = NULL;

  bool process_node = false;

  // Push this node if it's below a skeleton root (aka has a parent).
  process_node |= _parent != NULL;

  // Push this node as a new joint if it has a joint compatible attribute.
  FbxNodeAttribute* node_attribute = _node->GetNodeAttribute();
  process_node |=
    node_attribute &&
    node_attribute->GetAttributeType() == FbxNodeAttribute::eSkeleton;

  // Process node if required.
  if (process_node) {

    skeleton_found = true;

    RawSkeleton::Joint::Children* sibling = NULL;
    if (_parent) {
      sibling = &_parent->children;
    } else {
      sibling = &_skeleton->roots;
    }

    // Adds a new child.
    sibling->resize(sibling->size() + 1);
    this_joint = &sibling->back();  // Will not be resized inside recursion.
    this_joint->name = _node->GetName();

    // Outputs hierarchy on verbose stream.
    for (int i = 0; i < _depth; ++i) { ozz::log::LogV() << '.'; }
    ozz::log::LogV() << this_joint->name.c_str() << std::endl;

    // Extract bind pose.
    this_joint->transform =
      _converter->ConvertTransform(_parent?_node->EvaluateLocalTransform():
                                           _node->EvaluateGlobalTransform());

    // One level deeper in the hierarchy.
    _depth++;
  } else if (_parent) {
    // Ends recursion if this is not a joint, and part of a skeleton
    // hierarchy.
    return false;
  }

  // Iterate node's children.
  for (int i = 0; i < _node->GetChildCount(); i++) {
    FbxNode* child = _node->GetChild(i);
    skeleton_found |= RecurseNode(child, _converter, _skeleton, this_joint, _depth);
  }
  return skeleton_found;
}
}

bool ExtractSkeleton(FbxSceneLoader& _loader, RawSkeleton* _skeleton) {
  if (!RecurseNode(_loader.scene()->GetRootNode(), _loader.converter(), _skeleton, NULL, 0)) {
    ozz::log::Err() << "No skeleton found in Fbx scene." << std::endl;
    return false;
  }
  return true;
}
}  // fbx
}  // ozz
}  // offline
}  // animation
