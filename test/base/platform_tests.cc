//============================================================================//
//                                                                            //
// ozz-animation, 3d skeletal animation libraries and tools.                  //
// https://code.google.com/p/ozz-animation/                                   //
//                                                                            //
//----------------------------------------------------------------------------//
//                                                                            //
// Copyright (c) 2012-2015 Guillaume Blanc                                    //
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

#include "ozz/base/platform.h"

#include <cassert>
#include <climits>
#include <stdint.h>

#include "gtest/gtest.h"
#include "ozz/base/gtest_helper.h"

TEST(StaticAssertion, Platform) {
  OZZ_STATIC_ASSERT(2 == 2);
  // OZZ_STATIC_ASSERT(1 == 2);  // Must not compile.
}

namespace {
// Declares a structure that should have at least 8 bytes aligned.
struct Misc { double d; char c; int i; };

// Declares an aligned structure in order to test OZZ_ALIGN and AlignOf.
struct Aligned { OZZ_ALIGN(128) char c; };
}  // namespace

TEST(Alignment, Platform) {
  OZZ_STATIC_ASSERT(ozz::AlignOf<char>::value >= 1);
  OZZ_STATIC_ASSERT(ozz::AlignOf<double>::value >= 8);
  OZZ_STATIC_ASSERT(ozz::AlignOf<Misc>::value >= 8);
  OZZ_STATIC_ASSERT(ozz::AlignOf<Aligned>::value >= 128);

  Aligned alined;
  EXPECT_EQ(reinterpret_cast<uintptr_t>(&alined) & (128 - 1), 0u);
}

TEST(TypeSize, Platform) {
  // Checks sizes.
  OZZ_STATIC_ASSERT(CHAR_BIT == 8);
  OZZ_STATIC_ASSERT(sizeof(int8_t) == 1);
  OZZ_STATIC_ASSERT(sizeof(uint8_t) == 1);
  OZZ_STATIC_ASSERT(sizeof(int16_t) == 2);
  OZZ_STATIC_ASSERT(sizeof(uint16_t) == 2);
  OZZ_STATIC_ASSERT(sizeof(int32_t) == 4);
  OZZ_STATIC_ASSERT(sizeof(uint32_t) == 4);
  OZZ_STATIC_ASSERT(sizeof(int64_t) == 8);
  OZZ_STATIC_ASSERT(sizeof(uint64_t) == 8);
  OZZ_STATIC_ASSERT(sizeof(intptr_t) == sizeof(int*));
  OZZ_STATIC_ASSERT(sizeof(uintptr_t) == sizeof(unsigned int*));

  // Checks signs. Right shift maintains sign bit for signed types, and fills
  // with 0 for unsigned types.
  OZZ_STATIC_ASSERT((int8_t(-1) >> 1) == -1);
  OZZ_STATIC_ASSERT((int16_t(-1) >> 1) == -1);
  OZZ_STATIC_ASSERT((int32_t(-1) >> 1) == -1);
  OZZ_STATIC_ASSERT((int64_t(-1) >> 1) == -1);
  OZZ_STATIC_ASSERT((uint8_t(-1) >> 1) == 0x7f);
  OZZ_STATIC_ASSERT((uint16_t(-1) >> 1) == 0x7fff);
  OZZ_STATIC_ASSERT((uint32_t(-1) >> 1) == 0x7fffffff);
  OZZ_STATIC_ASSERT((uint64_t(-1) >> 1) == 0x7fffffffffffffffLL);

  // Assumes that an "int" is at least 32 bits.
  OZZ_STATIC_ASSERT(sizeof(int) >= 4);

  // "char" type is used to manipulate signed bytes.
  OZZ_STATIC_ASSERT(sizeof(char) == 1);
  OZZ_STATIC_ASSERT((char(-1) >> 1) == -1);
}

TEST(DebudNDebug, Platform) {
  OZZ_IF_DEBUG(assert(true));
  OZZ_IF_NDEBUG(assert(false));
}

TEST(ArraySize, Platform) {
  int ai[46];
  (void)ai;
  OZZ_STATIC_ASSERT(OZZ_ARRAY_SIZE(ai) == 46);

  char ac[] = "forty six";
  (void)ac;
  OZZ_STATIC_ASSERT(OZZ_ARRAY_SIZE(ac) == 10);
}

TEST(Range, Memory) {
  int ai[46];
  const size_t array_size = OZZ_ARRAY_SIZE(ai);

  ozz::Range<int> empty;
  EXPECT_TRUE(empty.begin == NULL);
  EXPECT_TRUE(empty.end == NULL);
  EXPECT_EQ(empty.Count(), 0u);
  EXPECT_EQ(empty.Size(), 0u);

  EXPECT_ASSERTION(empty[46], "Index out of range");

  ozz::Range<int> cs1(ai, ai + array_size);
  EXPECT_EQ(cs1.begin, ai);
  EXPECT_EQ(cs1.end, ai + array_size);
  EXPECT_EQ(cs1.Count(), array_size);
  EXPECT_EQ(cs1.Size(), sizeof(ai));

  // Re-inint
  ozz::Range<int> reinit;
  reinit = ai;
  EXPECT_EQ(reinit.begin, ai);
  EXPECT_EQ(reinit.end, ai + array_size);
  EXPECT_EQ(reinit.Count(), array_size);
  EXPECT_EQ(reinit.Size(), sizeof(ai));

  cs1[12] = 46;
  EXPECT_EQ(cs1[12], 46);
  EXPECT_ASSERTION(cs1[46], "Index out of range");

  ozz::Range<int> cs2(ai, array_size);
  EXPECT_EQ(cs2.begin, ai);
  EXPECT_EQ(cs2.end, ai + array_size);
  EXPECT_EQ(cs2.Count(), array_size);
  EXPECT_EQ(cs2.Size(), sizeof(ai));

  ozz::Range<int> carray(ai);
  EXPECT_EQ(carray.begin, ai);
  EXPECT_EQ(carray.end, ai + array_size);
  EXPECT_EQ(carray.Count(), array_size);
  EXPECT_EQ(carray.Size(), sizeof(ai));

  ozz::Range<int> copy(cs2);
  EXPECT_EQ(cs2.begin, copy.begin);
  EXPECT_EQ(cs2.end, copy.end);
  EXPECT_EQ(cs2.Size(), copy.Size());

  ozz::Range<const int> const_copy(cs2);
  EXPECT_EQ(cs2.begin, const_copy.begin);
  EXPECT_EQ(cs2.end, const_copy.end);
  EXPECT_EQ(cs2.Size(), const_copy.Size());

  EXPECT_EQ(cs2[12], 46);
  EXPECT_ASSERTION(cs2[46], "Index out of range");
}
