//============================================================================//
//                                                                            //
// ozz-animation, 3d skeletal animation libraries and tools.                  //
// https://code.google.com/p/ozz-animation/                                   //
//                                                                            //
//----------------------------------------------------------------------------//
//                                                                            //
// Copyright (c) 2012-2015 Guillaume Blanc                                    //
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

#include "ozz/animation/offline/animation_optimizer.h"

#include <cstddef>
#include <cassert>

#include "ozz/base/maths/math_constant.h"

#include "ozz/animation/offline/raw_animation.h"

namespace ozz {
namespace animation {
namespace offline {

// Setup default values (favoring quality).
AnimationOptimizer::AnimationOptimizer()
  : translation_tolerance(1e-3f),  // 1 mm.
    rotation_tolerance(.1f * math::kPi / 180.f),  // 0.1 degree.
    scale_tolerance(1e-3f) {  // 0.1%.
}

namespace {
// Copy _src keys to _dest but except the ones that can be interpolated.
template<typename _RawTrack, typename _Comparator, typename _Lerp>
void Filter(const _RawTrack& _src,
            const _Comparator& _comparator,
            const _Lerp& _lerp,
            float _tolerance,
            _RawTrack* _dest) {
  _dest->clear();  // Reset and reserve destination.
  _dest->reserve(_src.size());

  // Only copies the key that cannot be interpolated from the others.
  size_t last_src_pushed = 0;  // Index (in src) of the last pushed key.
  for (size_t i = 0; i < _src.size(); ++i) {
    // First and last keys are always pushed.
    if (i == 0 || i == _src.size() - 1) {
      _dest->push_back(_src[i]);
      last_src_pushed = i;
    } else {
      // Only inserts i key if keys in range ]last_src_pushed,i] cannot be
      // interpolated from keys last_src_pushed and i + 1.
      typename _RawTrack::const_reference left = _src[last_src_pushed];
      typename _RawTrack::const_reference right = _src[i + 1];
      for (size_t j = last_src_pushed + 1; j <= i; ++j) {
        typename _RawTrack::const_reference test = _src[j];
        const float alpha = (test.time - left.time) / (right.time - left.time);
        assert(alpha >= 0.f && alpha <= 1.f);
        if (!_comparator(_lerp(left.value, right.value, alpha),
                         test.value,
                         _tolerance)) {
          _dest->push_back(_src[i]);
          last_src_pushed = i;
          break;
        }
      }
    }
  }
  assert(_dest->size() <= _src.size());
}

// Translation filtering comparator.
bool CompareTranslation(const math::Float3& _a,
                   const math::Float3& _b,
                   float _tolerance) {
  return Compare(_a, _b, _tolerance);
}

// Translation interpolation method.
// This must be the same Lerp as the one used by the sampling job.
math::Float3 LerpTranslation(const math::Float3& _a,
                             const math::Float3& _b,
                             float _alpha) {
  return math::Lerp(_a, _b, _alpha);
}

// Rotation filtering comparator.
bool CompareRotation(const math::Quaternion& _a,
                     const math::Quaternion& _b,
                     float _tolerance) {
  return Compare(_a, _b, _tolerance);
}

// Rotation interpolation method.
// This must be the same Lerp as the one used by the sampling job.
math::Quaternion LerpRotation(const math::Quaternion& _a,
                              const math::Quaternion& _b,
                              float _alpha) {
  return math::NLerp(_a, _b, _alpha);
}

// Scale filtering comparator.
bool CompareScale(const math::Float3& _a,
                  const math::Float3& _b,
                  float _tolerance) {
  return Compare(_a, _b, _tolerance);
}

// Scale interpolation method.
// This must be the same Lerp as the one used by the sampling job.
math::Float3 LerpScale(const math::Float3& _a,
                       const math::Float3& _b,
                       float _alpha) {
  return math::Lerp(_a, _b, _alpha);
}
}  // namespace

bool AnimationOptimizer::operator()(const RawAnimation& _input,
                                    RawAnimation* _output) const {
  if (!_output) {
    return false;
  }
  // Reset output animation to default.
  *_output = RawAnimation();

  // Validate animation.
  if (!_input.Validate()) {
    return false;
  }

  // Rebuilds output animation.
  _output->duration = _input.duration;
  int num_tracks = _input.num_tracks();
  _output->tracks.resize(num_tracks);
  for (int i = 0; i < num_tracks; ++i) {
    Filter(_input.tracks[i].translations,
           CompareTranslation, LerpTranslation, translation_tolerance,
           &_output->tracks[i].translations);
    Filter(_input.tracks[i].rotations,
           CompareRotation, LerpRotation, rotation_tolerance,
           &_output->tracks[i].rotations);
    Filter(_input.tracks[i].scales,
           CompareScale, LerpScale, scale_tolerance,
           &_output->tracks[i].scales);
  }
  // Output animation is always valid.
  assert(_output->Validate());

  return true;
}
}  // offline
}  // animation
}  // ozz
