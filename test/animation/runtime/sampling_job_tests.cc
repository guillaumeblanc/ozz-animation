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

#include "gtest/gtest.h"
#include "ozz/animation/offline/animation_builder.h"
#include "ozz/animation/offline/raw_animation.h"
#include "ozz/animation/runtime/animation.h"
#include "ozz/animation/runtime/sampling_job.h"
#include "ozz/base/maths/gtest_math_helper.h"
#include "ozz/base/maths/soa_transform.h"
#include "ozz/base/memory/unique_ptr.h"

using ozz::animation::Animation;
using ozz::animation::SamplingCache;
using ozz::animation::SamplingJob;
using ozz::animation::offline::AnimationBuilder;
using ozz::animation::offline::RawAnimation;

TEST(JobValidity, SamplingJob) {
  RawAnimation raw_animation;
  raw_animation.duration = 1.f;
  raw_animation.tracks.resize(1);

  AnimationBuilder builder;
  ozz::unique_ptr<Animation> animation(builder(raw_animation));
  ASSERT_TRUE(animation);

  // Allocates cache.
  SamplingCache cache(1);

  {  // Empty/default job
    SamplingJob job;
    EXPECT_FALSE(job.Validate());
    EXPECT_FALSE(job.Run());
  }

  {  // Invalid output
    SamplingJob job;
    job.animation = animation.get();
    job.cache = &cache;
    EXPECT_FALSE(job.Validate());
    EXPECT_FALSE(job.Run());
  }

  {  // Invalid animation.
    ozz::math::SoaTransform output[1];

    SamplingJob job;
    job.cache = &cache;
    job.output = output;
    EXPECT_FALSE(job.Validate());
    EXPECT_FALSE(job.Run());
  }

  {  // Invalid cache.
    ozz::math::SoaTransform output[1];

    SamplingJob job;
    job.animation = animation.get();
    job.output = output;
    EXPECT_FALSE(job.Validate());
    EXPECT_FALSE(job.Run());
  }

  {  // Invalid cache size.
    SamplingCache zero_cache(0);
    ozz::math::SoaTransform output[1];

    SamplingJob job;
    job.animation = animation.get();
    job.cache = &zero_cache;
    job.output = output;
    EXPECT_FALSE(job.Validate());
    EXPECT_FALSE(job.Run());
  }

  {  // Invalid job with smaller output.
    ozz::math::SoaTransform* output = nullptr;
    SamplingJob job;
    job.ratio =
        2155.f;  // Any time ratio can be set, it's clamped in unit interval.
    job.animation = animation.get();
    job.cache = &cache;
    job.output = ozz::span<ozz::math::SoaTransform>(output, size_t(0));
    EXPECT_FALSE(job.Validate());
    EXPECT_FALSE(job.Run());
  }

  {  // Valid job.
    ozz::math::SoaTransform output[1];
    SamplingJob job;
    job.ratio = 2155.f;  // Any time can be set.
    job.animation = animation.get();
    job.cache = &cache;
    job.output = output;
    EXPECT_TRUE(job.Validate());
    EXPECT_TRUE(job.Run());
  }

  {  // Valid job with bigger cache.
    SamplingCache big_cache(2);
    ozz::math::SoaTransform output[1];
    SamplingJob job;
    job.ratio = 2155.f;  // Any time can be set.
    job.animation = animation.get();
    job.cache = &big_cache;
    job.output = output;
    EXPECT_TRUE(job.Validate());
    EXPECT_TRUE(job.Run());
  }

  {  // Valid job with bigger output.
    ozz::math::SoaTransform output[2];
    SamplingJob job;
    job.ratio = 2155.f;  // Any time can be set.
    job.animation = animation.get();
    job.cache = &cache;
    job.output = output;
    EXPECT_TRUE(job.Validate());
    EXPECT_TRUE(job.Run());
  }

  {  // Default animation.
    ozz::math::SoaTransform output[1];
    Animation default_animation;
    SamplingJob job;
    job.animation = &default_animation;
    job.cache = &cache;
    job.output = output;
    EXPECT_TRUE(job.Validate());
    EXPECT_TRUE(job.Run());
  }
}

