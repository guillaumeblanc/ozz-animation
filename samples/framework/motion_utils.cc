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
void MotionAccumulator::Update(const ozz::math::Transform& _new) {
  const ozz::math::Float3 delta_p = _new.translation - last.translation;
  current.translation = current.translation + delta_p;

  const ozz::math::Quaternion delta_r =
      Conjugate(last.rotation) * _new.rotation;
  current.rotation = Normalize(current.rotation *
                               delta_r);  // Avoid accumulating error that leads
                                          // to non-normalized quaternions.
  last = _new;
}

// Updates the accumulator with a new motion sample.
bool MotionAccumulator::Update(const MotionTrack& _motion, float _ratio,
                               int _loops) {
  ozz::math::Transform motion_transform;
  for (; _loops; _loops > 0 ? --_loops : ++_loops) {
    // When animation is looping, it's important to take into account the
    // motion done during the loop.

    // Samples motion at the loop end (begin or end of animation depending on
    // playback/loop direction).
    if (!SampleMotion(_motion, _loops > 0 ? 1.f : 0.f, &motion_transform)) {
      return false;
    }
    Update(motion_transform);

    // Samples motion at the new origin (begin or end of animation depending
    // on playback/loop direction).
    if (!SampleMotion(_motion, _loops > 0 ? 0.f : 1.f, &motion_transform)) {
      return false;
    }
    ResetOrigin(motion_transform);
  }

  // Samples motion at the current ratio (from last known position, or the
  // reset one).
  if (!SampleMotion(_motion, _ratio, &motion_transform)) {
    return false;
  }
  Update(motion_transform);

  return true;
}

bool DrawMotion(ozz::sample::Renderer* _renderer,
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
