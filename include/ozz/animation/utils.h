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

#ifndef OZZ_OZZ_ANIMATION_UTILS_H_
#define OZZ_OZZ_ANIMATION_UTILS_H_

// Provides animation layer helper functions.

#include <cstddef>

namespace ozz {

// Forward declarations.
namespace math {
struct SoaTransform;
struct Float4x4;
}
namespace memory { class Allocator; }

namespace animation {

// Defines the structure for local space soa transformations as returned by
// AllocateLocals helper function.
struct LocalsAlloc {
  math::SoaTransform* begin;
  const math::SoaTransform* end;
};
// Allocates an array of local space soa transformations that can store up to
// num_joints transformations. This function internally converts _num_joints
// to the required number of soa joints.
LocalsAlloc AllocateLocals(memory::Allocator* _allocator, std::size_t _num_joints);

// Defines the structure for model space matrices as returned by AllocateModels
// helper function.
struct ModelsAlloc {
  math::Float4x4* begin;
  const math::Float4x4* end;
};
// Allocates an array of model space matrices that can store up to num_joints
// matrices.
ModelsAlloc AllocateModels(memory::Allocator* _allocator, std::size_t _num_joints);
}  // animation
}  // ozz
#endif  // OZZ_OZZ_ANIMATION_UTILS_H_
