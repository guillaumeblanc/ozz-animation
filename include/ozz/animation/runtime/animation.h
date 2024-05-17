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

#ifndef OZZ_OZZ_ANIMATION_RUNTIME_ANIMATION_H_
#define OZZ_OZZ_ANIMATION_RUNTIME_ANIMATION_H_

#include "ozz/animation/runtime/export.h"
#include "ozz/base/io/archive_traits.h"
#include "ozz/base/platform.h"
#include "ozz/base/span.h"

namespace ozz {
namespace io {
class IArchive;
class OArchive;
}  // namespace io
namespace animation {

// Forward declares the AnimationBuilder, used to instantiate an Animation.
namespace offline {
class AnimationBuilder;
}

// Forward declaration of key frame's type.
namespace internal {
struct Float3Key;
struct QuaternionKey;
}  // namespace internal

// Defines a runtime skeletal animation clip.
// The runtime animation data structure stores animation keyframes, for all the
// joints of a skeleton. This structure is usually filled by the
// AnimationBuilder and deserialized/loaded at runtime.
// For each transformation type (translation, rotation and scale), Animation
// structure stores a single array of keyframes that contains all the tracks
// required to animate all the joints of a skeleton, matching breadth-first
// joints order of the runtime skeleton structure. In order to optimize cache
// coherency when sampling the animation, Keyframes in this array are sorted by
// time, then by track number.
class OZZ_ANIMATION_DLL Animation {
 public:
  // Builds a default animation.
  Animation() = default;

  // Allow moves.
  Animation(Animation&&);
  Animation& operator=(Animation&&);

  // Delete copies.
  Animation(Animation const&) = delete;
  Animation& operator=(Animation const&) = delete;

  // Declares the public non-virtual destructor.
  ~Animation();

  // Gets the animation clip duration.
  float duration() const { return duration_; }

  // Gets the number of animated tracks.
  int num_tracks() const { return num_tracks_; }

  // Returns the number of SoA elements matching the number of tracks of *this
  // animation. This value is useful to allocate SoA runtime data structures.
  int num_soa_tracks() const { return (num_tracks_ + 3) / 4; }

  // Gets animation name.
  const char* name() const { return name_ ? name_ : ""; }

  // Gets the buffer of time points.
  span<const float> timepoints() const { return timepoints_; }

  template <bool _Const>
  struct TKeyframesCtrl {
    size_t size_bytes() const {
      return ratios.size_bytes() + previouses.size_bytes() +
             iframe_entries.size_bytes() + iframe_desc.size_bytes();
    }

    // Implicit conversion to const.
    operator TKeyframesCtrl<true>() const {
      return {ratios, previouses, iframe_entries, iframe_desc, iframe_interval};
    }

    template <typename _Ty, bool>
    struct ConstQualifier {
      typedef const _Ty type;
    };

    template <typename _Ty>
    struct ConstQualifier<_Ty, false> {
      typedef _Ty type;
    };

    // Indices to timepoints. uint8_t or uint16_t depending on timepoints size.
    span<typename ConstQualifier<byte, _Const>::type> ratios;

    // Offsets from the previous keyframe of the same track.
    span<typename ConstQualifier<uint16_t, _Const>::type> previouses;

    // Cached iframe entries packed with GV4 encoding.
    span<typename ConstQualifier<byte, _Const>::type> iframe_entries;

    // 2 intergers per iframe:
    // 1. Offset in compressed entries
    // 2. Maximum key index (latest updated key).
    span<typename ConstQualifier<uint32_t, _Const>::type> iframe_desc;

    // Interval, used at runtime to index iframe_desc.
    float iframe_interval = 0.f;
  };

  typedef TKeyframesCtrl<true> KeyframesCtrlConst;
  typedef TKeyframesCtrl<false> KeyframesCtrl;

  // Gets the buffer of translations keys.
  KeyframesCtrlConst translations_ctrl() const { return translations_ctrl_; }
  span<const internal::Float3Key> translations_values() const {
    return translations_values_;
  }

  // Gets the buffer of rotation keys.
  KeyframesCtrlConst rotations_ctrl() const { return rotations_ctrl_; }
  span<const internal::QuaternionKey> rotations_values() const {
    return rotations_values_;
  }

  // Gets the buffer of scale keys.
  KeyframesCtrlConst scales_ctrl() const { return scales_ctrl_; }
  span<const internal::Float3Key> scales_values() const {
    return scales_values_;
  }

  // Get the estimated animation's size in bytes.
  size_t size() const;

  // Serialization functions.
  // Should not be called directly but through io::Archive << and >> operators.
  void Save(ozz::io::OArchive& _archive) const;
  void Load(ozz::io::IArchive& _archive, uint32_t _version);

 private:
  // AnimationBuilder class is allowed to instantiate an Animation.
  friend class offline::AnimationBuilder;

  // Internal memory management functions.
  struct AllocateParams {
    size_t name_len;
    size_t timepoints;

    size_t translations;
    size_t rotations;
    size_t scales;

    struct IFrames {
      size_t entries;
      size_t offsets;
    };

    IFrames translation_iframes;
    IFrames rotation_iframes;
    IFrames scale_iframes;
  };
  void Allocate(const AllocateParams& _params);
  void Deallocate();

  // Duration of the animation clip.
  float duration_ = 0.f;

  // The number of joint tracks. Can differ from the data stored in translation/
  // rotation/scale buffers because of SoA requirements.
  int num_tracks_ = 0;

  // Allocated buffer for the whole animation.
  void* allocation_ = nullptr;

  // Animation name.
  char* name_ = nullptr;

  // Stores all translation/rotation/scale keys.
  span<float> timepoints_;

  // Keyframes series controllers.
  KeyframesCtrl translations_ctrl_;
  KeyframesCtrl rotations_ctrl_;
  KeyframesCtrl scales_ctrl_;

  // Keyframes series values.
  span<internal::Float3Key> translations_values_;
  span<internal::QuaternionKey> rotations_values_;
  span<internal::Float3Key> scales_values_;
};
}  // namespace animation

namespace io {
OZZ_IO_TYPE_VERSION(7, animation::Animation)
OZZ_IO_TYPE_TAG("ozz-animation", animation::Animation)
}  // namespace io
}  // namespace ozz
#endif  // OZZ_OZZ_ANIMATION_RUNTIME_ANIMATION_H_
