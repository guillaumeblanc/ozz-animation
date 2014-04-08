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

#include "ozz/base/memory/allocator.h"

#include <cstdlib>
#include <cassert>
#include <memory.h>

#include "ozz/base/maths/math_ex.h"

namespace ozz {
namespace memory {

namespace {
struct Header {
  void* unaligned;
  std::size_t size;
};
}

// Implements the basic heap allocator->
// Will trace allocation count and assert in case of a memory leak.
class HeapAllocator : public Allocator {
 public:
  HeapAllocator() :
    allocation_count_(0) {
  }
  ~HeapAllocator() {
    assert(allocation_count_ == 0 && "Memory leak detected");
  }

 protected:
  void* Allocate(std::size_t _size, std::size_t _alignment) {
    // Allocates enough memory to store the header + required alignment space.
    const std::size_t to_allocate = _size + sizeof(Header) + _alignment - 1;
    char* unaligned = reinterpret_cast<char*>(malloc(to_allocate));
    if (!unaligned) {
      return NULL;
    }
    char* aligned = ozz::math::Align(unaligned + sizeof(Header), _alignment);
    assert(aligned + _size <= unaligned + to_allocate);  // Don't overrun.
    // Set the header
    Header* header = reinterpret_cast<Header*>(aligned - sizeof(Header));
    assert(reinterpret_cast<char*>(header) >= unaligned);
    header->unaligned = unaligned;
    header->size = _size;
    // Allocation's succeeded.
    ++allocation_count_;
    return aligned;
  }

  void* Reallocate(void* _block, std::size_t _size, std::size_t _alignment) {
    void* new_block = Allocate(_size, _alignment);
    // Copies and deallocate the old memory block.
    if (_block) {
      Header* old_header = reinterpret_cast<Header*>(
        reinterpret_cast<char*>(_block) - sizeof(Header));
      memcpy(new_block, _block, old_header->size);
      free(old_header->unaligned);

      // Deallocation completed.
      --allocation_count_;
    }
    return new_block;
  }

  void Deallocate(void* _block) {
    if (_block) {
      Header* header = reinterpret_cast<Header*>(
        reinterpret_cast<char*>(_block) - sizeof(Header));
      free(header->unaligned);
      // Deallocation completed.
      --allocation_count_;
    }
  }

 private:
  // Internal allocation count used to track memory leaks.
  // Should equals 0 at destruction time.
  int allocation_count_;
};

namespace {
// Instantiates the default heap allocator->
HeapAllocator g_heap_allocator;

// Instantiates the default heap allocator pointer.
Allocator* g_default_allocator = &g_heap_allocator;
}  // namespace

// Implements default allocator accessor.
Allocator* default_allocator() {
  return g_default_allocator;
}

// Implements default allocator setter.
Allocator* SetDefaulAllocator(Allocator* _allocator) {
  Allocator* previous = g_default_allocator;
  g_default_allocator = _allocator;
  return previous;
}
}  // memory
}  // ozz
