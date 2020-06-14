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

#include "ozz/base/maths/box.h"
#include "ozz/base/maths/simd_math.h"

#include "gtest/gtest.h"

#include "ozz/base/gtest_helper.h"
#include "ozz/base/maths/gtest_math_helper.h"

TEST(BoxValidity, ozz_math) {
  EXPECT_FALSE(ozz::math::Box().is_valid());
  EXPECT_FALSE(ozz::math::Box(ozz::math::Float3(0.f, 1.f, 2.f),
                              ozz::math::Float3(0.f, -1.f, 2.f))
                   .is_valid());
  EXPECT_TRUE(ozz::math::Box(ozz::math::Float3(0.f, -1.f, 2.f),
                             ozz::math::Float3(0.f, 1.f, 2.f))
                  .is_valid());
  EXPECT_TRUE(ozz::math::Box(ozz::math::Float3(0.f, 1.f, 2.f),
                             ozz::math::Float3(0.f, 1.f, 2.f))
                  .is_valid());
}

TEST(BoxInside, ozz_math) {
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

TEST(BoxMerge, ozz_math) {
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

  // One box is invalid.
  EXPECT_TRUE(Merge(invalid1, valid1).is_valid());
  EXPECT_TRUE(Merge(valid1, invalid1).is_valid());

  // Both boxes are valid.
  const ozz::math::Box merge = Merge(valid1, valid2);
  EXPECT_FLOAT3_EQ(merge.min, -1.f, -2.f, -8.f);
  EXPECT_FLOAT3_EQ(merge.max, 1.f, 6.f, 3.f);
}

TEST(BoxTransform, ozz_math) {
  const ozz::math::Box a(ozz::math::Float3(1.f, 2.f, 3.f),
                         ozz::math::Float3(4.f, 5.f, 6.f));

  const ozz::math::Box ia = TransformBox(ozz::math::Float4x4::identity(), a);
  EXPECT_FLOAT3_EQ(ia.min, 1.f, 2.f, 3.f);
  EXPECT_FLOAT3_EQ(ia.max, 4.f, 5.f, 6.f);

  const ozz::math::Box ta =
      TransformBox(ozz::math::Float4x4::Translation(
                       ozz::math::simd_float4::Load(2.f, -2.f, 3.f, 0.f)),
                   a);
  EXPECT_FLOAT3_EQ(ta.min, 3.f, 0.f, 6.f);
  EXPECT_FLOAT3_EQ(ta.max, 6.f, 3.f, 9.f);

  const ozz::math::Box ra =
      TransformBox(ozz::math::Float4x4::FromAxisAngle(
                       ozz::math::simd_float4::y_axis(),
                       ozz::math::simd_float4::LoadX(ozz::math::kPi)),
                   a);
  EXPECT_FLOAT3_EQ(ra.min, -4.f, 2.f, -6.f);
  EXPECT_FLOAT3_EQ(ra.max, -1.f, 5.f, -3.f);
}

TEST(BoxBuild, ozz_math) {
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

  // Builds from a single point
  const ozz::math::Box single_valid(points[1].value);
  EXPECT_TRUE(single_valid.is_valid());
  EXPECT_FLOAT3_EQ(single_valid.min, 1.f, -1.f, 0.f);
  EXPECT_FLOAT3_EQ(single_valid.max, 1.f, -1.f, 0.f);

  // Builds from multiple points
  EXPECT_ASSERTION(ozz::math::Box(&points->value, 1, OZZ_ARRAY_SIZE(points)),
                   "_stride must be greater or equal to sizeof\\(Float3\\)");

  const ozz::math::Box multi_invalid(&points->value, sizeof(points[0]), 0);
  EXPECT_FALSE(multi_invalid.is_valid());

  const ozz::math::Box multi_valid(&points->value, sizeof(points[0]),
                                   OZZ_ARRAY_SIZE(points));
  EXPECT_TRUE(multi_valid.is_valid());
  EXPECT_FLOAT3_EQ(multi_valid.min, -27.f, -1.f, 0.f);
  EXPECT_FLOAT3_EQ(multi_valid.max, 1.f, 58.f, 46.f);
}
