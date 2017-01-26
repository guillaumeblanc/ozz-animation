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

#ifndef OZZ_OZZ_ANIMATION_OFFLINE_RAW_CHANNEL_H_
#define OZZ_OZZ_ANIMATION_OFFLINE_RAW_CHANNEL_H_

#include "ozz/base/io/archive_traits.h"
#include "ozz/base/platform.h"

namespace ozz {
namespace animation {

// Forward declares the FloatTrackBuilder, used to instantiate a FloatTrack.
namespace offline {
class FloatTrackBuilder;
}

// Runtime float track data structure.
class FloatTrack {
 public:
  FloatTrack();

  ~FloatTrack();

  float duration() const { return duration_; }
  
  Range<float> times() const { return times_; }
  Range<float> values() const { return values_; }

  // Get the estimated track's size in bytes.
  size_t size() const;

  // Serialization functions.
  // Should not be called directly but through io::Archive << and >> operators.
  void Save(ozz::io::OArchive& _archive) const;
  void Load(ozz::io::IArchive& _archive, uint32_t _version);

 private:

  // FloatTrackBuilder class is allowed to instantiate an Animation.
  friend class offline::FloatTrackBuilder;

  // Internal destruction function.
  void Allocate(size_t _keys_count);
  void Deallocate();

  Range<float> times_;
  Range<float> values_;
  float duration_;
};

}  // animation
namespace io {
OZZ_IO_TYPE_VERSION(1, animation::FloatTrack)
OZZ_IO_TYPE_TAG("ozz-float_track", animation::FloatTrack)

// Should not be called directly but through io::Archive << and >> operators.
template <>
void Save(OArchive& _archive, const animation::FloatTrack* _tracks,
          size_t _count);

template <>
void Load(IArchive& _archive, animation::FloatTrack* _tracks, size_t _count,
          uint32_t _version);
}  // io
}  // ozz
#endif  // OZZ_OZZ_ANIMATION_OFFLINE_RAW_CHANNEL_H_
