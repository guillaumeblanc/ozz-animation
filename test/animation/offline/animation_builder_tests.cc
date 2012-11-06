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

#include "ozz/animation/offline/animation_builder.h"

#include "gtest/gtest.h"

#include "ozz/base/memory/allocator.h"
#include "ozz/animation/animation.h"
#include "ozz/animation/key_frame.h"

using ozz::animation::Animation;
using ozz::animation::offline::RawAnimation;
using ozz::animation::offline::AnimationBuilder;

TEST(Error, AnimationBuilder) {
  // Instantiates a builder objects with default parameters.
  AnimationBuilder builder;

  { // Building an empty Animation fails because animation duration
    // must be >= 0.
    RawAnimation raw_animation;
    raw_animation.duration = -1.f;  // Negative duration.
    EXPECT_FALSE(raw_animation.Validate());

    // Builds animation
    EXPECT_TRUE(!builder(raw_animation));
  }

  { // Building an empty Animation fails because animation duration
    // must be >= 0.
    RawAnimation raw_animation;
    raw_animation.duration = 0.f;  // Invalid duration.
    EXPECT_FALSE(raw_animation.Validate());

    // Builds animation
    EXPECT_TRUE(!builder(raw_animation));
  }

  { // Building default animation of 1' succeeds.
    RawAnimation raw_animation;
    EXPECT_EQ(raw_animation.duration, 1.f);
    EXPECT_TRUE(raw_animation.Validate());

    // Builds animation
    Animation* anim = builder(raw_animation);
    EXPECT_TRUE(anim != NULL);
    ozz::memory::default_allocator().Delete(anim);
  }
}

