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

#ifndef OZZ_OZZ_BASE_IO_STREAM_H_
#define OZZ_OZZ_BASE_IO_STREAM_H_

// Provides Stream interface used to read/write a memory buffer or a file with
// Crt fread/fwrite/fseek/ftell like functions.

#include "ozz/base/platform.h"

#include <cstddef>

namespace ozz {
namespace io {

// Declares a stream access interface that conforms with CRT FILE API.
// This interface should be used to remap io operations.
class Stream {
 public:
  // Tests whether a file is opened.
  virtual bool opened() const = 0;

  // Reads _size bytes of data to _buffer from the stream. _buffer must be big
  // enough to store _size bytes. The position indicator of the stream is
  // advanced by the total amount of bytes read.
  // Returns the number of bytes actually read, which may be less than _size.
  virtual size_t Read(void* _buffer, size_t _size) = 0;

  // Writes _size bytes of data from _buffer to the stream. The position
  // indicator of the stream is advanced by the total number of bytes written.
  // Returns the number of bytes actually written, which may be less than _size.
  virtual size_t Write(const void* _buffer, size_t _size) = 0;

  // Declares seeking origin enumeration.
  enum Origin {
    kCurrent,  // Current position of the stream pointer.
    kEnd,  // End of stream.
    kSet,  // Beginning of stream.
  };
  // Sets the position indicator associated with the stream to a new position
  // defined by adding _offset to a reference position specified by _origin.
  // Returns a zero value if successful, otherwise returns a non-zero value.
  virtual int Seek(int _offset, Origin _origin) = 0;

  // Returns the current value of the position indicator of the stream.
  // Returns -1 if an error occurs.
  virtual int Tell() const = 0;

 protected:

  // Required virtual destructor.
  virtual ~Stream() {
  }
};

// Implements Stream of type File.
class File : public Stream {
 public:
  // Open a file at path _filename with mode * _mode, in confromance with fopen
  // specifications.
  // Use opened() function to test opening result.
  File(const char* _filename, const char* _mode);

  // Gives _file ownership to the FileStream, which will be in charge of closing
  // it. _file must be NULL or a valid std::FILE pointer.
  explicit File(void* _file);

  // Close the file if it is opened.
  virtual ~File();

  // Close the file if it is opened.
  void Close();

  // See Stream::opened for details.
  virtual bool opened() const;

  // See Stream::Read for details.
  virtual size_t Read(void* _buffer, size_t _size);

  // See Stream::Write for details.
  virtual size_t Write(const void* _buffer, size_t _size);

  // See Stream::Seek for details.
  virtual int Seek(int _offset, Origin _origin);

  // See Stream::Tell for details.
  virtual int Tell() const;

 private:
  // The CRT file pointer.
  void* file_;
};

// Implements an in-memory Stream. Allows to use a memory buffer as a Stream.
// The opening mode is equivalent to fopen w+b (binary read/write).
class MemoryStream : public Stream {
 public:
  // Construct an empty memory stream opened in w+b mode.
  MemoryStream();

  // Closes the stream and deallocates memory buffer.
  virtual ~MemoryStream();

  // See Stream::opened for details.
  virtual bool opened() const;

  // See Stream::Read for details.
  virtual size_t Read(void* _buffer, size_t _size);

  // See Stream::Write for details.
  virtual size_t Write(const void* _buffer, size_t _size);

  // See Stream::Seek for details.
  virtual int Seek(int _offset, Origin _origin);

  // See Stream::Tell for details.
  virtual int Tell() const;

 private:

  // Resizes buffers size to _size bytes. If _size is less than the actual
  // buffer size, then it remains unchanged.
  // Returns true if the buffer can contains _size bytes.
  bool Resize(size_t _size);

  // Size of the buffer increment.
  static const size_t kBufferSizeIncrement;

  // Maximum stream size.
  static const size_t kMaxSize;

  // Buffer of data.
  char* buffer_;

  // The size of the buffer, which is greater or equal to the size of the data
  // it contains (end_).
  size_t alloc_size_;

  // The effective size of the data in the buffer.
  int end_;

  // The cursor position in the buffer of data.
  int tell_;
};
}  // io
}  // ozz
#endif  // OZZ_OZZ_BASE_IO_STREAM_H_
