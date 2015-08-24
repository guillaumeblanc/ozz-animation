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

namespace ozz {
namespace animation {
namespace offline {

struct RawSkeleton;

namespace collada {

class SkeletonVisitor;

bool ExtractSkeleton(const SkeletonVisitor& _skeleton_visitor,
                     RawSkeleton* _skeleton);

// Defines a collada joint.
struct ColladaJoint {
  // Converts collada transformation stack to a math::Transform.
  bool GetTransform(math::Transform* _transform) const;

  // Joint's name. Cannot be a c string as the name can be built.
  ozz::String::Std name;

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
