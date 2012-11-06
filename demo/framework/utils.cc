//============================================================================//
// Copyright (c) <2012> <Guillaume Blanc>                                     //
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
//============================================================================//

#include "framework/utils.h"

#include <limits>
#include <cassert>

#include "ozz/base/maths/box.h"
#include "ozz/base/maths/simd_math.h"
#include "ozz/base/memory/allocator.h"
#include "ozz/animation/animation.h"
#include "ozz/animation/skeleton.h"
#include "ozz/animation/local_to_model_job.h"
#include "ozz/animation/utils.h"

#include "framework/imgui.h"

namespace ozz {
namespace demo {

PlaybackController::PlaybackController()
  : time_(0.f),
    playback_speed_(1.f),
    play_(true) {
}

PlaybackController::~PlaybackController() {
}

void PlaybackController::Update(const animation::Animation& _animation,
                                float _dt) {
  if (!play_) {
    return;
  }
  const float new_time = time_ + _dt * playback_speed_;
  const float loops = new_time / _animation.duration();
  time_ = new_time - floorf(loops) * _animation.duration();
}

void PlaybackController::OnGui(const animation::Animation& _animation,
                               ImGui* _im_gui) {
  if (_im_gui->DoButton(play_ ? "Pause" : "Play")) {
    play_ = !play_;
  }
  char szLabel[64];
  std::sprintf(szLabel, "Animation time: %.2f", time_);
  if (_im_gui->DoSlider(szLabel, 0.f, _animation.duration(), &time_)) {
    // Pause the time if slider as moved.
    play_ = false;
  }
  std::sprintf(szLabel, "Playback speed: %.2f", playback_speed_);
  _im_gui->DoSlider(szLabel, -5.f, 5.f, &playback_speed_);

  // Allow to reset speed if it is not the default value.
  if (_im_gui->DoButton("Reset playback speed", playback_speed_ != 1.f)) {
    playback_speed_ = 1.f;
  }
}

// Uses LocalToModelJob to compute skeleton model space posture, then forwards
// to ComputePostureBounds
bool ComputeSkeletonBounds(const animation::Skeleton& _skeleton,
                           math::Box* _bound) {
  using ozz::math::Float4x4;

  if (!_bound) {
    return false;
  }

  const int num_joints = _skeleton.num_joints();
  if (!num_joints) {
    *_bound = math::Box();
    return true;
  }

  // Allocate matrix array, out of memory is handled by the LocalToModelJob.
  memory::Allocator& allocator = memory::default_allocator();
  ozz::animation::ModelsAlloc models = ozz::animation::AllocateModels(
    &allocator, num_joints);
  if (!models.begin) {
    return false;
  }

  // Compute model space bind pose.
  ozz::animation::LocalToModelJob job;
  job.input_begin = _skeleton.bind_pose();
  job.input_end = _skeleton.bind_pose_end();
  job.output_begin = models.begin;
  job.output_end = models.end;
  job.skeleton = &_skeleton;
  bool success = false;
  if (job.Run()) {
    // Forwards to posture function.
    success = ComputePostureBounds(models.begin, models.end, _bound);
  }

  allocator.Deallocate(models.begin);
  return success;
}

// Loop through matrices and collect min and max bounds.
bool ComputePostureBounds(const ozz::math::Float4x4* _begin,
                          const ozz::math::Float4x4* _end,
                          math::Box* _bound) {
  if (!_bound || !_begin || !_end) {
    return false;
  }
  if (_begin > _end) {
    return false;
  }

  math::SimdFloat4 min =
    math::simd_float4::Load1(std::numeric_limits<float>::max());
  math::SimdFloat4 max = -min;
  const ozz::math::Float4x4* current = _begin;
  while (current < _end) {
    min = math::Min(min, current->cols[3]);
    max = math::Max(max, current->cols[3]);
    current++;
  }

  math::Store3PtrU(min, &_bound->min.x);
  math::Store3PtrU(max, &_bound->max.x);

  return true;
}
}  // demo
}  // ozz
