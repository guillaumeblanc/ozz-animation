//----------------------------------------------------------------------------//
//                                                                            //
// ozz-animation is hosted at http://github.com/guillaumeblanc/ozz-animation  //
// and distributed under the MIT License (MIT).                               //
//                                                                            //
// Copyright (c) 2019 Guillaume Blanc                                         //
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

#ifndef OZZ_OZZ_BASE_MEMORY_ALLOCATOR_H_
#define OZZ_OZZ_BASE_MEMORY_ALLOCATOR_H_

#include <cstddef>
#include <new>

#include "ozz/base/platform.h"

namespace ozz {
namespace memory {

// Forwards declare Allocator class.
class Allocator;

// Defines the default allocator accessor.
Allocator* default_allocator();

// Set the default allocator, used for all dynamic allocation inside ozz.
// Returns current memory allocator, such that in can be restored if needed.
Allocator* SetDefaulAllocator(Allocator* _allocator);

// Defines an abstract allocator class.
// Implements helper methods to allocate/deallocate POD typed objects instead of
// raw memory.
// Implements New and Delete function to allocate C++ objects, as a replacement
// of new and delete operators.
class Allocator {
 public:
  // Default virtual destructor.
  virtual ~Allocator() {}

  // Next functions are the pure virtual functions that must be implemented by
  // allocator concrete classes.

  // Allocates _size bytes on the specified _alignment boundaries.
  // Allocate function conforms with standard malloc function specifications.
  virtual void* Allocate(size_t _size, size_t _alignment) = 0;

  // Frees a block that was allocated with Allocate or Reallocate.
  // Argument _block can be NULL.
  // Deallocate function conforms with standard free function specifications.
  virtual void Deallocate(void* _block) = 0;

  // Changes the size of a block that was allocated with Allocate.
  // Argument _block can be NULL.
  // Reallocate function conforms with standard realloc function specifications.
  virtual void* Reallocate(void* _block, size_t _size, size_t _alignment) = 0;

  // Next functions are helper functions used to provide typed and ranged
  // allocations.

  // Replaces operator new with no argument.
  // New function conforms with standard operator new specifications.
  template <typename _Ty>
  _Ty* New() {
    void* alloc =
        memory::default_allocator()->Allocate(sizeof(_Ty), OZZ_ALIGN_OF(_Ty));
    if (alloc) {
      return new (alloc) _Ty;
    }
    return NULL;
  }

  // Replaces operator new with one argument.
  // New function conforms with standard operator new specifications.
  template <typename _Ty, typename _Arg0>
  _Ty* New(const _Arg0& _arg0) {
    void* alloc =
        memory::default_allocator()->Allocate(sizeof(_Ty), OZZ_ALIGN_OF(_Ty));
    if (alloc) {
      return new (alloc) _Ty(_arg0);
    }
    return NULL;
  }

  // Replaces operator new with two arguments.
  // New function conforms with standard operator new specifications.
  template <typename _Ty, typename _Arg0, typename _Arg1>
  _Ty* New(const _Arg0& _arg0, const _Arg1& _arg1) {
    void* alloc =
        memory::default_allocator()->Allocate(sizeof(_Ty), OZZ_ALIGN_OF(_Ty));
    if (alloc) {
      return new (alloc) _Ty(_arg0, _arg1);
    }
    return NULL;
  }

  // Replaces operator new with three arguments.
  // New function conforms with standard operator new specifications.
  template <typename _Ty, typename _Arg0, typename _Arg1, typename _Arg2>
  _Ty* New(const _Arg0& _arg0, const _Arg1& _arg1, const _Arg2& _arg2) {
    void* alloc =
        memory::default_allocator()->Allocate(sizeof(_Ty), OZZ_ALIGN_OF(_Ty));
    if (alloc) {
      return new (alloc) _Ty(_arg0, _arg1, _arg2);
    }
    return NULL;
  }

  // Replaces operator new with four arguments.
  // New function conforms with standard operator new specifications.
  template <typename _Ty, typename _Arg0, typename _Arg1, typename _Arg2,
            typename _Arg3>
  _Ty* New(const _Arg0& _arg0, const _Arg1& _arg1, const _Arg2& _arg2,
           const _Arg3& _arg3) {
    void* alloc =
        memory::default_allocator()->Allocate(sizeof(_Ty), OZZ_ALIGN_OF(_Ty));
    if (alloc) {
      return new (alloc) _Ty(_arg0, _arg1, _arg2, _arg3);
    }
    return NULL;
  }

  // Replaces operator delete for objects allocated using one of the New
  // functions ot *this allocator.
  // Delete function conforms with standard operator delete specifications.
  template <typename _Ty>
  void Delete(_Ty* _object) {
    if (_object) {
      _object->~_Ty();
      Deallocate(_object);
    }
  }
};
}  // namespace memory
}  // namespace ozz
#endif  // OZZ_OZZ_BASE_MEMORY_ALLOCATOR_H_
