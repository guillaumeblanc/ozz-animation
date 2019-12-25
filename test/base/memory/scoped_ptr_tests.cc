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

#include "ozz/base/memory/scoped_ptr.h"

#include "ozz/base/gtest_helper.h"

TEST(Construction, ScopedPtr) {
  { const ozz::ScopedPtr<int> pi; }
  {
    const ozz::ScopedPtr<int> pi(
        OZZ_NEW(ozz::memory::default_allocator(), int));
  }
}

TEST(Assignment, ScopedPtr) {
  {
    ozz::ScopedPtr<int> pi;
    pi = NULL;
    pi = OZZ_NEW(ozz::memory::default_allocator(), int);
  }
  {
    ozz::ScopedPtr<int> pi(OZZ_NEW(ozz::memory::default_allocator(), int));
    pi = OZZ_NEW(ozz::memory::default_allocator(), int);
    pi = NULL;
  }
  {
    int* i = OZZ_NEW(ozz::memory::default_allocator(), int)(46);
    ozz::ScopedPtr<int> pi(i);
    EXPECT_ASSERTION(pi = i, "ScopedPtr cannot be reseted to the same value.");
  }
}

TEST(Reset, ScopedPtr) {
  {
    ozz::ScopedPtr<int> pi;
    pi.reset(NULL);

    pi.reset(OZZ_NEW(ozz::memory::default_allocator(), int));
  }
  {
    ozz::ScopedPtr<int> pi(OZZ_NEW(ozz::memory::default_allocator(), int));
    pi.reset(OZZ_NEW(ozz::memory::default_allocator(), int));
    pi.reset(NULL);
  }
  {
    int* i = OZZ_NEW(ozz::memory::default_allocator(), int)(46);
    ozz::ScopedPtr<int> pi(i);
    EXPECT_ASSERTION(pi.reset(i),
                     "ScopedPtr cannot be reseted to the same value.");
  }
}

struct A {
  int i;
};

TEST(Dereference, ScopedPtr) {
  {
    const ozz::ScopedPtr<int> pi;
    EXPECT_ASSERTION(*pi, "Dereferencing NULL pointer.");
    EXPECT_TRUE(pi.get() == NULL);
    EXPECT_FALSE(pi);
  }
  {
    const ozz::ScopedPtr<int> pi(
        OZZ_NEW(ozz::memory::default_allocator(), int)(46));
    EXPECT_EQ(*pi, 46);
    EXPECT_TRUE(pi.get() != NULL);
    EXPECT_TRUE(pi);
  }
  {
    const ozz::ScopedPtr<A> pa;
    EXPECT_ASSERTION(pa->i = 99, "Dereferencing NULL pointer.");
  }
  {
    const ozz::ScopedPtr<A> pa(OZZ_NEW(ozz::memory::default_allocator(), A));
    pa->i = 46;
    EXPECT_EQ((*pa).i, 46);
  }
}

TEST(Bool, ScopedPtr) {
  {
    const ozz::ScopedPtr<int> pi;
    EXPECT_TRUE(!pi);
    EXPECT_FALSE(pi);
  }
  {
    const ozz::ScopedPtr<int> pi(
        OZZ_NEW(ozz::memory::default_allocator(), int)(46));
    EXPECT_FALSE(!pi);
    EXPECT_TRUE(pi);
  }
}

TEST(Cast, ScopedPtr) {
  {
    const ozz::ScopedPtr<int> pi;
    int* i = pi;
    EXPECT_TRUE(i == NULL);
  }
  {
    const ozz::ScopedPtr<int> pi(
        OZZ_NEW(ozz::memory::default_allocator(), int)(46));
    int* i = pi;
    EXPECT_TRUE(i != NULL);
  }
}

TEST(Swap, ScopedPtr) {
  {
    int* i = OZZ_NEW(ozz::memory::default_allocator(), int)(46);
    ozz::ScopedPtr<int> pi;
    ozz::ScopedPtr<int> pii(i);
    EXPECT_TRUE(pi.get() == NULL);
    EXPECT_TRUE(pii.get() == i);

    pi.swap(pii);
    EXPECT_TRUE(pii.get() == NULL);
    EXPECT_TRUE(pi.get() == i);
  }
  {
    int* i = OZZ_NEW(ozz::memory::default_allocator(), int)(46);
    ozz::ScopedPtr<int> pi;
    ozz::ScopedPtr<int> pii(i);
    EXPECT_TRUE(pi.get() == NULL);
    EXPECT_TRUE(pii.get() == i);

    swap(pi, pii);
    EXPECT_TRUE(pii.get() == NULL);
    EXPECT_TRUE(pi.get() == i);
  }
}

TEST(Release, ScopedPtr) {
  {
    int* i = OZZ_NEW(ozz::memory::default_allocator(), int)(46);
    ozz::ScopedPtr<int> pi(i);
    int* ri = pi.release();
    EXPECT_EQ(i, ri);
    OZZ_DELETE(ozz::memory::default_allocator(), ri);
  }
}
