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

#include "ozz/base/io/archive.h"

#include <cstring>

#include "gtest/gtest.h"

#include "ozz/base/gtest_helper.h"

#include "archive_tests_objects.h"

TEST(Error, Archive) {
  {  // Invalid NULL stream.
    EXPECT_ASSERTION(ozz::io::OArchive o(NULL, ozz::GetNativeEndianness()),
                     "valid opened stream");
    EXPECT_ASSERTION(ozz::io::IArchive i(NULL), "valid opened stream");
  }
  {  // Invalid not opened streams.
    ozz::io::File stream("root_that_does_not_exist:/file.ozz", "r");
    EXPECT_ASSERTION(ozz::io::OArchive o(&stream, ozz::GetNativeEndianness()),
                     "valid opened stream");
    EXPECT_ASSERTION(ozz::io::IArchive i(&stream), "valid opened stream");
  }
}

TEST(Primitives, Archive) {
  for (int e = 0; e < 2; e++) {
    ozz::Endianness endianess = e == 0 ? ozz::kBigEndian : ozz::kLittleEndian;

    ozz::io::MemoryStream stream;
    ASSERT_TRUE(stream.opened());

    // Write primitive types.
    ozz::io::OArchive o(&stream, endianess);
    const ozz::int8 i8o = 46;
    o << i8o;
    const ozz::uint8 ui8o = 46;
    o << ui8o;
    const ozz::int16 i16o = 46;
    o << i16o;
    const ozz::uint16 ui16o = 46;
    o << ui16o;
    const ozz::int32 i32o = 46;
    o << i32o;
    const ozz::uint32 ui32o = 46;
    o << ui32o;
    const ozz::int64 i64o = 46;
    o << i64o;
    const ozz::uint64 ui64o = 46;
    o << ui64o;
    const bool bo = true;
    o << bo;
    const float fo = 46.f;
    o << fo;

    // Read primitive types.
    stream.Seek(0, ozz::io::Stream::kSet);
    ozz::io::IArchive i(&stream);
    ozz::int8 i8i;
    i >> i8i;
    EXPECT_EQ(i8i, i8o);
    ozz::uint8 ui8i;
    i >> ui8i;
    EXPECT_EQ(ui8i, ui8o);
    ozz::int16 i16i;
    i >> i16i;
    EXPECT_EQ(i16i, i16o);
    ozz::uint16 ui16i;
    i >> ui16i;
    EXPECT_EQ(ui16i, ui16o);
    ozz::int32 i32i;
    i >> i32i;
    EXPECT_EQ(i32i, i32o);
    ozz::uint32 ui32i;
    i >> ui32i;
    EXPECT_EQ(ui32i, ui32o);
    ozz::int64 i64i;
    i >> i64i;
    EXPECT_EQ(i64i, i64o);
    ozz::uint64 ui64i;
    i >> ui64i;
    EXPECT_EQ(ui64i, ui64o);
    bool bi;
    i >> bi;
    EXPECT_EQ(bi, bo);
    float fi;
    i >> fi;
    EXPECT_EQ(fi, fo);
  }
}

TEST(PrimitiveArrays, Archive) {
  for (int e = 0; e < 2; e++) {
    ozz::Endianness endianess = e == 0 ? ozz::kBigEndian : ozz::kLittleEndian;

    ozz::io::MemoryStream stream;
    ASSERT_TRUE(stream.opened());

    // Write primitive types.
    ozz::io::OArchive o(&stream, endianess);
    const ozz::int8 i8o[] = {46, 26, 14, 58, 99, 27};
    o << ozz::io::MakeArray(i8o);
    const ozz::uint8 ui8o[] = {46, 26, 14, 58, 99, 27};
    o << ozz::io::MakeArray(ui8o);
    const ozz::int16 i16o[] = {46, 26, 14, 58, 99, 27};
    o << ozz::io::MakeArray(i16o);
    const ozz::uint16 ui16o[] = {46, 26, 14, 58, 99, 27};
    o << ozz::io::MakeArray(ui16o);
    const ozz::int32 i32o[] = {46, 26, 14, 58, 99, 27};
    o << ozz::io::MakeArray(i32o);
    const ozz::uint32 ui32o[] = {46, 26, 14, 58, 99, 27};
    o << ozz::io::MakeArray(ui32o);
    const ozz::int64 i64o[] = {46, 26, 14, 58, 99, 27};
    o << ozz::io::MakeArray(i64o);
    const ozz::uint64 ui64o[] = {46, 26, 14, 58, 99, 27};
    o << ozz::io::MakeArray(ui64o);
    const bool bo[] = {true, false, true};
    o << ozz::io::MakeArray(bo);
    const float fo[] = {46.f, 26.f, 14.f, 58.f, 99.f, 27.f};
    o << ozz::io::MakeArray(fo);
    const ozz::uint32* po_null = NULL;
    o << ozz::io::MakeArray(po_null, 0);

    // Read primitive types.
    stream.Seek(0, ozz::io::Stream::kSet);
    ozz::io::IArchive i(&stream);
    ozz::int8 i8i[OZZ_ARRAY_SIZE(i8o)];
    i >> ozz::io::MakeArray(i8i);
    EXPECT_EQ(std::memcmp(i8i, i8o, sizeof(i8o)), 0);
    ozz::uint8 ui8i[OZZ_ARRAY_SIZE(ui8o)];
    i >> ozz::io::MakeArray(ui8i);
    EXPECT_EQ(std::memcmp(ui8i, ui8o, sizeof(ui8o)), 0);
    ozz::int16 i16i[OZZ_ARRAY_SIZE(i16o)];
    i >> ozz::io::MakeArray(i16i);
    EXPECT_EQ(std::memcmp(i16i, i16o, sizeof(i16o)), 0);
    ozz::uint16 ui16i[OZZ_ARRAY_SIZE(ui16o)];
    i >> ozz::io::MakeArray(ui16i);
    EXPECT_EQ(std::memcmp(ui16i, ui16o, sizeof(ui16o)), 0);
    ozz::int32 i32i[OZZ_ARRAY_SIZE(i32o)];
    i >> ozz::io::MakeArray(i32i);
    EXPECT_EQ(std::memcmp(i32i, i32o, sizeof(i32o)), 0);
    ozz::uint32 ui32i[OZZ_ARRAY_SIZE(ui32o)];
    i >> ozz::io::MakeArray(ui32i);
    EXPECT_EQ(std::memcmp(ui32i, ui32o, sizeof(ui32o)), 0);
    ozz::int64 i64i[OZZ_ARRAY_SIZE(i64o)];
    i >> ozz::io::MakeArray(i64i);
    EXPECT_EQ(std::memcmp(i64i, i64o, sizeof(i64o)), 0);
    ozz::uint64 ui64i[OZZ_ARRAY_SIZE(ui64o)];
    i >> ozz::io::MakeArray(ui64i);
    EXPECT_EQ(std::memcmp(ui64i, ui64o, sizeof(ui64o)), 0);
    bool bi[OZZ_ARRAY_SIZE(bo)];
    i >> ozz::io::MakeArray(bi);
    EXPECT_EQ(std::memcmp(bi, bo, sizeof(bo)), 0);
    float fi[OZZ_ARRAY_SIZE(fo)];
    i >> ozz::io::MakeArray(fi);
    EXPECT_EQ(std::memcmp(fi, fo, sizeof(fo)), 0);
    ozz::uint32* pi_null = NULL;
    i >> ozz::io::MakeArray(pi_null, 0);
  }
}

