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

#include "ozz/animation/runtime/skeleton_serialize.h"

#include "gtest/gtest.h"

#include "ozz/base/io/archive.h"
#include "ozz/base/io/stream.h"
#include "ozz/base/maths/soa_transform.h"
#include "ozz/base/memory/allocator.h"
#include "ozz/animation/offline/skeleton_builder.h"

using ozz::animation::Skeleton;
using ozz::animation::offline::RawSkeleton;
using ozz::animation::offline::SkeletonBuilder;

TEST(Empty, SkeletonSerialize) {
  ozz::io::MemoryStream stream;

  // Streams out.
  ozz::io::OArchive o(&stream, ozz::GetNativeEndianness());

  Skeleton o_skeleton;
  o << o_skeleton;

  // Streams in.
  stream.Seek(0, ozz::io::Stream::kSet);
  ozz::io::IArchive i(&stream);

  Skeleton i_skeleton;
  i >> i_skeleton;

  EXPECT_EQ(o_skeleton.num_joints(), i_skeleton.num_joints());
}

TEST(Filled, SkeletonSerialize) {
  Skeleton* o_skeleton = NULL;
  /* Builds output skeleton.
   3 joints

     *
     |
    root
    / \
   j0 j1
  */
  {
    RawSkeleton raw_skeleton;
    raw_skeleton.roots.resize(1);
    RawSkeleton::Joint& root = raw_skeleton.roots[0];
    root.name = "root";

    root.children.resize(2);
    root.children[0].name = "j0";
    root.children[1].name = "j1";

    EXPECT_TRUE(raw_skeleton.Validate());
    EXPECT_EQ(raw_skeleton.num_joints(), 3);

    SkeletonBuilder builder;
    o_skeleton = builder(raw_skeleton);
    ASSERT_TRUE(o_skeleton != NULL);
  }

  for (int e = 0; e < 2; e++) {
    ozz::Endianness endianess = e == 0 ? ozz::kBigEndian : ozz::kLittleEndian;
    ozz::io::MemoryStream stream;

    // Streams out.
    ozz::io::OArchive o(&stream, endianess);
    o << *o_skeleton;

    // Streams in.
    stream.Seek(0, ozz::io::Stream::kSet);
    ozz::io::IArchive i(&stream);

    Skeleton i_skeleton;
    i >> i_skeleton;

    // Compares skeletons.
    EXPECT_EQ(o_skeleton->num_joints(), i_skeleton.num_joints());
    for (int i = 0; i < i_skeleton.num_joints(); i++) {
      EXPECT_EQ(i_skeleton.joint_properties().begin[i].parent,
                o_skeleton->joint_properties().begin[i].parent);
      EXPECT_EQ(i_skeleton.joint_properties().begin[i].is_leaf,
                o_skeleton->joint_properties().begin[i].is_leaf);
      EXPECT_STREQ(i_skeleton.joint_names()[i],
                   o_skeleton->joint_names()[i]);
    }
    for (int i = 0; i < (i_skeleton.num_joints() + 3) / 4; i++) {
      EXPECT_TRUE(ozz::math::AreAllTrue(
        i_skeleton.bind_pose().begin[i].translation ==
        o_skeleton->bind_pose().begin[i].translation));
      EXPECT_TRUE(ozz::math::AreAllTrue(
        i_skeleton.bind_pose().begin[i].rotation ==
        o_skeleton->bind_pose().begin[i].rotation));
      EXPECT_TRUE(ozz::math::AreAllTrue(
        i_skeleton.bind_pose().begin[i].scale ==
        o_skeleton->bind_pose().begin[i].scale));
    }
  }
  ozz::memory::default_allocator()->Delete(o_skeleton);
}
