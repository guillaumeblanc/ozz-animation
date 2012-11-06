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

#include "ozz/animation/animation_serialize.h"

#include "gtest/gtest.h"

#include "ozz/base/io/archive.h"
#include "ozz/base/io/stream.h"
#include "ozz/base/memory/allocator.h"
#include "ozz/animation/key_frame.h"
#include "ozz/animation/offline/animation_builder.h"

using ozz::animation::Animation;
using ozz::animation::TranslationKey;
using ozz::animation::RotationKey;
using ozz::animation::ScaleKey;
using ozz::animation::offline::RawAnimation;
using ozz::animation::offline::AnimationBuilder;

TEST(Empty, AnimationSerialize) {
  ozz::io::MemoryStream stream;

  // Streams out.
  ozz::io::OArchive o(&stream, ozz::GetNativeEndianness());

  Animation o_animation;
  o << o_animation;

  // Streams in.
  stream.Seek(0, ozz::io::Stream::kSet);
  ozz::io::IArchive i(&stream);

  Animation i_animation;
  i >> i_animation;

  EXPECT_EQ(o_animation.num_tracks(), i_animation.num_tracks());
}

TEST(Filled, AnimationSerialize) {
  // Builds a valid animation.
  Animation* o_animation = NULL;
  {
    RawAnimation raw_animation;
    raw_animation.duration = 1.f;
    raw_animation.tracks.resize(1);

    RawAnimation::TranslationKey first_key = {0.7f, ozz::math::Float3::zero()};
    raw_animation.tracks[0].translations.push_back(first_key);

    AnimationBuilder builder;
    o_animation = builder(raw_animation);
    ASSERT_TRUE(o_animation != NULL);
  }

  for (int e = 0; e < 2; e++) {
    ozz::Endianness endianess = e == 0 ? ozz::kBigEndian : ozz::kLittleEndian;
    ozz::io::MemoryStream stream;

    // Streams out.
    ozz::io::OArchive o(&stream, endianess);
    o << *o_animation;

    // Streams in.
    stream.Seek(0, ozz::io::Stream::kSet);
    ozz::io::IArchive i(&stream);

    Animation i_animation;
    i >> i_animation;

    EXPECT_EQ(o_animation->num_tracks(), i_animation.num_tracks());
    ASSERT_EQ(o_animation->translations_end() - o_animation->translations(),
              i_animation.translations_end() - i_animation.translations());
    for (const TranslationKey* o_keys = o_animation->translations(),
         *i_keys = i_animation.translations();
         o_keys != o_animation->translations_end();
         o_keys++, i_keys++) {
      EXPECT_FLOAT_EQ(i_keys->time, o_keys->time);
      EXPECT_EQ(i_keys->track, o_keys->track);
      EXPECT_TRUE(i_keys->value == o_keys->value);
    }
    ASSERT_EQ(o_animation->rotations_end() - o_animation->rotations(),
              i_animation.rotations_end() - i_animation.rotations());
    for (const RotationKey* o_keys = o_animation->rotations(),
         *i_keys = i_animation.rotations();
         o_keys != o_animation->rotations_end();
         o_keys++, i_keys++) {
      EXPECT_FLOAT_EQ(i_keys->time, o_keys->time);
      EXPECT_EQ(i_keys->track, o_keys->track);
      EXPECT_TRUE(i_keys->value == o_keys->value);
    }
    ASSERT_EQ(o_animation->scales_end() - o_animation->scales(),
              i_animation.scales_end() - i_animation.scales());
    for (const ScaleKey* o_keys = o_animation->scales(),
         *i_keys = i_animation.scales();
         o_keys != o_animation->scales_end();
         o_keys++, i_keys++) {
      EXPECT_FLOAT_EQ(i_keys->time, o_keys->time);
      EXPECT_EQ(i_keys->track, o_keys->track);
      EXPECT_TRUE(i_keys->value == o_keys->value);
    }
  }
  ozz::memory::default_allocator().Delete(o_animation);
}