TEST(Class, Archive) {
  for (int e = 0; e < 2; e++) {
    ozz::Endianness endianess = e == 0 ? ozz::kBigEndian : ozz::kLittleEndian;

    ozz::io::MemoryStream stream;
    ASSERT_TRUE(stream.opened());

    // Write classes.
    ozz::io::OArchive o(&stream, endianess);
    const Intrusive oi(46);
    o << oi;
    const Intrusive oi_mutable(46);
    o << oi_mutable;
    const Extrusive oe = {58};
    o << oe;
    const Extrusive oe_mutable = {58};
    o << oe_mutable;

    // Read classes.
    stream.Seek(0, ozz::io::Stream::kSet);
    ozz::io::IArchive i(&stream);
    Intrusive ii;
    i >> ii;
    EXPECT_EQ(ii.i, oi.i);
    Intrusive ii_mutable;
    i >> ii_mutable;
    EXPECT_EQ(ii_mutable.i, oi_mutable.i);
    Extrusive ie;
    i >> ie;
    EXPECT_EQ(ie.i, oe.i);
    Extrusive ie_mutable;
    i >> ie_mutable;
    EXPECT_EQ(ie_mutable.i, oe_mutable.i);
  }
}

TEST(ClassArrays, Archive) {
  for (int e = 0; e < 2; e++) {
    ozz::Endianness endianess = e == 0 ? ozz::kBigEndian : ozz::kLittleEndian;

    ozz::io::MemoryStream stream;
    ASSERT_TRUE(stream.opened());

    // Write classes.
    ozz::io::OArchive o(&stream, endianess);
    const Intrusive oi[12];
    o << ozz::io::MakeArray(oi);
    const Extrusive oe[] = {{46}, {58}, {14}, {26}, {99}};
    o << ozz::io::MakeArray(oe);

    // Read classes.
    stream.Seek(0, ozz::io::Stream::kSet);
    ozz::io::IArchive i(&stream);
    Intrusive ii[OZZ_ARRAY_SIZE(oi)];
    i >> ozz::io::MakeArray(ii);
    EXPECT_EQ(std::memcmp(oi, ii, sizeof(oi)), 0);
    Extrusive ie[OZZ_ARRAY_SIZE(oe)];
    i >> ozz::io::MakeArray(ie);
    EXPECT_EQ(std::memcmp(oe, ie, sizeof(oe)), 0);
  }
}

TEST(Tag, Archive) {
  ozz::io::MemoryStream stream;
  ASSERT_TRUE(stream.opened());

  // Writes to archive.
  ozz::io::OArchive o(&stream, ozz::GetNativeEndianness());
  Tagged1 ot;
  o << ot;

  // Reads from archive.
  stream.Seek(0, ozz::io::Stream::kSet);
  ozz::io::IArchive i(&stream);

  // Tests and reads a wrong object (different tag).
  OZZ_IF_DEBUG(Tagged2 it2);
  EXPECT_FALSE(i.TestTag<Tagged2>());
  EXPECT_ASSERTION(i >> it2, "Type tag does not match archive content.");

  // Reads the right object (different tag).
  Tagged1 it1;
  EXPECT_TRUE(i.TestTag<Tagged1>());
  EXPECT_NO_FATAL_FAILURE(i >> it1);
}
