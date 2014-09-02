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

#include "animation/offline/collada/collada_transform.h"

#include <cassert>
#include <cstring>

#include "ozz/base/log.h"

namespace ozz {
namespace animation {
namespace offline {
namespace collada {

NodeTransform::NodeTransform() :
  sid_(""),
  type_(kUnknow) {
}

bool NodeTransform::PushElement(const TiXmlElement& _element) {
  if (std::strcmp(_element.Value(), "matrix") == 0) {
    type_ = kMatrix;
    // Collada matrices need to be transposed.
    int scan_ret = sscanf(_element.GetText(),
      "%f %f %f %f %f %f %f %f %f %f %f %f %f %f %f %f",
      values_ + 0, values_ + 1, values_ + 2, values_ + 3,
      values_ + 4, values_ + 5, values_ + 6, values_ + 7,
      values_ + 8, values_ + 9, values_ + 10, values_ + 11,
      values_ + 12, values_ + 13, values_ + 14, values_ + 15);
    if (scan_ret != 16) {
      log::Err() << "Failed to load bind pose matrix." << std::endl;
      return false;
    }
  } else if (std::strcmp(_element.Value(), "rotate") == 0) {
    type_ = kRotate;
    int scan_ret = sscanf(_element.GetText(),
      "%f %f %f %f", values_ + 0, values_ + 1, values_ + 2, values_ + 3);
    if (scan_ret != 4) {
      log::Err() << "Failed to load bind pose rotation." << std::endl;
      return false;
    }
  } else if (std::strcmp(_element.Value(), "scale") == 0) {
    type_ = kScale;
    int scan_ret = sscanf(_element.GetText(),
      "%f %f %f", values_ + 0, values_ + 1, values_ + 2);
    if (scan_ret != 3) {
      log::Err() << "Failed to load bind pose scale." << std::endl;
      return false;
    }
  } else if (std::strcmp(_element.Value(), "translate") == 0) {
    type_ = kTranslate;
    int scan_ret = sscanf(_element.GetText(),
      "%f %f %f", values_ + 0, values_ + 1, values_ + 2);
    if (scan_ret != 3) {
      log::Err() << "Failed to load bind pose translation." << std::endl;
      return false;
    }
  } else {
    log::Err() << "Unsupported \"" << _element.Value() << "\" transformation." <<
      std::endl;
    return false;
  }

  // Finds transform sid.
  sid_ = _element.Attribute("sid");
  if (!sid_) {
    // No id could mean that this node is not referenced, which isn't fatal.
    log::LogV() << "Failed to find transform sid" << std::endl;
  }

  return true;
}

bool NodeTransform::PushAnimation(const char* _member,
                                  const float* _values, size_t num_values) {
  if (type_ >= kUnknow) {
    log::Err() << "Unsupported transformation type" << std::endl;
    return false;
  }

  const struct {
    size_t num_values;
    int dim1;
    int dim2;
  } type_specs[] = {
    {16, 4, 4},  // kMatrix
    {4, 4, 1},  // kRotate
    {3, 3, 1},  // kScale
    {3, 3, 1}};  // kTranslate
  OZZ_STATIC_ASSERT(OZZ_ARRAY_SIZE(type_specs) == kUnknow);

  // Copy all values if no member is specified.
  if (!_member || _member[0] == '\0') {
    if (num_values != type_specs[type_].num_values) {
      log::Err() << "Invalid number of animated values for the transformation."
        << std::endl;
      return false;
    }
    for (size_t i = 0; i < num_values; ++i) {
      values_[i] = _values[i];
    }
    return true;
  }

  // Detect member specification.
  int index = -1;
  int a, b;
  if (sscanf(_member, "(%d)(%d)", &a, &b) == 2) {  // 2d array.
    if (a >= type_specs[type_].dim1 || b >= type_specs[type_].dim2) {
      log::Err() << "Invalid member selection \"" << _member <<
        "\" for the transformation." << std::endl;
      return false;
    }
    // Computes linear index from 2d array coordinates.
    index = type_specs[type_].dim1 * b + a;
  } else if (sscanf(_member, "(%d)", &index) == 1) {  // 1d array.
    if (index >= type_specs[type_].dim1 * type_specs[type_].dim2) {
      log::Err() << "Invalid member selection \"" << _member <<
        "\" for the transformation." << std::endl;
      return false;
    }
  } else {
    const struct {
      const char* semantic;
      int index;
    } member_to_index[] = {
      {".X", 0},
      {".Y", 1},
      {".Z", 2},
      {".W", 3},
      {".ANGLE", 3}};
    for (size_t i = 0; i < OZZ_ARRAY_SIZE(member_to_index); ++i) {
      if (std::strcmp(_member, member_to_index[i].semantic) == 0) {
        index = member_to_index[i].index;
        break;
      }
    }
  }
  if (index < 0 || static_cast<size_t>(index) > type_specs[type_].num_values) {
    log::Err() << "Invalid member selection \"" << _member <<
      "\" for the transformation." << std::endl;
    return false;
  }
  values_[index] = _values[0];
  return true;
}

bool NodeTransform::Build(TransformBuilder* _transform) const {
  switch (type_) {
    case kMatrix: {
      const math::Float4x4 matrix = {{math::simd_float4::LoadPtrU(values_ + 0),
                                      math::simd_float4::LoadPtrU(values_ + 4),
                                      math::simd_float4::LoadPtrU(values_ + 8),
                                      math::simd_float4::LoadPtrU(values_ + 12)}};
      return _transform->PushMatrix(Transpose(matrix));
    }
    case kRotate: {
      const math::Float4 rotate(
        values_[0], values_[1], values_[2], values_[3] * math::kPi / 180.f);
      return _transform->PushRotation(rotate);
    }
    case kScale: {
      const math::Float3 scale(values_[0], values_[1], values_[2]);
      return _transform->PushScale(scale);
    }
    case kTranslate: {
      const math::Float3 translate(values_[0], values_[1], values_[2]);
      return _transform->PushTranslation(translate);
    }
    default: {
      log::Err() << "Unsupported transformation type" << std::endl;
      return false;
    }
  }
}

void TransformBuilder::NodeInitialize() {
  state_ = kNone;
  matrix_ = math::Float4x4::identity();
  translation_ = math::Float3::zero();
  rotation_ = math::Quaternion::identity();
  scale_ = math::Float3::one();
}

bool TransformBuilder::PushMatrix(const math::Float4x4& _m) {
  if (state_ < kMatrix) {
    ToMatrixState();
  }
  matrix_ = matrix_ * _m;
  state_ = kMatrix;
  return true;
}

bool TransformBuilder::PushTranslation(const math::Float3& _v) {
  if (state_ <= kTranslation) {
    translation_ = translation_ + _v;
    state_ = kTranslation;
  } else {
    ToMatrixState();
    matrix_ = matrix_ * math::Float4x4::Translation(
      math::simd_float4::Load3PtrU(&_v.x));
  }
  return true;
}

bool TransformBuilder::PushRotation(const math::Float4& _axis_angle) {
  if (state_ <= kRotation) {
    rotation_ = rotation_ * math::Quaternion::FromAxisAngle(_axis_angle);
    state_ = kRotation;
  } else {
    ToMatrixState();
    matrix_ = matrix_ * math::Float4x4::FromAxisAngle(
      math::simd_float4::LoadPtrU(&_axis_angle.x));
  }
  return true;
}

bool TransformBuilder::PushScale(const math::Float3& _v) {
  if (state_ <= kScale) {
    scale_ = scale_ * _v;
    state_ = kScale;
  } else {
    ToMatrixState();
    matrix_ = matrix_ * math::Float4x4::Scaling(
      math::simd_float4::Load3PtrU(&_v.x));
  }
  return true;
}

bool TransformBuilder::PushSkew(const math::Float3& _v) {
  (void)_v;
  log::Err() << "Unsupported skew transformation." << std::endl;
  return false;
}

bool TransformBuilder::PushLookAt(const math::Float3& _v) {
  (void)_v;
  log::Err() << "Unsupported look-at transformation." << std::endl;
  return false;
}

void TransformBuilder::ToMatrixState() {
  if (state_ < kMatrix) {
    GetAsMatrix(&matrix_);
    state_ = kMatrix;
  }
}

bool TransformBuilder::GetAsMatrix(math::Float4x4* _matrix) const {
  if (state_ == kNone || state_ == kMatrix) {
    *_matrix = matrix_;
  } else {
    *_matrix = math::Float4x4::FromAffine(
      math::simd_float4::Load3PtrU(&translation_.x),
      math::simd_float4::LoadPtrU(&rotation_.x),
      math::simd_float4::Load3PtrU(&scale_.x));
  }
  return true;
}

bool TransformBuilder::GetAsTransform(math::Transform* _transform) const {
  if (state_ == kMatrix) {
    math::SimdFloat4 translation, rotation, scale;
    bool decomposed = ToAffine(matrix_, &translation, &rotation, &scale);
    if  (!decomposed) {
      log::Err() << "Affine matrix decomposition failed." << std::endl;
      return false;
    }
    math::Store3PtrU(translation, &_transform->translation.x);
    math::StorePtrU(rotation, &_transform->rotation.x);
    math::Store3PtrU(scale, &_transform->scale.x);
  } else {
    _transform->translation = translation_;
    _transform->rotation = rotation_;
    _transform->scale = scale_;
  }
  return true;
}
}  // collada
}  // ozz
}  // offline
}  // animation