TEST(Sampling, SamplingJob) {
  // Instantiates a builder objects with default parameters.
  AnimationBuilder builder;

  // Building an Animation with unsorted keys fails.
  RawAnimation raw_animation;
  raw_animation.duration = 1.f;
  raw_animation.tracks.resize(4);

  SamplingCache cache(4);

  // Raw animation inputs.
  //     0                 1
  // -----------------------
  // 0 - |  A              |
  // 1 - |                 |
  // 2 - B  C   D   E      F
  // 3 - |  G       H      |

  // Final animation.
  //     0                 1
  // -----------------------
  // 0 - A-1               4
  // 1 - 1                 5
  // 2 - B2 C6  D8 E10    F11
  // 3 - 3  G7     H9      12

  struct {
    float sample_time;
    float trans[12];
  } result[] = {
      {-.2f, {-1.f, 0.f, 2.f, 7.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f}},
      {.0f, {-1.f, 0.f, 2.f, 7.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f}},
      {.0000001f,
       {-1.f, 0.f, 2.f, 7.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f}},
      {.1f, {-1.f, 0.f, 4.f, 7.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f}},
      {.2f, {-1.f, 0.f, 6.f, 7.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f}},
      {.3f, {-1.f, 0.f, 7.f, 7.5, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f}},
      {.4f, {-1.f, 0.f, 8.f, 8.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f}},
      {.3999999f,
       {-1.f, 0.f, 8.f, 8.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f}},
      {.4000001f,
       {-1.f, 0.f, 8.f, 8.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f}},
      {.5f, {-1.f, 0.f, 9.f, 8.5f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f}},
      {.6f, {-1.f, 0.f, 10.f, 9.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f}},
      {.9999999f,
       {-1.f, 0.f, 11.f, 9.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f}},
      {1.f, {-1.f, 0.f, 11.f, 9.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f}},
      {1.000001f,
       {-1.f, 0.f, 11.f, 9.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f}},
      {.5f, {-1.f, 0.f, 9.f, 8.5f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f}},
      {.9999999f,
       {-1.f, 0.f, 11.f, 9.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f}},
      {.0000001f,
       {-1.f, 0.f, 2.f, 7.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f}}};

  RawAnimation::TranslationKey a = {.2f, ozz::math::Float3(-1.f, 0.f, 0.f)};
  raw_animation.tracks[0].translations.push_back(a);

  RawAnimation::TranslationKey b = {0.f, ozz::math::Float3(2.f, 0.f, 0.f)};
  raw_animation.tracks[2].translations.push_back(b);
  RawAnimation::TranslationKey c = {0.2f, ozz::math::Float3(6.f, 0.f, 0.f)};
  raw_animation.tracks[2].translations.push_back(c);
  RawAnimation::TranslationKey d = {0.4f, ozz::math::Float3(8.f, 0.f, 0.f)};
  raw_animation.tracks[2].translations.push_back(d);
  RawAnimation::TranslationKey e = {0.6f, ozz::math::Float3(10.f, 0.f, 0.f)};
  raw_animation.tracks[2].translations.push_back(e);
  RawAnimation::TranslationKey f = {1.f, ozz::math::Float3(11.f, 0.f, 0.f)};
  raw_animation.tracks[2].translations.push_back(f);

  RawAnimation::TranslationKey g = {0.2f, ozz::math::Float3(7.f, 0.f, 0.f)};
  raw_animation.tracks[3].translations.push_back(g);
  RawAnimation::TranslationKey h = {0.6f, ozz::math::Float3(9.f, 0.f, 0.f)};
  raw_animation.tracks[3].translations.push_back(h);

  // Builds animation
  ozz::unique_ptr<Animation> animation(builder(raw_animation));
  ASSERT_TRUE(animation);

  ozz::math::SoaTransform output[1];

  SamplingJob job;
  job.animation = animation.get();
  job.cache = &cache;
  job.output = output;

  for (size_t i = 0; i < OZZ_ARRAY_SIZE(result); ++i) {
    memset(output, 0xde, sizeof(output));
    job.ratio = result[i].sample_time / animation->duration();
    EXPECT_TRUE(job.Validate());
    EXPECT_TRUE(job.Run());

    EXPECT_SOAFLOAT3_EQ_EST(
        output[0].translation, result[i].trans[0], result[i].trans[1],
        result[i].trans[2], result[i].trans[3], result[i].trans[4],
        result[i].trans[5], result[i].trans[6], result[i].trans[7],
        result[i].trans[8], result[i].trans[9], result[i].trans[10],
        result[i].trans[11]);
    EXPECT_SOAQUATERNION_EQ_EST(output[0].rotation, 0.f, 0.f, 0.f, 0.f, 0.f,
                                0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 1.f, 1.f,
                                1.f, 1.f);
    EXPECT_SOAFLOAT3_EQ_EST(output[0].scale, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f,
                            1.f, 1.f, 1.f, 1.f, 1.f);
  }
}

