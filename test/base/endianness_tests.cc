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

#include "ozz/base/endianness.h"

#include "gtest/gtest.h"

TEST(NativeEndianness, Endianness) {
  // Uses pre-defined macro to check know endianness.
  // Endianness detection does not rely on this as this is not standard, but it
  // helps with testing.

  // x86 and x86-64 Little endian processors.
#if defined(i386) || defined(__i386__) ||  /*GNU C*/\
    defined(__X86__) ||  /*MinGW32*/\
    defined(__x86_64__) ||  /*Intel C/C++*/\
    defined(_M_IX86) ||  /*Visual Studio*/\
    defined(_M_X64)  /*Visual Studio*/
  EXPECT_EQ(ozz::GetNativeEndianness(), ozz::kLittleEndian);
#endif

  // PowerPc Big endian processors.
#if defined(__POWERPC__) || defined(__ppc__) ||\
    defined(__powerpc__) ||  /*GNU C*/\
    defined(__IA64__) || defined(__ia64__) ||\
    defined(_M_PPC)  /*Visual Studio*/
  EXPECT_EQ(ozz::GetNativeEndianness(), ozz::kBigEndian);
#endif

  // Itanium Big endian processors.
#if defined(_IA64) ||  /*GNU C*/\
    defined(_M_IA64) ||  /*Visual Studio*/\
    defined(_M_IA64)  /*Intel C/C++*/
  EXPECT_EQ(ozz::GetNativeEndianness(), ozz::kBigEndian);
#endif
}

TEST(Swap, Endianness) {
  {  // 1 byte swapping.
    ozz::uint8 uo = 0x46;
    EXPECT_EQ(ozz::EndianSwap(uo), 0x46);
  }
  {  // 1 byte array swapping.
    ozz::uint8 uo[] = {0x46, 0x58};
    ozz::EndianSwap(uo, OZZ_ARRAY_SIZE(uo));
    EXPECT_EQ(uo[0], 0x46);
    EXPECT_EQ(uo[1], 0x58);
  }
  {  // 2 bytes swapping.
    ozz::uint16 uo = 0x4699;
    EXPECT_EQ(ozz::EndianSwap(uo), 0x9946);
  }
  {  // 2 bytes array swapping.
    ozz::uint16 uo[] = {0x4699, 0x5814};
    ozz::EndianSwap(uo, OZZ_ARRAY_SIZE(uo));
    EXPECT_EQ(uo[0], 0x9946);
    EXPECT_EQ(uo[1], 0x1458);
  }
  {  // 4 bytes swapping.
    ozz::uint32 uo = 0x46992715;
    EXPECT_EQ(ozz::EndianSwap(uo), 0x15279946u);
  }
  {  // 2 bytes array swapping.
    ozz::uint32 uo[] = {0x46992715, 0x58142611};
    ozz::EndianSwap(uo, OZZ_ARRAY_SIZE(uo));
    EXPECT_EQ(uo[0], 0x15279946u);
    EXPECT_EQ(uo[1], 0x11261458u);
  }
  {  // 8 bytes swapping.
    ozz::uint64 uo = 0x4699271511190417ull;
    EXPECT_EQ(ozz::EndianSwap(uo), 0x1704191115279946ull);
  }
  {  // 8 bytes array swapping.
    ozz::uint64 uo[] = {0x4699271511190417ull, 0x5814264669080735ull};
    ozz::EndianSwap(uo, OZZ_ARRAY_SIZE(uo));
    EXPECT_EQ(uo[0], 0x1704191115279946ull);
    EXPECT_EQ(uo[1], 0x3507086946261458ull);
  }
}
