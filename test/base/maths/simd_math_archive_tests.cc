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

#include "ozz/base/maths/simd_math_archive.h"

#include "gtest/gtest.h"

#include "ozz/base/maths/gtest_math_helper.h"

#include "ozz/base/io/archive.h"
#include "ozz/base/maths/simd_math.h"

TEST(SimdMathArchive, ozz_simd_math) {
  for (int e = 0; e < 2; ++e) {
    ozz::Endianness endianess = e == 0 ? ozz::kBigEndian : ozz::kLittleEndian;

    ozz::io::MemoryStream stream;
    ASSERT_TRUE(stream.opened());

    // Write soa math types.
    ozz::io::OArchive o(&stream, endianess);
    const ozz::math::SimdFloat4 of4 =
        ozz::math::simd_float4::Load(46.f, 58.f, 14.f, 5.f);
    o << of4;
    const ozz::math::SimdInt4 oi4 = ozz::math::simd_int4::Load(46, 58, 14, 5);
    o << oi4;
    const ozz::math::Float4x4 of44 = {
        {ozz::math::simd_float4::Load(46.f, 58.f, 14.f, 5.f),
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
    EXPECT_FLOAT4x4_EQ(if44, 46.f, 58.f, 14.f, 5.f, 26.f, 35.f, 1.f, 27.f, 99.f,
                       11.f, 4.f, 46.f, 58.f, 26.f, 14.f, 99.f);
  }
}
