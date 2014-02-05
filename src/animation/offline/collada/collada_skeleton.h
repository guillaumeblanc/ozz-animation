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

#ifndef OZZ_ANIMATION_OFFLINE_COLLADA_COLLADA_SKELETON_H_
#define OZZ_ANIMATION_OFFLINE_COLLADA_COLLADA_SKELETON_H_

#ifndef OZZ_INCLUDE_PRIVATE_HEADER
#error "This header is private, it cannot be included from public headers."
#endif  // OZZ_INCLUDE_PRIVATE_HEADER

#include "animation/offline/collada/collada_base.h"
#include "animation/offline/collada/collada_transform.h"

#include "ozz/base/containers/string.h"
#include "ozz/base/containers/map.h"
#include "ozz/base/containers/vector.h"
#include "ozz/animation/offline/skeleton_builder.h"

namespace ozz {
namespace animation {
namespace offline {
namespace collada {

class SkeletonVisitor;

bool ExtractSkeleton(const SkeletonVisitor& _skeleton_visitor,
                     RawSkeleton* _skeleton);

// Defines a collada joint.
struct ColladaJoint {
  // Converts collada transformation stack to a math::Transform.
  bool GetTransform(math::Transform* _transform) const;

  // Joint's name. Cannot be a c string as the name can be built.
  ozz::String name;

  // Joint's id.
  const char* id;

  // Joint's children.
  ozz::Vector<ColladaJoint>::Std children;

  // Joint's tranforms that must be applied in-order.
  typedef ozz::Vector<NodeTransform>::Std Transforms;
  Transforms transforms;
};

// Collada xml document traverser, aiming to build skeletons.
class SkeletonVisitor : public BaseVisitor {
 public:
  SkeletonVisitor() {
  }

  typedef ozz::Vector<ColladaJoint>::Std Roots;

  // Get collected joints hierarchies.
  const Roots& roots() const {
    return roots_;
  }

 private:
  virtual bool VisitEnter(const TiXmlElement& _element,
                          const TiXmlAttribute* _firstAttribute);

  virtual bool VisitExit(const TiXmlElement& _element);

  virtual bool Visit(const TiXmlText& _text);

  bool HandleNodeEnter(const TiXmlElement& _element);

  bool HandleNodeExit(const TiXmlElement& _element);

  bool HandleTransform(const TiXmlElement& _element);

  // Collected joints hierarchies.
  Roots roots_;

  // Stack of parent joint in the xml nodes recursion.
  // Note that parent_joint_ can point to an element in a vector (without
  // fearing vector reallocation issue) because the parent's sibling vector
  // can not be modified while recursing its children.
  ozz::Vector<ColladaJoint*>::Std joint_stack_;
};
}  // collada
}  // ozz
}  // offline
}  // animation
#endif  // OZZ_ANIMATION_OFFLINE_COLLADA_COLLADA_SKELETON_H_
