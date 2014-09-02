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

#ifndef OZZ_OZZ_BASE_PLATFORM_H_
#define OZZ_OZZ_BASE_PLATFORM_H_

#include <cstddef>
#include <cassert>
#include <stdint.h>

namespace ozz {

// Compile time string concatenation.
#define OZZ_JOIN(_a, _b) _OZZ_JOIN(_a, _b)
// Compile time string concatenation implementation details.
#define _OZZ_JOIN(_a, _b) _OZZ_JOIN2(_a, _b)
#define _OZZ_JOIN2(_a, _b) _a##_b

// Compile time assertion. Breaks compiling if _condition is false.
// Defines an array with a negative number of elements if _condition is false,
// which generates a compiler error.
#define OZZ_STATIC_ASSERT(_condition)\
  struct OZZ_JOIN(_StaticAssert, __COUNTER__) { char x[(_condition) ? 1 : -1]; }

// Gets alignment in bytes required for any instance of the given type.
// Usage is AlignOf<MyStruct>::value.
template <typename _Ty>
struct AlignOf {
  enum { s = sizeof(_Ty), value = s ^ (s & (s - 1))};
};

// Finds the number of elements of a statically allocated array.
#define OZZ_ARRAY_SIZE(_array) (sizeof(_array) / sizeof(_array[0]))

// Specifies a minimum alignment (in bytes) for variables.
// Syntax is: "OZZ_ALIGN(16) int i;" which aligns "i" variable address to 16b.
#if defined(_MSC_VER)
#define OZZ_ALIGN(_alignment) __declspec(align(_alignment))
#else
#define OZZ_ALIGN(_alignment) __attribute__((aligned(_alignment)))
#endif

// Instructs the compiler to try to inline a function, regardless cost/benefit
// compiler analysis.
// Syntax is: "OZZ_INLINE void function();"
#if defined(_MSC_VER)
#define OZZ_INLINE __forceinline
#else
#define OZZ_INLINE inline __attribute__((always_inline))
#endif

// Tells the compiler to never inline a function.
// Syntax is: "OZZ_NO_INLINE void function();"
#if defined(_MSC_VER)
#define OZZ_NOINLINE __declspec(noinline)
#else
#define OZZ_NOINLINE __attribute__((noinline))
#endif

// Tells the compiler that the memory addressed by the restrict -qualified
// pointer is not aliased, aka no other pointer will access that same memory.
// Syntax is: void function(int* OZZ_RESTRICT _p);"
#define OZZ_RESTRICT __restrict

// Defines macro to help with DEBUG/NDEBUG syntax.
#if defined(NDEBUG)
#define OZZ_IF_DEBUG(...)
#define OZZ_IF_NDEBUG(...) __VA_ARGS__
#else  // NDEBUG
#define OZZ_IF_DEBUG(...) __VA_ARGS__
#define OZZ_IF_NDEBUG(...)
#endif  // NDEBUG

// Defines a range [begin,end[ of objects ot type _Ty.
template <typename _Ty>
struct Range {
  // Default constructor initializes range to empty.
  Range()
    : begin(NULL),
      end(NULL) {
  }
  // Constructs a range from its extreme values.
  Range(_Ty* _begin, _Ty* _end)
    : begin(_begin),
      end(_end) {
  }
  // Construct a range from a pointer to a buffer and its size, ie its number of elements.
  Range(_Ty* _begin, ptrdiff_t _size)
    : begin(_begin),
      end(_begin + _size) {
  }
  // Construct a range from an array, its size is automatically deduced.
  template <size_t _size>
  explicit Range(_Ty (&_array)[_size])
    : begin(_array),
      end(_array + _size) {
  }

  // Implement cast operator to allow conversions to Range<const _Ty>.
  operator Range<const _Ty> () const {
    return Range<const _Ty>(begin, end);
  }

  // Returns a const reference to element _i of range [begin,end[.
  const _Ty& operator[](size_t _i) const {
    assert(begin && &begin[_i] < end && "Index out of range");
    return begin[_i];
  }

  // Returns a reference to element _i of range [begin,end[.
  _Ty& operator[](size_t _i) {
    assert(begin && &begin[_i] < end && "Index out of range");
    return begin[_i];
  }

  // Gets the number of elements of the range.
  // This size isn't stored but computed from begin and end pointers.
  ptrdiff_t Size() const {
    return end - begin;
  }

  // Range begin pointer.
  _Ty* begin;

  // Range end pointer, declared as const as it should never be dereferenced.
  const _Ty* end;
};
}  // ozz
#endif  // OZZ_OZZ_BASE_PLATFORM_H_