TEST(SamplingNoTrack, SamplingJob) {
  RawAnimation raw_animation;
  raw_animation.duration = 46.f;

  SamplingCache cache(1);

  AnimationBuilder builder;
  ozz::unique_ptr<Animation> animation(builder(raw_animation));
  ASSERT_TRUE(animation);

  ozz::math::SoaTransform test_output[1];
  ozz::math::SoaTransform output[1];
  memset(test_output, 0xde, sizeof(test_output));
  memset(output, 0xde, sizeof(output));

  SamplingJob job;
  job.ratio = 0.f;
  job.animation = animation.get();
  job.cache = &cache;
  job.output = output;
  EXPECT_TRUE(job.Validate());
  EXPECT_TRUE(job.Run());

  // Tests output.
  EXPECT_EQ(memcmp(test_output, output, sizeof(output)), 0);
}

TEST(Sampling1Track0Key, SamplingJob) {
  RawAnimation raw_animation;
  raw_animation.duration = 46.f;
  raw_animation.tracks.resize(1);  // Adds a joint.

  SamplingCache cache(1);

  AnimationBuilder builder;
  ozz::unique_ptr<Animation> animation(builder(raw_animation));
  ASSERT_TRUE(animation);

  ozz::math::SoaTransform output[1];

  SamplingJob job;
  job.animation = animation.get();
  job.cache = &cache;
  job.output = output;

  for (float t = -.2f; t < 1.2f; t += .1f) {
    memset(output, 0xde, sizeof(output));
    job.ratio = t;
    EXPECT_TRUE(job.Validate());
    EXPECT_TRUE(job.Run());
    EXPECT_SOAFLOAT3_EQ_EST(output[0].translation, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f,
                            0.f, 0.f, 0.f, 0.f, 0.f, 0.f);
    EXPECT_SOAQUATERNION_EQ_EST(output[0].rotation, 0.f, 0.f, 0.f, 0.f, 0.f,
                                0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 1.f, 1.f,
                                1.f, 1.f);
    EXPECT_SOAFLOAT3_EQ_EST(output[0].scale, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f,
                            1.f, 1.f, 1.f, 1.f, 1.f);
  }
}

