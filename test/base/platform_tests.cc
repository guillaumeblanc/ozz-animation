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

#include <stdint.h>

#include <cassert>
#include <climits>

#include "gtest/gtest.h"
#include "ozz/base/gtest_helper.h"
#include "ozz/base/platform.h"

TEST(StaticAssertion, Platform) {
  static_assert(2 == 2, "Must compile.");
  // static_assert(1 == 2, "Must not compile.);
}

namespace {
// Declares a structure that should have at least 8 bytes aligned.
struct Misc {
  double d;
  char c;
  int i;
};

// Declares an aligned structure in order to test OZZ_ALIGN and AlignOf.
struct Aligned {
  alignas(128) char c;
};
}  // namespace

TEST(Alignment, Platform) {
  static_assert(alignof(char) == 1, "Unexpected type size");
  static_assert(alignof(double) == 8, "Unexpected type size");
  static_assert(alignof(Misc) == 8, "Unexpected type size");
  static_assert(alignof(Aligned) == 128, "Unexpected type size");

  Aligned alined;
  EXPECT_EQ(reinterpret_cast<uintptr_t>(&alined) & (128 - 1), 0u);
}

TEST(IntegerAlignment, Platform) {
  {
    short s = 0x1234;
    int aligned_s = ozz::Align(s, 128);
    EXPECT_TRUE(aligned_s == 0x1280);
    EXPECT_TRUE(ozz::IsAligned(aligned_s, 128));
  }

  {
    int i = 0x00a01234;
    int aligned_i = ozz::Align(i, 1024);
    EXPECT_TRUE(aligned_i == 0x00a01400);
    EXPECT_TRUE(ozz::IsAligned(aligned_i, 1024));
  }
}

TEST(PointerAlignment, Platform) {
  void* p = reinterpret_cast<void*>(0x00a01234);
  void* aligned_p = ozz::Align(p, 1024);
  EXPECT_TRUE(aligned_p == reinterpret_cast<void*>(0x00a01400));
  EXPECT_TRUE(ozz::IsAligned(aligned_p, 1024));
}

TEST(TypeSize, Platform) {
  // Checks sizes.
  static_assert(CHAR_BIT == 8, "Unexpected type size");
  static_assert(sizeof(int8_t) == 1, "Unexpected type size");
  static_assert(sizeof(uint8_t) == 1, "Unexpected type size");
  static_assert(sizeof(int16_t) == 2, "Unexpected type size");
  static_assert(sizeof(uint16_t) == 2, "Unexpected type size");
  static_assert(sizeof(int32_t) == 4, "Unexpected type size");
  static_assert(sizeof(uint32_t) == 4, "Unexpected type size");
  static_assert(sizeof(int64_t) == 8, "Unexpected type size");
  static_assert(sizeof(uint64_t) == 8, "Unexpected type size");
  static_assert(sizeof(intptr_t) == sizeof(int*), "Unexpected type size");
  static_assert(sizeof(uintptr_t) == sizeof(unsigned int*),
                "Unexpected type size");

  // Checks signs. Right shift maintains sign bit for signed types, and fills
  // with 0 for unsigned types.
  static_assert((int8_t(-1) >> 1) == -1, "Unexpected type sign");
  static_assert((int16_t(-1) >> 1) == -1, "Unexpected type sign");
  static_assert((int32_t(-1) >> 1) == -1, "Unexpected type sign");
  static_assert((int64_t(-1) >> 1) == -1, "Unexpected type sign");
  static_assert((uint8_t(-1) >> 1) == 0x7f, "Unexpected type sign");
  static_assert((uint16_t(-1) >> 1) == 0x7fff, "Unexpected type sign");
  static_assert((uint32_t(-1) >> 1) == 0x7fffffff, "Unexpected type sign");
  static_assert((uint64_t(-1) >> 1) == 0x7fffffffffffffffLL,
                "Unexpected type sign");

  // Assumes that an "int" is at least 32 bits.
  static_assert(sizeof(int) >= 4, "Unexpected type size");

  // "char" type is used to manipulate bytes. Can be signed or unsigned.
  static_assert(sizeof(char) == 1, "Unexpected type size");
}

