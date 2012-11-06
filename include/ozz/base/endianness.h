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

#ifndef OZZ_OZZ_BASE_ENDIANNESS_H_
#define OZZ_OZZ_BASE_ENDIANNESS_H_

// Declares endianness modes and functions to swap data from a mode to another.

#include "ozz/base/platform.h"
#include <cstddef>

namespace ozz {

// Declares supported endianness.
enum Endianness {
  kBigEndian,
  kLittleEndian,
};

// Get the native endianness of the targeted processor.
// This function does not rely on a pre-processor definition as no standard
// definition exists. It is rather implemented as a portable runtime function.
inline Endianness GetNativeEndianness() {
  const union {
    ozz::uint16 s;
    ozz::uint8 c[2];
  } u = {1};  // Initializes u.s -> then read u.c.
  return Endianness(u.c[0]);
}

// Declare the endian swapper struct that is aimed to be specialized (template
// meaning) for every type sizes.
// The swapper provides two functions:
// - void Swap(_Ty* _ty, std::size_t _count) swaps the array _ty of _count
// elements in-place.
// - _Ty Swap(_Ty _ty) returns a swapped copy of _ty.
// It can be used directly if _Ty is known or through EndianSwap function.
// The default value of template attribute _size enables automatic
// specialization selection.
template <typename _Ty, std::size_t _size = sizeof(_Ty)>
struct EndianSwapper;

// Internal macro used to swap two bytes.
#define _OZZ_BYTE_SWAP(_a, _b) {\
  const ozz::uint8 temp = _a;\
  _a = _b;\
  _b = temp;\
}

// EndianSwapper specialization for 1 byte types.
template <typename _Ty>
struct EndianSwapper<_Ty, 1> {
  OZZ_INLINE static void Swap(_Ty* /*_ty*/, std::size_t /*_count*/) {
  }
  OZZ_INLINE static _Ty Swap(_Ty _ty) {
    return _ty;
  }
};

// EndianSwapper specialization for 2 bytes types.
template <typename _Ty>
struct EndianSwapper<_Ty, 2> {
  OZZ_INLINE static void Swap(_Ty* _ty, std::size_t _count) {
    char* alias = reinterpret_cast<char*>(_ty);
    for (std::size_t i = 0; i < _count * 2; i += 2) {
      _OZZ_BYTE_SWAP(alias[i + 0], alias[i + 1]);
    }
  }
  OZZ_INLINE static _Ty Swap(_Ty _ty) {  // Pass by copy to swap _ty in-place.
    char* alias = reinterpret_cast<char*>(&_ty);
    _OZZ_BYTE_SWAP(alias[0], alias[1]);
    return _ty;
  }
};

// EndianSwapper specialization for 4 bytes types.
template <typename _Ty>
struct EndianSwapper<_Ty, 4> {
  OZZ_INLINE static void Swap(_Ty* _ty, std::size_t _count) {
    char* alias = reinterpret_cast<char*>(_ty);
    for (std::size_t i = 0; i < _count * 4; i += 4) {
      _OZZ_BYTE_SWAP(alias[i + 0], alias[i + 3]);
      _OZZ_BYTE_SWAP(alias[i + 1], alias[i + 2]);
    }
  }
  OZZ_INLINE static _Ty Swap(_Ty _ty) {  // Pass by copy to swap _ty in-place.
    char* alias = reinterpret_cast<char*>(&_ty);
    _OZZ_BYTE_SWAP(alias[0], alias[3]);
    _OZZ_BYTE_SWAP(alias[1], alias[2]);
    return _ty;
  }
};

// EndianSwapper specialization for 8 bytes types.
template <typename _Ty>
struct EndianSwapper<_Ty, 8> {
  OZZ_INLINE static void Swap(_Ty* _ty, std::size_t _count) {
    char* alias = reinterpret_cast<char*>(_ty);
    for (std::size_t i = 0; i < _count * 8; i += 8) {
      _OZZ_BYTE_SWAP(alias[i + 0], alias[i + 7]);
      _OZZ_BYTE_SWAP(alias[i + 1], alias[i + 6]);
      _OZZ_BYTE_SWAP(alias[i + 2], alias[i + 5]);
      _OZZ_BYTE_SWAP(alias[i + 3], alias[i + 4]);
    }
  }
  OZZ_INLINE static _Ty Swap(_Ty _ty) {  // Pass by copy to swap _ty in-place.
    char* alias = reinterpret_cast<char*>(&_ty);
    _OZZ_BYTE_SWAP(alias[0], alias[7]);
    _OZZ_BYTE_SWAP(alias[1], alias[6]);
    _OZZ_BYTE_SWAP(alias[2], alias[5]);
    _OZZ_BYTE_SWAP(alias[3], alias[4]);
    return _ty;
  }
};

// _OZZ_BYTE_SWAP is not useful anymore.
#undef _OZZ_BYTE_SWAP

// Helper function that swaps _count elements of the array _ty in place.
template<typename _Ty>
OZZ_INLINE void EndianSwap(_Ty* _ty, std::size_t _count) {
  EndianSwapper<_Ty>::Swap(_ty, _count);
}

// Helper function that swaps _ty in place.
template<typename _Ty>
OZZ_INLINE _Ty EndianSwap(_Ty _ty) {
  return EndianSwapper<_Ty>::Swap(_ty);
}
}  // ozz
#endif  // OZZ_OZZ_BASE_ENDIANNESS_H_
