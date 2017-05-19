//----------------------------------------------------------------------------//
//                                                                            //
// ozz-animation is hosted at http://github.com/guillaumeblanc/ozz-animation  //
// and distributed under the MIT License (MIT).                               //
//                                                                            //
// Copyright (c) 2017 Guillaume Blanc                                         //
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

#include "ozz/base/memory/allocator.h"

#include "gtest/gtest.h"

#include "ozz/base/maths/math_ex.h"

TEST(Malloc, Memory) {
  void* p = ozz::memory::default_allocator()->Allocate(12, 1024);
  EXPECT_TRUE(p != NULL);
  EXPECT_TRUE(ozz::math::IsAligned(p, 1024));

  // Fills allocated memory.
  memset(p, 0, 12);

  p = ozz::memory::default_allocator()->Reallocate(p, 46, 4096);
  EXPECT_TRUE(p != NULL);
  EXPECT_TRUE(ozz::math::IsAligned(p, 4096));

  // Fills allocated memory.
  memset(p, 0, 46);

  ozz::memory::default_allocator()->Deallocate(p);
}

TEST(Range, Memory) {
  ozz::Range<int> range =
      ozz::memory::default_allocator()->AllocateRange<int>(12);
  EXPECT_TRUE(range.begin != NULL);
  EXPECT_EQ(range.end, range.begin + 12);

  // Fills allocated memory.
  memset(range.begin, 0, sizeof(int) * 12);

  ozz::memory::default_allocator()->Reallocate(range, 46);
  EXPECT_TRUE(range.begin != NULL);
  EXPECT_EQ(range.end, range.begin + 46);

  // Fills allocated memory.
  memset(range.begin, 0, sizeof(int) * 46);

  ozz::memory::default_allocator()->Deallocate(range);

  EXPECT_TRUE(range.begin == NULL);
  EXPECT_TRUE(range.end == NULL);
}

TEST(MallocCompliance, Memory) {
  {  // Allocating 0 byte gives a valid pointer.
    void* p = ozz::memory::default_allocator()->Allocate(0, 1024);
    EXPECT_TRUE(p != NULL);
    ozz::memory::default_allocator()->Deallocate(p);
  }

  {  // Freeing of a NULL pointer is valid.
    ozz::memory::default_allocator()->Deallocate(NULL);
  }

  {  // Reallocating NULL pointer is valid
    void* p = ozz::memory::default_allocator()->Reallocate(NULL, 12, 1024);
    EXPECT_TRUE(p != NULL);

    // Fills allocated memory.
    memset(p, 0, 12);

    ozz::memory::default_allocator()->Deallocate(p);
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
  OZZ_ALIGN(64) int array[array_size];
};

TEST(TypedMalloc, Memory) {
  AlignedInts* p = ozz::memory::default_allocator()->Allocate<AlignedInts>(3);
  EXPECT_TRUE(p != NULL);
  EXPECT_TRUE(ozz::math::IsAligned(p, OZZ_ALIGN_OF(AlignedInts)));

  memset(p, 0, sizeof(AlignedInts) * 3);

  p = ozz::memory::default_allocator()->Reallocate<AlignedInts>(p, 46);
  EXPECT_TRUE(p != NULL);
  EXPECT_TRUE(ozz::math::IsAligned(p, OZZ_ALIGN_OF(AlignedInts)));

  memset(p, 0, sizeof(AlignedInts) * 46);

  ozz::memory::default_allocator()->Deallocate(p);
}

TEST(NewDelete, Memory) {
  AlignedInts* ai0 = ozz::memory::default_allocator()->New<AlignedInts>();
  ASSERT_TRUE(ai0 != NULL);
  for (int i = 0; i < ai0->array_size; ++i) {
    EXPECT_EQ(ai0->array[i], i);
  }
  ozz::memory::default_allocator()->Delete(ai0);

  AlignedInts* ai1 = ozz::memory::default_allocator()->New<AlignedInts>(46);
  ASSERT_TRUE(ai1 != NULL);
  EXPECT_EQ(ai1->array[0], 46);
  for (int i = 1; i < ai1->array_size; ++i) {
    EXPECT_EQ(ai1->array[i], i);
  }
  ozz::memory::default_allocator()->Delete(ai1);

  AlignedInts* ai2 = ozz::memory::default_allocator()->New<AlignedInts>(46, 69);
  ASSERT_TRUE(ai2 != NULL);
  EXPECT_EQ(ai2->array[0], 46);
  EXPECT_EQ(ai2->array[1], 69);
  for (int i = 2; i < ai2->array_size; ++i) {
    EXPECT_EQ(ai2->array[i], i);
  }
  ozz::memory::default_allocator()->Delete(ai2);

  AlignedInts* ai3 =
      ozz::memory::default_allocator()->New<AlignedInts>(46, 69, 58);
  ASSERT_TRUE(ai3 != NULL);
  EXPECT_EQ(ai3->array[0], 46);
  EXPECT_EQ(ai3->array[1], 69);
  EXPECT_EQ(ai3->array[2], 58);
  for (int i = 3; i < ai3->array_size; ++i) {
    EXPECT_EQ(ai3->array[i], i);
  }
  ozz::memory::default_allocator()->Delete(ai3);
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
  virtual void* Reallocate(void* _block, size_t _size, size_t _alignment) {
    (void)_block;
    (void)_size;
    (void)_alignment;
    return NULL;
  }

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
