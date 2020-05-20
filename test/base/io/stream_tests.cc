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

#include "ozz/base/io/stream.h"

#include <stdint.h>
#include <limits>

#include "gtest/gtest.h"

#include "ozz/base/platform.h"

void TestStream(ozz::io::Stream* _stream) {
  ASSERT_TRUE(_stream->opened());
  EXPECT_EQ(_stream->Size(), 0u);
  EXPECT_EQ(_stream->Seek(0, ozz::io::Stream::kSet), 0);
  EXPECT_EQ(_stream->Tell(), 0);
  typedef int Type;
  const Type to_write = 46;
  EXPECT_EQ(_stream->Write(&to_write, sizeof(Type)), sizeof(Type));
  EXPECT_EQ(_stream->Tell(), static_cast<int>(sizeof(Type)));
  EXPECT_EQ(_stream->Seek(0, ozz::io::Stream::kSet), 0);
  EXPECT_EQ(_stream->Tell(), 0);
  EXPECT_EQ(_stream->Size(), sizeof(Type));
  Type to_read = 0;
  EXPECT_EQ(_stream->Read(&to_read, sizeof(Type)), sizeof(Type));
  EXPECT_EQ(to_read, to_write);
  EXPECT_EQ(_stream->Tell(), static_cast<int>(sizeof(Type)));
  EXPECT_EQ(_stream->Size(), sizeof(Type));
}

void TestSeek(ozz::io::Stream* _stream) {
  ASSERT_TRUE(_stream->opened());
  EXPECT_EQ(_stream->Seek(0, ozz::io::Stream::kSet), 0);
  EXPECT_EQ(_stream->Tell(), 0);
  EXPECT_EQ(_stream->Size(), 0u);

  // Seeking before file's begin.
  EXPECT_NE(_stream->Seek(-1, ozz::io::Stream::kSet), 0);
  EXPECT_EQ(_stream->Tell(), 0);
  EXPECT_NE(_stream->Seek(-1, ozz::io::Stream::kCurrent), 0);
  EXPECT_EQ(_stream->Tell(), 0);
  EXPECT_NE(_stream->Seek(-1, ozz::io::Stream::kEnd), 0);
  EXPECT_EQ(_stream->Tell(), 0);
  EXPECT_EQ(_stream->Size(), 0u);

  // Bad seek argument.
  EXPECT_EQ(_stream->Seek(46, ozz::io::Stream::Origin(27)), -1);
  EXPECT_EQ(_stream->Tell(), 0);
  EXPECT_EQ(_stream->Size(), 0u);

  typedef int Type;
  const Type to_write = 46;
  EXPECT_EQ(_stream->Write(&to_write, sizeof(Type)), sizeof(Type));
  EXPECT_EQ(_stream->Tell(), static_cast<int>(sizeof(Type)));
  EXPECT_EQ(_stream->Size(), sizeof(Type));

  const int64_t kEnd = 465827;
  // Force file length to kEnd but do not write to the stream.
  EXPECT_EQ(_stream->Seek(kEnd - _stream->Tell(), ozz::io::Stream::kCurrent),
            0);
  EXPECT_EQ(_stream->Tell(), kEnd);
  EXPECT_EQ(_stream->Size(), sizeof(Type));

  Type to_read = 0;
  EXPECT_EQ(_stream->Seek(0, ozz::io::Stream::kSet), 0);
  EXPECT_EQ(_stream->Size(), sizeof(Type));
  EXPECT_EQ(_stream->Read(&to_read, sizeof(Type)), sizeof(Type));
  EXPECT_EQ(to_read, to_write);
  EXPECT_EQ(_stream->Tell(), static_cast<int>(sizeof(Type)));
  EXPECT_EQ(_stream->Read(&to_read, sizeof(Type)), 0u);
  EXPECT_EQ(_stream->Tell(), static_cast<int>(sizeof(Type)));
  EXPECT_EQ(_stream->Size(), sizeof(Type));

  // Force file length to kEnd + sizeof(Type) and write to the stream.
  EXPECT_EQ(
      _stream->Seek(kEnd - _stream->Tell() - static_cast<int>(sizeof(Type)),
                    ozz::io::Stream::kCurrent),
      0);
  EXPECT_EQ(_stream->Tell(), kEnd - static_cast<int>(sizeof(Type)));
  EXPECT_EQ(_stream->Write(&to_write, sizeof(Type)), sizeof(Type));
  EXPECT_EQ(_stream->Tell(), kEnd);
  EXPECT_EQ(
      _stream->Seek(-static_cast<int>(sizeof(Type)), ozz::io::Stream::kEnd), 0);
  EXPECT_EQ(_stream->Tell(), kEnd - static_cast<int>(sizeof(Type)));
  EXPECT_EQ(_stream->Read(&to_read, sizeof(Type)), sizeof(Type));
  EXPECT_EQ(to_read, to_write);
  EXPECT_EQ(_stream->Tell(), kEnd);
  EXPECT_EQ(
      _stream->Seek(-static_cast<int>(sizeof(Type)) * 2, ozz::io::Stream::kEnd),
      0);
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
  EXPECT_EQ(_stream->Size(), static_cast<size_t>(kEnd));
}

void TestTooBigStream(ozz::io::Stream* _stream) {
  const int max_size = std::numeric_limits<int>::max();
  ASSERT_TRUE(_stream->opened());
  EXPECT_EQ(_stream->Seek(0, ozz::io::Stream::kSet), 0);
  EXPECT_EQ(_stream->Tell(), 0);

  // Seeks outside of Stream valid range.
  EXPECT_EQ(_stream->Seek(max_size, ozz::io::Stream::kCurrent), 0);
  EXPECT_EQ(_stream->Tell(), max_size);
  EXPECT_EQ(_stream->Size(), 0u);
  EXPECT_NE(_stream->Seek(max_size, ozz::io::Stream::kCurrent), 0);
  EXPECT_EQ(_stream->Tell(), max_size);
  EXPECT_EQ(_stream->Size(), 0u);

  // Writes/Reads outside of Stream valid range.
  EXPECT_EQ(_stream->Seek(1, ozz::io::Stream::kSet), 0);
  EXPECT_EQ(_stream->Tell(), 1);
  char c;
  EXPECT_EQ(_stream->Write(&c, max_size), 0u);
  EXPECT_EQ(_stream->Read(&c, max_size), 0u);
  EXPECT_EQ(_stream->Size(), 0u);
}

TEST(File, Stream) {
  {
    ozz::io::File file(nullptr);
    EXPECT_FALSE(file.opened());
  }
  { EXPECT_FALSE(ozz::io::File::Exist("unexisting.file")); }
  {
    ozz::io::File file("test.bin", "w+t");
    EXPECT_TRUE(file.opened());
    TestSeek(&file);
  }
  { EXPECT_TRUE(ozz::io::File::Exist("test.bin")); }
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
