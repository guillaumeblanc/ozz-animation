//----------------------------------------------------------------------------//
//                                                                            //
// ozz-animation is hosted at http://github.com/guillaumeblanc/ozz-animation  //
// and distributed under the MIT License (MIT).                               //
//                                                                            //
// Copyright (c) 2015 Guillaume Blanc                                         //
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

#include "ozz/animation/runtime/skeleton.h"

#include <cstring>

#include "ozz/base/io/archive.h"
#include "ozz/base/maths/soa_math_archive.h"
#include "ozz/base/maths/soa_transform.h"
#include "ozz/base/memory/allocator.h"

namespace ozz {
namespace io {
// JointProperties' version can be declared locally as it will be saved from this
// cpp file only.
OZZ_IO_TYPE_VERSION(2, animation::Skeleton::JointProperties)

// Specializes Skeleton::JointProperties. This structure's bitset isn't written
// as-is because of endianness issues.
template <>
void Save(OArchive& _archive,
          const animation::Skeleton::JointProperties* _properties,
          size_t _count) {
  for (size_t i = 0; i < _count; ++i) {
    uint16_t parent = _properties[i].parent;
    _archive << parent;
    bool is_leaf = _properties[i].is_leaf != 0;
    _archive << is_leaf;
  }
}

template <>
void Load(IArchive& _archive,
          animation::Skeleton::JointProperties* _properties,
          size_t _count,
          uint32_t _version) {
  (void)_version;
  for (size_t i = 0; i < _count; ++i) {
    uint16_t parent;
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
  Deallocate();
}
/*
void Skeleton::Allocate(size_t _char_count, size_t _num_joints) {
}*/

void Skeleton::Deallocate() {
  memory::Allocator* allocator = memory::default_allocator();
  allocator->Deallocate(joint_properties_);
  allocator->Deallocate(bind_pose_);
  allocator->Deallocate(joint_names_);
}

// This function is not inlined in order to avoid the inclusion of SoaTransform.
Range<const math::SoaTransform> Skeleton::bind_pose() const {
  return Range<const math::SoaTransform>(bind_pose_,
                                         bind_pose_ + ((num_joints_ + 3) / 4));
}

void Skeleton::Save(ozz::io::OArchive& _archive) const {

  // Early out if skeleton's empty.
  _archive << static_cast<int32_t>(num_joints_);
  if (!num_joints_) {
    return;
  }

  // Stores names. They are all concatenated in the same buffer, starting at
  // joint_names_[0].
  size_t chars_count = 0;
  for (int i = 0; i < num_joints_; ++i) {
    chars_count += (std::strlen(joint_names_[i]) + 1) * sizeof(char);
  }
  _archive << static_cast<int32_t>(chars_count);
  _archive << ozz::io::MakeArray(joint_names_[0], chars_count);

  // Stores joint's properties.
  _archive << ozz::io::MakeArray(joint_properties_, num_joints_);

  // Stores bind poses.
  _archive << ozz::io::MakeArray(bind_pose_, num_soa_joints());
}

void Skeleton::Load(ozz::io::IArchive& _archive, uint32_t _version) {
  (void)_version;

  // Deallocate skeleton in case it was already used before.
  Deallocate();
  num_joints_ = 0;

  if (_version != 2) {
    return;
  }

  int32_t num_joints;
  _archive >> num_joints;
  num_joints_ = num_joints;

  // Early out if skeleton's empty.
  if (!num_joints) {
    return;
  }

  // Read names.
  int32_t chars_count;
  _archive >> chars_count;

  // Allocates and reads name's buffer. Names are stored at the end off the
  // array of pointers.
  const size_t buffer_size = num_joints * sizeof(char*) + chars_count;
  joint_names_ = memory::default_allocator()->Allocate<char*>(buffer_size);
  char* cursor = reinterpret_cast<char*>(joint_names_ + num_joints);
  _archive >> ozz::io::MakeArray(cursor, chars_count);

  // Fixes up array of pointers. Stops at num_joints - 1, so that it doesn't
  // read memory past the end of the buffer.
  for (int i = 0; i < num_joints - 1; ++i) {
    joint_names_[i] = cursor;
    cursor += std::strlen(joint_names_[i]) + 1;
  }
  // num_joints is > 0, as this was tested at the beginning of the function.
  joint_names_[num_joints - 1] = cursor;

  // Reads joint's properties.
  joint_properties_ = 
    memory::default_allocator()->Allocate<Skeleton::JointProperties>(num_joints);
  _archive >> ozz::io::MakeArray(joint_properties_, num_joints);

  // Reads bind pose.
  bind_pose_ =
    memory::default_allocator()->Allocate<math::SoaTransform>(num_soa_joints());
  _archive >> ozz::io::MakeArray(bind_pose_, num_soa_joints());
}
}  // animation
}  // ozz
