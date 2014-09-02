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

#ifndef OZZ_OZZ_BASE_IO_ARCHIVE_H_
#define OZZ_OZZ_BASE_IO_ARCHIVE_H_

// Provides input (IArchive) and output (OArchive) serialization containers.
// Archive are similar to c++ iostream. Data can be saved to a OArchive with
// the << operator, or loaded from a IArchive with the >> operator.
// Primitive data types are simply saved/loaded to/from archives, while struct
// and class are saved/loaded through Save/Load intrusive or non-intrusive
// functions:
// - The intrusive function prototypes are "void Save(ozz::io::OArchive*) const"
// and "void Load(ozz::io::IArchive*)".
// - The non-intrusive functions allow to work on arrays of objects. They must
// be implemented in ozz::io namespace with these prototypes:
// "void Save(OArchive& _archive, const _Ty* _ty, size_t _count) and
// "void Load(IArchive& _archive, _Ty* _ty, size_t _count).
//
// Arrays of struct/class or primitive types can be saved/loaded with the
// helper function ozz::io::MakeArray() that is then streamed in or out using
// << and >> archive operators: archive << ozz::io::MakeArray(my_array, count);
//
// Versioning can be done using OZZ_IO_TYPE_VERSION macros. Type version
// is saved in the OArchive, and is given back to Load functions to allow to
// manually handle version modifications. Versioning can be disabled using
// OZZ_IO_TYPE_NOT_VERSIONABLE like macros. It can not be re-enabled afterward.
//
// Objects can be assigned a tag using OZZ_IO_TYPE_TAG macros. A tag allows to
// check the type of the next object to read from an archive. An automatic
// assertion check is performed for each object that has a tag. It can also be
// done manually to ensure an archive has the expected content.
//
// Endianness (big-endian or little-endian) can be specified while constructing
// an output archive (ozz::io::OArchive). Input archives automatically handle
// endianness conversion if the native platform endian mode differs from the
// archive one.
//
// IArchive and OArchive expect valid streams as argument, respectively opened
// for reading and writing. Archives do NOT perform error detection while
// reading or writing. All errors are considered programming errors. This leads
// to the following assertions on the user side:
// - When writing: the stream must be big (or grow-able) enough to support the
// data being written.
// - When reading: Stream's tell (position in the stream) must match the object
// being read. To help with this requirement, archives provide a tag mechanism
// that allows to check the tag (ie the type) of the next object to read. Stream
// integrity, like data corruption or file truncation, must also be validated on
// the user side.

#include "ozz/base/endianness.h"
#include "ozz/base/platform.h"
#include "ozz/base/io/stream.h"

#include <cassert>
#include <stdint.h>

#include "ozz/base/io/archive_traits.h"

namespace ozz {
namespace io {
namespace internal {
// Defines Tagger helper object struct.
// The boolean template argument is used to automatically select a template
// specialisation, whether _Ty has a tag or not.
template<typename _Ty, bool _HasTag = internal::Tag<const _Ty>::kTagLength != 0>
struct Tagger;
}  // internal

// Implements output archive concept used to save/serialize data from a Stream.
// The output endianness mode is set at construction time. It is written to the
// stream to allow the IArchive to perform the required conversion to the native
// endianness mode while reading.
class OArchive {
 public:

  // Constructs an output archive from the Stream _stream that must be valid
  // and opened for writing.
  explicit OArchive(Stream* _stream, 
                    Endianness _endianness = GetNativeEndianness());

  // Returns true if an endian swap is required while writing.
  bool endian_swap() const {
    return endian_swap_;
  }

  // Saves _size bytes of binary data from _data.
  void SaveBinary(const void* _data, size_t _size) {
    stream_->Write(_data, _size);
  }

  // Class type saving.
  template <typename _Ty>
  void operator<<(const _Ty& _ty) {
    internal::Tagger<const _Ty>::Write(*this);
    SaveVersion<_Ty>();
    Save(*this, &_ty, 1);
  }

  // Primitive type saving.
#define _OZZ_IO_PRIMITIVE_TYPE(_type)\
  void operator<<(_type _v) {\
    _type v = endian_swap_ ? EndianSwapper<_type>::Swap(_v) : _v;\
    stream_->Write(&v, sizeof(v));\
  }

  _OZZ_IO_PRIMITIVE_TYPE(char)
  _OZZ_IO_PRIMITIVE_TYPE(int8_t)
  _OZZ_IO_PRIMITIVE_TYPE(uint8_t)
  _OZZ_IO_PRIMITIVE_TYPE(int16_t)
  _OZZ_IO_PRIMITIVE_TYPE(uint16_t)
  _OZZ_IO_PRIMITIVE_TYPE(int32_t)
  _OZZ_IO_PRIMITIVE_TYPE(uint32_t)
  _OZZ_IO_PRIMITIVE_TYPE(int64_t)
  _OZZ_IO_PRIMITIVE_TYPE(uint64_t)
  _OZZ_IO_PRIMITIVE_TYPE(bool)
  _OZZ_IO_PRIMITIVE_TYPE(float)
#undef _OZZ_IO_PRIMITIVE_TYPE

