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

#ifndef OZZ_OZZ_BASE_MEMORY_ALLOCATOR_H_
#define OZZ_OZZ_BASE_MEMORY_ALLOCATOR_H_

#include <cstddef>
#include <new>

#include "ozz/base/platform.h"

namespace ozz {
namespace memory {

// Declares a default alignment value.
static const std::size_t kDefaultAlignment = 16;

// Defines the default allocator accessor.
class Allocator& default_allocator();

// Defines an abstract allocator class.
// Implements helper methods to allocate/deallocate POD typed objects instead of
// raw memory.
// Implements New and Delete function to allocate C++ objects, as a replacement
// of new and delete operators.
class Allocator {
 public:
  // Default virtual destructor.
  virtual ~Allocator() {
  }
  // Allocates _size bytes on the specified _alignment boundaries.
  // Allocate function conforms with standard malloc function specifications.
  virtual void* Allocate(std::size_t _size, std::size_t _alignment) = 0;

  // Frees a block that was allocated with Allocate or Reallocate.
  // Argument _block can be NULL.
  // Deallocate function conforms with standard free function specifications.
  virtual void Deallocate(void* _block) = 0;

  // Changes the size of a block that was allocated with Allocate.
  // Argument _block can be NULL.
  // Reallocate function conforms with standard realloc function specifications.
  virtual void* Reallocate(void* _block,
                           std::size_t _size,
                           std::size_t _alignment) = 0;

  // Allocates an array of _count objects of type _Ty. Alignment is
  // automatically deduced from _Ty type.
  // Allocate function conforms with standard malloc function specifications.
  template<typename _Ty>
  _Ty* Allocate(std::size_t _count) {
    return reinterpret_cast<_Ty*>(
    Allocate(_count * sizeof(_Ty), AlignOf<_Ty>::value));
  }

  // Changes the size of a block that was allocated with Allocate.
  // Reallocates an array of _count objects of type _Ty. Alignment is
  // automatically deduced from _Ty type.
  // Reallocate function conforms with standard realloc function specifications.
  template<typename _Ty>
  _Ty* Reallocate(_Ty* _block, std::size_t _count) {
    return reinterpret_cast<_Ty*>(
      Reallocate(_block, _count * sizeof(_Ty), AlignOf<_Ty>::value));
  }

  // Replaces operator new with no argument.
  // New function conforms with standard operator new specifications.
  template<typename _Ty>
  _Ty* New() {
    return new(Allocate<_Ty>(1))_Ty;
  }

  // Replaces operator new with one argument.
  // New function conforms with standard operator new specifications.
  template<typename _Ty, typename _Arg0>
  _Ty* New(const _Arg0& _arg0) {
    return new(Allocate<_Ty>(1))_Ty(_arg0);
  }

  // Replaces operator new with two arguments.
  // New function conforms with standard operator new specifications.
  template<typename _Ty, typename _Arg0, typename _Arg1>
  _Ty* New(const _Arg0& _arg0, const _Arg1& _arg1) {
    return new(Allocate<_Ty>(1))_Ty(_arg0, _arg1);
  }

  // Replaces operator new with three arguments.
  // New function conforms with standard operator new specifications.
  template<typename _Ty, typename _Arg0, typename _Arg1, typename _Arg2>
  _Ty* New(const _Arg0& _arg0, const _Arg1& _arg1, const _Arg2& _arg2) {
    return new(Allocate<_Ty>(1))_Ty(_arg0, _arg1, _arg2);
  }

  // Replaces operator delete for objets allocated using one of the New
  // functions.
  // Delete function conforms with standard operator delete specifications.
  template<typename _Ty>
  void Delete(_Ty* _object) {
    if (_object) {
      _object->~_Ty();
      Deallocate(_object);
    }
  }
};
}  // memory
}  // ozz
#endif  // OZZ_OZZ_BASE_MEMORY_ALLOCATOR_H_
