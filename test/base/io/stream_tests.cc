//============================================================================//
//                                                                            //
// ozz-animation, 3d skeletal animation libraries and tools.                  //
// https://code.google.com/p/ozz-animation/                                   //
//                                                                            //
//----------------------------------------------------------------------------//
//                                                                            //
// Copyright (c) 2012-2014 Guillaume Blanc                                    //
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

#include "ozz/base/io/stream.h"

#include <limits>
#include <stdint.h>

#include "gtest/gtest.h"

#include "ozz/base/platform.h"

void TestStream(ozz::io::Stream* _stream) {
  ASSERT_TRUE(_stream->opened());
  EXPECT_EQ(_stream->Seek(0, ozz::io::Stream::kSet), 0);
  EXPECT_EQ(_stream->Tell(), 0);
  typedef int Type;
  const Type to_write = 46;
  EXPECT_EQ(_stream->Write(&to_write, sizeof(Type)), sizeof(Type));
  EXPECT_EQ(_stream->Tell(), static_cast<int>(sizeof(Type)));
  EXPECT_EQ(_stream->Seek(0, ozz::io::Stream::kSet), 0);
  EXPECT_EQ(_stream->Tell(), 0);
  Type to_read = 0;
  EXPECT_EQ(_stream->Read(&to_read, sizeof(Type)), sizeof(Type));
  EXPECT_EQ(to_read, to_write);
  EXPECT_EQ(_stream->Tell(), static_cast<int>(sizeof(Type)));
}

void TestSeek(ozz::io::Stream* _stream) {
  ASSERT_TRUE(_stream->opened());
  EXPECT_EQ(_stream->Seek(0, ozz::io::Stream::kSet), 0);
  EXPECT_EQ(_stream->Tell(), 0);

  // Seeking before file's begin.
  EXPECT_NE(_stream->Seek(-1, ozz::io::Stream::kSet), 0);
  EXPECT_EQ(_stream->Tell(), 0);
  EXPECT_NE(_stream->Seek(-1, ozz::io::Stream::kCurrent), 0);
  EXPECT_EQ(_stream->Tell(), 0);
  EXPECT_NE(_stream->Seek(-1, ozz::io::Stream::kEnd), 0);
  EXPECT_EQ(_stream->Tell(), 0);

  // Bad seek argument.
  EXPECT_EQ(_stream->Seek(46, ozz::io::Stream::Origin(27)), -1);
  EXPECT_EQ(_stream->Tell(), 0);

  typedef int Type;
  const Type to_write = 46;
  EXPECT_EQ(_stream->Write(&to_write, sizeof(Type)), sizeof(Type));
  EXPECT_EQ(_stream->Tell(), static_cast<int>(sizeof(Type)));

  const int64_t kEnd = 465827;
  // Force file length to kEnd but do not write to the stream.
  EXPECT_EQ(_stream->Seek(kEnd - _stream->Tell(),
                          ozz::io::Stream::kCurrent), 0);
  EXPECT_EQ(_stream->Tell(), kEnd);

  Type to_read = 0;
  EXPECT_EQ(_stream->Seek(0, ozz::io::Stream::kSet), 0);
  EXPECT_EQ(_stream->Read(&to_read, sizeof(Type)), sizeof(Type));
  EXPECT_EQ(to_read, to_write);
  EXPECT_EQ(_stream->Tell(), static_cast<int>(sizeof(Type)));
  EXPECT_EQ(_stream->Read(&to_read, sizeof(Type)), 0u);
  EXPECT_EQ(_stream->Tell(), static_cast<int>(sizeof(Type)));

  // Force file length to kEnd + sizeof(Type) and write to the stream.
  EXPECT_EQ(_stream->Seek(
    kEnd - _stream->Tell() - static_cast<int>(sizeof(Type)),
    ozz::io::Stream::kCurrent), 0);
  EXPECT_EQ(_stream->Tell(), kEnd - static_cast<int>(sizeof(Type)));
  EXPECT_EQ(_stream->Write(&to_write, sizeof(Type)), sizeof(Type));
  EXPECT_EQ(_stream->Tell(), kEnd);
  EXPECT_EQ(_stream->Seek(
    -static_cast<int>(sizeof(Type)), ozz::io::Stream::kEnd), 0);
  EXPECT_EQ(_stream->Tell(), kEnd - static_cast<int>(sizeof(Type)));
  EXPECT_EQ(_stream->Read(&to_read, sizeof(Type)), sizeof(Type));
  EXPECT_EQ(to_read, to_write);
  EXPECT_EQ(_stream->Tell(), kEnd);
  EXPECT_EQ(_stream->Seek(-static_cast<int>(sizeof(Type)) * 2,
                          ozz::io::Stream::kEnd), 0);
  EXPECT_EQ(_stream->Read(&to_read, sizeof(Type)), sizeof(Type));
  EXPECT_EQ(to_read, 0);
  EXPECT_EQ(_stream->Tell(), kEnd - static_cast<int>(sizeof(Type)));
  // Rewind from kEnd.
  EXPECT_EQ(_stream->Seek(-kEnd, ozz::io::Stream::kEnd), 0);
  EXPECT_EQ(_stream->Tell(), 0);
  EXPECT_EQ(_stream->Seek(kEnd, ozz::io::Stream::kSet), 0);
  EXPECT_EQ(_stream->Tell(), kEnd);

  // Read at the end of the file.
  EXPECT_EQ(_stream->Read(&to_read, sizeof(Type)), 0u);
  EXPECT_EQ(_stream->Tell(), kEnd);

  // Read after a seek beyond the end of the file.
  EXPECT_EQ(_stream->Seek(4, ozz::io::Stream::kCurrent), 0);
  EXPECT_EQ(_stream->Tell(), kEnd + 4);
  EXPECT_EQ(_stream->Read(&to_read, sizeof(Type)), 0u);
  EXPECT_EQ(_stream->Tell(), kEnd + 4);
}

void TestTooBigStream(ozz::io::Stream* _stream) {
  const int max_size = std::numeric_limits<int>::max();
  ASSERT_TRUE(_stream->opened());
  EXPECT_EQ(_stream->Seek(0, ozz::io::Stream::kSet), 0);
  EXPECT_EQ(_stream->Tell(), 0);

  // Seeks outside of Stream valid range.
  EXPECT_EQ(_stream->Seek(max_size, ozz::io::Stream::kCurrent), 0);
  EXPECT_EQ(_stream->Tell(), max_size);
  EXPECT_NE(_stream->Seek(max_size, ozz::io::Stream::kCurrent), 0);
  EXPECT_EQ(_stream->Tell(), max_size);

  // Writes/Reads outside of Stream valid range.
  EXPECT_EQ(_stream->Seek(1, ozz::io::Stream::kSet), 0);
  EXPECT_EQ(_stream->Tell(), 1);
  char c;
  EXPECT_EQ(_stream->Write(&c, max_size), 0u);
  EXPECT_EQ(_stream->Read(&c, max_size), 0u);
}

TEST(MemoryStream, Stream) {
  {
    ozz::io::MemoryStream stream;
    TestStream(&stream);
  }
  {
    ozz::io::MemoryStream stream;
    TestSeek(&stream);
  }
  {
    ozz::io::MemoryStream stream;
    TestTooBigStream(&stream);
  }
}

TEST(File, Stream) {
  {
    ozz::io::File file(NULL);
    ASSERT_TRUE(!file.opened());
  }
  {
    ozz::io::File file("test.bin", "w+t");
    TestSeek(&file);
  }
}
