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

#include "ozz/base/maths/simd_math_archive.h"

#include "gtest/gtest.h"

#include "ozz/base/maths/gtest_math_helper.h"

#include "ozz/base/io/archive.h"
#include "ozz/base/maths/simd_math.h"

TEST(SimdMaths, Archive) {
  for (int e = 0; e < 2; ++e) {
    ozz::Endianness endianess = e == 0 ? ozz::kBigEndian : ozz::kLittleEndian;

    ozz::io::MemoryStream stream;
    ASSERT_TRUE(stream.opened());

    // Write soa math types.
    ozz::io::OArchive o(&stream, endianess);
    const ozz::math::SimdFloat4 of4 =
      ozz::math::simd_float4::Load(46.f, 58.f, 14.f, 5.f);
    o << of4;
    const ozz::math::SimdInt4 oi4 =
      ozz::math::simd_int4::Load(46, 58, 14, 5);
    o << oi4;
    const ozz::math::Float4x4 of44 = {{
      ozz::math::simd_float4::Load(46.f, 58.f, 14.f, 5.f),
      ozz::math::simd_float4::Load(26.f, 35.f, 1.f, 27.f),
      ozz::math::simd_float4::Load(99.f, 11.f, 4.f, 46.f),
      ozz::math::simd_float4::Load(58.f, 26.f, 14.f, 99.f)}};
    o << of44;

    // Reads soa math types.
    stream.Seek(0, ozz::io::Stream::kSet);
    ozz::io::IArchive i(&stream);
    ozz::math::SimdFloat4 if4;
    i >> if4;
    EXPECT_SIMDFLOAT_EQ(if4, 46.f, 58.f, 14.f, 5.f);
    ozz::math::SimdInt4 ii4;
    i >> ii4;
    EXPECT_SIMDINT_EQ(ii4, 46, 58, 14, 5);
    ozz::math::Float4x4 if44;
    i >> if44;
    EXPECT_FLOAT4x4_EQ(if44, 46.f, 58.f, 14.f, 5.f,
                             26.f, 35.f, 1.f, 27.f,
                             99.f, 11.f, 4.f, 46.f,
                             58.f, 26.f, 14.f, 99.f);
  }
}
