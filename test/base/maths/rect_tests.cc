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

#include "ozz/base/maths/rect.h"

#include "gtest/gtest.h"

#include "ozz/base/gtest_helper.h"

TEST(RectInt, ozz_math) {
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

TEST(RectFloat, ozz_math) {
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
