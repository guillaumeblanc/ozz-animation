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
#include "ozz/base/containers/deque.h"
#include "ozz/base/containers/list.h"
#include "ozz/base/containers/map.h"
#include "ozz/base/containers/queue.h"
#include "ozz/base/containers/set.h"
#include "ozz/base/containers/stack.h"
#include "ozz/base/containers/string.h"
#include "ozz/base/containers/unordered_map.h"
#include "ozz/base/containers/unordered_set.h"
#include "ozz/base/containers/vector.h"
#include "ozz/base/gtest_helper.h"
#include "ozz/base/span.h"

TEST(Vector, Containers) {
  typedef ozz::vector<int> Container;
  Container container;
  container.push_back(1);
  container.insert(container.begin(), 0);
  container.push_back(2);
  container.push_back(std::move(3));
  EXPECT_EQ(container[0], 0);
  EXPECT_EQ(container[1], 1);
  EXPECT_EQ(container[2], 2);
  EXPECT_EQ(container[3], 3);

  Container container2 = std::move(container);
}

TEST(VectorExtensions, Containers) {
  typedef ozz::vector<int> Container;
  Container container;
  int* null = nullptr;

  // Non-mutable access.
  EXPECT_EQ(array_begin(container), null);
  EXPECT_EQ(array_end(container), null);
  EXPECT_EQ(array_end(container), array_begin(container));
  EXPECT_TRUE(make_span(container).empty());

  container.push_back(1);
  container.push_back(2);
  EXPECT_EQ(*array_begin(container), 1);
  EXPECT_EQ(*(array_begin(container) + 1), 2);
  EXPECT_EQ(array_begin(container) + 2, array_end(container));
  EXPECT_NE(array_end(container), null);
  EXPECT_EQ(*(array_end(container) - 2), 1);
  EXPECT_EQ(array_end(container) - array_begin(container), 2);
  EXPECT_EQ(make_span(container).begin(), array_begin(container));
  EXPECT_EQ(make_span(container).end(), array_end(container));

  const Container const_container(container);
  EXPECT_EQ(*array_begin(const_container), 1);
  EXPECT_EQ(*(array_begin(const_container) + 1), 2);
  EXPECT_EQ(array_begin(const_container) + 2, array_end(const_container));
  EXPECT_NE(array_end(const_container), null);
  EXPECT_EQ(*(array_end(const_container) - 2), 1);
  EXPECT_EQ(array_end(const_container) - array_begin(const_container), 2);
  EXPECT_EQ(make_span(const_container).begin(), array_begin(const_container));
  EXPECT_EQ(make_span(const_container).end(), array_end(const_container));

  // Mutable access.
  *array_begin(container) = 0;
  EXPECT_EQ(*array_begin(container), 0);
  EXPECT_EQ(*(array_begin(container) + 1), 2);
}

TEST(Deque, Containers) {
  typedef ozz::deque<int> Container;
  Container container;
  container.push_back(1);
  container.push_front(0);
  container.push_back(2);
  EXPECT_EQ(container[0], 0);
  EXPECT_EQ(container[1], 1);
  EXPECT_EQ(container[2], 2);
  container.clear();

  Container container2 = std::move(container);
}

TEST(List, Containers) {
  typedef ozz::list<int> Container;
  Container container;
  container.push_back(1);
  container.push_front(0);
  container.push_back(2);
  EXPECT_EQ(container.front(), 0);
  EXPECT_EQ(container.back(), 2);
  container.clear();

  Container container2 = std::move(container);
}

TEST(Stack, Containers) {
  typedef ozz::stack<int> Container;
  Container container;
  container.push(1);
  container.push(2);
  EXPECT_EQ(container.top(), 2);
  container.pop();
  EXPECT_EQ(container.top(), 1);
  container.pop();

  Container container2 = std::move(container);
}

TEST(Queue, Containers) {
  {
    typedef ozz::queue<int> Container;
    Container container;
    container.push(1);
    container.push(2);
    EXPECT_EQ(container.back(), 2);
    EXPECT_EQ(container.front(), 1);
    container.pop();
    EXPECT_EQ(container.back(), 2);
    container.pop();

    Container container2 = std::move(container);
  }
  {
    typedef ozz::priority_queue<int> Container;
    Container container;
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
    typedef ozz::set<int> Container;
    Container container;
    EXPECT_TRUE(container.insert('c').second);
    EXPECT_TRUE(container.insert('a').second);
    EXPECT_TRUE(container.insert('b').second);
    EXPECT_FALSE(container.insert('b').second);
    EXPECT_TRUE(container.find('a') == container.begin());
    EXPECT_TRUE(container.find('c') == --container.end());
    EXPECT_EQ(container.erase('c'), 1u);

    EXPECT_TRUE(container.find('b') == --container.end());
    container.clear();

    Container container2 = std::move(container);
  }
  {
    typedef ozz::multiset<int> Container;
    Container container;
    container.insert('c');
    container.insert('a');
    container.insert('b');
    container.insert('a');
    EXPECT_TRUE(container.find('a') == container.begin() ||
                container.find('a') == ++container.begin());
    EXPECT_TRUE(container.find('c') == --container.end());
    EXPECT_EQ(container.erase('c'), 1u);
    EXPECT_TRUE(container.find('c') == container.end());
    EXPECT_EQ(container.erase('a'), 2u);
    EXPECT_TRUE(container.find('a') == container.end());
    container.clear();
  }
}

