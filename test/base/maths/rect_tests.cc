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

#include "ozz/base/maths/rect.h"

#include "gtest/gtest.h"

#include "ozz/base/gtest_helper.h"

TEST(Int, Rect) {
  ozz::math::RectInt rect(10, 20, 30, 40);

  EXPECT_EQ(rect.right(), 40);
  EXPECT_EQ(rect.top(), 60);

  EXPECT_TRUE(rect.is_inside(10, 20));
  EXPECT_TRUE(rect.is_inside(39, 59));

  EXPECT_FALSE(rect.is_inside(9, 20));
  EXPECT_FALSE(rect.is_inside(10, 19));
  EXPECT_FALSE(rect.is_inside(40, 59));
  EXPECT_FALSE(rect.is_inside(39, 60));
}

TEST(Float, Rect) {
  ozz::math::RectFloat rect(10.f, 20.f, 30.f, 40.f);

  EXPECT_FLOAT_EQ(rect.right(), 40.f);
  EXPECT_FLOAT_EQ(rect.top(), 60.f);

  EXPECT_TRUE(rect.is_inside(10.f, 20.f));
  EXPECT_TRUE(rect.is_inside(39.f, 59.f));

  EXPECT_FALSE(rect.is_inside(9.f, 20.f));
  EXPECT_FALSE(rect.is_inside(10.f, 19.f));
  EXPECT_FALSE(rect.is_inside(40.f, 59.f));
  EXPECT_FALSE(rect.is_inside(39.f, 60.f));
}
