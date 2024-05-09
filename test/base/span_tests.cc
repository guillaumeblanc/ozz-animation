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

#include <stdint.h>

#include "gtest/gtest.h"
#include "ozz/base/gtest_helper.h"
#include "ozz/base/span.h"

TEST(Span, Platform) {
  int i = 20;
  int ai[] = {46, 26, 63, 20, 14, 93};
  const size_t kSize = OZZ_ARRAY_SIZE(ai);

  ozz::span<int> empty;
  EXPECT_TRUE(empty.begin() == nullptr);
  EXPECT_TRUE(empty.end() == nullptr);
  EXPECT_EQ(empty.size(), 0u);
  EXPECT_EQ(empty.size_bytes(), 0u);
  EXPECT_ASSERTION(empty.front(), "Empty span.");
  EXPECT_ASSERTION(empty.back(), "Empty span.");

  EXPECT_ASSERTION(empty[46], "Index out of range.");

  ozz::span<int> single(i);
  EXPECT_TRUE(single.begin() == &i);
  EXPECT_TRUE(single.end() == &i + 1);
  EXPECT_EQ(single.size(), 1u);
  EXPECT_EQ(single.size_bytes(), sizeof(i));

  EXPECT_ASSERTION(single[46], "Index out of range.");

  ozz::span<int> cs1(ai, ai + kSize);
  EXPECT_EQ(cs1.begin(), ai);
  EXPECT_TRUE(cs1.end() == ai + kSize);
  EXPECT_EQ(cs1.size(), kSize);
  EXPECT_EQ(cs1.size_bytes(), sizeof(ai));

  // Front and back
  EXPECT_EQ(cs1.front(), 46);
  EXPECT_EQ(cs1.back(), 93);

  // Re-inint
  ozz::span<int> reinit;
  reinit = ai;
  EXPECT_EQ(reinit.begin(), ai);
  EXPECT_TRUE(cs1.end() == ai + kSize);
  EXPECT_EQ(reinit.size(), kSize);
  EXPECT_EQ(reinit.size_bytes(), sizeof(ai));

  // Clear
  reinit = {};
  EXPECT_EQ(reinit.size(), 0u);
  EXPECT_EQ(reinit.size_bytes(), 0u);

  cs1[3] = 46;
  EXPECT_EQ(cs1[3], 46);
  EXPECT_ASSERTION(cs1[46], "Index out of range.");

  ozz::span<int> cs2(ai, kSize);
  EXPECT_EQ(cs2.begin(), ai);
  EXPECT_EQ(cs2.size(), kSize);
  EXPECT_EQ(cs2.size_bytes(), sizeof(ai));

  ozz::span<int> carray(ai);
  EXPECT_EQ(carray.begin(), ai);
  EXPECT_EQ(carray.size(), kSize);
  EXPECT_EQ(carray.size_bytes(), sizeof(ai));

  ozz::span<int> copy(cs2);
  EXPECT_EQ(cs2.begin(), copy.begin());
  EXPECT_EQ(cs2.size_bytes(), copy.size_bytes());

  ozz::span<const int> const_copy(cs2);
  EXPECT_EQ(cs2.begin(), const_copy.begin());
  EXPECT_EQ(cs2.size_bytes(), const_copy.size_bytes());

  EXPECT_EQ(cs2[3], 46);
  EXPECT_ASSERTION(cs2[46], "Index out of range.");

  // Invalid range
  EXPECT_ASSERTION(ozz::span<int>(ai, ai - kSize), "Invalid range.");
}

