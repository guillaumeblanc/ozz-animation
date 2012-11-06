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

#include "ozz/base/platform.h"

#include <cassert>
#include <climits>

#include "gtest/gtest.h"

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
  EXPECT_EQ(reinterpret_cast<ozz::uintptr>(&alined) & (128 - 1), 0u);
}

TEST(TypeSize, Platform) {
  // Checks sizes.
  OZZ_STATIC_ASSERT(CHAR_BIT == 8);
  OZZ_STATIC_ASSERT(sizeof(ozz::int8) == 1);
  OZZ_STATIC_ASSERT(sizeof(ozz::uint8) == 1);
  OZZ_STATIC_ASSERT(sizeof(ozz::int16) == 2);
  OZZ_STATIC_ASSERT(sizeof(ozz::uint16) == 2);
  OZZ_STATIC_ASSERT(sizeof(ozz::int32) == 4);
  OZZ_STATIC_ASSERT(sizeof(ozz::uint32) == 4);
  OZZ_STATIC_ASSERT(sizeof(ozz::int64) == 8);
  OZZ_STATIC_ASSERT(sizeof(ozz::uint64) == 8);
  OZZ_STATIC_ASSERT(sizeof(ozz::intptr) == sizeof(int*));
  OZZ_STATIC_ASSERT(sizeof(ozz::uintptr) == sizeof(unsigned int*));

  // Checks signs. Right shift maintains sign bit for signed types, and fills
  // with 0 for unsigned types.
  OZZ_STATIC_ASSERT((ozz::int8(-1) >> 1) == -1);
  OZZ_STATIC_ASSERT((ozz::int16(-1) >> 1) == -1);
  OZZ_STATIC_ASSERT((ozz::int32(-1) >> 1) == -1);
  OZZ_STATIC_ASSERT((ozz::int64(-1) >> 1) == -1);
  OZZ_STATIC_ASSERT((ozz::uint8(-1) >> 1) == 0x7f);
  OZZ_STATIC_ASSERT((ozz::uint16(-1) >> 1) == 0x7fff);
  OZZ_STATIC_ASSERT((ozz::uint32(-1) >> 1) == 0x7fffffff);
  OZZ_STATIC_ASSERT((ozz::uint64(-1) >> 1) == 0x7fffffffffffffffLL);

  
#if __WORDSIZE == 8
#endif 

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
