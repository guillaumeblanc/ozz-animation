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

#include "ozz/base/maths/soa_math_archive.h"

#include "gtest/gtest.h"

#include "ozz/base/maths/gtest_math_helper.h"

#include "ozz/base/io/archive.h"
#include "ozz/base/maths/soa_float.h"
#include "ozz/base/maths/soa_float4x4.h"
#include "ozz/base/maths/soa_quaternion.h"
#include "ozz/base/maths/soa_transform.h"

TEST(SoaMathArchive, ozz_soa_math) {
  for (int e = 0; e < 2; ++e) {
    ozz::Endianness endianess = e == 0 ? ozz::kBigEndian : ozz::kLittleEndian;

    ozz::io::MemoryStream stream;
    ASSERT_TRUE(stream.opened());

    // Write soa math types.
    ozz::io::OArchive o(&stream, endianess);
    const ozz::math::SoaFloat2 of2 = ozz::math::SoaFloat2::Load(
        ozz::math::simd_float4::Load(0.f, 1.f, 2.f, 3.f),
        ozz::math::simd_float4::Load(4.f, 5.f, 6.f, 7.f));
    o << of2;
    const ozz::math::SoaFloat3 of3 = ozz::math::SoaFloat3::Load(
        ozz::math::simd_float4::Load(0.f, 1.f, 2.f, 3.f),
        ozz::math::simd_float4::Load(4.f, 5.f, 6.f, 7.f),
        ozz::math::simd_float4::Load(8.f, 9.f, 10.f, 11.f));
    o << of3;
    const ozz::math::SoaFloat4 of4 = ozz::math::SoaFloat4::Load(
        ozz::math::simd_float4::Load(0.f, 1.f, 2.f, 3.f),
        ozz::math::simd_float4::Load(4.f, 5.f, 6.f, 7.f),
        ozz::math::simd_float4::Load(8.f, 9.f, 10.f, 11.f),
        ozz::math::simd_float4::Load(12.f, 13.f, 14.f, 15.f));
    o << of4;
    const ozz::math::SoaQuaternion oquat = ozz::math::SoaQuaternion::Load(
        ozz::math::simd_float4::Load(0.f, 1.f, 2.f, 3.f),
        ozz::math::simd_float4::Load(4.f, 5.f, 6.f, 7.f),
        ozz::math::simd_float4::Load(8.f, 9.f, 10.f, 11.f),
        ozz::math::simd_float4::Load(12.f, 13.f, 14.f, 15.f));
    o << oquat;
    const ozz::math::SoaTransform otrans = {of3, oquat, of3};
    o << otrans;
    const ozz::math::SoaFloat4x4 of44 = {
        {{ozz::math::simd_float4::Load(0.f, 1.f, 0.f, 0.f),
          ozz::math::simd_float4::Load(1.f, 0.f, -1.f, 0.f),
          ozz::math::simd_float4::Load(2.f, 0.f, 2.f, -1.f),
          ozz::math::simd_float4::Load(3.f, 0.f, 3.f, 0.f)},
         {ozz::math::simd_float4::Load(4.f, 0.f, -4.f, 0.f),
          ozz::math::simd_float4::Load(5.f, 1.f, 5.f, 1.f),
          ozz::math::simd_float4::Load(6.f, 0.f, 6.f, 0.f),
          ozz::math::simd_float4::Load(7.f, 0.f, -7.f, 0.f)},
         {ozz::math::simd_float4::Load(8.f, 0.f, 8.f, 1.f),
          ozz::math::simd_float4::Load(9.f, 0.f, -9.f, 0.f),
          ozz::math::simd_float4::Load(10.f, 1.f, -10.f, 0.f),
          ozz::math::simd_float4::Load(11.f, 0.f, 11.f, 0.f)},
         {ozz::math::simd_float4::Load(12.f, 0.f, -12.f, 0.f),
          ozz::math::simd_float4::Load(13.f, 0.f, 13.f, 0.f),
          ozz::math::simd_float4::Load(14.f, 0.f, -14.f, 0.f),
          ozz::math::simd_float4::Load(15.f, 1.f, 15.f, 1.f)}}};
    o << of44;

    // Reads soa math types.
    stream.Seek(0, ozz::io::Stream::kSet);
    ozz::io::IArchive i(&stream);
    ozz::math::SoaFloat2 if2;
    i >> if2;
    EXPECT_SOAFLOAT2_EQ(if2, 0.f, 1.f, 2.f, 3.f, 4.f, 5.f, 6.f, 7.f);
    ozz::math::SoaFloat3 if3;
    i >> if3;
    EXPECT_SOAFLOAT3_EQ(if3, 0.f, 1.f, 2.f, 3.f, 4.f, 5.f, 6.f, 7.f, 8.f, 9.f,
                        10.f, 11.f);
    ozz::math::SoaFloat4 if4;
    i >> if4;
    EXPECT_SOAFLOAT4_EQ(if4, 0.f, 1.f, 2.f, 3.f, 4.f, 5.f, 6.f, 7.f, 8.f, 9.f,
                        10.f, 11.f, 12.f, 13.f, 14.f, 15.f);
    ozz::math::SoaQuaternion iquat;
    i >> iquat;
    EXPECT_SOAQUATERNION_EQ(iquat, 0.f, 1.f, 2.f, 3.f, 4.f, 5.f, 6.f, 7.f, 8.f,
                            9.f, 10.f, 11.f, 12.f, 13.f, 14.f, 15.f);
    ozz::math::SoaTransform itrans;
    i >> itrans;
    EXPECT_SOAFLOAT3_EQ(itrans.translation, 0.f, 1.f, 2.f, 3.f, 4.f, 5.f, 6.f,
                        7.f, 8.f, 9.f, 10.f, 11.f);
    EXPECT_SOAQUATERNION_EQ(itrans.rotation, 0.f, 1.f, 2.f, 3.f, 4.f, 5.f, 6.f,
                            7.f, 8.f, 9.f, 10.f, 11.f, 12.f, 13.f, 14.f, 15.f);
    EXPECT_SOAFLOAT3_EQ(itrans.scale, 0.f, 1.f, 2.f, 3.f, 4.f, 5.f, 6.f, 7.f,
                        8.f, 9.f, 10.f, 11.f);
    ozz::math::SoaFloat4x4 if44;
    i >> if44;
    EXPECT_SOAFLOAT4x4_EQ(
        if44, 0.f, 1.f, 0.f, 0.f, 1.f, 0.f, -1.f, 0.f, 2.f, 0.f, 2.f, -1.f, 3.f,
        0.f, 3.f, 0.f, 4.f, 0.f, -4.f, 0.f, 5.f, 1.f, 5.f, 1.f, 6.f, 0.f, 6.f,
        0.f, 7.f, 0.f, -7.f, 0.f, 8.f, 0.f, 8.f, 1.f, 9.f, 0.f, -9.f, 0.f, 10.f,
        1.f, -10.f, 0.f, 11.f, 0.f, 11.f, 0.f, 12.f, 0.f, -12.f, 0.f, 13.f, 0.f,
        13.f, 0.f, 14.f, 0.f, -14.f, 0.f, 15.f, 1.f, 15.f, 1.f);
  }
}
