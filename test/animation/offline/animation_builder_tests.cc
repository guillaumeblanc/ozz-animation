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
#include "ozz/animation/runtime/skeleton.h"
#include "ozz/base/maths/gtest_math_helper.h"
#include "ozz/base/maths/soa_transform.h"
#include "ozz/base/memory/unique_ptr.h"

using ozz::animation::Animation;
using ozz::animation::offline::AnimationBuilder;
using ozz::animation::offline::RawAnimation;

TEST(Error, AnimationBuilder) {
  // Instantiates a builder objects with default parameters.
  AnimationBuilder builder;

  {  // Building an empty Animation fails because animation duration
    // must be >= 0.
    RawAnimation raw_animation;
    raw_animation.duration = -1.f;  // Negative duration.
    EXPECT_FALSE(raw_animation.Validate());

    // Builds animation
    EXPECT_TRUE(!builder(raw_animation));
  }

  {  // Building an empty Animation fails because animation duration
    // must be >= 0.
    RawAnimation raw_animation;
    raw_animation.duration = 0.f;  // Invalid duration.
    EXPECT_FALSE(raw_animation.Validate());

    // Builds animation
    EXPECT_TRUE(!builder(raw_animation));
  }

  {  // Building an animation with too much tracks fails.
    RawAnimation raw_animation;
    raw_animation.duration = 1.f;
    raw_animation.tracks.resize(ozz::animation::Skeleton::kMaxJoints + 1);
    EXPECT_FALSE(raw_animation.Validate());

    // Builds animation
    EXPECT_TRUE(!builder(raw_animation));
  }

  {  // Building default animation succeeds.
    RawAnimation raw_animation;
    EXPECT_EQ(raw_animation.duration, 1.f);
    EXPECT_TRUE(raw_animation.Validate());

    // Builds animation
    ozz::unique_ptr<Animation> anim(builder(raw_animation));
    EXPECT_TRUE(anim);
  }

  {  // Building an animation with max joints succeeds.
    RawAnimation raw_animation;
    raw_animation.tracks.resize(ozz::animation::Skeleton::kMaxJoints);
    EXPECT_EQ(raw_animation.num_tracks(), ozz::animation::Skeleton::kMaxJoints);
    EXPECT_TRUE(raw_animation.Validate());

    // Builds animation
    ozz::unique_ptr<Animation> anim(builder(raw_animation));
    EXPECT_TRUE(anim);
  }
}

TEST(Build, AnimationBuilder) {
  // Instantiates a builder objects with default parameters.
  AnimationBuilder builder;

  {  // Building an Animation with unsorted keys fails.
    RawAnimation raw_animation;
    raw_animation.duration = 1.f;
    raw_animation.tracks.resize(1);

    // Adds 2 unordered keys
    RawAnimation::TranslationKey first_key = {.8f, ozz::math::Float3::zero()};
    raw_animation.tracks[0].translations.push_back(first_key);
    RawAnimation::TranslationKey second_key = {.2f, ozz::math::Float3::zero()};
    raw_animation.tracks[0].translations.push_back(second_key);

    // Builds animation
    EXPECT_FALSE(builder(raw_animation));
  }

  {  // Building an Animation with invalid key frame's time fails.
    RawAnimation raw_animation;
    raw_animation.duration = 1.f;
    raw_animation.tracks.resize(1);

    // Adds a key with a time greater than animation duration.
    RawAnimation::TranslationKey first_key = {2.f, ozz::math::Float3::zero()};
    raw_animation.tracks[0].translations.push_back(first_key);

    // Builds animation
    EXPECT_FALSE(builder(raw_animation));
  }

  {  // Building an Animation with unsorted key frame's time fails.
    RawAnimation raw_animation;
    raw_animation.duration = 1.f;
    raw_animation.tracks.resize(2);

    // Adds 2 unsorted keys.
    RawAnimation::TranslationKey first_key = {0.7f, ozz::math::Float3::zero()};
    raw_animation.tracks[0].translations.push_back(first_key);
    RawAnimation::TranslationKey second_key = {0.1f, ozz::math::Float3::zero()};
    raw_animation.tracks[0].translations.push_back(second_key);

    // Builds animation
    EXPECT_FALSE(builder(raw_animation));
  }

  {  // Building an Animation with equal key frame's time fails.
    RawAnimation raw_animation;
    raw_animation.duration = 1.f;
    raw_animation.tracks.resize(2);

    // Adds 2 unsorted keys.
    RawAnimation::TranslationKey key = {0.7f, ozz::math::Float3::zero()};
    raw_animation.tracks[0].translations.push_back(key);
    raw_animation.tracks[0].translations.push_back(key);

    // Builds animation
    EXPECT_FALSE(builder(raw_animation));
  }

  {  // Building a valid Animation with empty tracks succeeds.
    RawAnimation raw_animation;
    raw_animation.duration = 46.f;
    raw_animation.tracks.resize(46);

    // Builds animation
    ozz::unique_ptr<Animation> anim(builder(raw_animation));
    EXPECT_TRUE(anim);
    EXPECT_EQ(anim->duration(), 46.f);
    EXPECT_EQ(anim->num_tracks(), 46);
  }

  {  // Building a valid Animation with 1 track succeeds.
    RawAnimation raw_animation;
    raw_animation.duration = 46.f;
    raw_animation.tracks.resize(1);

    RawAnimation::TranslationKey first_key = {0.7f, ozz::math::Float3::zero()};
    raw_animation.tracks[0].translations.push_back(first_key);

    // Builds animation
    ozz::unique_ptr<Animation> anim(builder(raw_animation));
    EXPECT_TRUE(anim);
    EXPECT_EQ(anim->duration(), 46.f);
    EXPECT_EQ(anim->num_tracks(), 1);
  }
}

