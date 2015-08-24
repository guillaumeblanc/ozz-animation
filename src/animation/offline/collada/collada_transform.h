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

#ifndef OZZ_ANIMATION_OFFLINE_COLLADA_COLLADA_TRANSFORM_H_
#define OZZ_ANIMATION_OFFLINE_COLLADA_COLLADA_TRANSFORM_H_

#ifndef OZZ_INCLUDE_PRIVATE_HEADER
#error "This header is private, it cannot be included from public headers."
#endif  // OZZ_INCLUDE_PRIVATE_HEADER

#include "tinyxml.h"

#include "ozz/base/maths/simd_math.h"
#include "ozz/base/maths/quaternion.h"
#include "ozz/base/maths/transform.h"

namespace ozz {
namespace animation {
namespace offline {
namespace collada {

class TransformBuilder;

class NodeTransform {
 public:
  // Construct a transform of unkown type.
  NodeTransform();

  // REturns transform sid.
  const char* sid() const { return sid_; }

  // Initialize from a Collada node element.
  bool PushElement(const TiXmlElement& _element);

  // Initialize from a Collada animated element.
  // _member argument specifies the sid structure memeber selection. It can be
  // NULL or empty if all members are targeted.
  bool PushAnimation(const char* _member,
                     const float* _values, size_t num_values);

  // Fill _builder with curren transform data.
  bool Build(TransformBuilder* _builder) const;

 private:
  // Declares transformation type enumeration.
  enum Type {
    kMatrix,
    kRotate,
    kScale,
    kTranslate,
    kUnknow,
  };

  // Transform sid.
  const char* sid_;

  // Transformation type.
  Type type_;

  // Trasformation values. According to the type, all the values might not be
  // used.
  float values_[16];
};

// Extract Collada <node> transformation.
// Builds the transformation stack according to the pushed unit transformation.
// Tries to maintain as much as posible splitted (T.R.S) transformations.
// Basically keeps transformation splited untill a matrix is pushed, and while
// the translation-rotation-scale pushing order is broken.
class TransformBuilder {
 public:
  // Initialization to a default/empty state.
  TransformBuilder() {
    // Uses the node initialization function to generate a deefaulte state.
    NodeInitialize();
  }

  // Notify new node entry.
  void NodeInitialize();

  // Push a matrix to the transformation stack.
  bool PushMatrix(const math::Float4x4& _m);

  // Push a translation to the transformation stack.
  bool PushTranslation(const math::Float3& _v);

  // Push a rotation to the transformation stack.
  // Uses an axis angle representation in radian.
  bool PushRotation(const math::Float4& _axis_angle);

  // Push a scale to the transformation stack.
  bool PushScale(const math::Float3& _v);

  // Push a skew to the transformation stack.
  bool PushSkew(const math::Float3& _v);

  // Push a look-at to the transformation stack.
  bool PushLookAt(const math::Float3& _v);

  // Gets the current stack as a matrix.
  bool GetAsMatrix(math::Float4x4* _matrix) const;

  // Gets the current stack as a transform.
  bool GetAsTransform(math::Transform* transform) const;

 private:
  // Convert current stack to kMatrix state.
  void ToMatrixState();

  // Current stack state.
  enum StackState {
    kNone,
    kTranslation,
    kRotation,
    kScale,
    kMatrix
  };
  StackState state_;

  // Matrix on top of the transformation stack.
  math::Float4x4 matrix_;

  // Translation on top of the transformation stack.
  math::Float3 translation_;

  // Rotation on top of the transformation stack.
  math::Quaternion rotation_;

  // Scale on top of the transformation stack.
  math::Float3 scale_;
};
}  // collada
}  // ozz
}  // offline
}  // animation
#endif  // OZZ_ANIMATION_OFFLINE_COLLADA_COLLADA_TRANSFORM_H_
