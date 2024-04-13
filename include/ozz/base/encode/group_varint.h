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

#ifndef OZZ_OZZ_BASE_ENCODE_GROUP_VARINT_H_
#define OZZ_OZZ_BASE_ENCODE_GROUP_VARINT_H_

#include "ozz/base/export.h"
#include "ozz/base/span.h"

// Implements group varint encoding, as described by Google in
// https://static.googleusercontent.com/media/research.google.com/en//people/jeff/WSDM09-keynote.pdf
// and used in Protocol Buffers. This is a variant of Variable-length quantity
// encoding, a compression technic that uses an arbitrary number of bytes to
// represent an arbitrarily large integer. Group Varint Encoding uses a single
// byte as a header for 4 variable-length uint32 values. The header byte has 4
// 2-bit numbers representing the storage length of each of the following 4
// uint32s. Such a layout eliminates the need to check and remove VLQ
// continuation bits. This layout reduces CPU branches, making GVE faster than
// VLQ. Implementation is inspired by
// https://github.com/facebook/folly/blob/master/folly/GroupVarint.h. It was
// made portable out of x86/64 world by removing unaligned accesses and little
// endian constrain.

namespace ozz {

namespace internal {

// Find the number of bytes (-1) required to store the integer value
inline uint8_t tag(uint32_t _v) {
  return (_v >= (1 << 24)) + (_v >= (1 << 16)) + (_v >= (1 << 8));
}

// Copies 4 bytes integer value to a byte buffer. All 4 bytes are written,
// whatever the value.
// Value is stored in little endian order. memcpy isn't used to ensure load and
// store can be done from different endianness systems.
inline void store(uint32_t _v, ozz::byte* _buffer) {
  _buffer[0] = _v & 0xff;
  _buffer[1] = (_v >> 8) & 0xff;
  _buffer[2] = (_v >> 16) & 0xff;
  _buffer[3] = (_v >> 24) & 0xff;
}
}  // namespace internal

// Encodes 4 unsigned integer to a buffer using group variable integer encoding.
// Output buffer must be big enough to store 4 unsigned integers (16 bytes) plus
// 1 byte (prefix), aka 17 bytes
// Returns the remaning unused buffer.
inline ozz::span<ozz::byte> EncodeGV4(const ozz::span<const uint32_t>& _input,
                                      const ozz::span<ozz::byte>& _buffer) {
  assert(_input.size() == 4 && "Input size must be 4");
  assert(_buffer.size_bytes() >= 4 * sizeof(uint32_t) + 1 &&
         "Output buffer is too small.");
  const uint8_t a_tag = internal::tag(_input[0]);
  const uint8_t b_tag = internal::tag(_input[1]);
  const uint8_t c_tag = internal::tag(_input[2]);
  const uint8_t d_tag = internal::tag(_input[3]);

  ozz::byte* out = _buffer.data();

  // Compute and store prefix.
  *out++ = static_cast<ozz::byte>((d_tag << 6) | (c_tag << 4) | (b_tag << 2) |
                                  a_tag);
  internal::store(_input[0], out);
  out += a_tag + 1;
  internal::store(_input[1], out);
  out += b_tag + 1;
  internal::store(_input[2], out);
  out += c_tag + 1;
  internal::store(_input[3], out);
  out += d_tag + 1;
  return {out, _buffer.end()};
}

namespace internal {

// Copies 4 bytes integer value from a byte buffer. All 4 bytes are read,
// whatever the value.
// Value is stored in little endian order. memcpy isn't used to ensure load and
// store can be done from different endianness systems.
inline uint32_t load(const ozz::byte* _in) {
  return static_cast<uint32_t>(_in[0]) | static_cast<uint32_t>(_in[1]) << 8 |
         static_cast<uint32_t>(_in[2]) << 16 |
         static_cast<uint32_t>(_in[3]) << 24;
}
}  // namespace internal

// Decodes 4 unsigned integer from a buffer created with EncodeGV4.
// Note that 0 to 3 more bytes (more than actually needed) can be read from the
// buffer. These 3 bytes value can be garbage, but must be redable.
// Returns the remaning unused buffer.
inline ozz::span<const ozz::byte> DecodeGV4(
    const ozz::span<const ozz::byte>& _buffer,
    const ozz::span<uint32_t>& _output) {
  assert(_buffer.size_bytes() >= 5 && "Input buffer is too small.");
  assert(_output.size() == 4 && "Output size must be 4");

  const ozz::byte* in = _buffer.data();
  const uint8_t prefix = *in++;

  constexpr uint32_t kMask[4] = {0xff, 0xffff, 0xffffff, 0xffffffff};
  const uint8_t k0 = prefix & 0x3;
  _output[0] = internal::load(in) & kMask[k0];
  in += k0 + 1;
  const uint8_t k1 = (prefix >> 2) & 0x3;
  _output[1] = internal::load(in) & kMask[k1];
  in += k1 + 1;
  const uint8_t k2 = (prefix >> 4) & 0x3;
  _output[2] = internal::load(in) & kMask[k2];
  in += k2 + 1;
  const uint8_t k3 = prefix >> 6;
  _output[3] = internal::load(in) & kMask[k3];
  in += k3 + 1;
  return {in, _buffer.end()};
}

// Compute worst case buffer size (15 bytes per groups of 4 integers).
inline size_t ComputeGV4WorstBufferSize(
    const ozz::span<const uint32_t>& _stream) {
  assert((_stream.size() % 4) == 0 && "Input stream must be multiple of 4");
  return _stream.size() * 4 + _stream.size() / 4;
}

// Encodes groups of 4 unsigned integer. Stream size must be multiple of 4. The
// output buffer must be big enough to store for the worst case (all full 32
// bits values). See ComputeGV4WorstBufferSize for determining maximum required
// size.
// Returns the remaning unused buffer.
inline ozz::span<ozz::byte> EncodeGV4Stream(
    const ozz::span<const uint32_t>& _stream,
    const ozz::span<ozz::byte>& _buffer) {
  assert((_stream.size() % 4) == 0 && "Input stream must be multiple of 4");
  assert(_buffer.size_bytes() >= ComputeGV4WorstBufferSize(_stream) &&
         "Output buffer is too small");

  ozz::span<ozz::byte> out = _buffer;
  for (const uint32_t* data = _stream.begin(); data < _stream.end();
       data += 4) {
    out = EncodeGV4({data, 4}, out);
  }

  return out;
}

// Decodes groups of 4 unsigned integer, encoded with EncodeGV4Stream. The
// number of integers to decode is fixed by _stream size. The input _buffer must
// contain the data for all these integers.
// Like with DecodeGV4, 0 to 3 more bytes (more than actually needed) can be
// read from the buffer. These 3 bytes value can be garbage, but must be
// redable.
// Returns the remaning unused buffer.
inline ozz::span<const ozz::byte> DecodeGV4Stream(
    const ozz::span<const ozz::byte>& _buffer,
    const ozz::span<uint32_t>& _stream) {
  assert((_stream.size() % 4) == 0 && "Input stream must be multiple of 4");
  // Check for minimum possible buffer size (5 bytes for 4 integers).
  assert(_buffer.size_bytes() >= (_stream.size() + _stream.size() / 4) &&
         "Output buffer is too small");

  ozz::span<const ozz::byte> in = _buffer;
  for (uint32_t* data = _stream.begin(); data < _stream.end(); data += 4) {
    in = DecodeGV4(in, {data, 4});
  }

  return in;
}

}  // namespace ozz
#endif  // OZZ_OZZ_BASE_ENCODE_GROUP_VARINT_H_