TEST(UnorderedSet, Containers) {
  {
    typedef ozz::unordered_set<int> Container;
    Container container;
    EXPECT_TRUE(container.insert('c').second);
    EXPECT_TRUE(container.insert('a').second);
    EXPECT_TRUE(container.insert('b').second);
    EXPECT_FALSE(container.insert('a').second);
    EXPECT_TRUE(container.find('a') != container.end());
    EXPECT_TRUE(container.find('c') != container.end());
    EXPECT_EQ(container.erase('c'), 1u);
    EXPECT_TRUE(container.find('c') == container.end());
    container.clear();
  }
  {
    typedef ozz::unordered_multiset<int> Container;
    Container container;
    container.insert('c');
    container.insert('a');
    container.insert('b');
    container.insert('a');
    EXPECT_TRUE(container.find('a') != container.end());
    EXPECT_TRUE(container.find('c') != container.end());
    EXPECT_EQ(container.erase('c'), 1u);
    EXPECT_TRUE(container.find('c') == container.end());
    EXPECT_EQ(container.erase('a'), 2u);
    EXPECT_TRUE(container.find('a') == container.end());
    container.clear();
  }
}

TEST(Map, Containers) {
  {
    typedef ozz::map<char, int> Container;
    Container container;
    container['a'] = -3;
    container['c'] = -1;
    container['b'] = -2;
    container['d'] = 1;
    EXPECT_EQ(container['a'], -3);
    EXPECT_EQ(container['b'], -2);
    EXPECT_EQ(container['c'], -1);
    EXPECT_EQ(container['d'], 1);
    EXPECT_EQ(container.erase('d'), 1u);
    EXPECT_TRUE(container.find('d') == container.end());
    container.clear();

    Container container2 = std::move(container);
  }
  {
    typedef ozz::multimap<char, int> Container;
    Container container;
    container.insert(std::pair<char, int>('a', -3));
    container.insert(std::pair<char, int>('c', -1));
    container.insert(std::pair<char, int>('b', -2));
    container.insert(std::pair<char, int>('d', 1));
    container.insert(std::pair<char, int>('d', 2));
    EXPECT_EQ(container.find('a')->second, -3);
    EXPECT_EQ(container.find('b')->second, -2);
    EXPECT_EQ(container.find('c')->second, -1);
    EXPECT_TRUE(container.find('d')->second == 1 ||
                container.find('d')->second == 2);
    EXPECT_EQ(container.erase('d'), 2u);
    EXPECT_TRUE(container.find('d') == container.end());
    container.clear();
  }
}

TEST(UnorderedMap, Containers) {
  {
    typedef ozz::unordered_map<char, int> Container;
    Container container;
    container['a'] = -3;
    container['c'] = -1;
    container['b'] = -2;
    container['d'] = 1;
    EXPECT_EQ(container['a'], -3);
    EXPECT_EQ(container['b'], -2);
    EXPECT_EQ(container['c'], -1);
    EXPECT_EQ(container['d'], 1);
    EXPECT_EQ(container.erase('d'), 1u);
    EXPECT_TRUE(container.find('d') == container.end());
    container.clear();
  }
  {
    typedef ozz::unordered_multimap<char, int> Container;
    Container container;
    container.insert(std::pair<char, int>('a', -3));
    container.insert(std::pair<char, int>('c', -1));
    container.insert(std::pair<char, int>('b', -2));
    container.insert(std::pair<char, int>('d', 1));
    container.insert(std::pair<char, int>('d', 2));
    EXPECT_EQ(container.find('a')->second, -3);
    EXPECT_EQ(container.find('b')->second, -2);
    EXPECT_EQ(container.find('c')->second, -1);
    EXPECT_TRUE(container.find('d')->second == 1 ||
                container.find('d')->second == 2);
    EXPECT_EQ(container.erase('d'), 2u);
    EXPECT_TRUE(container.find('d') == container.end());
    container.clear();
  }
}

TEST(string, Containers) {
  typedef ozz::string string;
  string str;
  EXPECT_EQ(str.size(), 0u);
  str += "a string";
  EXPECT_STREQ(str.c_str(), "a string");

  string str2 = std::move(str);
  EXPECT_STREQ(str2.c_str(), "a string");

  str2.clear();
  EXPECT_EQ(str2.size(), 0u);
}
