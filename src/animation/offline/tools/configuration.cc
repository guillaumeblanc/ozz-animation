//----------------------------------------------------------------------------//
//                                                                            //
// ozz-animation is hosted at http://github.com/guillaumeblanc/ozz-animation  //
// and distributed under the MIT License (MIT).                               //
//                                                                            //
// Copyright (c) 2017 Guillaume Blanc                                         //
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

#include "animation/offline/tools/configuration.h"

#include "ozz/animation/offline/additive_animation_builder.h"
#include "ozz/animation/offline/animation_builder.h"
#include "ozz/animation/offline/animation_optimizer.h"

#include "ozz/base/log.h"

namespace {

template <typename _Type>
struct ToJsonType;

template <>
struct ToJsonType<int> {
  static Json::ValueType type() { return Json::intValue; }
};
template <>
struct ToJsonType<unsigned int> {
  static Json::ValueType type() { return Json::uintValue; }
};
template <>
struct ToJsonType<float> {
  static Json::ValueType type() { return Json::realValue; }
};
template <>
struct ToJsonType<const char*> {
  static Json::ValueType type() { return Json::stringValue; }
};
template <>
struct ToJsonType<bool> {
  static Json::ValueType type() { return Json::booleanValue; }
};

const char* JsonTypeToString(Json::ValueType _type) {
  switch (_type) {
    case Json::nullValue:
      return "null";
    case Json::intValue:
      return "integer";
    case Json::uintValue:
      return "unsigned integer";
    case Json::realValue:
      return "float";
    case Json::stringValue:
      return "UTF-8 string";
    case Json::booleanValue:
      return "boolean";
    case Json::arrayValue:
      return "array";
    case Json::objectValue:
      return "object";
    default:
      assert(false && "unknown json type");
      return "unknown";
  }
}

bool IsCompatibleType(Json::ValueType _type, Json::ValueType _expected) {
  switch (_expected) {
    case Json::nullValue:
      return _type == Json::nullValue;
    case Json::intValue:
      return _type == Json::intValue || _type == Json::uintValue;
    case Json::uintValue:
      return _type == Json::uintValue;
    case Json::realValue:
      return _type == Json::realValue || _type == Json::intValue ||
             _type == Json::uintValue;
    case Json::stringValue:
      return _type == Json::stringValue;
    case Json::booleanValue:
      return _type == Json::booleanValue;
    case Json::arrayValue:
      return _type == Json::arrayValue;
    case Json::objectValue:
      return _type == Json::objectValue;
    default:
      assert(false && "unknown json type");
      return false;
  }
}

void MakeDefaultArray(Json::Value& _parent, const char* _name,
                      const char* _comment) {
  const Json::Value* found = _parent.find(_name, _name + strlen(_name));
  if (!found) {
    Json::Value& member = _parent[_name];
    member.resize(1);
    if (*_comment != 0) {
      member.setComment(std::string("//  ") + _comment, Json::commentBefore);
    }
    found = &member;
  }
  assert(found->isArray());
}

void MakeDefaultObject(Json::Value& _parent, const char* _name,
                       const char* _comment) {
  const Json::Value* found = _parent.find(_name, _name + strlen(_name));
  if (!found) {
    Json::Value& member = _parent[_name];
    member = Json::Value(Json::objectValue);
    if (*_comment != 0) {
      member.setComment(std::string("//  ") + _comment, Json::commentBefore);
    }
    found = &member;
  }
  assert(found->isObject());
}

template <typename _Type>
void MakeDefault(Json::Value& _parent, const char* _name, _Type _value,
                 const char* _comment) {
  const Json::Value* found = _parent.find(_name, _name + strlen(_name));
  if (!found) {
    Json::Value& member = _parent[_name];
    member = _value;
    if (*_comment != 0) {
      member.setComment(std::string("//  ") + _comment,
                        Json::commentAfterOnSameLine);
    }
    found = &member;
  }
  assert(IsCompatibleType(found->type(), ToJsonType<_Type>::type()));
}

bool SanitizeOptimizationTolerances(Json::Value& _root) {
  MakeDefault(
      _root, "translation",
      ozz::animation::offline::AnimationOptimizer().translation_tolerance,
      "Translation optimization tolerance, defined as the distance between two "
      "translation values in meters.");

  MakeDefault(_root, "rotation",
              ozz::animation::offline::AnimationOptimizer().rotation_tolerance,
              "Rotation optimization tolerance, ie: the angle between two "
              "rotation values in radian.");

  MakeDefault(_root, "scale",
              ozz::animation::offline::AnimationOptimizer().scale_tolerance,
              "Scale optimization tolerance, ie: the norm of the difference of "
              "two scales.");

  MakeDefault(
      _root, "hierarchical",
      ozz::animation::offline::AnimationOptimizer().hierarchical_tolerance,
      "Hierarchical translation optimization tolerance, ie: the maximum error "
      "(distance) that an optimization on a joint is allowed to generate on "
      "its whole child hierarchy.");

  return true;
}

bool SanitizeAnimation(Json::Value& _root) {
  MakeDefault(_root, "output", "*.ozz",
              "Specifies ozz animation output file(s). When importing multiple "
              "animations, use a \'*\' character to specify part(s) of the "
              "filename that should be replaced by the animation name.");

  MakeDefault(_root, "optimize", true, "Activates keyframes optimization.");

  MakeDefaultObject(_root, "optimization_tolerances",
                    "Optimization tolerances.");

  SanitizeOptimizationTolerances(_root["optimization_tolerances"]);

  MakeDefault(_root, "raw", false, "Outputs raw animation.");

  MakeDefault(
      _root, "additive", false,
      "Creates a delta animation that can be used for additive blending.");

  MakeDefault(
      _root, "additive", false,
      "Creates a delta animation that can be used for additive blending.");

  MakeDefault(_root, "sampling_rate", 0.f,
              "Selects animation sampling rate in hertz. Set a value = 0 to "
              "use imported scene frame rate.");
  if (_root["sampling_rate"].asFloat() < 0.f) {
    ozz::log::Err() << "Invalid sampling rate option (must be >= 0)."
                    << std::endl;
    return false;
  }

  return true;
}  // namespace

bool SanitizeRoot(Json::Value& _root) {
  MakeDefaultArray(_root, "animations", "Animations to extract.");

  Json::Value& animations = _root["animations"];
  for (Json::ArrayIndex i = 0; i < animations.size(); ++i) {
    if (!SanitizeAnimation(animations[i])) {
      return false;
    }
  }

  return true;
}

bool RecursiveSanitize(const Json::Value& _root, const Json::Value& _expected,
                       const char* _name) {
  if (!IsCompatibleType(_root.type(), _expected.type())) {
    // It's a failure to have a wrong member type.
    ozz::log::Err() << "Invalid type \"" << JsonTypeToString(_root.type())
                    << "\" for json member \"" << _name << "\". \""
                    << JsonTypeToString(_expected.type()) << "\" expected."
                    << std::endl;
    return false;
  }

  if (_root.isArray()) {
    assert(_expected.isArray());
    for (Json::ArrayIndex i = 0; i < _root.size(); ++i) {
      if (!RecursiveSanitize(_root[i], _expected[0], "[]")) {
        return false;
      }
    }
  } else if (_root.isObject()) {
    assert(_expected.isObject());
    for (Json::Value::iterator it = _root.begin(); it != _root.end(); it++) {
      const std::string& name = it.name();
      if (!_expected.isMember(name)) {
        ozz::log::Err() << "Invalid member \"" << name << "\"." << std::endl;
        return false;
      }
      const Json::Value& expected_member = _expected[name];
      if (!RecursiveSanitize(*it, expected_member, name.c_str())) {
        return false;
      }
    }
  }
  return true;
}
}  // namespace

bool Sanitize(Json::Value& _config) {
  // Build a default config to compare it with provided one and detect
  // unexpected members.
  Json::Value default_config;
  if (!SanitizeRoot(default_config)) {
    assert(false && "Failed to sanitized default configuration.");
  }

  // All format errors are reported within that function
  if (!RecursiveSanitize(_config, default_config, "root")) {
    return false;
  }

  // Sanitized provided config.
  return SanitizeRoot(_config);
}
