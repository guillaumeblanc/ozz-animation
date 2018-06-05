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

#ifndef OZZ_SAMPLES_FRAMEWORK_UTILS_H_
#define OZZ_SAMPLES_FRAMEWORK_UTILS_H_

#include "ozz/base/containers/vector.h"
#include "ozz/base/platform.h"

namespace ozz {
// Forward declarations.
namespace math {
struct Box;
struct Float4x4;
}  // namespace math
namespace animation {
class Animation;
class Skeleton;
class FloatTrack;
namespace offline {
struct RawAnimation;
struct RawSkeleton;
}  // namespace offline
}  // namespace animation
namespace sample {
class ImGui;
struct Mesh;

// Utility class that helps with controlling animation playback time. Time is
// computed every update according to the dt given by the caller, playback speed
// and "play" state.
// Internally time is stored as a ratio in unit interval [0,1], as expected by
// ozz runtime animation jobs.
// OnGui function allows to tweak controller parameters through the application
// Gui.
class PlaybackController {
 public:
  // Constructor.
  PlaybackController();

  // Sets animation current time.
  void set_time_ratio(float _time);

  // Gets animation current time.
  float time_ratio() const;

  // Gets animation time ratio of last update. Useful when the range between
  // previous and current frame needs to pe processed.
  float previous_time_ratio() const;

  // Sets playback speed.
  void set_playback_speed(float _speed) { playback_speed_ = _speed; }

  // Gets playback speed.
  float playback_speed() const { return playback_speed_; }

  // Sets loop modes. If true, animation time is always clamped between 0 and 1.
  void set_loop(bool _loop) { loop_ = _loop; }

  // Gets loop mode.
  bool loop() const { return loop_; }

  // Updates animation time if in "play" state, according to playback speed and
  // given frame time _dt.
  // Returns true if animation has looped during update
  void Update(const animation::Animation& _animation, float _dt);

  // Resets all parameters to their default value.
  void Reset();

  // Do controller Gui.
  // Returns true if animation time has been changed.
  bool OnGui(const animation::Animation& _animation, ImGui* _im_gui,
             bool _enabled = true, bool _allow_set_time = true);

 private:
  // Current animation time ratio, in the unit interval [0,1], where 0 is the
  // beginning of the animation, 1 is the end.
  float time_ratio_;

  // Time ratio of the previous update.
  float previous_time_ratio_;

  // Playback speed, can be negative in order to play the animation backward.
  float playback_speed_;

  // Animation play mode state: play/pause.
  bool play_;

  // Animation loop mode.
  bool loop_;
};

// Computes the bounding box of _skeleton. This is the box that encloses all
// skeleton's joints in model space.
// _bound must be a valid math::Box instance.
void ComputeSkeletonBounds(const animation::Skeleton& _skeleton,
                           math::Box* _bound);

// Computes the bounding box of posture defines be _matrices range.
// _bound must be a valid math::Box instance.
void ComputePostureBounds(ozz::Range<const ozz::math::Float4x4> _matrices,
                          math::Box* _bound);

// Loads a skeleton from an ozz archive file named _filename.
// This function will fail and return false if the file cannot be opened or if
// it is not a valid ozz skeleton archive. A valid skeleton archive can be
// produced with ozz tools (fbx2ozz) or using ozz skeleton serialization API.
// _filename and _skeleton must be non-NULL.
bool LoadSkeleton(const char* _filename, ozz::animation::Skeleton* _skeleton);

// Loads an animation from an ozz archive file named _filename.
// This function will fail and return false if the file cannot be opened or if
// it is not a valid ozz animation archive. A valid animation archive can be
// produced with ozz tools (fbx2ozz) or using ozz animation serialization API.
// _filename and _animation must be non-NULL.
bool LoadAnimation(const char* _filename,
                   ozz::animation::Animation* _animation);

// Loads a float track from an ozz archive file named _filename.
// This function will fail and return false if the file cannot be opened or if
// it is not a valid ozz float track archive. A valid float track archive can be
// produced with ozz tools (fbx2ozz) or using ozz serialization API.
// _filename and _track must be non-NULL.
bool LoadTrack(const char* _filename, ozz::animation::FloatTrack* _track);

// Loads a sample::Mesh from an ozz archive file named _filename.
// This function will fail and return false if the file cannot be opened or if
// it is not a valid ozz mesh archive. A valid mesh archive can be
// serialization API.
// _filename and _mesh must be non-NULL.
bool LoadMesh(const char* _filename, ozz::sample::Mesh* _mesh);

// Loads n sample::Mesh from an ozz archive file named _filename.
// This function will fail and return false if the file cannot be opened or if
// it is not a valid ozz mesh archive. A valid mesh archive can be
// produced with ozz tools (fbx2skin) or using ozz animation serialization API.
// _filename and _mesh must be non-NULL.
bool LoadMeshes(const char* _filename,
                ozz::Vector<ozz::sample::Mesh>::Std* _meshes);
}  // namespace sample
}  // namespace ozz
#endif  // OZZ_SAMPLES_FRAMEWORK_UTILS_H_
