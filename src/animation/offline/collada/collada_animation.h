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

#ifndef OZZ_ANIMATION_OFFLINE_COLLADA_COLLADA_ANIMATION_H_
#define OZZ_ANIMATION_OFFLINE_COLLADA_COLLADA_ANIMATION_H_

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
// Forward declares the runtime skeleton.
class Skeleton;
namespace offline {
struct RawAnimation;
namespace collada {
// Forward declares Collada visitors.
class AnimationVisitor;
class SkeletonVisitor;

// Extracts all animation tracks of _skeleton that were found in the Collada
// document (_animation_visitor + _skeleton_visitor).
bool ExtractAnimation(const AnimationVisitor& _animation_visitor,
                      const SkeletonVisitor& _skeleton_visitor,
                      const Skeleton& _skeleton,
                      RawAnimation* _animation);

// Collada xml document traverser, aiming to build extract anim ation channels.
class AnimationVisitor : public BaseVisitor {
 public:
  // Constructor.
  AnimationVisitor();

  // Destructor.
  virtual ~AnimationVisitor();

  // The start time marker for the interval.
  float start_time() const {
    return start_time_;
  }

  // The end time marker for the interval, or a negative value if end time is
  // not known.
  float end_time() const {
    return end_time_;
  }

  struct Channel {
    const char* source;
  };

  typedef ozz::Map<const char*, Channel, internal::str_less>::Std Channels;

  const Channels& channels() const {
    return channels_;
  }

  enum Semantic {
    kInterpolation,
    kInTagent,
    kOutTagent,
    kInput,
    kOutput,
  };
  static const int kSemanticCount = 5;

  typedef ozz::Map<Semantic, const char*>::Std Inputs;

  enum Behavior {
    kUndefined,
    kConstant,
    kGradient,
    kCycle,
    kOscillate,
    kCycleRelative,
  };
  static const int kBehaviorCount = 6;

  struct Sampler {
    Inputs inputs;
    Behavior pre_behavior;
    Behavior post_behavior;
  };

  typedef ozz::Map<const char*, Sampler, internal::str_less>::Std Samplers;
  const Samplers& samplers() const {
    return samplers_;
  }

  struct Source {
    size_t count;
    size_t offset;
    size_t stride;
    unsigned int binding;
  };

  struct FloatSource : public Source {
    ozz::Vector<float>::Std values;
  };

  const FloatSource* GetFloatSource(
    const Sampler& _sampler, Semantic _semantic) const;

  enum Interpolation {
    kLinear,
    kBezier,
    kBSpline,
    kHermite,
    kCardinal,
    kStep,
  };
  static const int kInterpolationCount = 6;

  struct InterpolationSource : public Source {
    ozz::Vector<Interpolation>::Std values;
  };

  const InterpolationSource* GetInterpolationSource(
    const Sampler& _sampler) const;

 private:
  virtual bool VisitEnter(const TiXmlElement& _element,
                          const TiXmlAttribute* _firstAttribute);

  virtual bool VisitExit(const TiXmlElement& _element);

  virtual bool Visit(const TiXmlText& _text);

  bool HandleSourceEnter(const TiXmlElement& _element);
  bool HandleSourceExit(const TiXmlElement& _element);

  bool HandleFloatArray(const TiXmlElement& _element);
  bool HandleNameArray(const TiXmlElement& _element);
  bool HandleAccessor(const TiXmlElement& _element);
  bool HandleSampler(const TiXmlElement& _element);
  bool HandleInput(const TiXmlElement& _element);
  bool HandleChannel(const TiXmlElement& _element);

  typedef ozz::Map<const char*, FloatSource, internal::str_less>::Std
    FloatSources;
  FloatSources float_sources_;

  typedef ozz::Map<const char*, InterpolationSource, internal::str_less>::Std
    InterpolationSources;
  InterpolationSources interpolation_sources_;

  Samplers samplers_;

  Channels channels_;

  // Shortcut to the source being processed, if any.
  Source* source_;

  // The start time marker for the interval.
  float start_time_;

  // The end time marker for the interval, or a negative value if end time is
  // not known.
  float end_time_;
};
}  // collada
}  // animation
}  // offline
}  // ozz
#endif  // OZZ_ANIMATION_OFFLINE_COLLADA_COLLADA_ANIMATION_H_