  // Returns output stream.
  Stream* stream() const {
    return stream_;
  }

 private:
  template <typename _Ty>
  void SaveVersion() {
    // Compilation could fail here if the version is not defined for _Ty, or if
    // the .h file containing its definition is not included by the caller of
    // this function.
    if (void(0), internal::Version<const _Ty>::kValue != 0) {
      uint32_t version = internal::Version<const _Ty>::kValue;
      *this << version;
    }
  }

  // The output stream.
  Stream* stream_;

  // Endian swap state, true if a conversion is required while writing.
  bool endian_swap_;
};

// Implements input archive concept used to load/de-serialize data to a Stream.
// Endianness conversions are automatically performed according to the Archive
// and the native formats.
class IArchive {
 public:

  // Constructs an input archive from the Stream _stream that must be opened for
  // reading, at the same tell (position in the stream) as when it was passed to
  // the OArchive.
  explicit IArchive(Stream* _stream);

  // Returns true if an endian swap is required while reading.
  bool endian_swap() const {
    return endian_swap_;
  }

  // Loads _size bytes of binary data to _data.
  void LoadBinary(void* _data, size_t _size) {
    stream_->Read(_data, _size);
  }

  // Class type loading.
  template <typename _Ty>
  void operator>>(_Ty& _ty) {
    // Only uses tag validation for assertions, as reading cannot fail.
    OZZ_IF_DEBUG(bool valid =) internal::Tagger<const _Ty>::Validate(*this);
    assert(valid && "Type tag does not match archive content.");

    // Loads instance.
    uint32_t version = LoadVersion<_Ty>();
    Load(*this, &_ty, 1, version);
  }

  // Primitive type loading.
#define _OZZ_IO_PRIMITIVE_TYPE(_type)\
  void operator>>(_type& _v) {\
    _type v;\
    stream_->Read(&v, sizeof(v));\
    _v = endian_swap_ ? EndianSwapper<_type>::Swap(v) : v;\
  }

  _OZZ_IO_PRIMITIVE_TYPE(char)
  _OZZ_IO_PRIMITIVE_TYPE(int8_t)
  _OZZ_IO_PRIMITIVE_TYPE(uint8_t)
  _OZZ_IO_PRIMITIVE_TYPE(int16_t)
  _OZZ_IO_PRIMITIVE_TYPE(uint16_t)
  _OZZ_IO_PRIMITIVE_TYPE(int32_t)
  _OZZ_IO_PRIMITIVE_TYPE(uint32_t)
  _OZZ_IO_PRIMITIVE_TYPE(int64_t)
  _OZZ_IO_PRIMITIVE_TYPE(uint64_t)
  _OZZ_IO_PRIMITIVE_TYPE(bool)
  _OZZ_IO_PRIMITIVE_TYPE(float)
#undef _OZZ_IO_PRIMITIVE_TYPE

  template <typename _Ty>
  bool TestTag() {
    // Only tagged types can be tested. If compilations fails here, it can
    // mean the file containing tag declaration is not included.
    OZZ_STATIC_ASSERT(internal::Tag<const _Ty>::kTagLength != 0);

    const int tell = stream_->Tell();
    bool valid = internal::Tagger<const _Ty>::Validate(*this);
    stream_->Seek(tell, Stream::kSet); // Rewinds before the tag test.
    return valid;
  }

  // Returns input stream.
  Stream* stream() const {
    return stream_;
  }

 private:
  template <typename _Ty>
  uint32_t LoadVersion() {
    uint32_t version = 0;
    if (void(0), internal::Version<const _Ty>::kValue != 0) {
      *this >> version;
    }
    return version;
  }

  // The input stream.
  Stream* stream_;

