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

#include "ozz/animation/runtime/skeleton.h"

#include <cstring>

#include "ozz/base/io/archive.h"
#include "ozz/base/io/archive_soa_maths.h"
#include "ozz/base/maths/soa_transform.h"
#include "ozz/base/memory/allocator.h"

namespace ozz {
namespace io {
// JointProperties' version can be declared localy as it will be saved from this
// cpp file only.
OZZ_IO_TYPE_VERSION(1, animation::Skeleton::JointProperties)

// Specializes Skeleton::JointProperties. This structure's bitset isn't written
// as-is because of endianness issues.
template <>
void Save(OArchive& _archive,
          const animation::Skeleton::JointProperties* _properties,
          std::size_t _count) {
  for (std::size_t i = 0; i < _count; ++i) {
    uint16 parent = _properties[i].parent;
    _archive << parent;
    bool is_leaf = _properties[i].is_leaf != 0;
    _archive << is_leaf;
  }
}

template <>
void Load(IArchive& _archive,
          animation::Skeleton::JointProperties* _properties,
          std::size_t _count,
          uint32 /*_version*/) {
  for (std::size_t i = 0; i < _count; ++i) {
    uint16 parent;
    _archive >> parent;
    _properties[i].parent = parent;
    bool is_leaf;
    _archive >> is_leaf;
    _properties[i].is_leaf = is_leaf;
  }
}
}  // io

namespace animation {

Skeleton::Skeleton()
    : joint_properties_(NULL),
      bind_pose_(NULL),
      joint_names_(NULL),
      num_joints_(0) {
}

Skeleton::~Skeleton() {
  memory::default_allocator()->Deallocate(joint_properties_);
  memory::default_allocator()->Deallocate(bind_pose_);
  memory::default_allocator()->Deallocate(joint_names_);
}

// This function is not inlined in order to avoid the inclusion of SoaTransform.
Range<const math::SoaTransform> Skeleton::bind_pose() const {
  return Range<const math::SoaTransform>(bind_pose_,
                                         bind_pose_ + ((num_joints_ + 3) / 4));
}

void Skeleton::Save(ozz::io::OArchive& _archive) const {
  _archive << static_cast<int32>(num_joints_);

  // Early out if skeleton's empty.
  if (!num_joints_) {
    return;
  }

  // Stores names. They are all concatenated in the same buffer, starting at
  // joint_names_[0].
  std::size_t chars_count = 0;
  for (int i = 0; i < num_joints_; ++i) {
    chars_count += (std::strlen(joint_names_[i]) + 1) * sizeof(char);
  }
  _archive << static_cast<int32>(chars_count);
  _archive << ozz::io::MakeArray(joint_names_[0], chars_count);

  // Stores joint's properties.
  _archive << ozz::io::MakeArray(joint_properties_, num_joints_);

  // Stores bind poses.
  _archive << ozz::io::MakeArray(bind_pose_, num_soa_joints());
}

void Skeleton::Load(ozz::io::IArchive& _archive, ozz::uint32 /*_version*/) {
  assert(!num_joints_ && !joint_names_ && !joint_properties_ && !bind_pose_);

  int32 num_joints;
  _archive >> num_joints;
  num_joints_ = num_joints;

  // Early out if skeleton's empty.
  if (!num_joints_) {
    return;
  }

  // Read names.
  ozz::int32 chars_count;
  _archive >> chars_count;

  // Allocates and reads name's buffer. Names are stored at the end off the
  // array of pointers.
  const std::size_t buffer_size = num_joints_ * sizeof(char*) + chars_count;
  joint_names_ = memory::default_allocator()->Allocate<char*>(buffer_size);
  char* cursor = reinterpret_cast<char*>(joint_names_ + num_joints_);
  _archive >> ozz::io::MakeArray(cursor, chars_count);

  // Fixes up array of pointers.
  for (int i = 0; i < num_joints_; ++i) {
    joint_names_[i] = cursor;
    cursor += std::strlen(joint_names_[i]) + 1;
  }

  // Reads joint's properties.
  joint_properties_ = 
    memory::default_allocator()->Allocate<Skeleton::JointProperties>(num_joints_);
  _archive >> ozz::io::MakeArray(joint_properties_, num_joints_);

  // Reads bind pose.
  bind_pose_ =
    memory::default_allocator()->Allocate<math::SoaTransform>(num_soa_joints());
  _archive >> ozz::io::MakeArray(bind_pose_, num_soa_joints());
}
}  // animation
}  // ozz
