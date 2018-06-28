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

#include "framework/utils.h"

#include <cassert>
#include <limits>

#include "ozz/base/maths/box.h"
#include "ozz/base/maths/simd_math.h"
#include "ozz/base/memory/allocator.h"

#include "ozz/animation/runtime/animation.h"
#include "ozz/animation/runtime/local_to_model_job.h"
#include "ozz/animation/runtime/skeleton.h"
#include "ozz/animation/runtime/track.h"

#include "ozz/geometry/runtime/skinning_job.h"

#include "ozz/base/io/archive.h"
#include "ozz/base/io/stream.h"
#include "ozz/base/log.h"

#include "framework/imgui.h"
#include "framework/mesh.h"

namespace ozz {
namespace sample {

PlaybackController::PlaybackController()
    : time_ratio_(0.f),
      previous_time_ratio_(0.f),
      playback_speed_(1.f),
      play_(true),
      loop_(true) {}

void PlaybackController::Update(const animation::Animation& _animation,
                                float _dt) {
  float new_time = time_ratio_;

  if (play_) {
    new_time = time_ratio_ + _dt * playback_speed_ / _animation.duration();
  }

  // Must be called even if time doesn't change, in order to update previous
  // frame time ratio. Uses set_time_ratio function in order to update
  // previous_time_ an wrap time value in the unit interval (depending on loop
  // mode).
  set_time_ratio(new_time);
}

void PlaybackController::set_time_ratio(float _time) {
  previous_time_ratio_ = time_ratio_;
  if (loop_) {
    // Wraps in the unit interval [0:1], even for negative values (the reason
    // for using floorf).
    time_ratio_ = _time - floorf(_time);
  } else {
    // Clamps in the unit interval [0:1].
    time_ratio_ = math::Clamp(0.f, _time, 1.f);
  }
}

// Gets animation current time.
float PlaybackController::time_ratio() const { return time_ratio_; }

// Gets animation time of last update.
float PlaybackController::previous_time_ratio() const {
  return previous_time_ratio_;
}

void PlaybackController::Reset() {
  previous_time_ratio_ = time_ratio_ = 0.f;
  playback_speed_ = 1.f;
  play_ = true;
}

bool PlaybackController::OnGui(const animation::Animation& _animation,
                               ImGui* _im_gui, bool _enabled,
                               bool _allow_set_time) {
  bool time_changed = false;

  if (_im_gui->DoButton(play_ ? "Pause" : "Play", _enabled)) {
    play_ = !play_;
  }

  _im_gui->DoCheckBox("Loop", &loop_, _enabled);

  char szLabel[64];
  std::sprintf(szLabel, "Animation time: %.2f", time_ratio_);

  // Uses a local copy of time_ so that set_time is used to actually apply
  // changes. Otherwise previous time would be incorrect.
  float time = this->time_ratio() * _animation.duration();
  if (_im_gui->DoSlider(szLabel, 0.f, _animation.duration(), &time, 1.f,
                        _enabled && _allow_set_time)) {
    set_time_ratio(time / _animation.duration());
    // Pause the time if slider as moved.
    play_ = false;
    time_changed = true;
  }
  std::sprintf(szLabel, "Playback speed: %.2f", playback_speed_);
  _im_gui->DoSlider(szLabel, -5.f, 5.f, &playback_speed_, 1.f, _enabled);

  // Allow to reset speed if it is not the default value.
  if (_im_gui->DoButton("Reset playback speed",
                        playback_speed_ != 1.f && _enabled)) {
    playback_speed_ = 1.f;
  }
  return time_changed;
}

// Uses LocalToModelJob to compute skeleton model space posture, then forwards
// to ComputePostureBounds
void ComputeSkeletonBounds(const animation::Skeleton& _skeleton,
                           math::Box* _bound) {
  using ozz::math::Float4x4;

  assert(_bound);

  // Set a default box.
  *_bound = ozz::math::Box();

  const int num_joints = _skeleton.num_joints();
  if (!num_joints) {
    return;
  }

  // Allocate matrix array, out of memory is handled by the LocalToModelJob.
  memory::Allocator* allocator = memory::default_allocator();
  ozz::Range<ozz::math::Float4x4> models =
      allocator->AllocateRange<ozz::math::Float4x4>(num_joints);
  if (!models.begin) {
    return;
  }

  // Compute model space bind pose.
  ozz::animation::LocalToModelJob job;
  job.input = _skeleton.bind_pose();
  job.output = models;
  job.skeleton = &_skeleton;
  if (job.Run()) {
    // Forwards to posture function.
    ComputePostureBounds(models, _bound);
  }

  allocator->Deallocate(models);
}

// Loop through matrices and collect min and max bounds.
void ComputePostureBounds(ozz::Range<const ozz::math::Float4x4> _matrices,
                          math::Box* _bound) {
  assert(_bound);

  // Set a default box.
  *_bound = ozz::math::Box();

  if (!_matrices.begin || !_matrices.end) {
    return;
  }
  if (_matrices.begin > _matrices.end) {
    return;
  }

  // Loops through matrices and stores min/max.
  // Matrices array cannot be empty, it was checked at the beginning of the
  // function.
  const ozz::math::Float4x4* current = _matrices.begin;
  math::SimdFloat4 min = current->cols[3];
  math::SimdFloat4 max = current->cols[3];
  ++current;
  while (current < _matrices.end) {
    min = math::Min(min, current->cols[3]);
    max = math::Max(max, current->cols[3]);
    ++current;
  }

  // Stores in math::Box structure.
  math::Store3PtrU(min, &_bound->min.x);
  math::Store3PtrU(max, &_bound->max.x);

  return;
}

bool LoadSkeleton(const char* _filename, ozz::animation::Skeleton* _skeleton) {
  assert(_filename && _skeleton);
  ozz::log::Out() << "Loading skeleton archive " << _filename << "."
                  << std::endl;
  ozz::io::File file(_filename, "rb");
  if (!file.opened()) {
    ozz::log::Err() << "Failed to open skeleton file " << _filename << "."
                    << std::endl;
    return false;
  }
  ozz::io::IArchive archive(&file);
  if (!archive.TestTag<ozz::animation::Skeleton>()) {
    ozz::log::Err() << "Failed to load skeleton instance from file "
                    << _filename << "." << std::endl;
    return false;
  }

  // Once the tag is validated, reading cannot fail.
  archive >> *_skeleton;

  return true;
}

bool LoadAnimation(const char* _filename,
                   ozz::animation::Animation* _animation) {
  assert(_filename && _animation);
  ozz::log::Out() << "Loading animation archive: " << _filename << "."
                  << std::endl;
  ozz::io::File file(_filename, "rb");
  if (!file.opened()) {
    ozz::log::Err() << "Failed to open animation file " << _filename << "."
                    << std::endl;
    return false;
  }
  ozz::io::IArchive archive(&file);
  if (!archive.TestTag<ozz::animation::Animation>()) {
    ozz::log::Err() << "Failed to load animation instance from file "
                    << _filename << "." << std::endl;
    return false;
  }

  // Once the tag is validated, reading cannot fail.
  archive >> *_animation;

  return true;
}

bool LoadTrack(const char* _filename, ozz::animation::FloatTrack* _track) {
  assert(_filename && _track);
  ozz::log::Out() << "Loading track archive: " << _filename << "." << std::endl;
  ozz::io::File file(_filename, "rb");
  if (!file.opened()) {
    ozz::log::Err() << "Failed to open track file " << _filename << "."
                    << std::endl;
    return false;
  }
  ozz::io::IArchive archive(&file);
  if (!archive.TestTag<ozz::animation::FloatTrack>()) {
    ozz::log::Err() << "Failed to load float track instance from file "
                    << _filename << "." << std::endl;
    return false;
  }

  // Once the tag is validated, reading cannot fail.
  archive >> *_track;

  return true;
}

bool LoadMesh(const char* _filename, ozz::sample::Mesh* _mesh) {
  assert(_filename && _mesh);
  ozz::log::Out() << "Loading mesh archive: " << _filename << "." << std::endl;
  ozz::io::File file(_filename, "rb");
  if (!file.opened()) {
    ozz::log::Err() << "Failed to open mesh file " << _filename << "."
                    << std::endl;
    return false;
  }
  ozz::io::IArchive archive(&file);
  if (!archive.TestTag<ozz::sample::Mesh>()) {
    ozz::log::Err() << "Failed to load mesh instance from file " << _filename
                    << "." << std::endl;
    return false;
  }

  // Once the tag is validated, reading cannot fail.
  archive >> *_mesh;

  return true;
}

bool LoadMeshes(const char* _filename,
                ozz::Vector<ozz::sample::Mesh>::Std* _meshes) {
  assert(_filename && _meshes);
  ozz::log::Out() << "Loading meshes archive: " << _filename << "."
                  << std::endl;
  ozz::io::File file(_filename, "rb");
  if (!file.opened()) {
    ozz::log::Err() << "Failed to open mesh file " << _filename << "."
                    << std::endl;
    return false;
  }
  ozz::io::IArchive archive(&file);

  while (archive.TestTag<ozz::sample::Mesh>()) {
    _meshes->resize(_meshes->size() + 1);
    archive >> _meshes->back();
  }

  return true;
}
}  // namespace sample
}  // namespace ozz
