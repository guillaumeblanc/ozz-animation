//----------------------------------------------------------------------------//
//                                                                            //
// ozz-animation is hosted at http://github.com/guillaumeblanc/ozz-animation  //
// and distributed under the MIT License (MIT).                               //
//                                                                            //
// Copyright (c) 2019 Guillaume Blanc                                         //
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

#include "ozz/base/containers/string_archive.h"
#include "ozz/base/containers/vector_archive.h"

#include <algorithm>

#include "gtest/gtest.h"

#include "ozz/base/io/archive.h"

TEST(String, Archive) {
  for (int e = 0; e < 2; ++e) {
    ozz::Endianness endianess = e == 0 ? ozz::kBigEndian : ozz::kLittleEndian;

    ozz::io::MemoryStream stream;
    ASSERT_TRUE(stream.opened());

    // Writes.
    ozz::io::OArchive o(&stream, endianess);
    ozz::String::Std empty_o;
    o << empty_o;

    ozz::String::Std small_o("Forty-six");
    o << small_o;

    ozz::String::Std big_o(
        "Forty-six is a Wedderburn-Etherington number, an "
        "enneagonal number and a centered triangular number. It is the sum of "
        "the totient function for the first twelve integers. 46 is the largest "
        "even integer that can't be expressed as a sum of two abundant numbers."
        "46 is the 16th semiprime. 46 is the third semiprime with a semiprime"
        "aliquot sum. The aliquot sequence of 46 is (46,26,16,15,9,4,3,1,0)."
        "Since it is possible to find sequences of 46 consecutive integers "
        "such that each inner member shares a factor with either the first or "
        "the last member, 46 is an Erdos-Woods number.");
    o << big_o;

    // Rewrite for the string reuse test.
    ozz::String::Std reuse_o("Forty-six");
    o << reuse_o;

    // Reads.
    stream.Seek(0, ozz::io::Stream::kSet);
    ozz::io::IArchive i(&stream);

    ozz::String::Std empty_i;
    i >> empty_i;
    EXPECT_STREQ(empty_o.c_str(), empty_i.c_str());

    ozz::String::Std small_i;
    i >> small_i;
    EXPECT_STREQ(small_o.c_str(), small_i.c_str());

    ozz::String::Std big_i;
    i >> big_i;
    EXPECT_STREQ(big_o.c_str(), big_i.c_str());

    ozz::String::Std reuse_i("already used string");
    i >> reuse_i;
    EXPECT_STREQ(reuse_o.c_str(), reuse_i.c_str());
  }
}

TEST(Vector, Archive) {
  for (int e = 0; e < 2; ++e) {
    ozz::Endianness endianess = e == 0 ? ozz::kBigEndian : ozz::kLittleEndian;

    ozz::io::MemoryStream stream;
    ASSERT_TRUE(stream.opened());

    // Writes.
    ozz::io::OArchive o(&stream, endianess);
    ozz::Vector<int>::Std empty_o;
    o << empty_o;

    ozz::Vector<int>::Std small_o(5);
    std::generate(small_o.begin(), small_o.end(), std::rand);
    o << small_o;

    ozz::Vector<int>::Std big_o(1263);
    std::generate(big_o.begin(), big_o.end(), std::rand);
    o << big_o;

    // Rewrite for the Vector reuse test.
    ozz::Vector<int>::Std reuse_o(46);
    std::generate(reuse_o.begin(), reuse_o.end(), std::rand);
    o << reuse_o;

    // Reads.
    stream.Seek(0, ozz::io::Stream::kSet);
    ozz::io::IArchive i(&stream);

    ozz::Vector<int>::Std empty_i;
    i >> empty_i;
    EXPECT_EQ(empty_i.size(), 0u);

    ozz::Vector<int>::Std small_i;
    i >> small_i;
    EXPECT_EQ(small_o.size(), small_i.size());
    for (size_t j = 0; j < empty_i.size(); ++j) {
      EXPECT_EQ(small_o[j], small_i[j]);
    }

    ozz::Vector<int>::Std big_i;
    i >> big_i;
    EXPECT_EQ(big_o.size(), big_i.size());
    for (size_t j = 0; j < big_i.size(); ++j) {
      EXPECT_EQ(big_o[j], big_i[j]);
    }

    ozz::Vector<int>::Std reuse_i(3);
    std::generate(reuse_i.begin(), reuse_i.end(), std::rand);
    i >> reuse_i;
    EXPECT_EQ(reuse_o.size(), reuse_i.size());
    for (size_t j = 0; j < reuse_i.size(); ++j) {
      EXPECT_EQ(reuse_o[j], reuse_i[j]);
    }
  }
}