TEST(SpanAsBytes, Platform) {
  const size_t kSize = 46;
  int ai[kSize];
  {
    ozz::span<int> si(ai);
    EXPECT_EQ(si.size(), kSize);

    ozz::span<const ozz::byte> ab = as_bytes(si);
    EXPECT_EQ(ab.size(), kSize * sizeof(int));

    ozz::span<ozz::byte> awb = as_writable_bytes(si);
    EXPECT_EQ(awb.size(), kSize * sizeof(int));
  }

  {  // mutable char
    char ac[kSize];
    ozz::span<char> sc(ac);
    EXPECT_EQ(sc.size(), kSize);

    ozz::span<const ozz::byte> ab = as_bytes(sc);
    EXPECT_EQ(ab.size(), sc.size());

    ozz::span<ozz::byte> awbc = as_writable_bytes(sc);
    EXPECT_EQ(awbc.size(), sc.size());
  }
  {  // const
    ozz::span<const int> si(ai);
    EXPECT_EQ(si.size(), kSize);

    ozz::span<const ozz::byte> ab = as_bytes(si);
    EXPECT_EQ(ab.size(), kSize * sizeof(int));
  }

  // const char
  {
    const char ac[kSize] = {};
    ozz::span<const char> sc(ac);
    EXPECT_EQ(sc.size(), kSize);

    ozz::span<const ozz::byte> ab = as_bytes(sc);
    EXPECT_EQ(ab.size(), sc.size());
  }
}

TEST(SpanFill, Platform) {
  alignas(alignof(int)) ozz::byte abuffer[16];
  ozz::span<ozz::byte> src(abuffer);

  ozz::span<int> ispan1 = ozz::fill_span<int>(src, 3);
  EXPECT_EQ(ispan1.size(), 3u);
  ozz::span<int> ispan2 = ozz::fill_span<int>(src, 1);
  EXPECT_EQ(ispan2.size(), 1u);
  EXPECT_TRUE(src.empty());
  EXPECT_ASSERTION(ozz::fill_span<int>(src, 1), "Invalid range.");

  // Bad alignment
  src = ozz::make_span(abuffer);

  ozz::span<char> cspan = ozz::fill_span<char>(src, 1);
  EXPECT_EQ(cspan.size(), 1u);
  EXPECT_ASSERTION(ozz::fill_span<int>(src, 1), "Invalid alignment.");
}

TEST(SpanRangeLoop, Platform) {
  const size_t kSize = 46;
  size_t ai[kSize];

  // non const
  ozz::span<size_t> si = ozz::make_span(ai);
  size_t i = 0;
  for (size_t& li : si) {
    li = i++;
  }

  EXPECT_EQ(i, kSize);

  // const
  ozz::span<const size_t> sci = si;
  i = 0;
  for (const size_t& li : sci) {
    EXPECT_EQ(i, li);
    i++;
  }
}

TEST(SpanSubSpan, Platform) {
  const size_t kSize = 46;
  size_t ai[kSize];
  for (size_t i = 0; i < kSize; ++i) {
    ai[i] = i;
  }

  {  // empty
    ozz::span<size_t> eai;
    ozz::span<size_t> seai = eai.subspan(0, 0);
    EXPECT_EQ(seai.size(), 0u);
  }

  {  // subspan
    ozz::span<size_t> ncai(ai);

    EXPECT_ASSERTION(ncai.subspan(kSize, 1), " count out of range");
    EXPECT_ASSERTION(ncai.subspan(1, kSize), " count out of range");
    EXPECT_ASSERTION(ncai.subspan(kSize + 1, 0), "Offset out of range");
    EXPECT_ASSERTION(ncai.subspan(0, kSize + 1), "Count out of range");

    EXPECT_EQ(ncai.subspan(0, 0).size(), 0u);
    EXPECT_EQ(ncai.subspan(0, kSize).size(), kSize);
    EXPECT_EQ(ncai.subspan(0, kSize - 10)[0], 0u);
    EXPECT_EQ(ncai.subspan(10, kSize - 10).size(), kSize - 10);
    EXPECT_EQ(ncai.subspan(10, kSize - 10)[0], 10u);
    EXPECT_EQ(ncai.subspan(0, kSize - 10).size(), kSize - 10);
  }

  {  // first - last
    ozz::span<size_t> ncai(ai);

    EXPECT_ASSERTION(ncai.first(kSize + 1), "Count out of range");
    EXPECT_EQ(ncai.first(0).size(), 0u);
    EXPECT_EQ(ncai.first(10).size(), 10u);
    EXPECT_EQ(ncai.first(10)[0], 0u);

    EXPECT_ASSERTION(ncai.last(kSize + 1), "Count out of range");
    EXPECT_EQ(ncai.last(0).size(), 0u);
    EXPECT_EQ(ncai.last(10).size(), 10u);
    EXPECT_EQ(ncai.last(10)[0], kSize - 10);
  }
}