TEST(DebudNDebug, Platform) {
  OZZ_IF_DEBUG(assert(true));
  OZZ_IF_NDEBUG(assert(false));
}

TEST(ArraySize, Platform) {
  int ai[46];
  (void)ai;
  static_assert(OZZ_ARRAY_SIZE(ai) == 46, "Unexpected array size");

  char ac[] = "forty six";
  (void)ac;
  static_assert(OZZ_ARRAY_SIZE(ac) == 10, "Unexpected array size");
}

TEST(StrMatch, Platform) {
  EXPECT_TRUE(ozz::strmatch("a", "a"));
  EXPECT_FALSE(ozz::strmatch("a", "b"));
  EXPECT_TRUE(ozz::strmatch("a", "a*"));
  EXPECT_FALSE(ozz::strmatch("a", "a?"));
  EXPECT_TRUE(ozz::strmatch("ab", "a?"));
  EXPECT_TRUE(ozz::strmatch("ab", "?b"));
  EXPECT_FALSE(ozz::strmatch("ab", "a"));
  EXPECT_TRUE(ozz::strmatch("ab", "ab"));
  EXPECT_TRUE(ozz::strmatch("", ""));
  EXPECT_TRUE(ozz::strmatch("", "*"));
  EXPECT_FALSE(ozz::strmatch("", "?"));
  EXPECT_FALSE(ozz::strmatch("ab", ""));
  EXPECT_FALSE(ozz::strmatch("ab", "?"));
  EXPECT_TRUE(ozz::strmatch("ab", "??"));
  EXPECT_TRUE(ozz::strmatch("a*b", "a*b"));
  EXPECT_TRUE(ozz::strmatch("a*b", "a?b"));
  EXPECT_TRUE(ozz::strmatch("ab", "ab*"));
  EXPECT_TRUE(ozz::strmatch("ab", "a*"));
  EXPECT_TRUE(ozz::strmatch("ab", "*b"));
  EXPECT_TRUE(ozz::strmatch("ab", "a*b"));
  EXPECT_TRUE(ozz::strmatch("acb", "a*b"));
  EXPECT_FALSE(ozz::strmatch("abc", "a*b"));
  EXPECT_TRUE(ozz::strmatch("abcdef", "a*c*"));
  EXPECT_TRUE(ozz::strmatch("abc.def", "a*c.*"));
  EXPECT_TRUE(ozz::strmatch("abc.def", "abc.def"));
  EXPECT_TRUE(ozz::strmatch("abc.def", "abc.def***"));
  EXPECT_FALSE(ozz::strmatch("abc.def", "abc.def?"));
  EXPECT_TRUE(ozz::strmatch("abc.def", "abc?def"));
  EXPECT_TRUE(ozz::strmatch("abc.def", "a*c?*"));
  EXPECT_TRUE(ozz::strmatch("abc.def", "a*.*"));
  EXPECT_TRUE(ozz::strmatch("abc.def", "a*c.*e?"));
  EXPECT_TRUE(ozz::strmatch("abc.def", "*"));
  EXPECT_TRUE(ozz::strmatch("abc.def", "*.*"));
  EXPECT_TRUE(ozz::strmatch("abc.def", "???.???"));
  EXPECT_FALSE(ozz::strmatch("abc.def", "??.???"));
  EXPECT_TRUE(ozz::strmatch("abc.def", "*??.???"));
  EXPECT_TRUE(ozz::strmatch("abc.def", "*??.??*"));
  EXPECT_TRUE(
      ozz::strmatch("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
                    "aaaaaaaaaaaaaaa",
                    "*a*??????a?????????a???????????????"));
}
