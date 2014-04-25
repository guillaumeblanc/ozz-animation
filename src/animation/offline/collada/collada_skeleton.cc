//============================================================================//
//                                                                            //
// ozz-animation, 3d skeletal animation libraries and tools.                  //
// https://code.google.com/p/ozz-animation/                                   //
//                                                                            //
//----------------------------------------------------------------------------//
//                                                                            //
// Copyright (c) 2012-2014 Guillaume Blanc                                    //
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

#include "animation/offline/collada/collada_skeleton.h"

#include <cassert>
#include <cstring>
#include <sstream>

#include "ozz/base/containers/set.h"

#include "ozz/base/log.h"

namespace ozz {
namespace animation {
namespace offline {
namespace collada {

bool ColladaJoint::GetTransform(math::Transform* _transform) const {
  TransformBuilder builder;
  for (std::size_t i = 0; i < transforms.size(); ++i) {
    if (!transforms[i].Build(&builder)) {
      return false;
    }
  }
  return builder.GetAsTransform(_transform);
}

bool SkeletonVisitor::VisitEnter(const TiXmlElement& _element,
                                 const TiXmlAttribute* _firstAttribute) {
  if (std::strcmp(_element.Value(), "library_visual_scenes") == 0) {
    return true;
  } else if (std::strcmp(_element.Value(), "visual_scene") == 0) {
    return true;
  } else if (std::strcmp(_element.Value(), "node") == 0) {
    return HandleNodeEnter(_element);
  }

  // Detects a transformation.
  if (!joint_stack_.empty()) {
    const char* transforms[] = {
      "matrix", "rotate", "scale", "translate", "lookat", "skew"};
    for (std::size_t i = 0; i < OZZ_ARRAY_SIZE(transforms); ++i) {
      if (std::strcmp(_element.Value(), transforms[i]) == 0) {
        if (HandleTransform(_element)) {
          return true;
        } else {
          return false;
        }
      }
    }
  }

  return BaseVisitor::VisitEnter(_element, _firstAttribute);
}

bool SkeletonVisitor::VisitExit(const TiXmlElement& _element) {
  if (std::strcmp(_element.Value(), "node") == 0) {
    return HandleNodeExit(_element);
  }
  return BaseVisitor::VisitExit(_element);
}

bool SkeletonVisitor::Visit(const TiXmlText& _text) {
  return BaseVisitor::Visit(_text);
}

bool SkeletonVisitor::HandleNodeEnter(const TiXmlElement& _element) {
  // Creates a skeleton if this is the first joint, otherwise pushes this node
  // as a child of the last joint of the stack.
  const char* type = _element.Attribute("type");
  if (!joint_stack_.empty() || (type && std::strcmp(type, "JOINT") == 0)) {
    ColladaJoint* joint = NULL;
    if (joint_stack_.empty()) {
      // Have found a new root.
      roots_.resize(roots_.size() + 1);
      joint = &roots_.back();
    } else {
      ColladaJoint* parent = joint_stack_.back();
      parent->children.resize(parent->children.size() + 1);
      joint = &parent->children.back();
    }

    // Find its name.
    const char* name = _element.Attribute("name");
    if (!name || *name == '\0') {
      // No name found for that joint This is not a definitive failure as it
      // will attempt to work with an automatic name.
      if (!joint_stack_.empty()) {
        std::stringstream str;
        str << joint_stack_.back()->name << "_child_" <<
          (joint_stack_.back()->children.size() - 1);
        joint->name = str.str().c_str();
      } else {
        joint->name = "root";
      }
      log::Err() << "Unnamed joint found. Assigning it \"" <<
        joint->name << "\" automatically." << std::endl;
    } else {
      joint->name = name;
    }

    // Find its id.
    const char* id = _element.Attribute("id");
    if (!id) {
      // No id could mean that this node is not referenced,. This isn't fatal.
      id = "";
    }
    joint->id = id;

    // This joint is the next parent in the recursion.
    joint_stack_.push_back(joint);
  }

  return true;
}

namespace {

typedef ozz::Set<const char*, internal::str_less>::Std JointNames;

bool MakeUniqueNames(ColladaJoint& _src, JointNames* _joints) {
  if (_joints->count(_src.name.c_str())) {  // The name isn't unique.
    for (int i = 0;; ++i) {
      std::stringstream unique;
      unique << _src.name.c_str() << '~' << i;
      if (!_joints->count(unique.str().c_str())) {  // Now it's unique.
        _src.name = unique.str().c_str();
        break;
      }
    }
  }
  _joints->insert(_src.name.c_str());

  // Now maps children.
  for (std::size_t i = 0; i < _src.children.size(); ++i) {
    if (!MakeUniqueNames(_src.children[i], _joints)) {
      return false;
    }
  }
  return true;
}
}  // namespace

bool SkeletonVisitor::HandleNodeExit(const TiXmlElement& _element) {
  if (!joint_stack_.empty()) {
    joint_stack_.pop_back();

    // The last joint was poped, makes joint names unique.
    if (joint_stack_.empty()) {
      JointNames joint_names;
      for (std::size_t i = 0; i < roots_.size(); ++i) {
        MakeUniqueNames(roots_[i], &joint_names);
      }
    }
  }
  return BaseVisitor::VisitExit(_element);
}

bool SkeletonVisitor::HandleTransform(const TiXmlElement& _element) {
  // Build the tranform and load it.
  ColladaJoint* joint = joint_stack_.back();
  NodeTransform transform;
  if (!transform.PushElement(_element)) {
    log::Err() << "Failed to load transformation of node \"" << joint->name <<
      "\"" << std::endl;
    set_error();
    return false;
  }

  // Ensure sid is unique.
  for (std::size_t i = 0; i < joint->transforms.size(); ++i) {
    if (!std::strcmp(joint->transforms[i].sid(), transform.sid())) {
      log::Err() << "Multiple tranforms with the same <sid> \"" <<*
        transform.sid() << "\" for node \"" << joint->name << "\"." <<
        std::endl;
      set_error();
      return false;
    }
  }

  // Adds transform now everything is valid.
  joint->transforms.push_back(transform);

  return true;
}
namespace {
bool CopyHierachy(const ColladaJoint& _src,
                  RawSkeleton::Joint* _dest,
                  const ColladaAsset& _asset) {
  _dest->name = _src.name;

  // Get transform and applies asset properties.
  math::Transform node_transform;
  if (!_src.GetTransform(&node_transform)) {
    return false;
  }
  _dest->transform = _asset.ConvertTransform(node_transform);

  // Adds and fills children.
  _dest->children.resize(_src.children.size());
  for (std::size_t i = 0; i < _src.children.size(); ++i) {
    if (!CopyHierachy(_src.children[i], &_dest->children[i], _asset)) {
      return false;
    }
  }
  return true;
}
}  // namespace

bool ExtractSkeleton(const SkeletonVisitor& _skeleton_visitor,
                     RawSkeleton* _skeleton) {
  _skeleton->roots.resize(_skeleton_visitor.roots().size());
  for (std::size_t i = 0; i < _skeleton_visitor.roots().size(); ++i) {
    if (!CopyHierachy(_skeleton_visitor.roots()[i],
                      &_skeleton->roots[i],
                      _skeleton_visitor.asset())) {
      log::Err() << "Skeleton import failed." << std::endl;
      return false;
    }
  }
  return true;
}
}  // collada
}  // ozz
}  // offline
}  // animation
