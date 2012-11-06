//============================================================================//
// Copyright (c) <2012> <Guillaume Blanc>                                     //
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
//============================================================================//

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
// Implements a string comparator that can be used by std algorythm like maps.
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

  void set_error() {
    error_ = true;
  }

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