TEST(Name, AnimationBuilder) {
  // Instantiates a builder objects with default parameters.
  AnimationBuilder builder;

  {  // Building an unnamed animation.
    RawAnimation raw_animation;
    raw_animation.duration = 1.f;
    raw_animation.tracks.resize(46);

    // Builds animation
    ozz::unique_ptr<Animation> anim(builder(raw_animation));
    EXPECT_TRUE(anim);

    // Should
    EXPECT_STREQ(anim->name(), "");
  }

  {  // Building an unnamed animation.
    RawAnimation raw_animation;
    raw_animation.duration = 1.f;
    raw_animation.tracks.resize(46);
    raw_animation.name = "46";

    // Builds animation
    ozz::unique_ptr<Animation> anim(builder(raw_animation));
    EXPECT_TRUE(anim);

    // Should
    EXPECT_STREQ(anim->name(), "46");
  }
}

TEST(Move, AnimationBuilder) {
  AnimationBuilder builder;
  RawAnimation raw_animation;

  {  // Move constructor
    raw_animation.name = "anim1";
    raw_animation.duration = 46.f;
    raw_animation.tracks.resize(46);
    ozz::unique_ptr<Animation> anim1(builder(raw_animation));
    const Animation canim(std::move(*anim1));
    EXPECT_FLOAT_EQ(canim.duration(), 46.f);
    EXPECT_STREQ(canim.name(), "anim1");
  }

  {  // Move assignment
    raw_animation.name = "anim1";
    raw_animation.duration = 46.f;
    raw_animation.tracks.resize(46);
    ozz::unique_ptr<Animation> anim1(builder(raw_animation));
    EXPECT_STREQ(anim1->name(), "anim1");
    EXPECT_EQ(anim1->num_tracks(), 46);

    raw_animation.name = "anim2";
    raw_animation.duration = 93.f;
    raw_animation.tracks.resize(93);
    ozz::unique_ptr<Animation> anim2(builder(raw_animation));
    EXPECT_STREQ(anim2->name(), "anim2");
    EXPECT_EQ(anim2->num_tracks(), 93);

    *anim2 = std::move(*anim1);
    EXPECT_FLOAT_EQ(anim2->duration(), 46.f);
    EXPECT_EQ(anim2->num_tracks(), 46);
    EXPECT_STREQ(anim2->name(), "anim1");
  }
}

