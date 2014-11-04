//============================================================================//
//                                                                            //
// ozz-animation, 3d skeletal animation libraries and tools.                  //
// https://code.google.com/p/ozz-animation/                                   //
//                                                                            //
//----------------------------------------------------------------------------//
//                                                                            //
// Copyright (c) 2012-2014 Guillaume Blanc                                    //
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

#include "ozz/animation/offline/raw_animation.h"

#include "gtest/gtest.h"

#include "ozz/base/io/archive.h"
#include "ozz/base/io/stream.h"

using ozz::animation::offline::RawAnimation;

TEST(Empty, RawAnimationSerialize) {
  ozz::io::MemoryStream stream;

  // Streams out.
  ozz::io::OArchive o(&stream, ozz::GetNativeEndianness());

  RawAnimation o_animation;
  o << o_animation;

  // Streams in.
  stream.Seek(0, ozz::io::Stream::kSet);
  ozz::io::IArchive i(&stream);

  RawAnimation i_animation;
  i >> i_animation;

  EXPECT_FLOAT_EQ(o_animation.duration, i_animation.duration);
  EXPECT_EQ(o_animation.num_tracks(), i_animation.num_tracks());
}

TEST(Filled, RawAnimationSerialize) {

  RawAnimation o_animation;
  o_animation.duration = 46.f;
  o_animation.tracks.resize(3);
  const RawAnimation::TranslationKey t_key = {0.f, ozz::math::Float3(46.f, 93.f, 99.f)};
  o_animation.tracks[0].translations.push_back(t_key);
  const RawAnimation::RotationKey r_key = {46.f, ozz::math::Quaternion(0.f, 1.f, 0.f, 0.f)};
  o_animation.tracks[1].rotations.push_back(r_key);
  const RawAnimation::ScaleKey s_key = {1.f, ozz::math::Float3(93.f, 46.f, 99.f)};
  o_animation.tracks[2].scales.push_back(s_key);

  EXPECT_TRUE(o_animation.Validate());
  EXPECT_EQ(o_animation.num_tracks(), 3);

  for (int e = 0; e < 2; ++e) {
    ozz::Endianness endianess = e == 0 ? ozz::kBigEndian : ozz::kLittleEndian;
    ozz::io::MemoryStream stream;

    // Streams out.
    ozz::io::OArchive o(&stream, endianess);
    o << o_animation;

    // Streams in.
    stream.Seek(0, ozz::io::Stream::kSet);
    ozz::io::IArchive i(&stream);

    RawAnimation i_animation;
    i >> i_animation;

    EXPECT_TRUE(i_animation.Validate());
    EXPECT_FLOAT_EQ(o_animation.duration, i_animation.duration);
    ASSERT_EQ(o_animation.num_tracks(), i_animation.num_tracks());

    for (size_t i = 0; i < o_animation.tracks.size(); ++i) {
      const RawAnimation::JointTrack& o_track = o_animation.tracks[i];
      const RawAnimation::JointTrack& i_track = i_animation.tracks[i];
      for (size_t j = 0; j < o_track.translations.size(); ++j) {
        const RawAnimation::TranslationKey& o_key = o_track.translations[j];
        const RawAnimation::TranslationKey& i_key = i_track.translations[j];
        EXPECT_FLOAT_EQ(o_key.time, i_key.time);
        EXPECT_TRUE(Compare(o_key.value, i_key.value, 0.f));
      }
      for (size_t j = 0; j < o_track.rotations.size(); ++j) {
        const RawAnimation::RotationKey& o_key = o_track.rotations[j];
        const RawAnimation::RotationKey& i_key = i_track.rotations[j];
        EXPECT_FLOAT_EQ(o_key.time, i_key.time);
        EXPECT_TRUE(Compare(o_key.value, i_key.value, .1f));
      }
      for (size_t j = 0; j < o_track.scales.size(); ++j) {
        const RawAnimation::ScaleKey& o_key = o_track.scales[j];
        const RawAnimation::ScaleKey& i_key = i_track.scales[j];
        EXPECT_FLOAT_EQ(o_key.time, i_key.time);
        EXPECT_TRUE(Compare(o_key.value, i_key.value, 0.f));
      }
    }
  }
}

TEST(AlreadyInitialized, RawAnimationSerialize) {

  RawAnimation o_animation;
  o_animation.duration = 46.f;
  o_animation.tracks.resize(1);

  ozz::io::MemoryStream stream;

  // Streams out.
  ozz::io::OArchive o(&stream);
  o << o_animation;

  // Streams out a second time.
  o_animation.duration = 93.f;
  o_animation.tracks.resize(2);
  o << o_animation;

  // Streams in.
  stream.Seek(0, ozz::io::Stream::kSet);
  ozz::io::IArchive i(&stream);

  RawAnimation i_animation;
  i >> i_animation;
  EXPECT_FLOAT_EQ(i_animation.duration, 46.f);
  ASSERT_EQ(i_animation.num_tracks(), 1);

  // A second time
  i >> i_animation;
  EXPECT_FLOAT_EQ(i_animation.duration, 93.f);
  ASSERT_EQ(i_animation.num_tracks(), 2);
}
