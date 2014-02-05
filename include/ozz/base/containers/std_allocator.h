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

#ifndef OZZ_OZZ_BASE_CONTAINERS_STD_ALLOCATOR_H_
#define OZZ_OZZ_BASE_CONTAINERS_STD_ALLOCATOR_H_

#include <new>

#include "ozz/base/memory/allocator.h"

namespace ozz {
// Define a STL allocator compliant allocator->
template<typename _Ty>
class StdAllocator {
public:
  typedef _Ty value_type;  // Element type.
  typedef value_type* pointer;  // Pointer to element.
  typedef value_type& reference;  // Reference to element.
  typedef const value_type* const_pointer;  // Constant pointer to element.
  typedef const value_type& const_reference;  // Constant reference to element.
  typedef std::size_t size_type;  // Quantities of elements.
  typedef std::ptrdiff_t difference_type;  // Difference between two pointers.

  // Converts an StdAllocator<_Ty> to an StdAllocator<_Other>.
  template<class _Other>
  struct rebind {
    typedef StdAllocator<_Other> other;
  };

  // Returns address of mutable _val.
  pointer address(reference _val) const {
    return (&_val);
  }

  // Returns address of non-mutable _val.
  const_pointer address(const_reference _val) const {
    return (&_val);
  }

  // Constructs default allocator (does nothing).
  StdAllocator() {
  }

  // Constructs by copying (does nothing).
  StdAllocator(const StdAllocator<_Ty>&) {
  }

  // Constructs from a related allocator (does nothing).
  template<class _Other>
  StdAllocator(const StdAllocator<_Other>&) {
  }

  // Assigns from a related allocator (does nothing).
  template<class _Other>
  StdAllocator<_Ty>& operator=(const StdAllocator<_Other>&) {
    return (*this);
  }

  // Deallocates object at _Ptr, ignores size.
  void deallocate(pointer _ptr, size_type) {
    memory::default_allocator()->Deallocate(_ptr);
  }

  // Allocates array of _Count elements.
  pointer allocate(size_type _count) {
    return memory::default_allocator()->Allocate<_Ty>(_count);
  }

  // Allocates array of _Count elements, ignores hint.
  pointer allocate(size_type _count, const void*) {
    return (allocate(_count));
  }

  // Constructs object at _Ptr with value _val.
  void construct(pointer _ptr, const _Ty& _val) {
    void* vptr = _ptr;
    ::new (vptr) _Ty(_val);
  }

  // Destroys object at _Ptr.
  void destroy(pointer _ptr) {
    if (_ptr) {
      _ptr->~_Ty();
    }
  }

  // Estimates maximum array size.
  std::size_t max_size() const {
    std::size_t count = static_cast<std::size_t>(-1) / sizeof (_Ty);
    return (count > 0 ? count : 1);
  }
};

// Tests for allocator equality (always true).
template<class _Ty, class _Other>
inline bool operator==(const StdAllocator<_Ty>&, const StdAllocator<_Other>&) {
  return true;
}

// Tests for allocator inequality (always false).
template<class _Ty, class _Other>
inline bool operator!=(const StdAllocator<_Ty>&, const StdAllocator<_Other>&) {
  return false;
}
}  // ozz
#endif  // OZZ_OZZ_BASE_CONTAINERS_STD_ALLOCATOR_H_