TEST(Sort, AnimationBuilder) {
  // Instantiates a builder objects with default parameters.
  AnimationBuilder builder;

  {
    RawAnimation raw_animation;
    raw_animation.duration = 1.f;
    raw_animation.tracks.resize(4);

    // Raw animation inputs.
    //     0              1
    // --------------------
    // 0 - A     B        |
    // 1 - C  D  E        |
    // 2 - F  G     H  I  J
    // 3 - K  L  M  N     |

    // Final animation.
    //     0              1
    // --------------------
    // 0 - 0     4       11
    // 1 - 1  5  8       12
    // 2 - 2  6     9 14 16
    // 3 - 3  7 10 13    15

    RawAnimation::TranslationKey a = {0.f * raw_animation.duration,
                                      ozz::math::Float3(1.f, 0.f, 0.f)};
    raw_animation.tracks[0].translations.push_back(a);
    RawAnimation::TranslationKey b = {.4f * raw_animation.duration,
                                      ozz::math::Float3(3.f, 0.f, 0.f)};
    raw_animation.tracks[0].translations.push_back(b);

    RawAnimation::TranslationKey c = {0.f * raw_animation.duration,
                                      ozz::math::Float3(2.f, 0.f, 0.f)};
    raw_animation.tracks[1].translations.push_back(c);
    RawAnimation::TranslationKey d = {0.2f * raw_animation.duration,
                                      ozz::math::Float3(6.f, 0.f, 0.f)};
    raw_animation.tracks[1].translations.push_back(d);
    RawAnimation::TranslationKey e = {0.4f * raw_animation.duration,
                                      ozz::math::Float3(8.f, 0.f, 0.f)};
    raw_animation.tracks[1].translations.push_back(e);

    RawAnimation::TranslationKey f = {0.f * raw_animation.duration,
                                      ozz::math::Float3(12.f, 0.f, 0.f)};
    raw_animation.tracks[2].translations.push_back(f);
    RawAnimation::TranslationKey g = {.2f * raw_animation.duration,
                                      ozz::math::Float3(11.f, 0.f, 0.f)};
    raw_animation.tracks[2].translations.push_back(g);
    RawAnimation::TranslationKey h = {.6f * raw_animation.duration,
                                      ozz::math::Float3(9.f, 0.f, 0.f)};
    raw_animation.tracks[2].translations.push_back(h);
    RawAnimation::TranslationKey i = {.8f * raw_animation.duration,
                                      ozz::math::Float3(7.f, 0.f, 0.f)};
    raw_animation.tracks[2].translations.push_back(i);
    RawAnimation::TranslationKey j = {1.f * raw_animation.duration,
                                      ozz::math::Float3(5.f, 0.f, 0.f)};
    raw_animation.tracks[2].translations.push_back(j);

    RawAnimation::TranslationKey k = {0.f * raw_animation.duration,
                                      ozz::math::Float3(1.f, 0.f, 0.f)};
    raw_animation.tracks[3].translations.push_back(k);
    RawAnimation::TranslationKey l = {.2f * raw_animation.duration,
                                      ozz::math::Float3(2.f, 0.f, 0.f)};
    raw_animation.tracks[3].translations.push_back(l);
    RawAnimation::TranslationKey m = {.4f * raw_animation.duration,
                                      ozz::math::Float3(3.f, 0.f, 0.f)};
    raw_animation.tracks[3].translations.push_back(m);
    RawAnimation::TranslationKey n = {.6f * raw_animation.duration,
                                      ozz::math::Float3(4.f, 0.f, 0.f)};
    raw_animation.tracks[3].translations.push_back(n);

    // Builds animation
    const float intervals[] = {-1.f, 0.f, .001f, .1f,   .5f,
                               .9f,  1.f, 2.f,   1000.f};
    for (float interval : intervals) {
      builder.iframe_interval = interval;
      ozz::unique_ptr<Animation> animation(builder(raw_animation));
      ASSERT_TRUE(animation);

      // Duration must be maintained.
      EXPECT_EQ(animation->duration(), raw_animation.duration);

      // Needs to sample to test the animation.
      ozz::animation::SamplingJob job;
      ozz::animation::SamplingJob::Context context(1);
      ozz::math::SoaTransform output[1];
      job.animation = animation.get();
      job.context = &context;
      job.output = output;

      // Samples and compares the two animations
      {  // Samples at t = 0
        job.ratio = 0.f;
        job.Run();
        EXPECT_SOAFLOAT3_EQ_EST(output[0].translation, 1.f, 2.f, 12.f, 1.f, 0.f,
                                0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f);
      }
      {  // Samples at t = .2
        job.ratio = .2f;
        job.Run();
        EXPECT_SOAFLOAT3_EQ_EST(output[0].translation, 2.f, 6.f, 11.f, 2.f, 0.f,
                                0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f);
      }
      {  // Samples at t = .4
        job.ratio = .4f;
        job.Run();
        EXPECT_SOAFLOAT3_EQ_EST(output[0].translation, 3.f, 8.f, 10.f, 3.f, 0.f,
                                0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f);
      }
      {  // Samples at t = .6
        job.ratio = .6f;
        job.Run();
        EXPECT_SOAFLOAT3_EQ_EST(output[0].translation, 3.f, 8.f, 9.f, 4.f, 0.f,
                                0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f);
      }
      {  // Samples at t = .8
        job.ratio = .8f;
        job.Run();
        EXPECT_SOAFLOAT3_EQ_EST(output[0].translation, 3.f, 8.f, 7.f, 4.f, 0.f,
                                0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f);
      }
      {  // Samples at t = 1
        job.ratio = 1.f;
        job.Run();
        EXPECT_SOAFLOAT3_EQ_EST(output[0].translation, 3.f, 8.f, 5.f, 4.f, 0.f,
                                0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f);
      }
    }
  }
}

