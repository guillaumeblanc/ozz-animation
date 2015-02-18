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

#ifndef OZZ_OZZ_BASE_CONTAINERS_VECTOR_H_
#define OZZ_OZZ_BASE_CONTAINERS_VECTOR_H_

#include <vector>

#include "ozz/base/containers/std_allocator.h"

namespace ozz {
// Redirects std::vector to ozz::Vector in order to replace std default
// allocator by ozz::StdAllocator.
// Extends std::vector with two functions that gives access to the begin and the
// end of its array of elements.
template <class _Ty, class _Allocator = ozz::StdAllocator<_Ty> >
struct Vector {
  typedef std::vector<_Ty, _Allocator> Std;
};

// Returns the mutable begin of the array of elements, or NULL if
// vector's empty.
template <class _Ty, class _Allocator>
inline _Ty* array_begin(std::vector<_Ty, _Allocator>& _vector) {
  size_t size = _vector.size();
  return size != 0 ? &_vector[0] : NULL;
}

// Returns the non-mutable begin of the array of elements, or NULL if
// vector's empty.
template <class _Ty, class _Allocator>
inline const _Ty* array_begin(const std::vector<_Ty, _Allocator>& _vector) {
  size_t size = _vector.size();
  return size != 0 ? &_vector[0] : NULL;
}

// Returns the mutable end of the array of elements, or NULL if
// vector's empty. Array end is one element past the last element of the
// array. It cannot be dereferenced.
template <class _Ty, class _Allocator>
inline _Ty* array_end(std::vector<_Ty, _Allocator>& _vector) {
  size_t size = _vector.size();
  return size != 0 ? (&_vector[size - 1]) + 1 : NULL;
}

// Returns the non-mutable end of the array of elements, or NULL if
// vector's empty. Array end is one element past the last element of the
// array. It cannot be dereferenced.
template <class _Ty, class _Allocator>
inline const _Ty* array_end(const std::vector<_Ty, _Allocator>& _vector) {
  size_t size = _vector.size();
  return size != 0 ? (&_vector[size - 1]) + 1 : NULL;
}
}  // ozz
#endif  // OZZ_OZZ_BASE_CONTAINERS_VECTOR_H_
