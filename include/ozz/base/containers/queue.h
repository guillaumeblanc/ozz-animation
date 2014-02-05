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

#ifndef OZZ_OZZ_BASE_CONTAINERS_QUEUE_H_
#define OZZ_OZZ_BASE_CONTAINERS_QUEUE_H_

#include <queue>

#include "deque.h"

namespace ozz {
// Redirects std::queue to ozz::Queue in order to replace std default allocator
// by ozz::StdAllocator.
template <class _Ty, class _Container = typename ozz::Deque<_Ty>::Std>
struct Queue {
  typedef std::queue<_Ty, _Container> Std;
};

// Redirects std::priority_queue to ozz::PriorityQueue in order to replace std
// default allocator by ozz::StdAllocator.
template <class _Ty,
          class _Container = typename ozz::Deque<_Ty>::Std,
          class _Pred = std::less<typename _Container::value_type> >
struct PriorityQueue {
  typedef std::priority_queue<_Ty, _Container, _Pred> Std;
};
}  // ozz
#endif  // OZZ_OZZ_BASE_CONTAINERS_QUEUE_H_
