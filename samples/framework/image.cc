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

#include "framework/image.h"

#include "ozz/base/io/stream.h"
#include "ozz/base/log.h"
#include "ozz/base/memory/allocator.h"

namespace ozz {
namespace sample {
namespace image {

bool HasAlpha(Format::Value _format) {
  return _format >= Format::kRGBA;
}

int Stride(Format::Value _format) {
  return _format <= Format::kBGR ? 3 : 4;
}

#define PUSH_PIXEL_RGB(_buffer, _size, _repetition, _pixel, _mapping) \
  _buffer[_size++] = 0x80 | ((_repetition - 1) & 0xff); \
  _buffer[_size++] = _pixel.c[_mapping[0]]; \
  _buffer[_size++] = _pixel.c[_mapping[1]]; \
  _buffer[_size++] = _pixel.c[_mapping[2]];

#define PUSH_PIXEL_RGBA(_buffer, _size, _repetition, _pixel, _mapping) \
  _buffer[_size++] = 0x80 | ((_repetition - 1) & 0xff); \
  _buffer[_size++] = _pixel.c[_mapping[0]]; \
  _buffer[_size++] = _pixel.c[_mapping[1]]; \
  _buffer[_size++] = _pixel.c[_mapping[2]]; \
  _buffer[_size++] = _pixel.c[_mapping[3]];

bool WriteTGA(const char* _filename,
              int _width, int _height,
              Format::Value _src_format,
              const ozz::uint8* _src_buffer,
              bool _write_alpha) {
  union Pixel { ozz::uint8 c[4]; ozz::uint32 p;};

  assert(_filename && _src_buffer);

  ozz::log::LogV() << "Write image to TGA file \"" << _filename << "\"."
    << std::endl;

  // Opens output file.
  ozz::io::File file(_filename, "wb");
  if (!file.opened()) {
    ozz::log::Err() << "Failed to open file \"" << _filename
      << "\" for writing." << std::endl;
    return false;
  }

  // Builds and writes tga header.
  const ozz::uint8 header[] = {
    0,  // ID length
    0,  // Color map type
    10,  // Image type (RLE true-color)
    0, 0, 0, 0, 0,  // Color map specification (no color map)
    0, 0,  // X-origin (2 bytes little-endian)
    0, 0,  // Y-origin (2 bytes little-endian)
    static_cast<ozz::uint8>(_width & 0xff),  // Width (2 bytes little-endian)
    static_cast<ozz::uint8>((_width >> 8) & 0xff),
    static_cast<ozz::uint8>(_height & 0xff),  // Height (2 bytes little-endian)
    static_cast<ozz::uint8>((_height >> 8) & 0xff),
    static_cast<ozz::uint8>(_write_alpha ? 32 : 24),  // Pixel depth
    0};  // Image descriptor
  OZZ_STATIC_ASSERT(sizeof(header) == 18);
  file.Write(header, sizeof(header));

  // Early out if no pixel to write.
  if (!_width || !_height) {
    return true;
  }

  // Writes pixels, with RLE compression.

  // Prepares component mappings from src to TARGA format.
  const ozz::uint8 mappings[4][4] = {
    {2, 1, 0, 0}, {0, 1, 2, 0}, {2, 1, 0, 3}, {0, 1, 2, 3}};
  const ozz::uint8* mapping = mappings[_src_format];
  // Allocates enough space to store RLE packets for the worst case scenario.
  ozz::uint8* dest_buffer = ozz::memory::default_allocator()->
    Allocate<ozz::uint8>((1 + (_write_alpha ? 4 : 3)) * _width * _height);
  std::size_t dest_size = 0;

  if (HasAlpha(_src_format)) {
    assert(Stride(_src_format) == 4);
    const int src_pitch = _width * 4;
    const int src_size = _height * src_pitch;
    if (_write_alpha) {
      for (int line = 0; line < src_size; line += src_pitch) {
        Pixel current = {{_src_buffer[line + 0], _src_buffer[line + 1],
                          _src_buffer[line + 2], _src_buffer[line + 3]}};
        int count = 1;
        for (int p = line + 4; p < line + _width * 4; p += 4, count++) {
          const Pixel next = {{_src_buffer[p + 0], _src_buffer[p + 1],
                               _src_buffer[p + 2], _src_buffer[p + 3]}};
          if (current.p != next.p || count == 128) {
            // Writes current packet.
            PUSH_PIXEL_RGBA(dest_buffer, dest_size, count, current, mapping);
            // Starts new RLE packet.
            current.p = next.p;
            count = 0;
          }
        }
        // Finishes the line.
        assert(count > 0 && count <= 128);
        PUSH_PIXEL_RGBA(dest_buffer, dest_size, count, current, mapping);
      }
    } else {
      for (int line = 0; line < src_size; line += src_pitch) {
        Pixel current = {{_src_buffer[line + 0], _src_buffer[line + 1],
                          _src_buffer[line + 2], 0}};
        int count = 1;
        for (int p = line + 4; p < line + _width * 4; p += 4, count++) {
          const Pixel next = {{_src_buffer[p + 0], _src_buffer[p + 1],
                               _src_buffer[p + 2], 0}};
          if (current.p != next.p || count == 128) {
            // Writes current packet.
            PUSH_PIXEL_RGB(dest_buffer, dest_size, count, current, mapping);
            // Starts new RLE packet.
            current.p = next.p;
            count = 0;
          }
        }
        // Finishes the line.
        assert(count > 0 && count <= 128);
        PUSH_PIXEL_RGB(dest_buffer, dest_size, count, current, mapping);
      }
    }
  } else {  // Source has no alpha channel.
    assert(Stride(_src_format) == 3);
    const int src_pitch = _width * 3;
    const int src_size = _height * src_pitch;
    if (_write_alpha) {
      for (int line = 0; line < src_size; line += src_pitch) {
        Pixel current = {{_src_buffer[line + 0], _src_buffer[line + 1],
                          _src_buffer[line + 2], 255}};
        int count = 1;
        for (int p = line + 3; p < line + _width * 3; p += 3, count++) {
          const Pixel next = {{
            _src_buffer[p + 0], _src_buffer[p + 1], _src_buffer[p + 2], 255}};
          if (current.p != next.p || count == 128) {
            // Writes current packet.
            PUSH_PIXEL_RGBA(dest_buffer, dest_size, count, current, mapping);
            // Starts new RLE packet.
            current.p = next.p;
            count = 0;
          }
        }
        // Finishes the line.
        assert(count > 0 && count <= 128);
        PUSH_PIXEL_RGBA(dest_buffer, dest_size, count, current, mapping);
      }
    } else {
      for (int line = 0; line < src_size; line += src_pitch) {
        Pixel current = {{
          _src_buffer[line + 0], _src_buffer[line + 1], _src_buffer[line + 2], 0}};
        int count = 1;
        for (int p = line + 3; p < line + _width * 3; p += 3, count++) {
          const Pixel next = {{
            _src_buffer[p + 0], _src_buffer[p + 1], _src_buffer[p + 2], 0}};
          if (current.p != next.p || count == 128) {
            // Writes current packet.
            PUSH_PIXEL_RGB(dest_buffer, dest_size, count, current, mapping);
            // Starts new RLE packet.
            current.p = next.p;
            count = 0;
          }
        }
        // Finishes the line.
        assert(count > 0 && count <= 128);
        PUSH_PIXEL_RGB(dest_buffer, dest_size, count, current, mapping);
      }
    }
  }

  // Writes all the RLE packets buffer at once.
  file.Write(dest_buffer, dest_size);

  ozz::memory::default_allocator()->Deallocate(dest_buffer);

  return true;
}
#undef PUSH_PIXEL_RGB
#undef PUSH_PIXEL_RGBA
}  // image
}  // sample
}  // ozz
