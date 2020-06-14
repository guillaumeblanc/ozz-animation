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

#include "ozz/animation/runtime/animation_utils.h"

#include "gtest/gtest.h"
#include "ozz/base/maths/gtest_math_helper.h"

#include "ozz/base/memory/unique_ptr.h"

#include "ozz/animation/offline/animation_builder.h"
#include "ozz/animation/offline/raw_animation.h"

using ozz::animation::Animation;
using ozz::animation::offline::RawAnimation;
using ozz::animation::offline::AnimationBuilder;

TEST(CountKeyframes, AnimationUtils) {
  // Builds a valid animation.
  ozz::unique_ptr<Animation> animation;

  {
    RawAnimation raw_animation;
    raw_animation.duration = 1.f;
    raw_animation.tracks.resize(2);

    RawAnimation::TranslationKey t_key0 = {0.f,
                                           ozz::math::Float3(93.f, 58.f, 46.f)};
    raw_animation.tracks[0].translations.push_back(t_key0);
    RawAnimation::TranslationKey t_key1 = {.9f,
                                           ozz::math::Float3(46.f, 58.f, 93.f)};
    raw_animation.tracks[0].translations.push_back(t_key1);
    RawAnimation::TranslationKey t_key2 = {1.f,
                                           ozz::math::Float3(46.f, 58.f, 99.f)};
    raw_animation.tracks[0].translations.push_back(t_key2);

    RawAnimation::RotationKey r_key = {
        0.7f, ozz::math::Quaternion(0.f, 1.f, 0.f, 0.f)};
    raw_animation.tracks[0].rotations.push_back(r_key);

    RawAnimation::ScaleKey s_key = {0.1f, ozz::math::Float3(99.f, 26.f, 14.f)};
    raw_animation.tracks[1].scales.push_back(s_key);

    AnimationBuilder builder;
    animation = builder(raw_animation);
    ASSERT_TRUE(animation);
  }

  // 4 more tracks than expected due to SoA
  EXPECT_EQ(ozz::animation::CountTranslationKeyframes(*animation, -1), 9);
  EXPECT_EQ(ozz::animation::CountTranslationKeyframes(*animation, 0), 3);
  EXPECT_EQ(ozz::animation::CountTranslationKeyframes(*animation, 1), 2);

  EXPECT_EQ(ozz::animation::CountRotationKeyframes(*animation, -1), 8);
  EXPECT_EQ(ozz::animation::CountRotationKeyframes(*animation, 0), 2);
  EXPECT_EQ(ozz::animation::CountRotationKeyframes(*animation, 1), 2);

  EXPECT_EQ(ozz::animation::CountScaleKeyframes(*animation, -1), 8);
  EXPECT_EQ(ozz::animation::CountScaleKeyframes(*animation, 0), 2);
  EXPECT_EQ(ozz::animation::CountScaleKeyframes(*animation, 1), 2);
}