TEST(Sampling1Track1Key, SamplingJob) {
  RawAnimation raw_animation;
  raw_animation.duration = 46.f;
  raw_animation.tracks.resize(1);  // Adds a joint.

  SamplingCache cache(1);

  const RawAnimation::TranslationKey tkey = {.3f,
                                             ozz::math::Float3(1.f, -1.f, 5.f)};
  raw_animation.tracks[0].translations.push_back(tkey);  // Adds a key.

  AnimationBuilder builder;
  ozz::unique_ptr<Animation> animation(builder(raw_animation));
  ASSERT_TRUE(animation);

  ozz::math::SoaTransform output[1];

  SamplingJob job;
  job.animation = animation.get();
  job.cache = &cache;
  job.output = output;

  for (float t = -.2f; t < 1.2f; t += .1f) {
    memset(output, 0xde, sizeof(output));
    job.ratio = t;
    EXPECT_TRUE(job.Validate());
    EXPECT_TRUE(job.Run());
    EXPECT_SOAFLOAT3_EQ_EST(output[0].translation, 1.f, 0.f, 0.f, 0.f, -1.f,
                            0.f, 0.f, 0.f, 5.f, 0.f, 0.f, 0.f);
    EXPECT_SOAQUATERNION_EQ_EST(output[0].rotation, 0.f, 0.f, 0.f, 0.f, 0.f,
                                0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 1.f, 1.f,
                                1.f, 1.f);
    EXPECT_SOAFLOAT3_EQ_EST(output[0].scale, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f,
                            1.f, 1.f, 1.f, 1.f, 1.f);
  }
}

TEST(Sampling1Track2Keys, SamplingJob) {
  RawAnimation raw_animation;
  raw_animation.duration = 46.f;
  raw_animation.tracks.resize(1);  // Adds a joint.

  SamplingCache cache(1);

  const RawAnimation::TranslationKey tkey0 = {.5f,
                                              ozz::math::Float3(1.f, 2.f, 4.f)};
  raw_animation.tracks[0].translations.push_back(tkey0);  // Adds a key.
  const RawAnimation::TranslationKey tkey1 = {.8f,
                                              ozz::math::Float3(2.f, 4.f, 8.f)};
  raw_animation.tracks[0].translations.push_back(tkey1);  // Adds a key.

  AnimationBuilder builder;
  ozz::unique_ptr<Animation> animation(builder(raw_animation));
  ASSERT_TRUE(animation);

  ozz::math::SoaTransform output[1];
  memset(output, 0xde, sizeof(output));

  SamplingJob job;
  job.animation = animation.get();
  job.cache = &cache;
  job.output = output;

  // Samples at t = 0.
  job.ratio = 0.f;
  EXPECT_TRUE(job.Validate());
  EXPECT_TRUE(job.Run());
  EXPECT_SOAFLOAT3_EQ_EST(output[0].translation, 1.f, 0.f, 0.f, 0.f, 2.f, 0.f,
                          0.f, 0.f, 4.f, 0.f, 0.f, 0.f);
  EXPECT_SOAQUATERNION_EQ_EST(output[0].rotation, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f,
                              0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 1.f, 1.f, 1.f, 1.f);
  EXPECT_SOAFLOAT3_EQ_EST(output[0].scale, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f,
                          1.f, 1.f, 1.f, 1.f, 1.f);

  // Samples at t = tkey0.
  job.ratio = tkey0.time / animation->duration();
  EXPECT_TRUE(job.Validate());
  EXPECT_TRUE(job.Run());
  EXPECT_SOAFLOAT3_EQ_EST(output[0].translation, 1.f, 0.f, 0.f, 0.f, 2.f, 0.f,
                          0.f, 0.f, 4.f, 0.f, 0.f, 0.f);
  EXPECT_SOAQUATERNION_EQ_EST(output[0].rotation, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f,
                              0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 1.f, 1.f, 1.f, 1.f);
  EXPECT_SOAFLOAT3_EQ_EST(output[0].scale, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f,
                          1.f, 1.f, 1.f, 1.f, 1.f);

  // Samples at t = tkey1.
  job.ratio = tkey1.time / animation->duration();
  EXPECT_TRUE(job.Validate());
  EXPECT_TRUE(job.Run());
  EXPECT_SOAFLOAT3_EQ_EST(output[0].translation, 2.f, 0.f, 0.f, 0.f, 4.f, 0.f,
                          0.f, 0.f, 8.f, 0.f, 0.f, 0.f);
  EXPECT_SOAQUATERNION_EQ_EST(output[0].rotation, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f,
                              0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 1.f, 1.f, 1.f, 1.f);
  EXPECT_SOAFLOAT3_EQ_EST(output[0].scale, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f,
                          1.f, 1.f, 1.f, 1.f, 1.f);

  // Samples at t = end.
  job.ratio = 1.f;
  EXPECT_TRUE(job.Validate());
  EXPECT_TRUE(job.Run());
  EXPECT_SOAFLOAT3_EQ_EST(output[0].translation, 2.f, 0.f, 0.f, 0.f, 4.f, 0.f,
                          0.f, 0.f, 8.f, 0.f, 0.f, 0.f);
  EXPECT_SOAQUATERNION_EQ_EST(output[0].rotation, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f,
                              0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 1.f, 1.f, 1.f, 1.f);
  EXPECT_SOAFLOAT3_EQ_EST(output[0].scale, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f,
                          1.f, 1.f, 1.f, 1.f, 1.f);

  // Samples at tkey0.time < t < tkey1.time.
  job.ratio = (tkey0.time / animation->duration() +
               tkey1.time / animation->duration()) /
              2.f;
  EXPECT_TRUE(job.Validate());
  EXPECT_TRUE(job.Run());
  EXPECT_SOAFLOAT3_EQ_EST(output[0].translation, 1.5f, 0.f, 0.f, 0.f, 3.f, 0.f,
                          0.f, 0.f, 6.f, 0.f, 0.f, 0.f);
  EXPECT_SOAQUATERNION_EQ_EST(output[0].rotation, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f,
                              0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 1.f, 1.f, 1.f, 1.f);
  EXPECT_SOAFLOAT3_EQ_EST(output[0].scale, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f,
                          1.f, 1.f, 1.f, 1.f, 1.f);
}

