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

#include "ozz/animation/runtime/skeleton.h"

#include "gtest/gtest.h"

#include "ozz/animation/offline/raw_skeleton.h"
#include "ozz/animation/offline/skeleton_builder.h"

#include "ozz/base/io/archive.h"
#include "ozz/base/io/stream.h"

#include "ozz/base/maths/soa_transform.h"

#include "ozz/base/memory/unique_ptr.h"

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
  ozz::unique_ptr<Skeleton> o_skeleton;
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
    ASSERT_TRUE(o_skeleton);
  }

  for (int e = 0; e < 2; ++e) {
    ozz::Endianness endianess = e == 0 ? ozz::kBigEndian : ozz::kLittleEndian;
    ozz::io::MemoryStream stream;

    // Streams out.
    ozz::io::OArchive o(&stream, endianess);
    o << *o_skeleton;

    // Streams in.
    stream.Seek(0, ozz::io::Stream::kSet);
    ozz::io::IArchive ia(&stream);

    Skeleton i_skeleton;
    ia >> i_skeleton;

    // Compares skeletons.
    EXPECT_EQ(o_skeleton->num_joints(), i_skeleton.num_joints());
    for (int i = 0; i < i_skeleton.num_joints(); ++i) {
      EXPECT_EQ(i_skeleton.joint_parents()[i],
                o_skeleton->joint_parents()[i]);
      EXPECT_STREQ(i_skeleton.joint_names()[i], o_skeleton->joint_names()[i]);
    }
    for (int i = 0; i < (i_skeleton.num_joints() + 3) / 4; ++i) {
      EXPECT_TRUE(ozz::math::AreAllTrue(
          i_skeleton.joint_bind_poses()[i].translation ==
          o_skeleton->joint_bind_poses()[i].translation));
      EXPECT_TRUE(ozz::math::AreAllTrue(
          i_skeleton.joint_bind_poses()[i].rotation ==
          o_skeleton->joint_bind_poses()[i].rotation));
      EXPECT_TRUE(
          ozz::math::AreAllTrue(i_skeleton.joint_bind_poses()[i].scale ==
                                o_skeleton->joint_bind_poses()[i].scale));
    }
  }
}

TEST(AlreadyInitialized, SkeletonSerialize) {
  ozz::unique_ptr<Skeleton> o_skeleton[2];
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
    raw_skeleton.roots[0].name = "root0";

    SkeletonBuilder builder;
    o_skeleton[0] = builder(raw_skeleton);
    ASSERT_TRUE(o_skeleton[0]);

    raw_skeleton.roots.resize(2);
    raw_skeleton.roots[1].name = "root1";
    o_skeleton[1] = builder(raw_skeleton);
    ASSERT_TRUE(o_skeleton[1]);
  }

  {
    // Streams out.
    ozz::io::MemoryStream stream;
    ozz::io::OArchive o(&stream);
    o << *o_skeleton[0];
    o << *o_skeleton[1];

    // Streams in.
    stream.Seek(0, ozz::io::Stream::kSet);
    ozz::io::IArchive i(&stream);

    // Reads and check the first skeleton.
    Skeleton i_skeleton;
    i >> i_skeleton;
    ASSERT_EQ(o_skeleton[0]->num_joints(), 1);
    EXPECT_STREQ(i_skeleton.joint_names()[0], o_skeleton[0]->joint_names()[0]);

    // Reuse the skeleton a second time.
    i >> i_skeleton;
    ASSERT_EQ(o_skeleton[1]->num_joints(), 2);
    EXPECT_STREQ(i_skeleton.joint_names()[0], o_skeleton[1]->joint_names()[0]);
    EXPECT_STREQ(i_skeleton.joint_names()[1], o_skeleton[1]->joint_names()[1]);
  }
}
