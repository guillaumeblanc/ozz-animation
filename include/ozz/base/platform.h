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

#ifndef OZZ_OZZ_BASE_PLATFORM_H_
#define OZZ_OZZ_BASE_PLATFORM_H_

#include <cstddef>

namespace ozz {

// Defines fixed size integer types. Serialization, bit patterns and some other
// cases requires to use fixed size integers.
// "int" is the default runtime integer type. Assumes that an "int" is at least
// 32 bits.
// Ozz do not use <stdint.h> as this header is not available with all compilers.
typedef char int8;
typedef unsigned char uint8;
typedef short int16;
typedef unsigned short uint16;
typedef int int32;
typedef unsigned int uint32;
typedef long long int64;
typedef unsigned long long uint64;

namespace internal {
// Declares a template helper to help selecting intptr and uintptr types
// according to pointers size.
template <bool _B = (sizeof(int32*) == 4)> struct IntPtrSel{
  typedef int32 type;
};
template <> struct IntPtrSel<false> { typedef int64 type; };
template <bool _B = (sizeof(uint32*) == 4)> struct UIntPtrSel{
  typedef uint32 type;
};
template <> struct UIntPtrSel<false> { typedef uint64 type; };
} // internal

typedef internal::IntPtrSel<>::type intptr;
typedef internal::UIntPtrSel<>::type uintptr;

// Compile time string concatenation.
#define OZZ_JOIN(_a, _b) _OZZ_JOIN(_a, _b)
// Compile time string concatenation implementation details.
#define _OZZ_JOIN(_a, _b) _OZZ_JOIN2(_a, _b)
#define _OZZ_JOIN2(_a, _b) _a##_b

// Compile time assertion. Breaks compiling if _condition is false.
// Defines an array with a negative number of elements if _condition is false,
// which generates a compiler error.
#define OZZ_STATIC_ASSERT(_condition)\
  struct OZZ_JOIN(_StaticAssert, __LINE__) { char x[(_condition) ? 1 : -1]; }

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
}  // ozz
#endif  // OZZ_OZZ_BASE_PLATFORM_H_
