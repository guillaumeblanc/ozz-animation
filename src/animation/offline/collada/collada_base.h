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

#ifndef OZZ_ANIMATION_OFFLINE_COLLADA_COLLADA_BASE_H_
#define OZZ_ANIMATION_OFFLINE_COLLADA_COLLADA_BASE_H_

#ifndef OZZ_INCLUDE_PRIVATE_HEADER
#error "This header is private, it cannot be included from public headers."
#endif  // OZZ_INCLUDE_PRIVATE_HEADER

#include "tinyxml.h"

#include "ozz/base/maths/vec_float.h"
#include "ozz/base/maths/simd_math.h"
#include "ozz/base/maths/quaternion.h"

namespace ozz {
namespace math { struct Transform; }
namespace animation {
namespace offline {
namespace collada {

namespace internal {
// Implements a string comparator that can be used by std algorithm like maps.
struct str_less {
  bool operator()(const char* const& _left, const char* const& _right) const;
};
}  // internal

// Extract Collada <asset> element.
// Provides function to convert Collada transforms to ozz Y_UP/meter
// coordinate system.
class ColladaAsset {
 public:
  // Specifies which axis is considered upward.
  enum UpAxis { kXUp, kYUp, kZUp };

  // Initialize to default Collada values.
  ColladaAsset();

  // Sets which axis is considered upward.
  void SetUpAxis(UpAxis _up_axis);

  // Gets which axis is considered upward.
  UpAxis up_axis() const {
    return up_axis_;
  }

  // Sets the unit of distance for COLLADA elements and objects.
  void SetUnit(float _unit);

  // Gets the unit of distance for COLLADA elements and objects.
  float unit() const {
    return unit_;
  }

  // Converts a matrix from Collada document coordinate system base and unit to
  // ozz right handed (y_up) metric base.
  // If _root is false, then only unit conversion is performed. Base conversion
  // should only be done once (to the hierarchy root) indeed.
  math::Float4x4 ConvertMatrix(const math::Float4x4& _m) const;

  // Converts a point from Collada document coordinate system base and unit to
  // ozz right handed (y_up) metric base.
  math::Float3 ConvertPoint(const math::Float3& _t) const;

  // Converts a quaternion from Collada document coordinate system base and unit
  // to ozz right handed (y_up) metric base.
  math::Quaternion ConvertRotation(const math::Quaternion& _q) const;

  // Converts a scale from Collada document coordinate system base and unit to
  // ozz right handed (y_up) metric base.
  // If _root is false, then only unit conversion is performed. Base conversion
  // should only be done once (to the hierarchy root) indeed.
  math::Float3 ConvertScale(const math::Float3& _s) const;

  // Converts an affine transform from Collada document coordinate system base
  // and unit to ozz right handed (y_up) metric base.
  // If _root is false, then only unit conversion is performed. Base conversion
  // should only be done once (to the hierarchy root) indeed.
  math::Transform ConvertTransform(const math::Transform& _t) const;

 private:
  // Internally rebuild corrdinate system conversion helpers.
  void RebuildConversionHelpers();

  // The vector used to convert Collada unit to ozz metric system.
  math::SimdFloat4 convert_unit_;

  // The matrix used to convert from up_axis_ to ozz y_up coordinate system
  // base.
  math::Float4x4 convert_matrix_;

  // Specifies which axis is considered upward.
  UpAxis up_axis_;

  // Defines unit of distance for COLLADA elements and objects.
  float unit_;
};

// Collada xml base document traverser.
class BaseVisitor : public TiXmlVisitor {
 public:
  BaseVisitor();

  const ColladaAsset& asset() const {
    return asset_;
  }

 protected:
  virtual bool VisitEnter(const TiXmlElement& _element,
                          const TiXmlAttribute* _firstAttribute);

  virtual bool VisitExit(const TiXmlElement& _element);

  virtual bool VisitExit(const TiXmlDocument& _element);

  void set_error();

 private:
  bool HandleCollada(const TiXmlElement& _element);

  bool HandleUnit(const TiXmlElement& _element);

  bool HandleUpAxis(const TiXmlElement& _element);

  ColladaAsset asset_;

  // Used to detect an valid Collada document.
  bool valid_collada_document_;

  // Stores if an error was reported during parsing.
  bool error_;
};
}  // collada
}  // ozz
}  // offline
}  // animation
#endif  // OZZ_ANIMATION_OFFLINE_COLLADA_COLLADA_BASE_H_
