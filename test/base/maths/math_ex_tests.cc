//============================================================================//
//                                                                            //
// ozz-animation, 3d skeletal animation libraries and tools.                  //
// https://code.google.com/p/ozz-animation/                                   //
//                                                                            //
//----------------------------------------------------------------------------//
//                                                                            //
// Copyright (c) 2012-2014 Guillaume Blanc                                    //
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
//                                                                            //
//============================================================================//

#include "ozz/base/maths/math_ex.h"
#include "ozz/base/maths/math_constant.h"

#include "gtest/gtest.h"

#include "ozz/base/gtest_helper.h"

TEST(Trigonometry, MathEx) {
  EXPECT_FLOAT_EQ(ozz::math::kPi, 3.1415926535897932384626433832795f);
  EXPECT_FLOAT_EQ(ozz::math::kPi * ozz::math::kRadianToDegree, 180.f);
  EXPECT_FLOAT_EQ(180.f * ozz::math::kDegreeToRadian, ozz::math::kPi);
}

TEST(FloatArithmetic, MathEx) {
  EXPECT_FLOAT_EQ(ozz::math::Lerp(0.f, 1.f, 0.f), 0.f);
  EXPECT_FLOAT_EQ(ozz::math::Lerp(0.f, 1.f, 1.f), 1.f);
  EXPECT_FLOAT_EQ(ozz::math::Lerp(0.f, 1.f, .3f), .3f);
  EXPECT_FLOAT_EQ(ozz::math::Lerp(0.f, 1.f, 12.f), 12.f);
  EXPECT_FLOAT_EQ(ozz::math::Lerp(0.f, 1.f, -12.f), -12.f);
}

TEST(FloatComparison, MathEx) {
  const float a = {.5f};
  const float b = {4.f};
  const float c = {2.f};

  const float min = ozz::math::Min(a, b);
  EXPECT_FLOAT_EQ(min, a);

  const float max = ozz::math::Max(a, b);
  EXPECT_FLOAT_EQ(max, b);

  const float clamp = ozz::math::Clamp(a, c, b);
  EXPECT_FLOAT_EQ(clamp, c);

  const float clamp0 = ozz::math::Clamp(a, b, c);
  EXPECT_FLOAT_EQ(clamp0, c);

  const float clamp1 = ozz::math::Clamp(c, a, b);
  EXPECT_FLOAT_EQ(clamp1, c);
}

TEST(Select, MathEx) {
  int a = -27, b = 46;
  int* pa = &a, *pb = &b;
  int* cpa = &a, *cpb = &b;

  { // Integer select
    EXPECT_EQ(ozz::math::Select(true, a, b), a);
    EXPECT_EQ(ozz::math::Select(true, b, a), b);
    EXPECT_EQ(ozz::math::Select(false, a, b), b);
  }

  { // Float select
    EXPECT_FLOAT_EQ(ozz::math::Select(true, 46.f, 27.f), 46.f);
    EXPECT_FLOAT_EQ(ozz::math::Select(false, 99.f, 46.f), 46.f);
  }

  { // Pointer select
    EXPECT_EQ(ozz::math::Select(true, pa, pb), pa);
    EXPECT_EQ(ozz::math::Select(true, pb, pa), pb);
    EXPECT_EQ(ozz::math::Select(false, pa, pb), pb);
  }

  { // Const pointer select
    EXPECT_EQ(ozz::math::Select(true, cpa, cpb), cpa);
    EXPECT_EQ(ozz::math::Select(true, cpb, cpa), cpb);
    EXPECT_EQ(ozz::math::Select(false, cpa, cpb), cpb);
    EXPECT_EQ(ozz::math::Select(false, pa, cpb), cpb);
  }
}

TEST(IntegerAlignment, Memory) {
  {
    short s = 0x1234;
    int aligned_s = ozz::math::Align(s, 128);
    EXPECT_TRUE(aligned_s == 0x1280);
    EXPECT_TRUE(ozz::math::IsAligned(aligned_s, 128));
  }

  {
    int i = 0x00a01234;
    int aligned_i = ozz::math::Align(i, 1024);
    EXPECT_TRUE(aligned_i == 0x00a01400);
    EXPECT_TRUE(ozz::math::IsAligned(aligned_i, 1024));
  }
}

TEST(PointerAlignment, Memory) {
  void* p = reinterpret_cast<void*>(0x00a01234);
  void* aligned_p = ozz::math::Align(p, 1024);
  EXPECT_TRUE(aligned_p == reinterpret_cast<void*>(0x00a01400));
  EXPECT_TRUE(ozz::math::IsAligned(aligned_p, 1024));
}
