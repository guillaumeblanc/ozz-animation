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

#include "ozz/base/containers/array.h"
#include "ozz/base/encode/group_varint.h"
#include "ozz/base/gtest_helper.h"

TEST(Validity, GroupVarint4) {
  ozz::byte buffer[17];
  const uint32_t iarray[4] = {0, 0, 0, 0};
  EXPECT_ASSERTION(ozz::EncodeGV4(iarray, {}), "Output buffer is too small.");
  EXPECT_ASSERTION(ozz::EncodeGV4(iarray, ozz::span<ozz::byte>(buffer, 16)),
                   "Output buffer is too small.");

  uint32_t oarray[4];
  EXPECT_ASSERTION(ozz::DecodeGV4({}, oarray), "Input buffer is too small.");
  EXPECT_ASSERTION(ozz::DecodeGV4(ozz::span<ozz::byte>(buffer, 4), oarray),
                   "Input buffer is too small.");
}

TEST(OutputBuffer, GroupVarint4) {
  ozz::byte buffer[20];
  const uint32_t iarray[4] = {0, 0, 0, 0};
  uint32_t oarray[4];

  {  // Test output buffer perfect fit
    EXPECT_EQ(
        ozz::EncodeGV4(iarray, ozz::span<ozz::byte>(buffer, 17)).size_bytes(),
        12u);
    EXPECT_EQ(
        ozz::DecodeGV4(ozz::span<ozz::byte>(buffer, 5), oarray).size_bytes(),
        0u);
    EXPECT_EQ(oarray[0] + oarray[1] + oarray[2] + oarray[3], 0u);
  }

  {  // Test output buffer bigger
    EXPECT_EQ(
        ozz::EncodeGV4(iarray, ozz::span<ozz::byte>(buffer, 18)).size_bytes(),
        13u);
    EXPECT_EQ(
        ozz::DecodeGV4(ozz::span<ozz::byte>(buffer, 6), oarray).size_bytes(),
        1u);
    EXPECT_EQ(oarray[0] + oarray[1] + oarray[2] + oarray[3], 0u);
  }
}

void EncodeDecode(uint32_t _a, uint32_t _b, uint32_t _c, uint32_t _d) {
  ozz::byte buffer[18];
  const uint32_t iarray[4] = {_a, _b, _c, _d};
  ozz::span<ozz::byte> remain_out =
      ozz::EncodeGV4(iarray, ozz::span<ozz::byte>(buffer));
  EXPECT_GE(remain_out.size_bytes(), 1u);

  uint32_t oarray[4];
  ozz::span<const ozz::byte> remain_in =
      ozz::DecodeGV4(ozz::span<ozz::byte>(buffer, 17), oarray);
  EXPECT_EQ(remain_out.begin(), remain_in.begin());
  EXPECT_EQ(std::memcmp(iarray, oarray, sizeof(iarray)), 0);
}

TEST(Value, GroupVarint4) {
  EncodeDecode(0, 0, 0, 0);
  EncodeDecode(0, 1, 2, 3);
  EncodeDecode(255, 255, 255, 255);
  EncodeDecode(65535, 65535, 65535, 65535);
  EncodeDecode(16777215, 16777215, 16777215, 16777215);
  EncodeDecode(4294967295, 4294967295, 4294967295, 4294967295);
  EncodeDecode(255, 65535, 16777215, 4294967295);
  EncodeDecode(4294967295, 255, 65535, 16777215);
  EncodeDecode(16777215, 4294967295, 255, 65535);
  EncodeDecode(65535, 16777215, 4294967295, 255);
  EncodeDecode(46, 9399, 52026, 88332141);
  EncodeDecode(88332141, 9399, 52026, 46);
}

TEST(Validity, GroupVarint4Stream) {
  ozz::byte buffer[1 << 10];
  uint32_t stream[] = {0, 0,   0,     0,        1,          2,
                       3, 255, 65535, 16777215, 4294967295, 46};

  // Encode
  // Output buffer too small
  EXPECT_ASSERTION(ozz::EncodeGV4Stream(stream, {}),
                   "Output buffer is too small");
  EXPECT_ASSERTION(ozz::EncodeGV4Stream(stream, {buffer, 5}),
                   "Output buffer is too small");
  // Input not multiple of 4
  EXPECT_ASSERTION(ozz::EncodeGV4Stream({stream, 1}, buffer),
                   "Input stream must be multiple of 4");
  // Empty stream and output supported
  EXPECT_TRUE(ozz::EncodeGV4Stream({}, {}).empty());

  // Decode
  // Output buffer not multiple of 4
  EXPECT_ASSERTION(ozz::DecodeGV4Stream(buffer, {stream, 3}),
                   "Input stream must be multiple of 4");
  // Stream too small
  EXPECT_ASSERTION(ozz::DecodeGV4Stream({buffer, 6}, {stream, 8}),
                   "Output buffer is too small");

  // Empty stream and output supported
  EXPECT_TRUE(ozz::DecodeGV4Stream({}, {}).empty());
}

TEST(Size, GroupVarint4Stream) {
  uint32_t stream[] = {0, 0,   0,     0,        1,          2,
                       3, 255, 65535, 16777215, 4294967295, 46};
  EXPECT_ASSERTION(ozz::ComputeGV4WorstBufferSize({stream, 1}),
                   "Input stream must be multiple of 4");
  EXPECT_ASSERTION(ozz::ComputeGV4WorstBufferSize({stream, 7}),
                   "Input stream must be multiple of 4");

  EXPECT_EQ(ozz::ComputeGV4WorstBufferSize({}), 0u);
  EXPECT_EQ(ozz::ComputeGV4WorstBufferSize({stream, 4}), 17u);
  EXPECT_EQ(ozz::ComputeGV4WorstBufferSize(stream), 51u);
}

TEST(Value, GroupVarint4Stream) {
  ozz::byte buffer[51];
  uint32_t in_stream[] = {0, 0,   0,     0,        1,          2,
                          3, 255, 64535, 16677215, 4194967295, 46};
  uint32_t out_stream[OZZ_ARRAY_SIZE(in_stream)];

  EXPECT_EQ(ozz::EncodeGV4Stream({in_stream, 4}, buffer).size_bytes(), 46u);
  EXPECT_EQ(ozz::DecodeGV4Stream({buffer, 5}, {out_stream, 4}).size_bytes(),
            1u);
  EXPECT_EQ(std::memcmp(in_stream, out_stream, 4 * sizeof(uint32_t)), 0);

  EXPECT_EQ(ozz::EncodeGV4Stream(in_stream, buffer).size_bytes(), 30u);
  EXPECT_EQ(ozz::DecodeGV4Stream({buffer, 21}, out_stream).size_bytes(), 0u);
  EXPECT_EQ(std::memcmp(in_stream, out_stream, sizeof(out_stream)), 0);

  auto buf = ozz::make_span(buffer);
  auto remain = ozz::EncodeGV4Stream({in_stream, 4}, buf);
  auto used = buf.subspan(0, buf.size() - remain.size());
  EXPECT_EQ(used.size(), 5u);
}