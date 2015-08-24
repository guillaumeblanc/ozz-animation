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

#define OZZ_INCLUDE_PRIVATE_HEADER  // Allows to include private headers.

#include "animation/offline/collada/collada_animation.h"

#include <cstdlib>
#include <cstring>
#include <limits>

#include "ozz/base/log.h"

#include "ozz/animation/offline/raw_animation.h"
#include "ozz/animation/runtime/skeleton.h"
#include "ozz/animation/runtime/skeleton_utils.h"

#include "animation/offline/collada/collada_skeleton.h"

namespace ozz {
namespace animation {
namespace offline {
namespace collada {

AnimationVisitor::AnimationVisitor()
    : source_(NULL),
      start_time_(0.f),
      end_time_(-1.f) {
}

AnimationVisitor::~AnimationVisitor() {
}

bool AnimationVisitor::VisitEnter(const TiXmlElement& _element,
                                  const TiXmlAttribute* _firstAttribute) {
  if (std::strcmp(_element.Value(), "library_animations") == 0) {
    return true;
  } else if (std::strcmp(_element.Value(), "animation") == 0) {
    return true;
  } else if (std::strcmp(_element.Value(), "source") == 0) {
    return HandleSourceEnter(_element);
  } else if (std::strcmp(_element.Value(), "float_array") == 0) {
    return HandleFloatArray(_element);
  } else if (std::strcmp(_element.Value(), "Name_array") == 0) {
    return HandleNameArray(_element);
  } else if (std::strcmp(_element.Value(), "technique_common") == 0) {
    return true;
  } else if (std::strcmp(_element.Value(), "accessor") == 0) {
    return HandleAccessor(_element);
  } else if (std::strcmp(_element.Value(), "sampler") == 0) {
    return HandleSampler(_element);
  } else if (std::strcmp(_element.Value(), "input") == 0) {
    return HandleInput(_element);
  } else if (std::strcmp(_element.Value(), "channel") == 0) {
    return HandleChannel(_element);
  }
  return BaseVisitor::VisitEnter(_element, _firstAttribute);
}

bool AnimationVisitor::VisitExit(const TiXmlElement& _element) {
  if (std::strcmp(_element.Value(), "source") == 0) {
    return HandleSourceExit(_element);
  }
  return BaseVisitor::VisitExit(_element);
}

bool AnimationVisitor::Visit(const TiXmlText& _text) {
  return BaseVisitor::Visit(_text);
}

const AnimationVisitor::FloatSource* AnimationVisitor::GetFloatSource(
  const AnimationVisitor::Sampler& _sampler,
  AnimationVisitor::Semantic _semantic) const {
  const AnimationVisitor::Inputs::const_iterator it_url =
    _sampler.inputs.find(_semantic);
  if (it_url == _sampler.inputs.end()) {
    return NULL;
  }
  const AnimationVisitor::FloatSources::const_iterator it_source =
    float_sources_.find(it_url->second);
  if (it_source == float_sources_.end()) {
    return NULL;
  }
  return &it_source->second;
}

const AnimationVisitor::InterpolationSource*
  AnimationVisitor::GetInterpolationSource(
  const AnimationVisitor::Sampler& _sampler) const {
  const AnimationVisitor::Inputs::const_iterator it_url =
    _sampler.inputs.find(AnimationVisitor::kInterpolation);
  if (it_url == _sampler.inputs.end()) {
    return NULL;
  }
  const AnimationVisitor::InterpolationSources::const_iterator
    it_interpolation = interpolation_sources_.find(it_url->second);
  if (it_interpolation == interpolation_sources_.end()) {
    return NULL;
  }
  return &it_interpolation->second;
}

namespace {
const char* GetParentId(const char* _value, const TiXmlElement& _element) {
  // Ensures enclosing is a <_value>.
  const TiXmlNode* parent_node = _element.Parent();
  if (!parent_node || std::strcmp(parent_node->Value(), _value)) {
    log::Err() << "Failed to find parent node with value \"" << _value << "\"."
      << std::endl;
    return NULL;
  }

  // So parent is a TiXmlElement.
  const TiXmlElement* parent = parent_node->ToElement();
  if (!parent) {
    log::Err() << "Failed to find parent node with value \"" << _value << "\"."
      << std::endl;
    return NULL;
  }

  // Finds name of the enclosing <source>
  const char* name = parent->Attribute("id");
  if (!name) {
    log::Err() << "Failed to find parent node id (value \"" << _value << "\")."
      << std::endl;
    return NULL;
  }
  return name;
}
}  // namespace

bool AnimationVisitor::HandleSourceEnter(const TiXmlElement& _element) {
  (void)_element;
  // Don't create the source here as its type is unknown.
  return true;
}

bool AnimationVisitor::HandleSourceExit(const TiXmlElement& _element) {
  source_ = NULL;
  return BaseVisitor::VisitExit(_element);
}

bool AnimationVisitor::HandleAccessor(const TiXmlElement& _element) {
  if (!source_) {
    return false;
  }

  // The number of times the array is accessed. Required.
  int count;
  if (!_element.Attribute("count", &count)) {
    source_->count = 0;
    log::Err() << "Failed to find accesor \"count\" attribute." << std::endl;
    set_error();
    return false;
  }
  source_->count = count;

  // The index of the first value to be read from the array. The default is 0.
  // Optional.
  int offset;
  if (!_element.Attribute("offset", &offset)) {
    offset = 0;
  }
  source_->offset = offset;

  // The number of values that are to be considered a unit during each access to
  // the array. The default is 1, indicating that a single value is accessed.
  // Optional.
  int stride;
  if (!_element.Attribute("stride", &stride)) {
    stride = 1;
  }
  source_->stride = stride;

  // Iterates <param> child elements. The number and order of <param> elements
  // define the output of the <accessor> element. Parameters are bound to values
  // in the order in which both are specified. No reordering of the data can
  // occur. A <param> element without a name attribute indicates that the value
  // is not part of the output, so the element is unbound.
  int param_index = 0;
  for (const TiXmlNode* child = _element.FirstChild("param");
       child;
       child = child->NextSibling("param")) {
    if (param_index == 32) {
      log::Err() << "Too many <accessor><param> elements, maximum is 32." <<
        std::endl;
      set_error();
      return false;
    }
    if (child->ToElement()->Attribute("name")) {
      source_->binding |= 1 << param_index;
    }
    ++param_index;
  }

  // No need to go deeper.
  return false;
}

bool AnimationVisitor::HandleFloatArray(const TiXmlElement& _element) {
  // Get the name of the source.
  const char* source_id = GetParentId("source", _element);
  if (!source_id) {
    set_error();
    return false;
  }

  if (float_sources_.count(source_id)) {
    log::Err() << "Multiple <source> with name \"" << source_id << "\" found." <<
      std::endl;
    set_error();
    return false;
  }

  // Adds a float array with the name we found.
  FloatSource& source = float_sources_[source_id];
  source_ = &source;

  // The number of values in the array. Required.
  int num_elements;
  if (!_element.Attribute("count", &num_elements)) {
    log::Err() << "Failed to find float_array element count." << std::endl;
    set_error();
    return false;
  }

  source.values.reserve(num_elements);
  const char* text = _element.GetText();
  if (text) {
    char* end;
    for (double double_value = strtod(text, &end);
         text != end;
         double_value = strtod(text, &end)) {
      source.values.push_back(static_cast<float>(double_value));
      text = end;
    }
  }
  if (source.values.size() != static_cast<size_t>(num_elements)) {
    log::Err() << "Failed to read all float_array values." << std::endl;
    set_error();
    return false;
  }
  return true;
}

bool AnimationVisitor::HandleNameArray(const TiXmlElement& _element) {
  // Get the name of the source.
  const char* source_id = GetParentId("source", _element);
  if (!source_id) {
    set_error();
    return false;
  }

  if (interpolation_sources_.count(source_id)) {
    log::Err() << "Multiple <source> with name \"" << source_id << "\" found." <<
      std::endl;
    set_error();
    return false;
  }

  // Adds a float array with the name we found.
  InterpolationSource& source = interpolation_sources_[source_id];
  source_ = &source;

  // Read the number of array elements.
  int count;
  if (!_element.Attribute("count", &count)) {
    log::Err() << "Failed to find float_array element count." << std::endl;
    set_error();
    return false;
  }
  size_t num_elements = count;

  source.values.reserve(num_elements);
  const char* text = _element.GetText();
  if (text) {
    const char* semantics[] = {
      "LINEAR",  // kLinear
      "BEZIER",  // kBezier
      "BSPLINE",  // kBSpline
      "HERMITE",  // kHermite
      "CARDINAL",  // kCardinal
      "STEP"};  // kStep
    OZZ_STATIC_ASSERT(OZZ_ARRAY_SIZE(semantics) == kInterpolationCount);
    const int semantic_len[] = {
      6,  // "LINEAR"
      6,  // "BEZIER"
      7,  // "BSPLINE"
      7,  // "HERMITE"
      8,  // "CARDINAL
      4};  // "STEP"
    OZZ_STATIC_ASSERT(OZZ_ARRAY_SIZE(semantic_len) == kInterpolationCount);
    for (size_t i = 0; i < num_elements && i == source.values.size(); ++i) {
      for (int j = 0; j < kInterpolationCount; ++j) {
        if (strncmp(semantics[j], text, semantic_len[j]) == 0) {
          source.values.push_back(Interpolation(j));
          text += semantic_len[j] + 1;  // Skips space also.
          break;
        }
      }
    }
  }
  if (source.values.size() != static_cast<size_t>(num_elements)) {
    log::Err() << "Failed to read all Name_array values." << std::endl;
    set_error();
    return false;
  }

  return true;
}

bool AnimationVisitor::HandleSampler(const TiXmlElement& _element) {
  const char* id = _element.Attribute("id");
  if (!id || *id == '\0') {
    log::Err() << "Failed to find <sampler> id." << std::endl;
    set_error();
    return false;
  }

  if (samplers_.count(id)) {
    log::Err() << "Multiple <sampler> with id \"" << id << "\" found." <<
      std::endl;
    set_error();
    return false;
  }

  // Prepares the sampler.
  Sampler sampler;

  // Finds pre and post behaviors.
  const char* behaviors[] = {
    "UNDEFINED",
    "CONSTANT",
    "GRADIENT",
    "CYCLE",
    "OSCILLATE",
    "CYCLERELATIVE"};
  OZZ_STATIC_ASSERT(OZZ_ARRAY_SIZE(behaviors) == kBehaviorCount);

  sampler.pre_behavior = kConstant;
  const char* pre = _element.Attribute("pre_behavior");
  if (pre) {
    int i = 0;
    for (; i < kBehaviorCount; ++i) {
      if (std::strcmp(pre, behaviors[i]) == 0) {
        sampler.pre_behavior = Behavior(i);
        break;
      }
    }
    if (i == kBehaviorCount) {
      log::Err() << "Unkown behavior \"" << pre << "\" found." << std::endl;
      set_error();
      return false;
    }
  }
  sampler.post_behavior = kConstant;
  const char* post = _element.Attribute("post_behavior");
  if (post) {
    int i = 0;
    for (; i < kBehaviorCount; ++i) {
      if (std::strcmp(post, behaviors[i]) == 0) {
        sampler.post_behavior = Behavior(i);
        break;
      }
    }
    if (i == kBehaviorCount) {
      log::Err() << "Unkown behavior \"" << post << "\" found." << std::endl;
      set_error();
      return false;
    }
  }

  // Insert this sampler.
  samplers_[id] = sampler;

  return true;
}

bool AnimationVisitor::HandleInput(const TiXmlElement& _element) {
  // Get parent sampler.
  const char* sampler_id = GetParentId("sampler", _element);
  if (!sampler_id) {
    set_error();
    return false;
  }

  const char* semantic_name = _element.Attribute("semantic");
  if (!semantic_name) {
    log::Err() << "Failed to find <input> semantic." << std::endl;
    set_error();
    return false;
  }

  // Match semantic with an enumerated value
  const char* semantics[] = {
    "INTERPOLATION",  // kInterpolation,
    "IN_TANGENT",  // kInTagent,
    "OUT_TANGENT",  // kOutTagent,
    "INPUT",  // kInput,
    "OUTPUT"};  //  kOutput,
  OZZ_STATIC_ASSERT(OZZ_ARRAY_SIZE(semantics) == kSemanticCount);

  Semantic semantic;
  int i = 0;
  for (; i < kSemanticCount; ++i) {
    if (!std::strcmp(semantics[i], semantic_name)) {
      semantic = Semantic(i);
      break;
    }
  }
  if (i == kSemanticCount) {
    log::Err() << "Unknow semantic \"" << semantic_name << "\" for input id \""
      << sampler_id << "\"." << std::endl;
    return true;  // This is not a fatal error.
  }

  const char* source = _element.Attribute("source");
  if (!source || *source != '#') {
    log::Err() << "Failed to find <input> source." << std::endl;
    set_error();
    return false;
  }
  ++source;  // Skip # url character.

  // Fills the sampler.
  Sampler& sampler = samplers_[sampler_id];
  sampler.inputs[semantic] = source;

  return true;
}

bool AnimationVisitor::HandleChannel(const TiXmlElement& _element) {
  const char* source = _element.Attribute("source");
  if (!source || *source != '#') {
    log::Err() << "Failed to find <channel> source." << std::endl;
    set_error();
    return false;
  }
  ++source;  // Skip # url character.

  const char* target = _element.Attribute("target");
  if (!target || *target == '\0') {
    log::Err() << "Failed to find <channel> target." << std::endl;
    set_error();
    return false;
  }

  if (channels_.count(target)) {
    log::Err() << "Multiple <channels> with target \"" << target << "\" found."
      << std::endl;
    set_error();
    return false;
  }

  Channel& channel = channels_[target];
  channel.source = source;

  return true;
}

namespace {

typedef ozz::CStringMap<const ColladaJoint*>::Std
  JointsByName;

bool MapJointsByName(const ColladaJoint& _src, JointsByName* _joints) {
  // Detects non unique names
  if (_joints->count(_src.name.c_str())) {
    log::Err() << "Multiple joints with the same name \"" << _src.name <<
      "\" found." << std::endl;
    return false;
  }

  // This name is unique, maps it.
  (*_joints)[_src.name.c_str()] = &_src;

  // Now maps children.
  for (size_t i = 0; i < _src.children.size(); ++i) {
    if (!MapJointsByName(_src.children[i], _joints)) {
      return false;
    }
  }
  return true;
}

struct Sampler {
  size_t transform;
  const char* member;
  AnimationVisitor::Behavior pre_behavior;
  AnimationVisitor::Behavior post_behavior;
  const AnimationVisitor::FloatSource* input;
  const AnimationVisitor::FloatSource* output;
  const AnimationVisitor::InterpolationSource* interpolation;
  const AnimationVisitor::FloatSource* in_tangent;
  const AnimationVisitor::FloatSource* out_tangent;
};
typedef ozz::Vector<Sampler>::Std Samplers;

struct Track {
  const ColladaJoint* joint;
  Samplers samplers;
};
typedef ozz::Vector<Track>::Std Tracks;

// Find all the sampler sources for a joint.
bool FindSamplers(const AnimationVisitor& _visitor,
                  const ColladaJoint& _joint,
                  const char* _joint_name,
                  Samplers* _samplers) {
  assert(_samplers->empty());

  // Find the first channel that animates this joint.
  const ozz::String::Std target_id = ozz::String::Std(_joint.id) + '/';
  AnimationVisitor::Channels::const_iterator it_channel =
    _visitor.channels().lower_bound(target_id.c_str());

  if (it_channel == _visitor.channels().end() ||
      strstr(it_channel->first, target_id.c_str()) != it_channel->first) {
    // A joint might not be animated, this is not an error.
    log::LogV() << "No Channel found for joint \"" << _joint_name << "\"." <<
      std::endl;
    return true;
  }

  // Iterates all channels that targets that joint and collect their curves.
  for (;
       it_channel != _visitor.channels().end() &&
       strstr(it_channel->first, target_id.c_str()) == it_channel->first;
       ++it_channel) {
    Sampler sampler;

    // Find transformation target.
    const char* transform_sid = strchr(it_channel->first, '/');
    if (!transform_sid ||  // A single '/' is supported.
        (strrchr(it_channel->first, '/') != transform_sid)) {
      log::LogV() << "Unsupported sid syntax \"" << it_channel->first <<
        "\"." << std::endl;
      return false;
    }
    ++transform_sid;  // Skip '/' character.

    // Looks for a member definition.
    const char* member = NULL;
    size_t cmp_count = strlen(transform_sid);
    const char* dot = strchr(transform_sid, '.');
    if (dot) {
      member = dot;
      cmp_count = dot - transform_sid;
    }
    const char* parenthesis = strchr(transform_sid, '(');
    if (parenthesis) {
      member = parenthesis;
      cmp_count = parenthesis - transform_sid;
    }

    // Finds matching transform.
    size_t transform_index = 0;
    for (; transform_index < _joint.transforms.size(); ++transform_index) {
      const NodeTransform& transform = _joint.transforms[transform_index];
      if (!strncmp(transform.sid(), transform_sid, cmp_count)) {
        break;
      }
    }
    if (transform_index == _joint.transforms.size()) {
      // This is not a fatal error as the channel could animation something else
      // but a transform.
      continue;
    }
    sampler.transform = transform_index;
    sampler.member = member;

    // Get channel and find sources.
    const AnimationVisitor::Channel& channel = it_channel->second;
    const char* sampler_name = channel.source;

    // Find sampler.
    AnimationVisitor::Samplers::const_iterator it_sampler =
      _visitor.samplers().find(sampler_name);
    if (it_sampler == _visitor.samplers().end()) {
      log::Err() << "Sampler \"" << sampler_name << "\" not found." << std::endl;
      return false;
    }
    const AnimationVisitor::Sampler& visitor_sampler = it_sampler->second;

    // Copies pre and post behaviors.
    sampler.pre_behavior = visitor_sampler.pre_behavior;
    sampler.post_behavior = visitor_sampler.post_behavior;

    // Find sources.
    const AnimationVisitor::FloatSource* input =
      _visitor.GetFloatSource(visitor_sampler, AnimationVisitor::kInput);
    if (!input) {
      log::Err() << "Input source not found for Sampler \"" << sampler_name <<
        "\"." << std::endl;
      return false;
    }
    sampler.input = input;

    const AnimationVisitor::FloatSource* output =
      _visitor.GetFloatSource(visitor_sampler, AnimationVisitor::kOutput);
    if (!output) {
      log::Err() << "Output source not found for Sampler \"" << sampler_name <<
        "\"." << std::endl;
      return false;
    }
    sampler.output = output;

    // Will consider interpolation LINEAR if no source is specified.
    sampler.interpolation = _visitor.GetInterpolationSource(visitor_sampler);

    // Tangents might not be present according to interpolation type.
    sampler.in_tangent =
      _visitor.GetFloatSource(visitor_sampler, AnimationVisitor::kInTagent);
    sampler.out_tangent =
      _visitor.GetFloatSource(visitor_sampler, AnimationVisitor::kOutTagent);

    // Push sampler now it's complete.
    _samplers->push_back(sampler);
  }
  return true;
}

// _start_time and _end_time are the inclusive time interval.
bool FindSampleKeysUnion(const Samplers& _samplers,
                         ozz::Vector<float>::Std* _times,
                         float _start_time, float _end_time) {
  // Find inputs key-frame's time union.
  for (size_t i = 0; i < _samplers.size(); ++i) {
    // Preallocates times vector.
    const ozz::Vector<float>::Std& values = _samplers[i].input->values;
    _times->reserve(_times->size() + values.size());

    // Compares "values" key with "times" ones and insert non existing ones.
    // Cares to maintain key frame order.
    size_t v = 0;
    for (size_t s = 0; s < _times->size() && v < values.size(); ++s) {
      const float v_value = values[v];
      // Ensure key-frames are strictly ordered.
      if (v > 0 && v_value <= values[v - 1]) {
        return false;
      }
      const float s_value = (*_times)[s];
      if (v_value == s_value) {  // Existing key.
        ++v;
      } else if (v_value < s_value) {  // new key to insert.
        _times->insert(_times->begin() + s, v_value);
        ++v;
      } else {  // Will see later if this key's needed.
      }
      // Output keys should be ordered.
      assert(s == 0 || (*_times)[s - 1] < (*_times)[s]);
    }

    // Pushes all remaining values.
    for (; v < values.size(); ++v) {
      const float v_value = values[v];
      // Ensure key-frames are strictly ordered.
      if (v > 0 && v_value <= values[v - 1]) {
        return false;
      }
      _times->push_back(v_value);
      static bool b = true;
      if (b && v_value > 0.03f && v_value < 0.07f) {
        _times->push_back(v_value + 0.01f);
        b = false;
      }
      // Output keys should be ordered.
      assert(_times->size() < 2 ||
             (*_times)[_times->size() - 2] < _times->back());
    }
  }

  // Makes sure _times includes _start_time and _end_time.
  if (!_times->empty()) {
    if (_times->front() != _start_time) {
      _times->insert(_times->begin(), _start_time);
    }
    if (_times->back() != _end_time) {
      _times->push_back(_end_time);
    }
  }
  return true;
}

struct EvaluationCache {
  AnimationVisitor::Behavior pre_behavior;
  AnimationVisitor::Behavior post_behavior;
  const float* inputs;
  const float* inputs_end;
  size_t input_stride;
  const float* outputs;
  size_t output_stride;
  const AnimationVisitor::Interpolation* interpolations;
  size_t interpolation_stride;
  const float* in_tangents;
  size_t in_tangent_stride;
  const float* out_tangents;
  size_t out_tangent_stride;
};

typedef ozz::Vector<EvaluationCache>::Std Caches;

bool SetupCaches(const Samplers& _samplers, Caches* _caches) {
  _caches->resize(_samplers.size());

  for (size_t j = 0; j < _samplers.size(); ++j) {
    const Sampler& sampler = _samplers[j];

    // Validates sampler.
    bool valid_input = sampler.input &&
      sampler.input->count * sampler.input->stride ==
      sampler.input->values.size();
    bool valid_output = sampler.output &&
      sampler.output->count * sampler.output->stride ==
      sampler.output->values.size();
    bool valid_interpolation = !sampler.interpolation ||
      ((sampler.interpolation->count * sampler.interpolation->stride ==
        sampler.interpolation->values.size()) &&
      (sampler.interpolation->count == sampler.input->count));
    bool valid_in_tangent = !sampler.in_tangent ||
      ((sampler.in_tangent->count * sampler.in_tangent->stride ==
        sampler.in_tangent->values.size()) &&
      (sampler.in_tangent->count == sampler.output->count));
    bool valid_out_tangent = !sampler.out_tangent ||
      ((sampler.out_tangent->count * sampler.out_tangent->stride ==
        sampler.out_tangent->values.size()) &&
      (sampler.out_tangent->count == sampler.output->count));

    if (!valid_input || !valid_output || !valid_interpolation ||
        !valid_out_tangent || !valid_in_tangent) {
      log::Out() << "Unsupported number of array elements." << std::endl;
      return false;
    }

    // Prepares cache.
    EvaluationCache& cache = _caches->at(j);

    cache.pre_behavior = sampler.pre_behavior;
    cache.post_behavior = sampler.post_behavior;
    cache.inputs = sampler.input->values.empty() ?
      NULL : &sampler.input->values[sampler.input->offset];
    cache.inputs_end = cache.inputs +
      sampler.input->count * sampler.input->stride;
    cache.input_stride = sampler.input->stride;
    cache.outputs = &sampler.output->values[sampler.output->offset];
    cache.output_stride = sampler.output->stride;
    cache.interpolations =
      !sampler.interpolation || sampler.interpolation->values.empty() ?
      NULL : &sampler.interpolation->values[sampler.interpolation->offset];
    cache.interpolation_stride = cache.interpolations ?
      sampler.interpolation->stride : 0;
    cache.in_tangents =
      !sampler.in_tangent || sampler.in_tangent->values.empty() ?
      NULL : &sampler.in_tangent->values[sampler.in_tangent->offset];
    cache.in_tangent_stride = cache.in_tangents ?
      sampler.in_tangent->stride : 0;
    cache.out_tangents =
      !sampler.out_tangent || sampler.out_tangent->values.empty() ?
      NULL : &sampler.out_tangent->values[sampler.out_tangent->offset];
    cache.out_tangent_stride = cache.out_tangents ?
      sampler.out_tangent->stride : 0;
  }
  return true;
}
// Finds the next source key frame that matches _time.
void StepCache(float _time, EvaluationCache* _cache) {
  while (_cache->inputs < (_cache->inputs_end - 1) &&
         _time >= _cache->inputs[_cache->input_stride]) {
    _cache->inputs += _cache->input_stride;
    _cache->outputs += _cache->output_stride;
    _cache->interpolations += _cache->interpolation_stride;
    _cache->in_tangents += _cache->in_tangent_stride;
    _cache->out_tangents += _cache->out_tangent_stride;
  }
}

struct InterpKey {
  float input;
  float output;
  float in_tangent;
  float out_tangent;
};

// See Collada specification about "Curve Interpolation" for an explanation of
// this function's arguments.
float EvaluateCubicCurve(const math::Float4 _m[4], const math::Float4& _c,
                         float _alpha) {
    math::Float4 s(_alpha * _alpha * _alpha, _alpha * _alpha, _alpha, 1.f);
    return Dot(s, math::Float4(
      Dot(_m[0], _c), Dot(_m[1], _c), Dot(_m[2], _c), Dot(_m[3], _c)));
}

// Finds alpha using a dichotomic algorithm.
float ApproximateAlpha(const math::Float4 _m[4], const math::Float4& _c,
                       float _time) {
  const float kTolerance = 1e-6f;

  // Early out extreme cases.
  if (_time - _c.x < kTolerance) {
    return 0.f;
  } else if (_c.w - _time < kTolerance) {
    return 1.f;
  }

  // Iteratively subdivide to approach _time.
  float begin = 0.f;
  float end = 1.f;
  float alpha = 0.f;
  const int kMaxIterations = 16;  // 16 loops = 1 / (1<<16) = 1e-5f.
  for (int i = 0; i < kMaxIterations; ++i) {
    alpha = (begin + end) * .5f;  // Dichotomy.
    float output = EvaluateCubicCurve(_m, _c, alpha);
    if (fabs(output - _time) < kTolerance) {
      break;
    } else if (output > _time) {
      end = alpha;  // Selects [begin,alpha] range.
    } else {
      begin = alpha;  // Selects [alpha,end] range.
    }
  }
  return alpha;
}

// Evaluate a single float at t = _time.
bool Evaluate(float _time, const EvaluationCache& _cache,
              size_t _param_index, float* _output, bool* _subsample) {
  // Does not require sub-sampling by default.
  *_subsample = false;

  float output = 0.f;
  if (_time < *_cache.inputs) {
    if (_cache.pre_behavior != AnimationVisitor::kConstant) {
      log::Err() << "Unsuported pre_behavior, only CONSTANT is supported." <<
        std::endl;
      return false;
    }
    // Pre-infinity case.
    output = _cache.outputs[_param_index];
  } else if (_cache.inputs == _cache.inputs_end - _cache.input_stride) {
    if (_cache.post_behavior != AnimationVisitor::kConstant) {
      log::Err() << "Unsuported post_behavior, only CONSTANT is supported." <<
        std::endl;
      return false;
    }
    // Post-infinity case.
    output = _cache.outputs[_param_index];
  } else {  // Interpolates.
    assert(_time >= _cache.inputs[0] &&
           _time < _cache.inputs[0 + _cache.input_stride]);

    const AnimationVisitor::Interpolation interpolation =
      _cache.interpolations ?
        *_cache.interpolations : AnimationVisitor::kLinear;

    switch (interpolation) {
      case AnimationVisitor::kLinear: {
        float alpha = (_time - _cache.inputs[0]) /
          (_cache.inputs[0 + _cache.input_stride] - _cache.inputs[0]);
        output = math::Lerp(_cache.outputs[_param_index],
                            _cache.outputs[_param_index + _cache.output_stride],
                            alpha);
        break;
      }
      case AnimationVisitor::kBezier: {
        if (!_cache.in_tangents || !_cache.out_tangents) {
          log::Err() << "Failed to find curve tangents" << std::endl;
          return false;
        }
        const math::Float4 m[4] = {math::Float4(-1.f, 3.f, -3.f, 1.f),
                                   math::Float4(3.f, -6.f, 3.f, 0.f),
                                   math::Float4(-3.f, 3.f, 0.f, 0.f),
                                   math::Float4(1.f, 0.f, 0.f, 0.f)};
        const math::Float4 ct(
          _cache.inputs[0],
          _cache.out_tangents[_param_index * 2],
          _cache.in_tangents[_param_index * 2 + _cache.in_tangent_stride],
          _cache.inputs[0 + _cache.input_stride]);
        const float alpha = ApproximateAlpha(m, ct, _time);
        const math::Float4 c(
          _cache.outputs[_param_index],
          _cache.out_tangents[_param_index * 2 + 1],
          _cache.in_tangents[_param_index * 2 + _cache.in_tangent_stride + 1],
          _cache.outputs[_param_index + _cache.output_stride]);
        output = EvaluateCubicCurve(m, c, alpha);
        *_subsample = true;
        break;
      }
      case AnimationVisitor::kHermite: {
        if (!_cache.in_tangents || !_cache.out_tangents) {
          log::Err() << "Failed to find curve tangents" << std::endl;
          return false;
        }
        const math::Float4 m[4] = {math::Float4(2.f, -2.f, 1.f, 1.f),
                                   math::Float4(-3.f, 3.f, -2.f, -1.f),
                                   math::Float4(0.f, 0.f, 1.f, 0.f),
                                   math::Float4(1.f, 0.f, 0.f, 0.f)};
        const math::Float4 ct(
          _cache.inputs[0],
          _cache.out_tangents[_param_index * 2],
          _cache.in_tangents[_param_index * 2 + _cache.in_tangent_stride],
          _cache.inputs[0 + _cache.input_stride]);
        const float alpha = ApproximateAlpha(m, ct, _time);
        const math::Float4 c(
          _cache.outputs[_param_index],
          _cache.outputs[_param_index + _cache.output_stride],
          _cache.out_tangents[_param_index * 2 + 1],
          _cache.in_tangents[_param_index * 2 + _cache.in_tangent_stride + 1]);
        output = EvaluateCubicCurve(m, c, alpha);
        *_subsample = true;
        break;
      }
      case AnimationVisitor::kBSpline: {
        *_subsample = true;
        log::Err() << "Unsupported BSpline animation curve type" << std::endl;
        return false;
      }
      case AnimationVisitor::kCardinal: {
        if (!_cache.in_tangents || !_cache.out_tangents) {
          log::Err() << "Failed to find Cardinal curve tangents" << std::endl;
          return false;
        }
        *_subsample = true;
        log::Err() << "Unsupported Cardinal animation curve type" << std::endl;
        return false;
      }
      case AnimationVisitor::kStep: {
        output = _cache.outputs[_param_index];
        // Needs sub-sampling in order to avoid linear interpolation with the
        // next key.
        *_subsample = true;
        break;
      }
      default: {
        log::Err() << "Unsupported animation curve type" << std::endl;
        return false;
      }
    }
  }
  _output[_param_index] = output;
  return true;
}

// Evalute _sampler at t = _time. Outputs result in _builder.
bool Evaluate(const Sampler& _sampler,
              float _time,
              EvaluationCache* _cache,
              NodeTransform* _transform,
              bool* _subsample) {
  *_subsample = false;

  // TOGO(gblanc) <param> node binding should be tested here. Unfortunately
  // exporters fails to dump <source> correctly so I hope for sequential
  // values.
  // The number and order of <param> elements define the output of the
  // <accessor> element. Parameters are bound to values in the order in which
  // both are specified. No reordering of the data can occur. A <param>
  // element without a name attribute indicates that the value is not part of
  // the output, so the element is unbound.

  // Finds the source key frame to interpolate.
  StepCache(_time, _cache);

  // Traverse all sampler values and fills output.
  // Maximum output size is arbitrary fixed to a 4x4 matrix.
  float outputs[16] = {};
  // Ensure output fits the array.
  if (_sampler.output->stride > OZZ_ARRAY_SIZE(outputs)) {
    log::Out() << "Unsupported number of <source> array output elements." <<
      std::endl;
    return false;
  }

  // Evaluate all outputs. Uses a local cache in order to freely increment
  // outputs and tangents.
  for (size_t i = 0; i < _sampler.output->stride; ++i) {
    bool subsample;
    if (!Evaluate(_time, *_cache, i, outputs, &subsample)) {
      return false;
    }
    *_subsample |= subsample;
  }

  // Creates a local transform that can be modified without altering skeleton.
  if (!_transform->PushAnimation(_sampler.member,
                                 outputs,
                                 _sampler.output->stride)) {
    return false;
  }
  return true;
}

// Push key frames {_time, _transform} to _track. Triplets of redundant key
// frames are optimized: the on in the middle is rejected.
void PushKeys(const math::Transform& _transform, float _time,
              RawAnimation::JointTrack* _track) {
  // Tolerances are set in order to compare equivalent floating point values,
  // not to degrade and optimize the number of keys.
  const float translation_tolerance = 1e-6f;  // 0.001mm.
  const float rotation_tolerance = 1e-3f * math::kPi / 180.f;  // 0.001 degree.
  const float scale_tolerance = 1e-5f;  // 0.001%.

  { // Translation
    RawAnimation::TranslationKey key = {_time, _transform.translation};
    RawAnimation::JointTrack::Translations& translations = _track->translations;
    const size_t count = translations.size();
    if (count < 2 ||
        !Compare(translations[count - 2].value,
                 translations.back().value,
                 translation_tolerance) ||
        !Compare(translations.back().value,
                 _transform.translation,
                 translation_tolerance)) {
      translations.push_back(key);
    } else {
      translations.back() = key;
    }
  }

  { // Rotation
    RawAnimation::RotationKey key = {_time, _transform.rotation};
    RawAnimation::JointTrack::Rotations& rotations = _track->rotations;
    const size_t count = rotations.size();
    if (count < 2 ||
        !Compare(rotations[count - 2].value,
                 rotations.back().value,
                 rotation_tolerance) ||
        !Compare(rotations.back().value,
                 _transform.rotation,
                 rotation_tolerance)) {
      rotations.push_back(key);
    } else {
      rotations.back() = key;
    }
  }

  { // Scale
    RawAnimation::ScaleKey key = {_time, _transform.scale};
    RawAnimation::JointTrack::Scales& scales = _track->scales;
    const size_t count = scales.size();
    if (count < 2 ||
        !Compare(scales[count - 2].value,
                 scales.back().value,
                 scale_tolerance) ||
        !Compare(scales.back().value,
                 _transform.scale,
                 scale_tolerance)) {
      scales.push_back(key);
    } else {
      scales.back() = key;
    }
  }
}
}  // namespace

bool ExtractAnimation(const AnimationVisitor& _animation_visitor,
                      const SkeletonVisitor& _skeleton_visitor,
                      const Skeleton& _skeleton,
                      RawAnimation* _animation) {
  // Build a joint-name mapping
  JointsByName joints;
  for (size_t i = 0; i < _skeleton_visitor.roots().size(); ++i) {
    if (!MapJointsByName(_skeleton_visitor.roots()[i], &joints)) {
      log::Err() << "Failed to build joint-name mapping." << std::endl;
      return false;
    }
  }

  // Gather tracks that matches skeleton joints.
  Tracks tracks;
  tracks.resize(_skeleton.num_joints());
  float start_time = std::numeric_limits<float>::max();
  float end_time = -std::numeric_limits<float>::max();
  for (int i = 0; i < _skeleton.num_joints(); ++i) {
    const char* joint_name = _skeleton.joint_names()[i];

    // Find the id of the imported joint that has the same name as the run-time
    // joint.
    JointsByName::const_iterator it = joints.find(joint_name);
    if (it == joints.end()) {
      log::Out() << "No animation found for joint \"" << joint_name << "\"." <<
        std::endl;
      tracks[i].joint = NULL;
      continue;
    }
    const ColladaJoint* joint = it->second;
    tracks[i].joint = joint;

    // Get all the samplers that animates this joint.
    if (!FindSamplers(_animation_visitor, *joint, joint_name,
                      &tracks[i].samplers)) {
      return false;
    }

    // Accumulates animation min and max times.
    for (size_t j = 0; j < tracks[i].samplers.size(); ++j) {
      const Sampler& sampler = tracks[i].samplers[j];
      if (!sampler.input->values.empty()) {
        start_time = math::Min(sampler.input->values.front(), start_time);
        end_time = math::Max(sampler.input->values.back(), end_time);
      }
    }
  }

  // If no key frame is found, let the duration by default.
  if (end_time >= start_time) {
    _animation->duration = end_time - start_time;
  }

  // Fills animation tracks.
  _animation->tracks.resize(_skeleton.num_joints());
  for (int i = 0; i < _skeleton.num_joints(); ++i) {
    const Track& track = tracks[i];

    // Get the union of all the key-frame's time.
    ozz::Vector<float>::Std times;
    if (!FindSampleKeysUnion(track.samplers, &times, start_time, end_time)) {
      return false;
    }

    // Evaluates all samplers for all sampler keys.
    RawAnimation::JointTrack& output_track = _animation->tracks[i];
    if (!track.joint || track.samplers.empty() || times.empty()) {
      ozz::log::Err() << "No animation track found for joint \"" <<
        _skeleton.joint_names()[i] << "\". Using skeleton bind-pose instead" <<
        std::endl;
      
      // Get joint's bind pose.
      const ozz::math::Transform& bind_pose =
        ozz::animation::GetJointBindPose(_skeleton, i);

      PushKeys(bind_pose, 0.f, &output_track);
    } else {  // Uses animated transformations.
      // Uses a local copy of joint NodeTransform's in order to keep joint
      // unchanged. Declares transforms container outside of the loop to avoid
      // vector reallocation.
      ozz::Vector<NodeTransform>::Std transforms;

      // Initializes sampling evaluation chache.
      Caches caches;
      if (!SetupCaches(track.samplers, &caches)) {
        return false;
      }

      for (size_t j = 0; j < times.size(); ++j) {  // For all the key-frames.
        float time = times[j];

        // Does evaluation requires to subsample between keys? This is the case
        // for all non-linear interpolations.
        bool subsample;
        do {  // Subsampling loop.
          subsample = false;

          transforms = track.joint->transforms;  // Reset output transforms.
          for (size_t k = 0; k < track.samplers.size(); ++k) {
            const Sampler& sampler = track.samplers[k];
            if (!Evaluate(sampler, time, &caches[k],
                          &transforms[sampler.transform], &subsample)) {
              return false;
            }
          }

          // Concatenates all transforms.
          TransformBuilder builder;
          for (size_t t = 0; t < transforms.size(); ++t) {
            if (!transforms[t].Build(&builder)) {
              return false;
            }
          }

          // Push key to animation track.
          math::Transform transform;
          if (!builder.GetAsTransform(&transform)) {
            log::Err() << "Failed to build affine tranformation for joint \"" <<
              track.joint->name << "\" at t=" << time << "." << std::endl;
            return false;
          }
          // Convert to ozz y_up/meter system coordinate.
          transform = _animation_visitor.asset().ConvertTransform(transform);

          // Shift all keys such that the first key is a t = 0.
          const float key_time = time - start_time;
          assert(key_time >= 0.f);

          // Adds those key-frames to the current track.
          PushKeys(transform, key_time, &output_track);

          // Subsample while next key-frame is not reached.
          const float subsampling_rate = 1.f / 30.f;
          time += subsampling_rate;
        } while (subsample && j != times.size() - 1 && time < times[j + 1]);
      }
    }
  }
  return true;
}
}  // collada
}  // ozz
}  // offline
}  // animation
