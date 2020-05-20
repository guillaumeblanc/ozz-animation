//----------------------------------------------------------------------------//
//                                                                            //
// ozz-animation is hosted at http://github.com/guillaumeblanc/ozz-animation  //
// and distributed under the MIT License (MIT).                               //
//                                                                            //
// Copyright (c) Guillaume Blanc                                              //
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

#include "gtest/gtest.h"
#include "ozz/base/maths/math_ex.h"
#include "ozz/base/memory/allocator.h"

TEST(Allocate, Memory) {
  void* p = ozz::memory::default_allocator()->Allocate(12, 1024);
  EXPECT_TRUE(p != nullptr);
  EXPECT_TRUE(ozz::IsAligned(p, 1024));

  // Fills allocated memory.
  memset(p, 0, 12);

  ozz::memory::default_allocator()->Deallocate(p);
}

TEST(MallocCompliance, Memory) {
  {  // Allocating 0 byte gives a valid pointer.
    void* p = ozz::memory::default_allocator()->Allocate(0, 1024);
    EXPECT_TRUE(p != nullptr);
    ozz::memory::default_allocator()->Deallocate(p);
  }

  {  // Freeing of a nullptr pointer is valid.
    ozz::memory::default_allocator()->Deallocate(nullptr);
  }
}

struct AlignedInts {
  AlignedInts() {
    for (int i = 0; i < array_size; ++i) {
      array[i] = i;
    }
  }

  AlignedInts(int _i0) {
    array[0] = _i0;
    for (int i = 1; i < array_size; ++i) {
      array[i] = i;
    }
  }

  AlignedInts(int _i0, int _i1) {
    array[0] = _i0;
    array[1] = _i1;
    for (int i = 2; i < array_size; ++i) {
      array[i] = i;
    }
  }

  AlignedInts(int _i0, int _i1, int _i2) {
    array[0] = _i0;
    array[1] = _i1;
    array[2] = _i2;
    for (int i = 3; i < array_size; ++i) {
      array[i] = i;
    }
  }

  static const int array_size = 517;
  alignas(64) int array[array_size];
};

TEST(NewDelete, Memory) {
  AlignedInts* ai0 = ozz::New<AlignedInts>();
  ASSERT_TRUE(ai0 != nullptr);
  for (int i = 0; i < ai0->array_size; ++i) {
    EXPECT_EQ(ai0->array[i], i);
  }
  ozz::Delete(ai0);

  AlignedInts* ai1 = ozz::New<AlignedInts>(46);
  ASSERT_TRUE(ai1 != nullptr);
  EXPECT_EQ(ai1->array[0], 46);
  for (int i = 1; i < ai1->array_size; ++i) {
    EXPECT_EQ(ai1->array[i], i);
  }
  ozz::Delete(ai1);

  AlignedInts* ai2 = ozz::New<AlignedInts>(46, 69);
  ASSERT_TRUE(ai2 != nullptr);
  EXPECT_EQ(ai2->array[0], 46);
  EXPECT_EQ(ai2->array[1], 69);
  for (int i = 2; i < ai2->array_size; ++i) {
    EXPECT_EQ(ai2->array[i], i);
  }
  ozz::Delete(ai2);

  AlignedInts* ai3 = ozz::New<AlignedInts>(46, 69, 58);
  ASSERT_TRUE(ai3 != nullptr);
  EXPECT_EQ(ai3->array[0], 46);
  EXPECT_EQ(ai3->array[1], 69);
  EXPECT_EQ(ai3->array[2], 58);
  for (int i = 3; i < ai3->array_size; ++i) {
    EXPECT_EQ(ai3->array[i], i);
  }
  ozz::Delete(ai3);
}

class TestAllocator : public ozz::memory::Allocator {
 public:
  TestAllocator() : hard_coded_address_(&hard_coded_address_) {}

  void* hard_coded_address() const { return hard_coded_address_; }

 private:
  virtual void* Allocate(size_t _size, size_t _alignment) {
    (void)_size;
    (void)_alignment;
    return hard_coded_address_;
  }
  virtual void Deallocate(void* _block) { (void)_block; }

  void* hard_coded_address_;
};

TEST(AllocatorOverride, Memory) {
  TestAllocator test_allocator;
  ozz::memory::Allocator* previous =
      ozz::memory::SetDefaulAllocator(&test_allocator);
  ozz::memory::Allocator* current = ozz::memory::default_allocator();

  void* alloc = current->Allocate(1, 1);
  EXPECT_EQ(alloc, test_allocator.hard_coded_address());
  current->Deallocate(alloc);

  EXPECT_EQ(ozz::memory::SetDefaulAllocator(previous), current);
}
