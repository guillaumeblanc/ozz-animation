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

// Encodes 4 unsigned integer to a buffer using group variable integer encoding.
// Output buffer must be big enough to store 4 unsigned integers (16 bytes) plus
// 1 byte (prefix), aka 17 bytes
// Returns the remaning unused buffer.
OZZ_BASE_DLL ozz::span<ozz::byte> EncodeGV4(
    const ozz::span<const uint32_t>& _input,
    const ozz::span<ozz::byte>& _buffer);

// Decodes 4 unsigned integer from a buffer created with EncodeGV4.
// Note that 0 to 3 more bytes (more than actually needed) can be read from the
// buffer. These 3 bytes value can be garbage, but must be readable.
// Returns the remaining unused buffer.
OZZ_BASE_DLL ozz::span<const ozz::byte> DecodeGV4(
    const ozz::span<const ozz::byte>& _buffer,
    const ozz::span<uint32_t>& _output);

// Compute worst case buffer size (15 bytes per groups of 4 integers).
// _stream size must be a multiple of 4.
OZZ_BASE_DLL size_t
ComputeGV4WorstBufferSize(const ozz::span<const uint32_t>& _stream);

// Encodes groups of 4 unsigned integer. Stream size must be multiple of 4. The
// output buffer must be big enough to store for the worst case (all full 32
// bits values). See ComputeGV4WorstBufferSize for determining maximum required
// size.
// Returns the remaning unused buffer.
OZZ_BASE_DLL ozz::span<ozz::byte> EncodeGV4Stream(
    const ozz::span<const uint32_t>& _stream,
    const ozz::span<ozz::byte>& _buffer);

// Decodes groups of 4 unsigned integer, encoded with EncodeGV4Stream. The
// number of integers to decode is fixed by _stream size. The input _buffer must
// contain the data for all these integers.
// Like with DecodeGV4, 0 to 3 more bytes (more than actually needed) can be
// read from the buffer. These 3 bytes value can be garbage, but must be
// readable.
// Returns the remaining unused buffer.
OZZ_BASE_DLL ozz::span<const ozz::byte> DecodeGV4Stream(
    const ozz::span<const ozz::byte>& _buffer,
    const ozz::span<uint32_t>& _stream);

}  // namespace ozz

#endif  // OZZ_OZZ_BASE_ENCODE_GROUP_VARINT_H_
