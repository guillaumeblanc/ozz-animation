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

#include "ozz/animation/offline/raw_track.h"

#include "gtest/gtest.h"

#include "ozz/base/io/archive.h"
#include "ozz/base/io/stream.h"

#include "ozz/base/maths/gtest_math_helper.h"

using ozz::animation::offline::RawFloatTrack;
using ozz::animation::offline::RawFloat2Track;
using ozz::animation::offline::RawFloat3Track;
using ozz::animation::offline::RawFloat4Track;
using ozz::animation::offline::RawQuaternionTrack;
using ozz::animation::offline::RawTrackInterpolation;

TEST(Empty, RawTrackSerialize) {
  ozz::io::MemoryStream stream;

  // Streams out.
  ozz::io::OArchive o(&stream, ozz::GetNativeEndianness());

  RawFloatTrack o_track;
  o << o_track;

  // Streams in.
  stream.Seek(0, ozz::io::Stream::kSet);
  ozz::io::IArchive i(&stream);

  RawFloatTrack i_track;
  i >> i_track;

  EXPECT_EQ(o_track.keyframes.size(), i_track.keyframes.size());
  EXPECT_STREQ(o_track.name.c_str(), i_track.name.c_str());
}

TEST(Filled, RawAnimationSerialize) {
  RawFloatTrack o_track;
  o_track.name = "test track";

  const RawFloatTrack::Keyframe first_key = {RawTrackInterpolation::kLinear,
                                             .5f, 46.f};
  o_track.keyframes.push_back(first_key);
  const RawFloatTrack::Keyframe second_key = {RawTrackInterpolation::kLinear,
                                              .7f, 0.f};
  o_track.keyframes.push_back(second_key);

  EXPECT_TRUE(o_track.Validate());
  EXPECT_EQ(o_track.keyframes.size(), 2u);

  for (int e = 0; e < 2; ++e) {
    ozz::Endianness endianess = e == 0 ? ozz::kBigEndian : ozz::kLittleEndian;
    ozz::io::MemoryStream stream;

    // Streams out.
    ozz::io::OArchive o(&stream, endianess);
    o << o_track;

    // Streams in.
    stream.Seek(0, ozz::io::Stream::kSet);
    ozz::io::IArchive ia(&stream);

    RawFloatTrack i_track;
    ia >> i_track;

    EXPECT_TRUE(i_track.Validate());
    ASSERT_EQ(o_track.keyframes.size(), i_track.keyframes.size());
    EXPECT_STREQ(o_track.name.c_str(), i_track.name.c_str());

    for (size_t i = 0; i < o_track.keyframes.size(); ++i) {
      const RawFloatTrack::Keyframe& o_key = o_track.keyframes[i];
      const RawFloatTrack::Keyframe& i_key = i_track.keyframes[i];

      EXPECT_EQ(o_key.interpolation, i_key.interpolation);
      EXPECT_FLOAT_EQ(o_key.ratio, i_key.ratio);
      EXPECT_EQ(o_key.value, i_key.value);
    }
  }
}

TEST(AlreadyInitialized, RawAnimationSerialize) {
  RawFloatTrack o_track;

  ozz::io::MemoryStream stream;

  // Streams out.
  ozz::io::OArchive o(&stream);
  o_track.name = "test track";
  o << o_track;

  // Streams out a second ratio.
  o_track.keyframes.resize(2);
  o_track.name = "test track 2";
  o << o_track;

  // Streams in.
  stream.Seek(0, ozz::io::Stream::kSet);
  ozz::io::IArchive i(&stream);

  RawFloatTrack i_track;
  i >> i_track;
  ASSERT_EQ(i_track.keyframes.size(), 0u);

  // A second ratio
  i >> i_track;
  ASSERT_EQ(i_track.keyframes.size(), 2u);
  EXPECT_STREQ(o_track.name.c_str(), i_track.name.c_str());
}

TEST(Float2, RawAnimationSerialize) {
  RawFloat2Track o_track;
  o_track.name = "test track";

  const RawFloat2Track::Keyframe first_key = {
      RawTrackInterpolation::kLinear, .5f, ozz::math::Float2(46.f, 99.f)};
  o_track.keyframes.push_back(first_key);
  const RawFloat2Track::Keyframe second_key = {
      RawTrackInterpolation::kLinear, .7f, ozz::math::Float2(16.f, 93.f)};
  o_track.keyframes.push_back(second_key);

  EXPECT_TRUE(o_track.Validate());
  EXPECT_EQ(o_track.keyframes.size(), 2u);

  for (int e = 0; e < 2; ++e) {
    ozz::Endianness endianess = e == 0 ? ozz::kBigEndian : ozz::kLittleEndian;
    ozz::io::MemoryStream stream;

    // Streams out.
    ozz::io::OArchive o(&stream, endianess);
    o << o_track;

    // Streams in.
    stream.Seek(0, ozz::io::Stream::kSet);
    ozz::io::IArchive ia(&stream);

    RawFloat2Track i_track;
    ia >> i_track;

    EXPECT_TRUE(i_track.Validate());
    ASSERT_EQ(o_track.keyframes.size(), i_track.keyframes.size());
    EXPECT_STREQ(o_track.name.c_str(), i_track.name.c_str());

    for (size_t i = 0; i < o_track.keyframes.size(); ++i) {
      const RawFloat2Track::Keyframe& o_key = o_track.keyframes[i];
      const RawFloat2Track::Keyframe& i_key = i_track.keyframes[i];

      EXPECT_EQ(o_key.interpolation, i_key.interpolation);
      EXPECT_FLOAT_EQ(o_key.ratio, i_key.ratio);
      EXPECT_EQ(o_key.value, i_key.value);
    }
  }
}

