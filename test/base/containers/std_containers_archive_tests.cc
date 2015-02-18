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

    ozz::String::Std big_o("Forty-six is a Wedderburn-Etherington number, an "\
      "enneagonal number and a centered triangular number. It is the sum of "\
      "the totient function for the first twelve integers. 46 is the largest "\
      "even integer that can't be expressed as a sum of two abundant numbers."\
      "46 is the 16th semiprime. 46 is the third semiprime with a semiprime"\
      "aliquot sum. The aliquot sequence of 46 is (46,26,16,15,9,4,3,1,0)."\
      "Since it is possible to find sequences of 46 consecutive integers such "\
      "that each inner member shares a factor with either the first or the "\
      "last member, 46 is an Erdos-Woods number.");
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
