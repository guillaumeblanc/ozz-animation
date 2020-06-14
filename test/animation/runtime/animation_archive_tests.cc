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

#include "ozz/animation/runtime/animation.h"

#include "gtest/gtest.h"
#include "ozz/base/maths/gtest_math_helper.h"

#include "ozz/base/io/archive.h"
#include "ozz/base/io/stream.h"
#include "ozz/base/memory/unique_ptr.h"

#include "ozz/base/maths/soa_transform.h"

#include "ozz/animation/runtime/sampling_job.h"

#include "ozz/animation/offline/animation_builder.h"
#include "ozz/animation/offline/raw_animation.h"

using ozz::animation::Animation;
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
  ozz::unique_ptr<Animation> o_animation;
  {
    RawAnimation raw_animation;
    raw_animation.duration = 1.f;
    raw_animation.tracks.resize(1);

    RawAnimation::TranslationKey t_key0 = {0.f,
                                           ozz::math::Float3(93.f, 58.f, 46.f)};
    raw_animation.tracks[0].translations.push_back(t_key0);
    RawAnimation::TranslationKey t_key1 = {.9f,
                                           ozz::math::Float3(46.f, 58.f, 93.f)};
    raw_animation.tracks[0].translations.push_back(t_key1);

    RawAnimation::RotationKey r_key = {
        0.7f, ozz::math::Quaternion(0.f, 1.f, 0.f, 0.f)};
    raw_animation.tracks[0].rotations.push_back(r_key);

    RawAnimation::ScaleKey s_key = {0.1f, ozz::math::Float3(99.f, 26.f, 14.f)};
    raw_animation.tracks[0].scales.push_back(s_key);

    AnimationBuilder builder;
    o_animation = builder(raw_animation);
    ASSERT_TRUE(o_animation);
  }

  for (int e = 0; e < 2; ++e) {
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

    ASSERT_FLOAT_EQ(o_animation->duration(), i_animation.duration());
    ASSERT_EQ(o_animation->num_tracks(), i_animation.num_tracks());
    EXPECT_EQ(o_animation->size(), i_animation.size());

    // Needs to sample to test the animation.
    ozz::animation::SamplingJob job;
    ozz::animation::SamplingCache cache(1);
    ozz::math::SoaTransform output[1];
    job.animation = o_animation.get();
    job.cache = &cache;
    job.output = output;

    // Samples and compares the two animations
    {  // Samples at t = 0
      job.ratio = 0.f;
      job.Run();
      EXPECT_SOAFLOAT3_EQ_EST(output[0].translation, 93.f, 0.f, 0.f, 0.f, 58.f,
                              0.f, 0.f, 0.f, 46.f, 0.f, 0.f, 0.f);
      EXPECT_SOAQUATERNION_EQ_EST(output[0].rotation, 0.f, 0.f, 0.f, 0.f, 1.f,
                                  0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 1.f,
                                  1.f, 1.f);
      EXPECT_SOAFLOAT3_EQ_EST(output[0].scale, 99.f, 1.f, 1.f, 1.f, 26.f, 1.f,
                              1.f, 1.f, 14.f, 1.f, 1.f, 1.f);
    }
    {  // Samples at t = 1
      job.ratio = 1.f;
      job.Run();
      EXPECT_SOAFLOAT3_EQ_EST(output[0].translation, 46.f, 0.f, 0.f, 0.f, 58.f,
                              0.f, 0.f, 0.f, 93.f, 0.f, 0.f, 0.f);
      EXPECT_SOAQUATERNION_EQ_EST(output[0].rotation, 0.f, 0.f, 0.f, 0.f, 1.f,
                                  0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 1.f,
                                  1.f, 1.f);
      EXPECT_SOAFLOAT3_EQ_EST(output[0].scale, 99.f, 1.f, 1.f, 1.f, 26.f, 1.f,
                              1.f, 1.f, 14.f, 1.f, 1.f, 1.f);
    }
  }
}

TEST(AlreadyInitialized, AnimationSerialize) {
  ozz::io::MemoryStream stream;

  {
    ozz::io::OArchive o(&stream);

    RawAnimation raw_animation;
    raw_animation.duration = 1.f;
    raw_animation.tracks.resize(1);

    AnimationBuilder builder;
    ozz::unique_ptr<Animation> o_animation(builder(raw_animation));
    ASSERT_TRUE(o_animation);
    o << *o_animation;

    raw_animation.duration = 2.f;
    raw_animation.tracks.resize(2);
    o_animation = builder(raw_animation);
    ASSERT_TRUE(o_animation);
    o << *o_animation;
  }

  {
    // Streams in.
    stream.Seek(0, ozz::io::Stream::kSet);
    ozz::io::IArchive i(&stream);

    // Reads and check the first animation.
    Animation i_animation;
    i >> i_animation;
    EXPECT_FLOAT_EQ(i_animation.duration(), 1.f);
    EXPECT_EQ(i_animation.num_tracks(), 1);

    // Reuse the animation a second time.
    i >> i_animation;
    EXPECT_FLOAT_EQ(i_animation.duration(), 2.f);
    ASSERT_EQ(i_animation.num_tracks(), 2);
  }
}
