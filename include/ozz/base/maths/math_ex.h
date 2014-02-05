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

#ifndef OZZ_OZZ_BASE_MATHS_MATH_EX_H_
#define OZZ_OZZ_BASE_MATHS_MATH_EX_H_

#include <cmath>
#include <cassert>

#include "ozz/base/platform.h"

namespace ozz {
namespace math {

// Returns the linear interpolation of _a and _b with coefficient _f.
// _f is not limited to range [0,1].
OZZ_INLINE float Lerp(float _a, float _b, float _f) {
  return (_b - _a) * _f + _a;
}

// Returns the minimum of _a and _b. Comparison's based on operator <.
template<typename _Ty>
OZZ_INLINE _Ty Min(_Ty _a, _Ty _b) {
  return (_a < _b)? _a : _b;
}

// Returns the maximum of _a and _b. Comparison's based on operator <.
template<typename _Ty>
OZZ_INLINE _Ty Max(_Ty _a, _Ty _b) {
  return (_b < _a)? _a : _b;
}

// Clamps _x between _a and _b. Comparison's based on operator <.
// Result is unknown if _a is not less or equal to _b.
template<typename _Ty>
OZZ_INLINE _Ty Clamp(_Ty _a, _Ty _x, _Ty _b) {
  const _Ty min = _x < _b? _x : _b;
  return min < _a? _a : min;
}

// Implements int selection, avoiding branching.
OZZ_INLINE int Select(bool _b, int _true, int _false) {
  return _false ^ (-static_cast<int>(_b) & (_true ^ _false));
}

// Implements float selection, avoiding branching.
OZZ_INLINE float Select(bool _b, float _true, float _false) {
  union {float f; int32 i;} t = {_true};
  union {float f; int32 i;} f = {_false};
  union {int32 i; float f;} r = {f.i^ (-static_cast<int32>(_b) & (t.i ^ f.i))};
  return r.f;
}

// Implements pointer selection, avoiding branching.
template<typename _Ty>
OZZ_INLINE _Ty* Select(bool _b, _Ty* _true, _Ty* _false) {
  union {_Ty* p; intptr i;} t = {_true};
  union {_Ty* p; intptr i;} f = {_false};
  union {intptr i; _Ty* p;} r =
    {f.i ^ (-static_cast<intptr>(_b) & (t.i ^ f.i))};
  return r.p;
}

// Implements const pointer selection, avoiding branching.
template<typename _Ty>
OZZ_INLINE const _Ty* Select(bool _b, const _Ty* _true, const _Ty* _false) {
  union {const _Ty* p; intptr i;} t = {_true};
  union {const _Ty* p; intptr i;} f = {_false};
  union {intptr i; const _Ty* p;} r =
    {f.i ^ (-static_cast<intptr>(_b) & (t.i ^ f.i))};
  return r.p;
}

// Tests whether _block is aligned to _alignment boundary.
template<typename _Ty>
OZZ_INLINE bool IsAligned(_Ty _value, std::size_t _alignment) {
  return (_value & (_alignment - 1)) == 0;
}
template<typename _Ty>
OZZ_INLINE bool IsAligned(_Ty* _address, std::size_t _alignment) {
  return (reinterpret_cast<intptr>(_address) & (_alignment - 1)) == 0;
}

// Aligns _block address to the first greater address that is aligned to
// _alignment boundaries.
template<typename _Ty>
OZZ_INLINE _Ty Align(_Ty _value, std::size_t _alignment) {
  return static_cast<_Ty>(_value + (_alignment - 1)) & (0 - _alignment);
}
template<typename _Ty>
OZZ_INLINE _Ty* Align(_Ty* _address, std::size_t _alignment) {
  return reinterpret_cast<_Ty*>(
    (reinterpret_cast<intptr>(_address) + (_alignment - 1)) & (0 - _alignment));
}

// Strides a pointer of _stride bytes.
template<typename _Ty>
OZZ_INLINE _Ty* Stride(_Ty* _value, intptr _stride) {
  return reinterpret_cast<const _Ty*>(
         reinterpret_cast<intptr>(_value) + _stride);
}
}  // math
}  // ozz
#endif  // OZZ_OZZ_BASE_MATHS_MATH_EX_H_