TEST(Sampling4Track2Keys, SamplingJob) {
  RawAnimation raw_animation;
  raw_animation.duration = 1.f;
  raw_animation.tracks.resize(4);  // Adds a joint.

  SamplingCache cache(1);

  const RawAnimation::TranslationKey tkey00 = {
      .5f, ozz::math::Float3(1.f, 2.f, 4.f)};
  raw_animation.tracks[0].translations.push_back(tkey00);  // Adds a key.
  const RawAnimation::TranslationKey tkey01 = {
      .8f, ozz::math::Float3(2.f, 4.f, 8.f)};
  raw_animation.tracks[0].translations.push_back(tkey01);  // Adds a key.

  // This quaternion will be negated as the builder ensures that the first key
  // is in identity quaternion hemisphere.
  const RawAnimation::RotationKey rkey10 = {
      0.f, ozz::math::Quaternion(0.f, 0.f, 0.f, -1.f)};
  raw_animation.tracks[1].rotations.push_back(rkey10);  // Adds a key.
  const RawAnimation::RotationKey rkey11 = {
      1.f, ozz::math::Quaternion(0.f, 1.f, 0.f, 0.f)};
  raw_animation.tracks[1].rotations.push_back(rkey11);  // Adds a key.

  const RawAnimation::ScaleKey skey20 = {.5f, ozz::math::Float3(0.f, 0.f, 0.f)};
  raw_animation.tracks[2].scales.push_back(skey20);  // Adds a key.
  const RawAnimation::ScaleKey skey21 = {.8f,
                                         ozz::math::Float3(-1.f, -1.f, -1.f)};
  raw_animation.tracks[2].scales.push_back(skey21);  // Adds a key.

  const RawAnimation::TranslationKey tkey30 = {
      0.f, ozz::math::Float3(-1.f, -2.f, -4.f)};
  raw_animation.tracks[3].translations.push_back(tkey30);  // Adds a key.
  const RawAnimation::TranslationKey tkey31 = {
      1.f, ozz::math::Float3(-2.f, -4.f, -8.f)};
  raw_animation.tracks[3].translations.push_back(tkey31);  // Adds a key.

  AnimationBuilder builder;
  ozz::unique_ptr<Animation> animation(builder(raw_animation));
  ASSERT_TRUE(animation);

  ozz::math::SoaTransform output[1];
  memset(output, 0xde, sizeof(output));

  SamplingJob job;
  job.animation = animation.get();
  job.cache = &cache;
  job.output = output;

  // Samples at t = 0.
  job.ratio = 0.f;
  EXPECT_TRUE(job.Validate());
  EXPECT_TRUE(job.Run());
  EXPECT_SOAFLOAT3_EQ_EST(output[0].translation, 1.f, 0.f, 0.f, -1.f, 2.f, 0.f,
                          0.f, -2.f, 4.f, 0.f, 0.f, -4.f);
  EXPECT_SOAQUATERNION_EQ_EST(output[0].rotation, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f,
                              0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 1.f, 1.f, 1.f, 1.f);
  EXPECT_SOAFLOAT3_EQ_EST(output[0].scale, 1.f, 1.f, 0.f, 1.f, 1.f, 1.f, 0.f,
                          1.f, 1.f, 1.f, 0.f, 1.f);

  // Samples at t = tkey00.
  job.ratio = tkey00.time / animation->duration();
  EXPECT_TRUE(job.Validate());
  EXPECT_TRUE(job.Run());
  EXPECT_SOAFLOAT3_EQ_EST(output[0].translation, 1.f, 0.f, 0.f, -1.5f, 2.f, 0.f,
                          0.f, -3.f, 4.f, 0.f, 0.f, -6.f);
  EXPECT_SOAQUATERNION_EQ_EST(output[0].rotation, 0.f, 0.f, 0.f, 0.f, 0.f,
                              .7071067f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 1.f,
                              .7071067f, 1.f, 1.f);
  EXPECT_SOAFLOAT3_EQ_EST(output[0].scale, 1.f, 1.f, 0.f, 1.f, 1.f, 1.f, 0.f,
                          1.f, 1.f, 1.f, 0.f, 1.f);

  // Samples at t = end.
  job.ratio = 1.f;
  EXPECT_TRUE(job.Validate());
  EXPECT_TRUE(job.Run());
  EXPECT_SOAFLOAT3_EQ_EST(output[0].translation, 2.f, 0.f, 0.f, -2.f, 4.f, 0.f,
                          0.f, -4.f, 8.f, 0.f, 0.f, -8.f);
  EXPECT_SOAQUATERNION_EQ_EST(output[0].rotation, 0.f, 0.f, 0.f, 0.f, 0.f, 1.f,
                              0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 1.f, 1.f);
  EXPECT_SOAFLOAT3_EQ_EST(output[0].scale, 1.f, 1.f, -1.f, 1.f, 1.f, 1.f, -1.f,
                          1.f, 1.f, 1.f, -1.f, 1.f);
}