TEST(Float3, RawAnimationSerialize) {
  RawFloat3Track o_track;

  const RawFloat3Track::Keyframe first_key = {
      RawTrackInterpolation::kLinear, .5f, ozz::math::Float3(46.f, 99.f, 25.f)};
  o_track.keyframes.push_back(first_key);
  const RawFloat3Track::Keyframe second_key = {
      RawTrackInterpolation::kLinear, .7f, ozz::math::Float3(16.f, 93.f, 4.f)};
  o_track.keyframes.push_back(second_key);

  EXPECT_TRUE(o_track.Validate());
  EXPECT_EQ(o_track.keyframes.size(), 2u);

  for (int e = 0; e < 2; ++e) {
    ozz::Endianness endianess = e == 0 ? ozz::kBigEndian : ozz::kLittleEndian;
    ozz::io::MemoryStream stream;

    // Streams out.
    ozz::io::OArchive o(&stream, endianess);
    o << o_track;

    // Streams in.
    stream.Seek(0, ozz::io::Stream::kSet);
    ozz::io::IArchive ia(&stream);

    RawFloat3Track i_track;
    ia >> i_track;

    EXPECT_TRUE(i_track.Validate());
    ASSERT_EQ(o_track.keyframes.size(), i_track.keyframes.size());

    for (size_t i = 0; i < o_track.keyframes.size(); ++i) {
      const RawFloat3Track::Keyframe& o_key = o_track.keyframes[i];
      const RawFloat3Track::Keyframe& i_key = i_track.keyframes[i];

      EXPECT_EQ(o_key.interpolation, i_key.interpolation);
      EXPECT_FLOAT_EQ(o_key.ratio, i_key.ratio);
      EXPECT_EQ(o_key.value, i_key.value);
    }
  }
}

TEST(Float4, RawAnimationSerialize) {
  RawFloat4Track o_track;

  const RawFloat4Track::Keyframe first_key = {
      RawTrackInterpolation::kLinear, .5f,
      ozz::math::Float4(46.f, 99.f, 25.f, 5.f)};
  o_track.keyframes.push_back(first_key);
  const RawFloat4Track::Keyframe second_key = {
      RawTrackInterpolation::kLinear, .7f,
      ozz::math::Float4(16.f, 93.f, 4.f, 46.f)};
  o_track.keyframes.push_back(second_key);

  EXPECT_TRUE(o_track.Validate());
  EXPECT_EQ(o_track.keyframes.size(), 2u);

  for (int e = 0; e < 2; ++e) {
    ozz::Endianness endianess = e == 0 ? ozz::kBigEndian : ozz::kLittleEndian;
    ozz::io::MemoryStream stream;

    // Streams out.
    ozz::io::OArchive o(&stream, endianess);
    o << o_track;

    // Streams in.
    stream.Seek(0, ozz::io::Stream::kSet);
    ozz::io::IArchive ia(&stream);

    RawFloat4Track i_track;
    ia >> i_track;

    EXPECT_TRUE(i_track.Validate());
    ASSERT_EQ(o_track.keyframes.size(), i_track.keyframes.size());

    for (size_t i = 0; i < o_track.keyframes.size(); ++i) {
      const RawFloat4Track::Keyframe& o_key = o_track.keyframes[i];
      const RawFloat4Track::Keyframe& i_key = i_track.keyframes[i];

      EXPECT_EQ(o_key.interpolation, i_key.interpolation);
      EXPECT_FLOAT_EQ(o_key.ratio, i_key.ratio);
      EXPECT_EQ(o_key.value, i_key.value);
    }
  }
}

TEST(Quaternion, RawAnimationSerialize) {
  RawQuaternionTrack o_track;

  const RawQuaternionTrack::Keyframe first_key = {
      RawTrackInterpolation::kLinear, .5f,
      ozz::math::Quaternion(0.f, .70710677f, 0.f, .70710677f)};
  o_track.keyframes.push_back(first_key);
  const RawQuaternionTrack::Keyframe second_key = {
      RawTrackInterpolation::kLinear, .7f,
      ozz::math::Quaternion(.6172133f, .1543033f, 0.f, .7715167f)};
  o_track.keyframes.push_back(second_key);

  EXPECT_TRUE(o_track.Validate());
  EXPECT_EQ(o_track.keyframes.size(), 2u);

  for (int e = 0; e < 2; ++e) {
    ozz::Endianness endianess = e == 0 ? ozz::kBigEndian : ozz::kLittleEndian;
    ozz::io::MemoryStream stream;

    // Streams out.
    ozz::io::OArchive o(&stream, endianess);
    o << o_track;

    // Streams in.
    stream.Seek(0, ozz::io::Stream::kSet);
    ozz::io::IArchive ia(&stream);

    RawQuaternionTrack i_track;
    ia >> i_track;

    EXPECT_TRUE(i_track.Validate());
    ASSERT_EQ(o_track.keyframes.size(), i_track.keyframes.size());

    for (size_t i = 0; i < o_track.keyframes.size(); ++i) {
      const RawQuaternionTrack::Keyframe& o_key = o_track.keyframes[i];
      const RawQuaternionTrack::Keyframe& i_key = i_track.keyframes[i];

      EXPECT_EQ(o_key.interpolation, i_key.interpolation);
      EXPECT_FLOAT_EQ(o_key.ratio, i_key.ratio);
      EXPECT_EQ(o_key.value, i_key.value);
    }
  }
}
