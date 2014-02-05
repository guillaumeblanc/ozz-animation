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

#ifndef OZZ_OZZ_BASE_CONTAINERS_DEQUE_H_
#define OZZ_OZZ_BASE_CONTAINERS_DEQUE_H_

#include <deque>

#include "ozz/base/containers/std_allocator.h"

namespace ozz {
// Redirects std::deque to ozz::Deque in order to replace std default allocator by
// ozz::StdAllocator.
template <class _Ty, class _Allocator = ozz::StdAllocator<_Ty> >
struct Deque {
  typedef std::deque<_Ty, _Allocator> Std;
};
}  // ozz
#endif  // OZZ_OZZ_BASE_CONTAINERS_DEQUE_H_
