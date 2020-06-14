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

#include "framework/utils.h"

#include <cassert>
#include <limits>

#include "ozz/base/maths/box.h"
#include "ozz/base/maths/simd_math.h"
#include "ozz/base/maths/simd_quaternion.h"
#include "ozz/base/maths/soa_transform.h"

#include "ozz/base/memory/allocator.h"

#include "ozz/animation/runtime/animation.h"
#include "ozz/animation/runtime/local_to_model_job.h"
#include "ozz/animation/runtime/skeleton.h"
#include "ozz/animation/runtime/track.h"

#include "ozz/animation/offline/raw_skeleton.h"

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

void PlaybackController::set_time_ratio(float _ratio) {
  previous_time_ratio_ = time_ratio_;
  if (loop_) {
    // Wraps in the unit interval [0:1], even for negative values (the reason
    // for using floorf).
    time_ratio_ = _ratio - floorf(_ratio);
  } else {
    // Clamps in the unit interval [0:1].
    time_ratio_ = math::Clamp(0.f, _ratio, 1.f);
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

  // Uses a local copy of time_ so that set_time is used to actually apply
  // changes. Otherwise previous time would be incorrect.
  float ratio = time_ratio();
  std::sprintf(szLabel, "Animation time: %.2f", ratio * _animation.duration());
  if (_im_gui->DoSlider(szLabel, 0.f, 1.f, &ratio, 1.f,
                        _enabled && _allow_set_time)) {
    set_time_ratio(ratio);
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

namespace {
bool OnRawSkeletonJointGui(
    ozz::sample::ImGui* _im_gui,
    ozz::animation::offline::RawSkeleton::Joint::Children* _children,
    ozz::vector<bool>::iterator* _oc_state) {
  char txt[255];

  bool modified = false;
  for (size_t i = 0; i < _children->size(); ++i) {
    ozz::animation::offline::RawSkeleton::Joint& joint = _children->at(i);

    bool opened = *(*_oc_state);
    ozz::sample::ImGui::OpenClose oc(_im_gui, joint.name.c_str(), &opened);
    *(*_oc_state)++ = opened;  // Updates state and increment for next joint.
    if (opened) {
      // Translation
      ozz::math::Float3& translation = joint.transform.translation;
      _im_gui->DoLabel("Translation");
      sprintf(txt, "x %.2g", translation.x);
      modified |= _im_gui->DoSlider(txt, -1.f, 1.f, &translation.x);
      sprintf(txt, "y %.2g", translation.y);
      modified |= _im_gui->DoSlider(txt, -1.f, 1.f, &translation.y);
      sprintf(txt, "z %.2g", translation.z);
      modified |= _im_gui->DoSlider(txt, -1.f, 1.f, &translation.z);

      // Rotation (in euler form)
      ozz::math::Quaternion& rotation = joint.transform.rotation;
      _im_gui->DoLabel("Rotation");
      ozz::math::Float3 euler = ToEuler(rotation) * ozz::math::kRadianToDegree;
      sprintf(txt, "x %.3g", euler.x);
      bool euler_modified = _im_gui->DoSlider(txt, -180.f, 180.f, &euler.x);
      sprintf(txt, "y %.3g", euler.y);
      euler_modified |= _im_gui->DoSlider(txt, -180.f, 180.f, &euler.y);
      sprintf(txt, "z %.3g", euler.z);
      euler_modified |= _im_gui->DoSlider(txt, -180.f, 180.f, &euler.z);
      if (euler_modified) {
        modified = true;
        ozz::math::Float3 euler_rad = euler * ozz::math::kDegreeToRadian;
        rotation = ozz::math::Quaternion::FromEuler(euler_rad.x, euler_rad.y,
                                                    euler_rad.z);
      }

      // Scale (must be uniform and not 0)
      _im_gui->DoLabel("Scale");
      ozz::math::Float3& scale = joint.transform.scale;
      sprintf(txt, "%.2g", scale.x);
      if (_im_gui->DoSlider(txt, -1.f, 1.f, &scale.x)) {
        modified = true;
        scale.y = scale.z = scale.x = scale.x != 0.f ? scale.x : .01f;
      }
      // Recurse children
      modified |= OnRawSkeletonJointGui(_im_gui, &joint.children, _oc_state);
    }
  }
  return modified;
}
}  // namespace

bool RawSkeletonEditor::OnGui(animation::offline::RawSkeleton* _skeleton,
                              ImGui* _im_gui) {
  open_close_states.resize(_skeleton->num_joints(), false);

  ozz::vector<bool>::iterator begin = open_close_states.begin();
  return OnRawSkeletonJointGui(_im_gui, &_skeleton->roots, &begin);
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
  ozz::vector<ozz::math::Float4x4> models(num_joints);

  // Compute model space bind pose.
  ozz::animation::LocalToModelJob job;
  job.input = _skeleton.joint_bind_poses();
  job.output = make_span(models);
  job.skeleton = &_skeleton;
  if (job.Run()) {
    // Forwards to posture function.
    ComputePostureBounds(job.output, _bound);
  }
}

// Loop through matrices and collect min and max bounds.
void ComputePostureBounds(ozz::span<const ozz::math::Float4x4> _matrices,
                          math::Box* _bound) {
  assert(_bound);

  // Set a default box.
  *_bound = ozz::math::Box();

  if (_matrices.empty()) {
    return;
  }

  // Loops through matrices and stores min/max.
  // Matrices array cannot be empty, it was checked at the beginning of the
  // function.
  const ozz::math::Float4x4* current = _matrices.begin();
  math::SimdFloat4 min = current->cols[3];
  math::SimdFloat4 max = current->cols[3];
  ++current;
  while (current < _matrices.end()) {
    min = math::Min(min, current->cols[3]);
    max = math::Max(max, current->cols[3]);
    ++current;
  }

  // Stores in math::Box structure.
  math::Store3PtrU(min, &_bound->min.x);
  math::Store3PtrU(max, &_bound->max.x);

  return;
}

void MultiplySoATransformQuaternion(
    int _index, const ozz::math::SimdQuaternion& _quat,
    const ozz::span<ozz::math::SoaTransform>& _transforms) {
  assert(_index >= 0 && static_cast<size_t>(_index) < _transforms.size() * 4 &&
         "joint index out of bound.");

  // Convert soa to aos in order to perform quaternion multiplication, and gets
  // back to soa.
  ozz::math::SoaTransform& soa_transform_ref = _transforms[_index / 4];
  ozz::math::SimdQuaternion aos_quats[4];
  ozz::math::Transpose4x4(&soa_transform_ref.rotation.x, &aos_quats->xyzw);

  ozz::math::SimdQuaternion& aos_quat_ref = aos_quats[_index & 3];
  aos_quat_ref = aos_quat_ref * _quat;

  ozz::math::Transpose4x4(&aos_quats->xyzw, &soa_transform_ref.rotation.x);
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

namespace {
template <typename _Track>
bool LoadTrackImpl(const char* _filename, _Track* _track) {
  assert(_filename && _track);
  ozz::log::Out() << "Loading track archive: " << _filename << "." << std::endl;
  ozz::io::File file(_filename, "rb");
  if (!file.opened()) {
    ozz::log::Err() << "Failed to open track file " << _filename << "."
                    << std::endl;
    return false;
  }
  ozz::io::IArchive archive(&file);
  if (!archive.TestTag<_Track>()) {
    ozz::log::Err() << "Failed to load float track instance from file "
                    << _filename << "." << std::endl;
    return false;
  }

  // Once the tag is validated, reading cannot fail.
  archive >> *_track;

  return true;
}
}  // namespace

bool LoadTrack(const char* _filename, ozz::animation::FloatTrack* _track) {
  return LoadTrackImpl(_filename, _track);
}
bool LoadTrack(const char* _filename, ozz::animation::Float2Track* _track) {
  return LoadTrackImpl(_filename, _track);
}
bool LoadTrack(const char* _filename, ozz::animation::Float3Track* _track) {
  return LoadTrackImpl(_filename, _track);
}
bool LoadTrack(const char* _filename, ozz::animation::Float4Track* _track) {
  return LoadTrackImpl(_filename, _track);
}
bool LoadTrack(const char* _filename, ozz::animation::QuaternionTrack* _track) {
  return LoadTrackImpl(_filename, _track);
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
                ozz::vector<ozz::sample::Mesh>* _meshes) {
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

namespace {
// Moller–Trumbore intersection algorithm
// https://en.wikipedia.org/wiki/M%C3%B6ller%E2%80%93Trumbore_intersection_algorithm
bool RayIntersectsTriangle(const ozz::math::Float3& _ray_origin,
                           const ozz::math::Float3& _ray_direction,
                           const ozz::math::Float3& _p0,
                           const ozz::math::Float3& _p1,
                           const ozz::math::Float3& _p2,
                           ozz::math::Float3* _intersect,
                           ozz::math::Float3* _normal) {
  const float kEpsilon = 0.0000001f;

  const ozz::math::Float3 edge1 = _p1 - _p0;
  const ozz::math::Float3 edge2 = _p2 - _p0;
  const ozz::math::Float3 h = Cross(_ray_direction, edge2);

  const float a = Dot(edge1, h);
  if (a > -kEpsilon && a < kEpsilon) {
    return false;  // This ray is parallel to this triangle.
  }

  const float inv_a = 1.f / a;
  const ozz::math::Float3 s = _ray_origin - _p0;
  const float u = Dot(s, h) * inv_a;
  if (u < 0.f || u > 1.f) {
    return false;
  }

  const ozz::math::Float3 q = Cross(s, edge1);
  const float v = ozz::math::Dot(_ray_direction, q) * inv_a;
  if (v < 0.f || u + v > 1.f) {
    return false;
  }

  // At this stage we can compute t to find out where the intersection point is
  // on the line.
  const float t = Dot(edge2, q) * inv_a;

  if (t > kEpsilon) {  // Ray intersection
    *_intersect = _ray_origin + _ray_direction * t;
    *_normal = Normalize(Cross(edge1, edge2));
    return true;
  } else {  // This means that there is a line intersection but not a ray
            // intersection.
    return false;
  }
}
}  // namespace

bool RayIntersectsMesh(const ozz::math::Float3& _ray_origin,
                       const ozz::math::Float3& _ray_direction,
                       const ozz::sample::Mesh& _mesh,
                       ozz::math::Float3* _intersect,
                       ozz::math::Float3* _normal) {
  assert(_mesh.parts.size() == 1 && !_mesh.skinned());

  bool intersected = false;
  ozz::math::Float3 intersect, normal;
  const float* vertices = array_begin(_mesh.parts[0].positions);
  const uint16_t* indices = array_begin(_mesh.triangle_indices);
  for (int i = 0; i < _mesh.triangle_index_count(); i += 3) {
    const float* pf0 = vertices + indices[i + 0] * 3;
    const float* pf1 = vertices + indices[i + 1] * 3;
    const float* pf2 = vertices + indices[i + 2] * 3;
    ozz::math::Float3 lcl_intersect, lcl_normal;
    if (RayIntersectsTriangle(_ray_origin, _ray_direction,
                              ozz::math::Float3(pf0[0], pf0[1], pf0[2]),
                              ozz::math::Float3(pf1[0], pf1[1], pf1[2]),
                              ozz::math::Float3(pf2[0], pf2[1], pf2[2]),
                              &lcl_intersect, &lcl_normal)) {
      // Is it closer to start point than the previous intersection.
      if (!intersected || LengthSqr(lcl_intersect - _ray_origin) <
                              LengthSqr(intersect - _ray_origin)) {
        intersect = lcl_intersect;
        normal = lcl_normal;
      }
      intersected = true;
    }
  }

  // Copy output
  if (intersected) {
    if (_intersect) {
      *_intersect = intersect;
    }
    if (_normal) {
      *_normal = normal;
    }
  }
  return intersected;
}

bool RayIntersectsMeshes(const ozz::math::Float3& _ray_origin,
                         const ozz::math::Float3& _ray_direction,
                         const ozz::span<const ozz::sample::Mesh>& _meshes,
                         ozz::math::Float3* _intersect,
                         ozz::math::Float3* _normal) {
  bool intersected = false;
  ozz::math::Float3 intersect, normal;
  for (size_t i = 0; i < _meshes.size(); ++i) {
    ozz::math::Float3 lcl_intersect, lcl_normal;
    if (RayIntersectsMesh(_ray_origin, _ray_direction, _meshes[i],
                          &lcl_intersect, &lcl_normal)) {
      // Is it closer to start point than the previous intersection.
      if (!intersected || LengthSqr(lcl_intersect - _ray_origin) <
                              LengthSqr(intersect - _ray_origin)) {
        intersect = lcl_intersect;
        normal = lcl_normal;
      }
      intersected = true;
    }
  }

  // Copy output
  if (intersected) {
    if (_intersect) {
      *_intersect = intersect;
    }
    if (_normal) {
      *_normal = normal;
    }
  }
  return intersected;
}
}  // namespace sample
}  // namespace ozz
