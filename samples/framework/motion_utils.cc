//----------------------------------------------------------------------------//
//                                                                            //
// ozz-animation is hosted at http://github.com/guillaumeblanc/ozz-animation  //
// and distributed under the MIT License (MIT).                               //
//                                                                            //
// Copyright (c) Guillaume Blanc                                              //
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

#include "framework/motion_utils.h"

#include <cassert>

#include "framework/renderer.h"
#include "framework/utils.h"
#include "ozz/animation/runtime/track_sampling_job.h"
#include "ozz/base/containers/vector.h"
#include "ozz/base/io/archive.h"
#include "ozz/base/io/stream.h"
#include "ozz/base/log.h"
#include "ozz/base/maths/simd_math.h"
#include "ozz/base/maths/transform.h"

namespace ozz {
namespace sample {

bool LoadMotionTrack(const char* _filename, MotionTrack* _track) {
  assert(_filename && _track);
  ozz::log::Out() << "Loading motion tracks archive: " << _filename << "."
                  << std::endl;
  ozz::io::File file(_filename, "rb");
  if (!file.opened()) {
    ozz::log::Err() << "Failed to open motion tracks file " << _filename << "."
                    << std::endl;
    return false;
  }
  ozz::io::IArchive archive(&file);

  // Once the tag is validated, reading cannot fail.
  {
    ProfileFctLog profile{"Motion tracks loading time"};

    if (!archive.TestTag<ozz::animation::Float3Track>()) {
      ozz::log::Err()
          << "Failed to load position motion track instance from file "
          << _filename << "." << std::endl;
      return false;
    }
    archive >> _track->position;

    if (!archive.TestTag<ozz::animation::QuaternionTrack>()) {
      ozz::log::Err()
          << "Failed to load rotation motion track instance from file "
          << _filename << "." << std::endl;
      return false;
    }
    archive >> _track->rotation;
  }

  return true;
}

bool SampleMotion(const MotionTrack& _tracks, float _ratio,
                  ozz::math::Transform* _transform) {
  // Get position from motion track
  ozz::animation::Float3TrackSamplingJob position_sampler;
  position_sampler.track = &_tracks.position;
  position_sampler.result = &_transform->translation;
  position_sampler.ratio = _ratio;
  if (!position_sampler.Run()) {
    return false;
  }

  // Get rotation from motion track
  ozz::animation::QuaternionTrackSamplingJob rotation_sampler;
  rotation_sampler.track = &_tracks.rotation;
  rotation_sampler.result = &_transform->rotation;
  rotation_sampler.ratio = _ratio;
  if (!rotation_sampler.Run()) {
    return false;
  }

  _transform->scale = ozz::math::Float3::one();

  return true;
}

// Accumulates motion deltas (new transform - last).
void MotionAccumulator::Update(const ozz::math::Transform& _new,
                               const ozz::math::Quaternion& _rot) {
  // Accumulates rotation.
  // Normalizes to avoid accumulating error.
  accum_rotation = Normalize(accum_rotation * _rot);

  // Computes delta translation.
  const ozz::math::Float3 delta_p = _new.translation - last.translation;
  current.translation =
      current.translation + TransformVector(accum_rotation, delta_p);

  // Computes delta rotation.
  const ozz::math::Quaternion delta_r =
      Conjugate(last.rotation) * _new.rotation;
  current.rotation =
      Normalize(current.rotation * delta_r *
                _rot);  // Normalizes to avoid accumulating error.

  // Next time, delta will be computed from the _new transform.
  last = _new;
}

void MotionAccumulator::ResetOrigin(const ozz::math::Transform& _origin) {
  last = _origin;
}

void MotionAccumulator::Teleport(const ozz::math::Transform& _origin) {
  // Resets current transform to new _origin
  current = last = _origin;

  accum_rotation = ozz::math::Quaternion::identity();
}

// Updates the accumulator with a new motion sample.
bool MotionSampler::Update(const MotionTrack& _motion, float _ratio, int _loops,
                           const ozz::math::Quaternion& _rot) {
  ozz::math::Transform sample;

  if (_loops != 0) {
    // When animation is looping, it's important to take into account the
    // motion done during the loop(s).

    // Uses a local accumulator to accumulate motion during loops.
    MotionAccumulator local_accumulator{last, last};

    for (; _loops; _loops > 0 ? --_loops : ++_loops) {
      // Samples motion at loop end (or begin depending on playback direction).
      if (!SampleMotion(_motion, _loops > 0 ? 1.f : 0.f, &sample)) {
        return false;
      }
      local_accumulator.Update(sample, ozz::math::Quaternion::identity());

      // Samples motion at the new origin.
      if (!SampleMotion(_motion, _loops > 0 ? 0.f : 1.f, &sample)) {
        return false;
      }
      local_accumulator.ResetOrigin(sample);
    }

    // Samples motion at the current ratio (from last known position, or the
    // reset one).
    if (!SampleMotion(_motion, _ratio, &sample)) {
      return false;
    }
    local_accumulator.Update(sample, ozz::math::Quaternion::identity());

    // Update "this" accumulator with the one accumulated during the loop(s).
    // This way, _rot is applied to the whole motion, including what happened
    // during the loop(s).
    MotionAccumulator::Update(local_accumulator.current, _rot);

    // Next time, delta will be computed from the new origin, aka after the
    // loop.
    ResetOrigin(sample);
  } else {
    // Samples motion at the current ratio (from last known position, or the
    // reset one).
    if (!SampleMotion(_motion, _ratio, &sample)) {
      return false;
    }
    // Apply motion
    MotionAccumulator::Update(sample, _rot);
  }

  return true;
}

bool DrawMotion(ozz::sample::Renderer* _renderer,
                const MotionTrack& _motion_track, float _from, float _at,
                float _to, float _step, const ozz::math::Float4x4& _transform,
                const ozz::math::Quaternion& _rot) {
  if (_step <= 0.f || _to <= _from) {
    return false;
  }

  bool success = true;

  // Find track current transform in order to correctly place the motion at
  // character transform.
  ozz::math::Transform at_transform;
  SampleMotion(_motion_track, _at, &at_transform);
  const auto transform =
      _transform * Invert(ozz::math::Float4x4::FromAffine(at_transform));

  // const float step = 1.f / (_loop_duration * 100.f);  // xHz sampling
  // auto rot = SLerp(ozz::math::Quaternion::identity(), _rot, _step);
  // auto rot = _rot;

  MotionSampler sampler;
  ozz::vector<ozz::math::Float3> points;
  auto sample = [&sampler, &_motion_track, &points](
                    float _t, float _prev, const ozz::math::Quaternion& _rot) {
    int loop = static_cast<int>(std::floor(_t) - std::floor(_prev));
    bool success =
        sampler.Update(_motion_track, _t - std::floor(_t), loop, _rot);
    points.push_back(sampler.current.translation);
    return success;
  };

  // Present to past
  sampler.Teleport(at_transform);
  for (float t = _at, prev = t; t > _from - _step; prev = t, t -= _step) {
    success &= sample(ozz::math::Max(t, _from), prev, Conjugate(_rot));
  }
  success &= _renderer->DrawLineStrip(make_span(points), ozz::sample::kGreen,
                                      transform);

  // Present to future
  points.clear();
  sampler.Teleport(at_transform);
  for (float t = _at, prev = t; t < _to + _step; prev = t, t += _step) {
    success &= sample(ozz::math::Min(t, _to), prev, _rot);
  }
  success &= _renderer->DrawLineStrip(make_span(points), ozz::sample::kWhite,
                                      transform);

  return success;
}

bool DrawMotion2(ozz::sample::Renderer* _renderer,
                 const MotionTrack& _motion_track, float _at, float _duration,
                 const ozz::math::Float4x4& _transform) {
  if (_duration <= 0.f) {
    return false;
  }

  // Clamps _at to [0, 1]
  _at = ozz::math::Clamp(0.f, _at, 1.f);

  // Find track current transform in order to correctly place the motion at
  // character transform.
  ozz::math::Transform at_transform;
  SampleMotion(_motion_track, _at, &at_transform);
  const auto transform =
      _transform * Invert(ozz::math::Float4x4::FromAffine(at_transform));

  // Samples points along the track.
  ozz::vector<ozz::math::Float3> points;
  auto sample = [&track = _motion_track.position, &points](float _t) {
    ozz::math::Float3 point;
    ozz::animation::Float3TrackSamplingJob sampler;
    sampler.track = &track;
    sampler.result = &point;
    sampler.ratio = _t;
    if (!sampler.Run()) {
      return false;
    }
    points.push_back(point);
    return true;
  };

  size_t at = 0;
  const float step = 1.f / (_duration * 30.f);  // 30Hz sampling
  for (float t = 0.f; t < 1.f + step; t += step) {
    if (t > _at && t <= _at + step) {  // Inject a point at t = _at
      at = points.size();
      if (!sample(_at)) {
        return false;
      }
    }
    if (!sample(t)) {
      return false;
    }
  }

  // Draws from begin to _at (+1 to include _at)
  const size_t end = points.size();
  assert(at + 1 <= end);
  if (!_renderer->DrawLineStrip(ozz::span(points.data(), at + 1),
                                ozz::sample::kGreen, transform)) {
    return false;
  }

  // Draws from _at to end
  if (!_renderer->DrawLineStrip(ozz::span(points.data() + at, end - at),
                                ozz::sample::kWhite, transform)) {
    return false;
  }

  return true;
}

}  // namespace sample
}  // namespace ozz
