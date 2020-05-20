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

#include "ozz/base/maths/math_archive.h"

#include "gtest/gtest.h"

#include "ozz/base/maths/gtest_math_helper.h"

#include "ozz/base/io/archive.h"
#include "ozz/base/maths/box.h"
#include "ozz/base/maths/quaternion.h"
#include "ozz/base/maths/rect.h"
#include "ozz/base/maths/transform.h"
#include "ozz/base/maths/vec_float.h"

TEST(MathArchive, ozz_math) {
  for (int e = 0; e < 2; ++e) {
    ozz::Endianness endianess = e == 0 ? ozz::kBigEndian : ozz::kLittleEndian;

    ozz::io::MemoryStream stream;
    ASSERT_TRUE(stream.opened());

    // Write math types.
    ozz::io::OArchive o(&stream, endianess);
    const ozz::math::Float2 of2(46.f, 69.f);
    o << of2;
    const ozz::math::Float3 of3(46.f, 69.f, 58.f);
    o << of3;
    const ozz::math::Float4 of4(46.f, 69.f, 58.f, 35.f);
    o << of4;
    const ozz::math::Quaternion oquat(46.f, 69.f, 58.f, 35.f);
    o << oquat;
    const ozz::math::Transform otrans = {of3, oquat, of3};
    o << otrans;
    const ozz::math::Box o_box(ozz::math::Float3(14.f, 26.f, 46.f),
                               ozz::math::Float3(58.f, 69.f, 99.f));
    o << o_box;
    const ozz::math::RectFloat o_rect_float(46.f, 69.f, 58.f, 35.f);
    o << o_rect_float;
    const ozz::math::RectInt o_rect_int(46, 69, 58, 35);
    o << o_rect_int;

    // Reads math types.
    stream.Seek(0, ozz::io::Stream::kSet);
    ozz::io::IArchive i(&stream);
    ozz::math::Float2 if2;
    i >> if2;
    EXPECT_FLOAT2_EQ(if2, 46.f, 69.f);
    ozz::math::Float3 if3;
    i >> if3;
    EXPECT_FLOAT3_EQ(if3, 46.f, 69.f, 58.f);
    ozz::math::Float4 if4;
    i >> if4;
    EXPECT_FLOAT4_EQ(if4, 46.f, 69.f, 58.f, 35.f);
    ozz::math::Quaternion iquat;
    i >> iquat;
    EXPECT_QUATERNION_EQ(iquat, 46.f, 69.f, 58.f, 35.f);
    ozz::math::Transform itrans;
    i >> itrans;
    EXPECT_FLOAT3_EQ(itrans.translation, 46.f, 69.f, 58.f);
    EXPECT_QUATERNION_EQ(itrans.rotation, 46.f, 69.f, 58.f, 35.f);
    EXPECT_FLOAT3_EQ(itrans.scale, 46.f, 69.f, 58.f);
    ozz::math::Box i_box;
    i >> i_box;
    EXPECT_FLOAT3_EQ(i_box.min, 14.f, 26.f, 46.f);
    EXPECT_FLOAT3_EQ(i_box.max, 58.f, 69.f, 99.f);
    ozz::math::RectFloat i_rect_float;
    i >> i_rect_float;
    EXPECT_FLOAT_EQ(i_rect_float.left, 46.f);
    EXPECT_FLOAT_EQ(i_rect_float.bottom, 69.f);
    EXPECT_FLOAT_EQ(i_rect_float.width, 58.f);
    EXPECT_FLOAT_EQ(i_rect_float.height, 35.f);
    ozz::math::RectInt i_rect_int;
    i >> i_rect_int;
    EXPECT_EQ(i_rect_int.left, 46);
    EXPECT_EQ(i_rect_int.bottom, 69);
    EXPECT_EQ(i_rect_int.width, 58);
    EXPECT_EQ(i_rect_int.height, 35);
  }
}
