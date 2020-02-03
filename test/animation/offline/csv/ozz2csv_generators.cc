//----------------------------------------------------------------------------//
//                                                                            //
// ozz-animation is hosted at http://github.com/guillaumeblanc/ozz-animation  //
// and distributed under the MIT License (MIT).                               //
//                                                                            //
// Copyright (c) 2019 Guillaume Blanc                                         //
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

#include "ozz2csv.h"

#include "ozz/animation/offline/animation_builder.h"
#include "ozz/animation/offline/animation_optimizer.h"
#include "ozz/animation/offline/raw_animation.h"
#include "ozz/animation/offline/raw_animation_utils.h"

#include "ozz/animation/runtime/animation.h"
#include "ozz/animation/runtime/animation_utils.h"
#include "ozz/animation/runtime/sampling_job.h"
#include "ozz/animation/runtime/skeleton.h"

#include "ozz/base/containers/vector.h"
#include "ozz/base/memory/scoped_ptr.h"

#include "ozz/base/maths/soa_transform.h"
#include "ozz/base/maths/transform.h"

#include <json/json.h>

class OzzPassthrough : public Generator {
 protected:
  virtual bool Build(const ozz::animation::offline::RawAnimation& _animation,
                     const ozz::animation::Skeleton& _skeleton,
                     const Json::Value&) {
    if (_skeleton.num_joints() != _animation.num_tracks()) {
      return false;
    }
    samples_.resize(_animation.num_tracks());
    animation_ = _animation;
    return true;
  }

 private:
  virtual size_t Size() const { return animation_.size(); }

  virtual float Duration() const { return animation_.duration; }

  virtual int GetKeyframesCount(Transformation _transformation, int _track) {
    if (_track < 0 || _track > animation_.num_tracks()) {
      return 0;
    }

    const ozz::animation::offline::RawAnimation::JointTrack& track =
        animation_.tracks[_track];
    switch (_transformation) {
      case kTranslation: {
        return static_cast<int>(track.translations.size());
      }
      case kRotation: {
        return static_cast<int>(track.rotations.size());
      }
      case kScale: {
        return static_cast<int>(track.scales.size());
      }
      default:
        assert(false);
    }

    return 0;
  }

  virtual bool Sample(float _time, bool) {
    return SampleAnimation(animation_, _time, make_range(samples_));
  }

  virtual bool ReadBack(
      const ozz::Range<ozz::math::Transform>& _transforms) const {
    if (_transforms.count() < samples_.size()) {
      return false;
    }
    for (size_t i = 0; i < _transforms.count(); ++i) {
      _transforms[i] = samples_[i];
    }
    return true;
  }

  ozz::Vector<ozz::math::Transform>::Std samples_;
  ozz::animation::offline::RawAnimation animation_;
};

class OzzOptimizer : public OzzPassthrough {
  virtual bool Build(const ozz::animation::offline::RawAnimation& _animation,
                     const ozz::animation::Skeleton& _skeleton,
                     const Json::Value& _config) {
    ozz::animation::offline::AnimationOptimizer optimizer;
    optimizer.setting.tolerance =
        _config.get("tolerance", optimizer.setting.tolerance).asFloat();
    optimizer.setting.distance =
        _config.get("distance", optimizer.setting.distance).asFloat();
    optimizer.fast =
        _config.get("fast", optimizer.fast).asBool();

    ozz::animation::offline::RawAnimation optimized;
    if (!optimizer(_animation, _skeleton, &optimized)) {
      return false;
    }

    return OzzPassthrough::Build(optimized, _skeleton, _config);
  }
};

class OzzRuntime : public Generator {
 protected:
  virtual bool Build(const ozz::animation::offline::RawAnimation& _animation,
                     const ozz::animation::Skeleton& _skeleton,
                     const Json::Value& _config) {
    if (_skeleton.num_joints() != _animation.num_tracks()) {
      return false;
    }

    ozz::animation::offline::RawAnimation raw;
    if (_config.get("optimize", true).asBool()) {
      ozz::animation::offline::AnimationOptimizer optimizer;
      optimizer.setting.tolerance =
          _config.get("tolerance", optimizer.setting.tolerance).asFloat();
      optimizer.setting.distance =
          _config.get("distance", optimizer.setting.distance).asFloat();
    optimizer.fast =
        _config.get("fast", optimizer.fast).asBool();
        
      if (!optimizer(_animation, _skeleton, &raw)) {
        return false;
      }
    } else {
      raw = _animation;
    }

    ozz::animation::offline::AnimationBuilder builder;
    animation_ = builder(raw);
    if (!animation_) {
      return false;
    }

    cache.Resize(animation_->num_tracks());
    samples_.resize(animation_->num_soa_tracks());

    return true;
  }

 private:
  virtual size_t Size() const { return animation_->size(); }

  virtual float Duration() const { return animation_->duration(); }

  virtual int GetKeyframesCount(Transformation _transformation, int _track) {
    if (_track < 0 || _track > animation_->num_tracks()) {
      return 0;
    }

    switch (_transformation) {
      case kTranslation: {
        return ozz::animation::CountTranslationKeyframes(*animation_, _track);
      }
      case kRotation: {
        return ozz::animation::CountRotationKeyframes(*animation_, _track);
      }
      case kScale: {
        return ozz::animation::CountScaleKeyframes(*animation_, _track);
      }
      default:
        assert(false);
    }

    return 0;
  }

  virtual bool Sample(float _time, bool _reset) {
    if (_reset) {
      cache.Invalidate();
    }

    ozz::animation::SamplingJob job;
    job.animation = animation_.get();
    job.cache = &cache;
    job.ratio = _time / animation_->duration();
    job.output = make_range(samples_);
    return job.Run();
  }

  virtual bool ReadBack(
      const ozz::Range<ozz::math::Transform>& _transforms) const {
    if (_transforms.count() < static_cast<size_t>(animation_->num_tracks())) {
      return false;
    }

    ozz::math::SimdFloat4 translations[4];
    ozz::math::SimdFloat4 rotations[4];
    ozz::math::SimdFloat4 scales[4];
    for (int i = 0; i < animation_->num_soa_tracks(); ++i) {
      // Unpack SoA to AoS
      ozz::math::Transpose3x4(&samples_[i].translation.x, translations);
      ozz::math::Transpose4x4(&samples_[i].rotation.x, rotations);
      ozz::math::Transpose3x4(&samples_[i].scale.x, scales);
      // Copy to output
      const int loops = ozz::math::Min(animation_->num_tracks() - i * 4, 4);
      for (int j = 0; j < loops; ++j) {
        ozz::math::Transform& transform = _transforms[i * 4 + j];
        ozz::math::Store3PtrU(translations[j], &transform.translation.x);
        ozz::math::StorePtrU(rotations[j], &transform.rotation.x);
        ozz::math::Store3PtrU(scales[j], &transform.scale.x);
      }
    }
    return true;
  }

  ozz::Vector<ozz::math::SoaTransform>::Std samples_;
  ozz::ScopedPtr<ozz::animation::Animation> animation_;
  ozz::animation::SamplingCache cache;
};

bool RegisterDefaultGenerators(Ozz2Csv* _ozz2csv) {
  bool success = true;
  static OzzPassthrough passthrough;
  success &= _ozz2csv->RegisterGenerator(&passthrough, "passthrough");
  static OzzOptimizer optimize;
  success &= _ozz2csv->RegisterGenerator(&optimize, "optimize");
  static OzzRuntime runtime;
  success &= _ozz2csv->RegisterGenerator(&runtime, "runtime");

  return success;
}
