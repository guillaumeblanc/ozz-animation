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
};

// ozz replacement for c++ operator new with, used to allocate with an
// ozz::memory::Allocator. OZZ_DELETE must be used to deallocate such object.
// It can be used for constructor with no argument:
// Type* object = OZZ_NEW(allocator, Type)
// or any number of argument:
// Type* object = OZZ_NEW(allocator, Type)(1,2,3,4)
// Using a macro is motivated by the fact that it's not possible prior to c++11
// to forward every possible type of argument (&, const&, rvalue, ...) to
// constructors, especially if big number of arguments is required.
#define OZZ_NEW(x_allocator, x_type) \
  new ((x_allocator)->Allocate(sizeof(x_type), OZZ_ALIGN_OF(x_type))) x_type

// ozz replacement for c++ delete. Must be used for objects allocated with
// OZZ_NEW and the same ozz::memory::Allocator.
// OZZ_DELETE conforms with standard operator delete specifications.
#define OZZ_DELETE(x_allocator, x_object)              \
                                                       \
  do {                                                 \
    if ((x_object) != NULL) {                          \
      ozz::memory::internal::CallDestructor(x_object); \
      (x_allocator)->Deallocate(x_object);             \
    }                                                  \
  }                                                    \
                                                       \
  while (void(0), 0)

// Function used internally to deduce object type and thus be able to call its
// destructor.
namespace internal {
template <typename _Ty>
void CallDestructor(_Ty* _object) {
  (void)_object;  // prevents from false "unreferenced parameter" warning when
                  // _Ty has no explicit destructor.
  _object->~_Ty();
}
}  // namespace internal
}  // namespace memory
}  // namespace ozz
#endif  // OZZ_OZZ_BASE_MEMORY_ALLOCATOR_H_