TEST(ManyKeys, SamplingJob) {
  const size_t kMaxKey = 65500;

  RawAnimation raw_animation;
  raw_animation.duration = 1.f;
  raw_animation.tracks.resize(4);  // Adds a joint

  // Track 0 has big hole
  {
    const RawAnimation::TranslationKey key0 = {
        0.f, ozz::math::Float3(0.f, 0.f, 0.f)};
    raw_animation.tracks[0].translations.push_back(key0);
    const RawAnimation::TranslationKey key1 = {
        .001f, ozz::math::Float3(10.f, 10.f, 10.f)};
    raw_animation.tracks[0].translations.push_back(key1);
    const RawAnimation::TranslationKey key2 = {
        .98f, ozz::math::Float3(20.f, 20.f, 20.f)};
    raw_animation.tracks[0].translations.push_back(key2);
  }

  // Track 1 has lots of keys
  {
    for (size_t i = 0; i < kMaxKey; ++i) {
      const RawAnimation::TranslationKey key = {i * 1.f / kMaxKey,
                                                ozz::math::Float3(0, 0, 0)};
      raw_animation.tracks[1].translations.push_back(key);
    }
  }

  // Track 2 has lots of keys (same timepoints as track 1 as number of
  // trackpoints is limited)
  {
    for (size_t i = 0; i < kMaxKey; ++i) {
      const RawAnimation::TranslationKey key = {
          i * 1.f / kMaxKey,
          ozz::math::Float3(std::cos(ozz::math::kPi * i / kMaxKey))};
      raw_animation.tracks[2].translations.push_back(key);
    }
  }

  // Track 3 has big hole
  {
    const RawAnimation::TranslationKey key0 = {
        0.f, ozz::math::Float3(0.f, 0.f, 0.f)};
    raw_animation.tracks[3].translations.push_back(key0);
    const RawAnimation::TranslationKey key1 = {
        0.001f, ozz::math::Float3(1.f, 1.f, 1.f)};
    raw_animation.tracks[3].translations.push_back(key1);
    const RawAnimation::TranslationKey key2 = {
        0.9f, ozz::math::Float3(2.f, 2.f, 2.f)};
    raw_animation.tracks[3].translations.push_back(key2);
    const RawAnimation::TranslationKey key3 = {
        0.91f, ozz::math::Float3(3.f, 3.f, 3.f)};
    raw_animation.tracks[3].translations.push_back(key3);
  }

  for (auto interval : {-1.f, 0.f, .0005f, .4f, 1.f, 200.f}) {
    AnimationBuilder builder;
    builder.iframe_interval = interval;

    ozz::unique_ptr<Animation> animation = builder(raw_animation);
    ASSERT_TRUE(animation);

    ozz::animation::SamplingJob job;
    job.animation = animation.get();
    ozz::animation::SamplingJob::Context context(4);
    job.context = &context;
    ozz::math::SoaTransform output[1];
    job.output = output;

    job.ratio = 0.f;
    EXPECT_TRUE(job.Validate());
    EXPECT_TRUE(job.Run());

    EXPECT_SOAFLOAT3_EQ_EST(output[0].translation, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f,
                            1.f, 0.f, 0.f, 0.f, 1.f, 0.f);
    EXPECT_SOAQUATERNION_EQ_EST(output[0].rotation, 0.f, 0.f, 0.f, 0.f, 0.f,
                                0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 1.f, 1.f,
                                1.f, 1.f);
    EXPECT_SOAFLOAT3_EQ_EST(output[0].scale, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f,
                            1.f, 1.f, 1.f, 1.f, 1.f);

    job.ratio = .99f;
    EXPECT_TRUE(job.Validate());
    EXPECT_TRUE(job.Run());

    EXPECT_SOAFLOAT3_EQ_EST(output[0].translation, 20.f, 0.f, -1.f, 3.f, 20.f,
                            0.f, -1.f, 3.f, 20.f, 0.f, -1.f, 3.f);

    job.ratio = .5f;
    EXPECT_TRUE(job.Validate());
    EXPECT_TRUE(job.Run());

    EXPECT_SOAFLOAT3_EQ_EST(output[0].translation, 15.097f, 0.f, 0.f, 1.555f,
                            15.097f, 0.f, 0.f, 1.555f, 15.097f, 0.f, 0.f,
                            1.555f);

    job.ratio = 1.f;
    EXPECT_TRUE(job.Validate());
    EXPECT_TRUE(job.Run());

    EXPECT_SOAFLOAT3_EQ_EST(output[0].translation, 20.f, 0.f, -1.f, 3.f, 20.f,
                            0.f, -1.f, 3.f, 20.f, 0.f, -1.f, 3.f);
  }
}