TEST(Cache, SamplingJob) {
  RawAnimation raw_animation;
  raw_animation.duration = 46.f;
  raw_animation.tracks.resize(1);  // Adds a joint.
  const RawAnimation::TranslationKey empty_key = {
      0.f, RawAnimation::TranslationKey::identity()};
  raw_animation.tracks[0].translations.push_back(empty_key);

  SamplingCache cache(1);
  ozz::unique_ptr<Animation> animations[2];

  {
    const RawAnimation::TranslationKey tkey = {
        .3f, ozz::math::Float3(1.f, -1.f, 5.f)};
    raw_animation.tracks[0].translations[0] = tkey;

    AnimationBuilder builder;
    animations[0] = builder(raw_animation);
    ASSERT_TRUE(animations[0]);
  }
  {
    const RawAnimation::TranslationKey tkey = {
        .3f, ozz::math::Float3(-1.f, 1.f, -5.f)};
    raw_animation.tracks[0].translations[0] = tkey;

    AnimationBuilder builder;
    animations[1] = builder(raw_animation);
    ASSERT_TRUE(animations[1]);
  }

  ozz::math::SoaTransform output[1];

  SamplingJob job;
  job.animation = animations[0].get();
  job.cache = &cache;
  job.ratio = 0.f;
  job.output = output;

  EXPECT_TRUE(job.Validate());
  EXPECT_TRUE(job.Run());
  EXPECT_SOAFLOAT3_EQ_EST(output[0].translation, 1.f, 0.f, 0.f, 0.f, -1.f, 0.f,
                          0.f, 0.f, 5.f, 0.f, 0.f, 0.f);
  EXPECT_SOAQUATERNION_EQ_EST(output[0].rotation, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f,
                              0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 1.f, 1.f, 1.f, 1.f);
  EXPECT_SOAFLOAT3_EQ_EST(output[0].scale, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f,
                          1.f, 1.f, 1.f, 1.f, 1.f);

  // Re-uses cache.
  EXPECT_TRUE(job.Validate());
  EXPECT_TRUE(job.Run());
  EXPECT_SOAFLOAT3_EQ_EST(output[0].translation, 1.f, 0.f, 0.f, 0.f, -1.f, 0.f,
                          0.f, 0.f, 5.f, 0.f, 0.f, 0.f);

  // Invalidates cache.
  cache.Invalidate();

  EXPECT_TRUE(job.Validate());
  EXPECT_TRUE(job.Run());
  EXPECT_SOAFLOAT3_EQ_EST(output[0].translation, 1.f, 0.f, 0.f, 0.f, -1.f, 0.f,
                          0.f, 0.f, 5.f, 0.f, 0.f, 0.f);

  // Changes animation.
  job.animation = animations[1].get();
  EXPECT_TRUE(job.Validate());
  EXPECT_TRUE(job.Run());
  EXPECT_SOAFLOAT3_EQ_EST(output[0].translation, -1.f, 0.f, 0.f, 0.f, 1.f, 0.f,
                          0.f, 0.f, -5.f, 0.f, 0.f, 0.f);
  EXPECT_SOAQUATERNION_EQ_EST(output[0].rotation, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f,
                              0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 1.f, 1.f, 1.f, 1.f);
  EXPECT_SOAFLOAT3_EQ_EST(output[0].scale, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f,
                          1.f, 1.f, 1.f, 1.f, 1.f);

  // Invalidates and changes animation.
  job.animation = animations[1].get();
  EXPECT_TRUE(job.Validate());
  EXPECT_TRUE(job.Run());
  EXPECT_SOAFLOAT3_EQ_EST(output[0].translation, -1.f, 0.f, 0.f, 0.f, 1.f, 0.f,
                          0.f, 0.f, -5.f, 0.f, 0.f, 0.f);
}

TEST(CacheResize, SamplingJob) {
  RawAnimation raw_animation;
  raw_animation.duration = 46.f;
  raw_animation.tracks.resize(7);

  AnimationBuilder builder;
  ozz::unique_ptr<Animation> animation(builder(raw_animation));
  ASSERT_TRUE(animation);

  // Empty cache by default
  SamplingCache cache;

  ozz::math::SoaTransform output[7];

  SamplingJob job;
  job.animation = animation.get();
  job.cache = &cache;
  job.ratio = 0.f;
  job.output = output;

  // Cache is too small
  EXPECT_FALSE(job.Validate());

  // Cache is ok.
  cache.Resize(7);
  EXPECT_TRUE(job.Validate());
  EXPECT_TRUE(job.Run());

  // Cache is too small
  cache.Resize(1);
  EXPECT_FALSE(job.Validate());
}