TEST(Build, AnimationBuilder) {
  // Instantiates a builder objects with default parameters.
  AnimationBuilder builder;

  { // Building an Animation with unsorted keys fails.
    RawAnimation raw_animation;
    raw_animation.duration = 1.f;
    raw_animation.tracks.resize(1);

    // Adds 2 unordered keys
    RawAnimation::TranslationKey first_key = {.8f, ozz::math::Float3::zero()};
    raw_animation.tracks[0].translations.push_back(first_key);
    RawAnimation::TranslationKey second_key = {.2f, ozz::math::Float3::zero()};
    raw_animation.tracks[0].translations.push_back(second_key);

    // Builds animation
    EXPECT_TRUE(!builder(raw_animation));
  }

  { // Building an Animation with invalid key frame's time fails.
    RawAnimation raw_animation;
    raw_animation.duration = 1.f;
    raw_animation.tracks.resize(1);

    // Adds a key with a time greater than animation duration.
    RawAnimation::TranslationKey first_key = {2.f, ozz::math::Float3::zero()};
    raw_animation.tracks[0].translations.push_back(first_key);

    // Builds animation
    EXPECT_TRUE(!builder(raw_animation));
  }

  { // Building an Animation with unsorted key frame's time fails.
    RawAnimation raw_animation;
    raw_animation.duration = 1.f;
    raw_animation.tracks.resize(2);

    // Adds 2 unsorted keys.
    RawAnimation::TranslationKey first_key = {0.7f, ozz::math::Float3::zero()};
    raw_animation.tracks[0].translations.push_back(first_key);
    RawAnimation::TranslationKey second_key = {0.1f, ozz::math::Float3::zero()};
    raw_animation.tracks[0].translations.push_back(second_key);

    // Builds animation
    EXPECT_TRUE(!builder(raw_animation));
  }

  { // Building an Animation with equal key frame's time fails.
    RawAnimation raw_animation;
    raw_animation.duration = 1.f;
    raw_animation.tracks.resize(2);

    // Adds 2 unsorted keys.
    RawAnimation::TranslationKey key = {0.7f, ozz::math::Float3::zero()};
    raw_animation.tracks[0].translations.push_back(key);
    raw_animation.tracks[0].translations.push_back(key);

    // Builds animation
    EXPECT_TRUE(!builder(raw_animation));
  }

  { // Building a valid Animation with empty tracks succeeds.
    RawAnimation raw_animation;
    raw_animation.duration = 1.f;
    raw_animation.tracks.resize(46);

    // Builds animation
    Animation* anim = builder(raw_animation);
    EXPECT_TRUE(anim != NULL);
    EXPECT_EQ(anim->num_tracks(), 46);
    ozz::memory::default_allocator().Delete(anim);
  }

  { // Building a valid Animation with 1 track succeeds.
    RawAnimation raw_animation;
    raw_animation.duration = 1.f;
    raw_animation.tracks.resize(1);

    RawAnimation::TranslationKey first_key = {0.7f, ozz::math::Float3::zero()};
    raw_animation.tracks[0].translations.push_back(first_key);

    // Builds animation
    Animation* anim = builder(raw_animation);
    EXPECT_TRUE(anim != NULL);
    EXPECT_EQ(anim->num_tracks(), 1);
    ozz::memory::default_allocator().Delete(anim);
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
    //     0                 1
    // -----------------------
    // 0 - |  A              |
    // 1 - |                 |
    // 2 - B  C   D   E      F
    // 3 - |  G       H      |

    // Final animation.
    //     0                 1
    // -----------------------
    // 0 - A0                4
    // 1 - 1                 5
    // 2 - B2 C6  D8 E10    F11
    // 3 - 3  G7     H9      12

    struct {int track; float time; float x;} results[] = {
      {0, 0.f, 0.f},
      {1, 0.f, 0.f},
      {2, 0.f, 2.f},
      {3, 0.f, 7.f},
      {0, 1.f, 0.f},
      {1, 1.f, 0.f},
      {2, .2f, 6.f},
      {3, .2f, 7.f},
      {2, .4f, 8.f},
      {3, .6f, 9.f},
      {2, .6f, 12.f},
      {2, 1.f, 11.f},
      {3, 1.f, 9.f}};

    RawAnimation::TranslationKey a = {.2f, ozz::math::Float3(0.f, 0.f, 0.f)};
    raw_animation.tracks[0].translations.push_back(a);

    RawAnimation::TranslationKey b = {0.f, ozz::math::Float3(2.f, 0.f, 0.f)};
    raw_animation.tracks[2].translations.push_back(b);
    RawAnimation::TranslationKey c = {0.2f, ozz::math::Float3(6.f, 0.f, 0.f)};
    raw_animation.tracks[2].translations.push_back(c);
    RawAnimation::TranslationKey d = {0.4f, ozz::math::Float3(8.f, 0.f, 0.f)};
    raw_animation.tracks[2].translations.push_back(d);
    RawAnimation::TranslationKey e = {0.6f, ozz::math::Float3(12.f, 0.f, 0.f)};
    raw_animation.tracks[2].translations.push_back(e);
    RawAnimation::TranslationKey f = {1.f, ozz::math::Float3(11.f, 0.f, 0.f)};
    raw_animation.tracks[2].translations.push_back(f);

    RawAnimation::TranslationKey g = {0.2f, ozz::math::Float3(7.f, 0.f, 0.f)};
    raw_animation.tracks[3].translations.push_back(g);
    RawAnimation::TranslationKey h = {0.6f, ozz::math::Float3(9.f, 0.f, 0.f)};
    raw_animation.tracks[3].translations.push_back(h);

    // Builds animation
    Animation* anim = builder(raw_animation);
    EXPECT_TRUE(anim != NULL);

    const std::size_t key_count = anim->translations_end() - anim->translations();
    EXPECT_EQ(key_count, 13u);
    for(std::size_t i = 0; i < key_count; i++) {
      EXPECT_EQ(anim->translations()[i].track, results[i].track);
      EXPECT_FLOAT_EQ(anim->translations()[i].time, results[i].time);
      EXPECT_FLOAT_EQ(anim->translations()[i].value.x, results[i].x);
    }

    ozz::memory::default_allocator().Delete(anim);
  }
}
