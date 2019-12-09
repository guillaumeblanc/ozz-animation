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

#ifndef OZZ_OZZ_BASE_MEMORY_SCOPED_PTR_H_
#define OZZ_OZZ_BASE_MEMORY_SCOPED_PTR_H_

#include "ozz/base/memory/allocator.h"

#include <cassert>

namespace ozz {

// ScopedPtr is a smart pointer implementation that guarantees the object will
// be deleted, either on destruction of the ScopedPtr, or via an explicit reset
// / assignation. Object must have been allocated with OZZ_NEW with the default allocator, as it will be deleted by OZZ_DELTE with default allocator.
// In order to mimic raw pointer behavior, ScopedPtr provides assignation
// operator and implicit cast to pointer type. This is motivated by the fact
// that ScopePtr are used in simple cases. I isn't a safety choice, as it
// requires more care if using ScopedPtr in non trivial cases.
template <typename _Type>
class ScopedPtr {
 public:
  // Construct a NULL or valid ScopedPtr.
  // _pointer must have been allocated with OZZ_NEW with the default allocator.
  explicit ScopedPtr(_Type* _pointer = NULL) : pointer_(_pointer) {}

  // Deletes pointed object if any.
  ~ScopedPtr() { OZZ_DELETE(ozz::memory::default_allocator(), pointer_); }

  // Dereferences pointed object. Function will assert if NULL.
  _Type& operator*() const {
    assert(pointer_ != NULL && "Dereferencing NULL pointer.");
    return *pointer_;
  }

  // Returns a pointer to the referenced object. Function will assert if NULL.
  _Type* operator->() const {
    assert(pointer_ != NULL && "Dereferencing NULL pointer.");
    return pointer_;
  }

  // Implicit cast to the referenced object
  operator _Type*() const { return pointer_; }

  // Explicit get of the object pointer.
  _Type* get() const { return pointer_; }

  // Returns true if pointer is not NULL.
  operator bool() const { return pointer_ != NULL; }

  // Returns true if pointer is NULL.
  bool operator!() const { return pointer_ == NULL; }

  // Assign a new value to the pointer. Previous pointer will be deleted if not
  // NULL. Function will assert if reassigned to the same value, as this would
  // create a dangling pointer.
  void operator=(_Type* _pointer) { reset(_pointer); }

  // Reset pointer to a new value. Previous pointer will be deleted if not
  // NULL. Function will assert if reassigned to the same value, as this would
  // create a dangling pointer.
  void reset(_Type* _pointer = NULL) {
    // Reseting to the same value would deallocate twice.
    assert((_pointer == NULL || _pointer != pointer_) &&
           "ScopedPtr cannot be reseted to the same value.");
    OZZ_DELETE(ozz::memory::default_allocator(), pointer_);
    pointer_ = _pointer;
  }

  // Release pointer ownership, allowing caller to manage object lifetime /
  // destruction.
  _Type* release() {
    _Type* temp = pointer_;
    pointer_ = NULL;
    return temp;
  }

  // Swap two sopedPtr. No object is deleted during swap.
  void swap(ScopedPtr& _scope) {
    _Type* temp = _scope.pointer_;
    _scope.pointer_ = pointer_;
    pointer_ = temp;
  }

 private:
  // Prevent from copy and assignment.
  ScopedPtr(const ScopedPtr&);
  void operator=(const ScopedPtr&);

  // Internal pointed object.
  _Type* pointer_;
};

// External swap function.
template <class _Type>
inline void swap(ScopedPtr<_Type>& _a, ScopedPtr<_Type>& _b) {
  _a.swap(_b);
}
}  // namespace ozz
#endif  // OZZ_OZZ_BASE_MEMORY_SCOPED_PTR_H_
