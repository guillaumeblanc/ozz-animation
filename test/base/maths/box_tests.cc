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

#include "ozz/base/maths/box.h"

#include "gtest/gtest.h"

#include "ozz/base/gtest_helper.h"
#include "ozz/base/maths/gtest_math_helper.h"

TEST(Validity, Box) {
  EXPECT_FALSE(ozz::math::Box().is_valid());
  EXPECT_FALSE(ozz::math::Box(ozz::math::Float3(0.f, 1.f, 2.f),
                              ozz::math::Float3(0.f, -1.f, 2.f)).is_valid());
  EXPECT_TRUE(ozz::math::Box(ozz::math::Float3(0.f, -1.f, 2.f),
                             ozz::math::Float3(0.f, 1.f, 2.f)).is_valid());
  EXPECT_TRUE(ozz::math::Box(ozz::math::Float3(0.f, 1.f, 2.f),
                             ozz::math::Float3(0.f, 1.f, 2.f)).is_valid());
}

TEST(Inside, Box) {
  const ozz::math::Box invalid(ozz::math::Float3(0.f, 1.f, 2.f),
                               ozz::math::Float3(0.f, -1.f, 2.f));
  EXPECT_FALSE(invalid.is_valid());
  EXPECT_FALSE(invalid.is_inside(ozz::math::Float3(0.f, 1.f, 2.f)));
  EXPECT_FALSE(invalid.is_inside(ozz::math::Float3(0.f, -.5f, 2.f)));
  EXPECT_FALSE(invalid.is_inside(ozz::math::Float3(-1.f, -2.f, -3.f)));

  const ozz::math::Box valid(ozz::math::Float3(-1.f, -2.f, -3.f),
                             ozz::math::Float3(1.f, 2.f, 3.f));
  EXPECT_TRUE(valid.is_valid());
  EXPECT_FALSE(valid.is_inside(ozz::math::Float3(0.f, -3.f, 0.f)));
  EXPECT_FALSE(valid.is_inside(ozz::math::Float3(0.f, 3.f, 0.f)));
  EXPECT_TRUE(valid.is_inside(ozz::math::Float3(-1.f, -2.f, -3.f)));
  EXPECT_TRUE(valid.is_inside(ozz::math::Float3(0.f, 0.f, 0.f)));
}

TEST(Merge, Box) {
  const ozz::math::Box invalid1(ozz::math::Float3(0.f, 1.f, 2.f),
                                ozz::math::Float3(0.f, -1.f, 2.f));
  EXPECT_FALSE(invalid1.is_valid());
  const ozz::math::Box invalid2(ozz::math::Float3(0.f, -1.f, 2.f),
                                ozz::math::Float3(0.f, 1.f, -2.f));
  EXPECT_FALSE(invalid2.is_valid());

  const ozz::math::Box valid1(ozz::math::Float3(-1.f, -2.f, -3.f),
                              ozz::math::Float3(1.f, 2.f, 3.f));
  EXPECT_TRUE(valid1.is_valid());
  const ozz::math::Box valid2(ozz::math::Float3(0.f, 5.f, -8.f),
                              ozz::math::Float3(1.f, 6.f, 0.f));
  EXPECT_TRUE(valid2.is_valid());

  // Both boxes are invalid.
  EXPECT_FALSE(Merge(invalid1, invalid2).is_valid());

  // One boxe is invalid.
  EXPECT_TRUE(Merge(invalid1, valid1).is_valid());
  EXPECT_TRUE(Merge(valid1, invalid1).is_valid());

  // Both boxes are valid.
  const ozz::math::Box merge = Merge(valid1, valid2);
  EXPECT_FLOAT3_EQ(merge.min, -1.f, -2.f, -8.f);
  EXPECT_FLOAT3_EQ(merge.max, 1.f, 6.f, 3.f);
}

TEST(Build, Box) {

  const struct {
    ozz::math::Float3 value;
    char pad;
  } points[] = {
    {ozz::math::Float3(0.f, 0.f, 0.f), 0},
    {ozz::math::Float3(1.f, -1.f, 0.f), 0},
    {ozz::math::Float3(0.f, 0.f, 46.f), 0},
    {ozz::math::Float3(-27.f, 0.f, 0.f), 0},
    {ozz::math::Float3(0.f, 58.f, 0.f), 0},
  };

  const ozz::math::Box invalid(&points->value,
                               sizeof(points[0]),
                               0);
  EXPECT_FALSE(invalid.is_valid());

  const ozz::math::Box valid(&points->value,
                             sizeof(points[0]),
                             OZZ_ARRAY_SIZE(points));
  EXPECT_TRUE(valid.is_valid());
  EXPECT_FLOAT3_EQ(valid.min, -27.f, -1.f, 0.f);
  EXPECT_FLOAT3_EQ(valid.max, 1.f, 58.f, 46.f);
}
