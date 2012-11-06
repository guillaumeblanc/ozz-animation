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

#include "ozz/animation/utils.h"

#include "ozz/base/maths/soa_transform.h"
#include "ozz/base/memory/allocator.h"

namespace ozz {
namespace animation {

LocalsAlloc AllocateLocals(memory::Allocator* _allocator, std::size_t _num_joints) {
  assert(_allocator);
  const std::size_t num_soa_joints = (_num_joints + 3) / 4;
  LocalsAlloc locals = {
    _allocator->Allocate<ozz::math::SoaTransform>(num_soa_joints), NULL};
  if (locals.begin) {
    locals.end = locals.begin + num_soa_joints;
  }
  return locals;
}

ModelsAlloc AllocateModels(memory::Allocator* _allocator, std::size_t _num_joints) {
  assert(_allocator);
  ModelsAlloc models = {
    _allocator->Allocate<ozz::math::Float4x4>(_num_joints), NULL};
  if (models.begin) {
    models.end = models.begin + _num_joints;
  }
  return models;
}
}  // animation
}  // ozz
