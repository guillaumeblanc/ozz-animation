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

#include "ozz/base/endianness.h"

#include "gtest/gtest.h"

TEST(NativeEndianness, Endianness) {
// Uses pre-defined macro to check know endianness.
// Endianness detection does not rely on this as this is not standard, but it
// helps with testing.

// x86 and x86-64 Little endian processors.
#if defined(i386) || defined(__i386__) || /*GNU C*/         \
    defined(__X86__) ||                   /*MinGW32*/       \
    defined(__x86_64__) ||                /*Intel C/C++*/   \
    defined(_M_IX86) ||                   /*Visual Studio*/ \
    defined(_M_X64)                       /*Visual Studio*/
  EXPECT_EQ(ozz::GetNativeEndianness(), ozz::kLittleEndian);
#endif

// PowerPc Big endian processors.
#if defined(__POWERPC__) || defined(__ppc__) || \
    defined(__powerpc__) || /*GNU C*/           \
    defined(__IA64__) || defined(__ia64__) ||   \
    defined(_M_PPC) /*Visual Studio*/
  EXPECT_EQ(ozz::GetNativeEndianness(), ozz::kBigEndian);
#endif

// Itanium Big endian processors.
#if defined(_IA64) ||   /*GNU C*/         \
    defined(_M_IA64) || /*Visual Studio*/ \
    defined(_M_IA64)    /*Intel C/C++*/
  EXPECT_EQ(ozz::GetNativeEndianness(), ozz::kBigEndian);
#endif
}

TEST(Swap, Endianness) {
  {  // 1 byte swapping.
    uint8_t uo = 0x46;
    EXPECT_EQ(ozz::EndianSwap(uo), 0x46);
  }
  {  // 1 byte array swapping.
    uint8_t uo[] = {0x46, 0x58};
    ozz::EndianSwap(uo, OZZ_ARRAY_SIZE(uo));
    EXPECT_EQ(uo[0], 0x46);
    EXPECT_EQ(uo[1], 0x58);
  }
  {  // 2 bytes swapping.
    uint16_t uo = 0x4699;
    EXPECT_EQ(ozz::EndianSwap(uo), 0x9946);
  }
  {  // 2 bytes array swapping.
    uint16_t uo[] = {0x4699, 0x5814};
    ozz::EndianSwap(uo, OZZ_ARRAY_SIZE(uo));
    EXPECT_EQ(uo[0], 0x9946);
    EXPECT_EQ(uo[1], 0x1458);
  }
  {  // 4 bytes swapping.
    uint32_t uo = 0x46992715;
    EXPECT_EQ(ozz::EndianSwap(uo), 0x15279946u);
  }
  {  // 2 bytes array swapping.
    uint32_t uo[] = {0x46992715, 0x58142611};
    ozz::EndianSwap(uo, OZZ_ARRAY_SIZE(uo));
    EXPECT_EQ(uo[0], 0x15279946u);
    EXPECT_EQ(uo[1], 0x11261458u);
  }
  {  // 8 bytes swapping.
    uint64_t uo = 0x4699271511190417ull;
    EXPECT_EQ(ozz::EndianSwap(uo), 0x1704191115279946ull);
  }
  {  // 8 bytes array swapping.
    uint64_t uo[] = {0x4699271511190417ull, 0x5814264669080735ull};
    ozz::EndianSwap(uo, OZZ_ARRAY_SIZE(uo));
    EXPECT_EQ(uo[0], 0x1704191115279946ull);
    EXPECT_EQ(uo[1], 0x3507086946261458ull);
  }
}
