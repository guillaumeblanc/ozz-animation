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

#include "animation/offline/collada/collada_base.h"

#include <cstring>

#include "ozz/base/log.h"

#include "ozz/base/maths/vec_float.h"
#include "ozz/base/maths/simd_math.h"
#include "ozz/base/maths/transform.h"

namespace ozz {
namespace animation {
namespace offline {
namespace collada {

namespace internal {
bool str_less::operator()(const char* const& _left,
                          const char* const& _right) const {
  return strcmp(_left, _right) < 0;
}
}  // internal

ColladaAsset::ColladaAsset()
    : up_axis_(kYUp),
      unit_(1.f) {
  RebuildConversionHelpers();
}

void ColladaAsset::SetUpAxis(UpAxis _up_axis) {
  assert(_up_axis <= kZUp);
  up_axis_ = _up_axis;
  RebuildConversionHelpers();
}

void ColladaAsset::SetUnit(float _unit) {
  unit_ = _unit;
  RebuildConversionHelpers();
}

void ColladaAsset::RebuildConversionHelpers() {
  convert_unit_ = math::simd_float4::Load(unit_, unit_, unit_, 0.f);
  switch (up_axis_) {
    case kXUp: {
      convert_matrix_.cols[0] = math::simd_float4::Load(0.f, 1.f, 0.f, 0.f);
      convert_matrix_.cols[1] = math::simd_float4::Load(0.f, 0.f, -1.f, 0.f);
      convert_matrix_.cols[2] = math::simd_float4::Load(-1.f, 0.f, 0.f, 0.f);
      convert_matrix_.cols[3] = math::simd_float4::w_axis();
      break;
    }
    case kYUp: {
      convert_matrix_ = math::Float4x4::identity();
      break;
    }
    case kZUp: {
      convert_matrix_.cols[0] = math::simd_float4::Load(1.f, 0.f, 0.f, 0.f);
      convert_matrix_.cols[1] = math::simd_float4::Load(0.f, 0.f, -1.f, 0.f);
      convert_matrix_.cols[2] = math::simd_float4::Load(0.f, 1.f, 0.f, 0.f);
      convert_matrix_.cols[3] = math::simd_float4::w_axis();
      break;
    }
    default: {
      assert(false);
      break;
    }
  }
}

math::Float4x4 ColladaAsset::ConvertMatrix(const math::Float4x4& _m) const {
  math::Float4x4 ret = convert_matrix_ * _m;
  ret.cols[3] = ret.cols[3] * convert_unit_;
  return ret;
}

math::Float3 ColladaAsset::ConvertPoint(const math::Float3& _t) const {
  switch (up_axis_) {
    case kXUp: return math::Float3(-_t.y * unit_, _t.x * unit_, _t.z * unit_);
    case kYUp: return math::Float3(_t.x * unit_, _t.y * unit_, _t.z * unit_);
    case kZUp: return math::Float3(_t.x * unit_, _t.z * unit_, -_t.y * unit_);
    default: {
      assert(false);
      return _t;
    }
  }
}

math::Quaternion ColladaAsset::ConvertRotation(
  const math::Quaternion& _q) const {
  switch (up_axis_) {
    case kXUp: return math::Quaternion(-_q.y, _q.x, _q.z, _q.w);
    case kYUp: return _q;
    case kZUp: return math::Quaternion(_q.x, _q.z, -_q.y, _q.w);
    default: {
      assert(false);
      return _q;
    }
  }
}

math::Float3 ColladaAsset::ConvertScale(const math::Float3& _s) const {
  switch (up_axis_) {
    case kXUp: return math::Float3(_s.y, _s.x, _s.z);
    case kYUp: return _s;
    case kZUp: return math::Float3(_s.x, _s.z, _s.y);
    default: {
      assert(false);
      return _s;
    }
  }
}

math::Transform ColladaAsset::ConvertTransform(
  const math::Transform& _t) const {
  const math::Transform transform = {ConvertPoint(_t.translation),
                                     ConvertRotation(_t.rotation),
                                     ConvertScale(_t.scale)};
  return transform;
}

BaseVisitor::BaseVisitor()
    : valid_collada_document_(false),
      error_(0) {
}

void BaseVisitor::set_error() {
  error_ = true;
}

bool BaseVisitor::VisitEnter(const TiXmlElement& _element,
                             const TiXmlAttribute* _firstAttribute) {
  (void)_firstAttribute;

  if (std::strcmp(_element.Value(), "COLLADA") == 0) {
    return HandleCollada(_element);
  } else if (std::strcmp(_element.Value(), "asset") == 0) {
    return true;
  } else if (std::strcmp(_element.Value(), "unit") == 0) {
    return HandleUnit(_element);
  } else if (std::strcmp(_element.Value(), "up_axis") == 0) {
    return HandleUpAxis(_element);
  }

  return false;
}

bool BaseVisitor::VisitExit(const TiXmlElement& _element) {
  (void)_element;
  return !error_;
}

bool BaseVisitor::HandleCollada(const TiXmlElement& _element) {
  if (valid_collada_document_ == false) {
    valid_collada_document_ = true;  // This is a Collada file.

    // Finds Collada version.
    const char* version = _element.Attribute("version");
    if (version) {
      log::Log() << "Collada version is: " << version << std::endl;
    }
    return true;
  } else {
    log::Err() << "The XML file is not a valid Collada document." << std::endl;
    set_error();
    return false;
  }
}

bool BaseVisitor::HandleUnit(const TiXmlElement& _element) {
  const char* meter = _element.Attribute("meter");
  if (meter) {
    log::Log() << "Unit of distance is: " << meter << " meter." << std::endl;
    float unit;
    if (sscanf(meter, "%f", &unit)) {
      asset_.SetUnit(unit);
    } else {
      log::Err() << "Unable to read \"unit\" value." << std::endl;
      set_error();
      return false;
    }
  } else {
    log::Log() << "Unit of distance is: 1 meter." << std::endl;
  }
  return true;
}

bool BaseVisitor::HandleUpAxis(const TiXmlElement& _element) {
  const char* axis = _element.GetText();
  if (axis) {
    log::Log() << "Upward axis is: \"" << axis << "\"." << std::endl;
    if (std::strcmp(axis, "X_UP") == 0) {
      asset_.SetUpAxis(ColladaAsset::kXUp);
    } else if (std::strcmp(axis, "Y_UP") == 0) {
      asset_.SetUpAxis(ColladaAsset::kYUp);
    } else if (std::strcmp(axis, "Z_UP") == 0) {
      asset_.SetUpAxis(ColladaAsset::kZUp);
    } else {
      log::Err() << "Unsupported upward axis \"" << axis << "\"." << std::endl;
      set_error();
      return false;
    }
  }
  return true;
}
}  // collada
}  // ozz
}  // offline
}  // animation