  // Endian swap state, true if a conversion is required while reading.
  bool endian_swap_;
};

// Primitive type are not versionable.
OZZ_IO_TYPE_NOT_VERSIONABLE(char)
OZZ_IO_TYPE_NOT_VERSIONABLE(int8_t)
OZZ_IO_TYPE_NOT_VERSIONABLE(uint8_t)
OZZ_IO_TYPE_NOT_VERSIONABLE(int16_t)
OZZ_IO_TYPE_NOT_VERSIONABLE(uint16_t)
OZZ_IO_TYPE_NOT_VERSIONABLE(int32_t)
OZZ_IO_TYPE_NOT_VERSIONABLE(uint32_t)
OZZ_IO_TYPE_NOT_VERSIONABLE(int64_t)
OZZ_IO_TYPE_NOT_VERSIONABLE(uint64_t)
OZZ_IO_TYPE_NOT_VERSIONABLE(bool)
OZZ_IO_TYPE_NOT_VERSIONABLE(float)

// Default loading and saving external implementation.
template <typename _Ty>
inline void Save(OArchive& _archive, const _Ty* _ty, size_t _count) {
  for (size_t i = 0; i < _count; ++i) {
    _ty[i].Save(_archive);
  }
}
template <typename _Ty>
inline void Load(IArchive& _archive,
                 _Ty* _ty,
                 size_t _count,
                 uint32_t _version) {
  for (size_t i = 0; i < _count; ++i) {
    _ty[i].Load(_archive, _version);
  }
}

// Wrapper for dynamic array serialization.
// Must be used through ozz::io::MakeArray.
namespace internal {
template <typename _Ty>
struct Array {
  OZZ_INLINE void Save(OArchive& _archive) const {
    ozz::io::Save(_archive, array, count);
  }
  OZZ_INLINE void Load(IArchive& _archive, uint32_t _version) const {
    ozz::io::Load(_archive, array, count, _version);
  }
  _Ty* array;
  size_t count;
};

// Array copies version from the type it contains.
// Definition of Array of _Ty version: _Ty version.
template <typename _Ty> struct Version<const Array<_Ty> > {
  enum { kValue = Version<const _Ty>::kValue };
};

// Specializes Array Save/Load for primitive types.
#define _OZZ_IO_PRIMITIVE_TYPE(_type)\
template <>\
inline void Array<const _type>::Save(OArchive& _archive) const {\
  if (_archive.endian_swap()) {\
    /* Save element by element as swapping in place the whole buffer is not*/\
    /* possible.*/\
    for (size_t i = 0; i < count; ++i) {\
      _archive << array[i];\
    }\
  } else {\
    _archive.SaveBinary(array, count * sizeof(_type));\
  }\
}\
template <>\
inline void Array<_type>::Save(OArchive& _archive) const {\
  if (_archive.endian_swap()) {\
    /* Save element by element as swapping in place the whole buffer is not*/\
    /* possible.*/\
    for (size_t i = 0; i < count; ++i) {\
      _archive << array[i];\
    }\
  } else {\
    _archive.SaveBinary(array, count * sizeof(_type));\
  }\
}\
template <>\
inline void Array<_type>::Load(IArchive& _archive, uint32_t /*_version*/) const {\
  _archive.LoadBinary(array, count * sizeof(_type));\
  if (_archive.endian_swap()) { /*Can swap in-place.*/\
    EndianSwapper<_type>::Swap(array, count);\
  }\
}

_OZZ_IO_PRIMITIVE_TYPE(char)
_OZZ_IO_PRIMITIVE_TYPE(int8_t)
_OZZ_IO_PRIMITIVE_TYPE(uint8_t)
_OZZ_IO_PRIMITIVE_TYPE(int16_t)
_OZZ_IO_PRIMITIVE_TYPE(uint16_t)
_OZZ_IO_PRIMITIVE_TYPE(int32_t)
_OZZ_IO_PRIMITIVE_TYPE(uint32_t)
_OZZ_IO_PRIMITIVE_TYPE(int64_t)
_OZZ_IO_PRIMITIVE_TYPE(uint64_t)
_OZZ_IO_PRIMITIVE_TYPE(bool)
_OZZ_IO_PRIMITIVE_TYPE(float)
#undef _OZZ_IO_PRIMITIVE_TYPE
}  // internal

// Utility function that instantiates Array wrapper.
template <typename _Ty>
OZZ_INLINE const internal::Array<_Ty> MakeArray(_Ty* _array,
                                                size_t _count) {
  const internal::Array<_Ty> array = {_array, _count};
  return array;
}
template <typename _Ty>
OZZ_INLINE const internal::Array<const _Ty> MakeArray(const _Ty* _array,
                                                      size_t _count) {
  const internal::Array<const _Ty> array = {_array, _count};
  return array;
}
template <typename _Ty, size_t _count>
OZZ_INLINE const internal::Array<_Ty> MakeArray(_Ty (&_array)[_count]) {
  const internal::Array<_Ty> array = {_array, _count};
  return array;
}
template <typename _Ty, size_t _count>
OZZ_INLINE const internal::Array<const _Ty> MakeArray(
  const _Ty (&_array)[_count]) {
  const internal::Array<const _Ty> array = {_array, _count};
  return array;
}

namespace internal {
// Specialisation of the Tagger helper for tagged types.
template<typename _Ty>
struct Tagger<_Ty, true> {
  static void Write(OArchive& _archive) {
    typedef internal::Tag<const _Ty> Tag;
    _archive << ozz::io::MakeArray(Tag::Get(), Tag::kTagLength);
  }
  static bool Validate(IArchive& _archive) {
    typedef internal::Tag<const _Ty> Tag;
    char buf[Tag::kTagLength];
    _archive >> ozz::io::MakeArray(buf, Tag::kTagLength);
    const char* tag = Tag::Get();
    size_t i = 0;
    for (; i < Tag::kTagLength && buf[i] == tag[i]; ++i) {
    }
    return i == Tag::kTagLength;
  }
};

// Specialisation of the Tagger helper for types with no tag.
template<typename _Ty>
struct Tagger<_Ty, false> {
  static void Write(OArchive& /*_archive*/) {
  }
  static bool Validate(IArchive& /*_archive*/) {
    return true;
  }
};
}  // internal
}  // io
}  // ozz
#endif  // OZZ_OZZ_BASE_IO_ARCHIVE_H_
