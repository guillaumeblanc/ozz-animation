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

#ifndef OZZ_OZZ_ANIMATION_OFFLINE_RAW_ANIMATION_ARCHIVE_H_
#define OZZ_OZZ_ANIMATION_OFFLINE_RAW_ANIMATION_ARCHIVE_H_

#include "ozz/base/platform.h"
#include "ozz/base/io/archive_traits.h"

#include "ozz/animation/offline/raw_animation.h"

namespace ozz {
namespace io {
OZZ_IO_TYPE_VERSION(1, animation::offline::RawAnimation)
OZZ_IO_TYPE_TAG("ozz-raw_animation", animation::offline::RawAnimation)

// Should not be called directly but through io::Archive << and >> operators.
template <>
void Save(OArchive& _archive,
          const animation::offline::RawAnimation* _animations,
          size_t _count);

template <>
void Load(IArchive& _archive,
          animation::offline::RawAnimation* _animations,
          size_t _count,
          uint32_t _version);
}  // io
}  // ozz
#endif  // OZZ_OZZ_ANIMATION_OFFLINE_RAW_ANIMATION_ARCHIVE_H_
