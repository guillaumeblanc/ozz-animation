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

#include "ozz/base/containers/deque.h"
#include "ozz/base/containers/list.h"
#include "ozz/base/containers/map.h"
#include "ozz/base/containers/queue.h"
#include "ozz/base/containers/stack.h"
#include "ozz/base/containers/set.h"
#include "ozz/base/containers/string.h"
#include "ozz/base/containers/vector.h"

#include "gtest/gtest.h"

#include "ozz/base/gtest_helper.h"

TEST(Vector, Containers) {
  typedef ozz::Vector<int> Container;
  Container::Std container;
  container.push_back(1);
  container.insert(container.begin(), 0);
  container.push_back(2);
  EXPECT_EQ(container[0], 0);
  EXPECT_EQ(container[1], 1);
  EXPECT_EQ(container[2], 2);
  container.clear();
}

TEST(VectorExtensions, Containers) {
  typedef ozz::Vector<int> Container;
  Container::Std container;
  int* null = NULL;

  // Non-mutable access.
  EXPECT_EQ(array_begin(container), null);
  EXPECT_EQ(array_end(container), null);
  EXPECT_EQ(array_end(container), array_begin(container));

  container.push_back(1);
  container.push_back(2);
  EXPECT_EQ(*array_begin(container), 1);
  EXPECT_EQ(*(array_begin(container) + 1), 2);
  EXPECT_EQ(array_begin(container) + 2, array_end(container));
  EXPECT_NE(array_end(container), null);
  EXPECT_EQ(*(array_end(container) - 2), 1);
  EXPECT_EQ(array_end(container) - array_begin(container), 2);

  // Mutable access.
  *array_begin(container) = 0;
  EXPECT_EQ(*array_begin(container), 0);
  EXPECT_EQ(*(array_begin(container) + 1), 2);
}

TEST(Deque, Containers) {
  typedef ozz::Deque<int> Container;
  Container::Std container;
  container.push_back(1);
  container.push_front(0);
  container.push_back(2);
  EXPECT_EQ(container[0], 0);
  EXPECT_EQ(container[1], 1);
  EXPECT_EQ(container[2], 2);
  container.clear();
}

TEST(List, Containers) {
  typedef ozz::List<int> Container;
  Container::Std container;
  container.push_back(1);
  container.push_front(0);
  container.push_back(2);
  EXPECT_EQ(container.front(), 0);
  EXPECT_EQ(container.back(), 2);
  container.clear();
}

TEST(Stack, Containers) {
  typedef ozz::Stack<int> Container;
  Container::Std container;
  container.push(1);
  container.push(2);
  EXPECT_EQ(container.top(), 2);
  container.pop();
  EXPECT_EQ(container.top(), 1);
  container.pop();
}

TEST(Queue, Containers) {
  {
    typedef ozz::Queue<int> Container;
    Container::Std container;
    container.push(1);
    container.push(2);
    EXPECT_EQ(container.back(), 2);
    EXPECT_EQ(container.front(), 1);
    container.pop();
    EXPECT_EQ(container.back(), 2);
    container.pop();
  }
  {
    typedef ozz::PriorityQueue<int> Container;
    Container::Std container;
    container.push(1);
    container.push(2);
    container.push(0);
    EXPECT_EQ(container.top(), 2);
    container.pop();
    EXPECT_EQ(container.top(), 1);
    container.pop();
    EXPECT_EQ(container.top(), 0);
    container.pop();
  }
}

TEST(Set, Containers) {
  {
    typedef ozz::Set<int> Container;
    Container::Std container;
    container.insert('c');
    container.insert('a');
    container.insert('b');
    EXPECT_TRUE(container.find('a') == container.begin());
    EXPECT_TRUE(container.find('c') == --container.end());
    container.erase('c');
    EXPECT_TRUE(container.find('b') == --container.end());
    container.clear();
  }
  {
    typedef ozz::MultiSet<int> Container;
    Container::Std container;
    container.insert('c');
    container.insert('a');
    container.insert('b');
    container.insert('a');
    EXPECT_TRUE(container.find('a') == container.begin() ||
                container.find('a') == ++container.begin());
    EXPECT_TRUE(container.find('c') == --container.end());
    container.erase('c');
    EXPECT_TRUE(container.find('b') == --container.end());
    container.clear();
  }
}

TEST(Map, Containers) {
  {
    typedef ozz::Map<char, int> Container;
    Container::Std container;
    container['a'] = -3;
    container['c'] = -1;
    container['b'] = -2;
    container['d'] = 1;
    EXPECT_EQ(container['a'], -3);
    EXPECT_EQ(container['b'], -2);
    EXPECT_EQ(container['c'], -1);
    EXPECT_EQ(container['d'], 1);
    container.clear();
  }
  {
    typedef ozz::MultiMap<char, int> Container;
    Container::Std container;
    container.insert(std::pair<char,int>('a', -3));
    container.insert(std::pair<char,int>('c', -1));
    container.insert(std::pair<char,int>('b', -2));
    container.insert(std::pair<char,int>('d', 1));
    container.insert(std::pair<char,int>('d', 2));
    EXPECT_EQ(container.find('a')->second, -3);
    EXPECT_EQ(container.find('b')->second, -2);
    EXPECT_EQ(container.find('c')->second, -1);
    EXPECT_TRUE(container.find('d')->second == 1 ||
                container.find('d')->second == 2);
    container.clear();
  }
}

TEST(String, Containers) {
  typedef ozz::String String;
  String str;
  EXPECT_EQ(str.size(), 0u);
  str += "a string";
  EXPECT_STREQ(str.c_str(), "a string");
  str.clear();
  EXPECT_EQ(str.size(), 0u);
}
