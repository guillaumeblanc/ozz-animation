//============================================================================//
//                                                                            //
// ozz-animation, 3d skeletal animation libraries and tools.                  //
// https://code.google.com/p/ozz-animation/                                   //
//                                                                            //
//----------------------------------------------------------------------------//
//                                                                            //
// Copyright (c) 2012-2015 Guillaume Blanc                                    //
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

#include "ozz/animation/offline/raw_skeleton.h"

#include "gtest/gtest.h"

#include "ozz/base/io/archive.h"
#include "ozz/base/io/stream.h"

using ozz::animation::offline::RawSkeleton;

TEST(Empty, RawSkeletonSerialize) {
  ozz::io::MemoryStream stream;

  // Streams out.
  ozz::io::OArchive o(&stream, ozz::GetNativeEndianness());

  RawSkeleton o_skeleton;
  o << o_skeleton;

  // Streams in.
  stream.Seek(0, ozz::io::Stream::kSet);
  ozz::io::IArchive i(&stream);

  RawSkeleton i_skeleton;
  i >> i_skeleton;

  EXPECT_EQ(o_skeleton.num_joints(), i_skeleton.num_joints());
}

TEST(Filled, RawSkeletonSerialize) {
  /* Builds output skeleton.
   4 joints

     *
     |
    root
    / \
   j0 j1
   |
   j2
  */

  RawSkeleton o_skeleton;
  o_skeleton.roots.resize(1);
  RawSkeleton::Joint& root = o_skeleton.roots[0];
  root.name = "root";
  root.transform = ozz::math::Transform::identity();
  root.children.resize(2);
  root.children[0].name = "j0";
  root.children[0].transform = ozz::math::Transform::identity();
  root.children[0].transform.translation.x = 46.f;
  root.children[1].name = "j1";
  root.children[1].transform = ozz::math::Transform::identity();
  root.children[1].transform.scale.y = 99.f;
  root.children[0].children.resize(1);
  root.children[0].children[0].name = "j2";
  root.children[0].children[0].transform = ozz::math::Transform::identity();
  root.children[0].children[0].transform.rotation =
    ozz::math::Quaternion(0.f, 0.f, 1.f, 0.f);

  EXPECT_TRUE(o_skeleton.Validate());
  EXPECT_EQ(o_skeleton.num_joints(), 4);

  for (int e = 0; e < 2; ++e) {
    ozz::Endianness endianess = e == 0 ? ozz::kBigEndian : ozz::kLittleEndian;
    ozz::io::MemoryStream stream;

    // Streams out.
    ozz::io::OArchive o(&stream, endianess);
    o << o_skeleton;

    // Streams in.
    stream.Seek(0, ozz::io::Stream::kSet);
    ozz::io::IArchive i(&stream);

    RawSkeleton i_skeleton;
    i >> i_skeleton;

    EXPECT_TRUE(i_skeleton.Validate());
    ASSERT_EQ(o_skeleton.num_joints(), i_skeleton.num_joints());

    // Compares skeletons joint's name.
    EXPECT_STREQ(o_skeleton.roots[0].name.c_str(),
                 i_skeleton.roots[0].name.c_str());
    EXPECT_STREQ(o_skeleton.roots[0].children[0].name.c_str(),
                 i_skeleton.roots[0].children[0].name.c_str());
    EXPECT_STREQ(o_skeleton.roots[0].children[0].children[0].name.c_str(),
                 i_skeleton.roots[0].children[0].children[0].name.c_str());
    EXPECT_STREQ(o_skeleton.roots[0].children[1].name.c_str(),
                 i_skeleton.roots[0].children[1].name.c_str());

    // Compares skeletons joint's transform.
    EXPECT_TRUE(Compare(o_skeleton.roots[0].transform.translation,
                        i_skeleton.roots[0].transform.translation,
                        0.f));
    EXPECT_TRUE(Compare(o_skeleton.roots[0].transform.rotation,
                        i_skeleton.roots[0].transform.rotation,
                        0.f));
    EXPECT_TRUE(Compare(o_skeleton.roots[0].transform.scale,
                        i_skeleton.roots[0].transform.scale,
                        0.f));

    EXPECT_TRUE(Compare(o_skeleton.roots[0].children[0].transform.translation,
                        i_skeleton.roots[0].children[0].transform.translation,
                        0.f));
    EXPECT_TRUE(Compare(o_skeleton.roots[0].children[0].transform.rotation,
                        i_skeleton.roots[0].children[0].transform.rotation, 0.f));
    EXPECT_TRUE(Compare(o_skeleton.roots[0].children[0].transform.scale,
                        i_skeleton.roots[0].children[0].transform.scale,
                        0.f));

    EXPECT_TRUE(Compare(o_skeleton.roots[0].children[0].children[0].transform.translation,
                        i_skeleton.roots[0].children[0].children[0].transform.translation,
                        0.f));
    EXPECT_TRUE(Compare(o_skeleton.roots[0].children[0].children[0].transform.rotation,
                        i_skeleton.roots[0].children[0].children[0].transform.rotation,
                        0.f));
    EXPECT_TRUE(Compare(o_skeleton.roots[0].children[0].children[0].transform.scale,
                        i_skeleton.roots[0].children[0].children[0].transform.scale,
                        0.f));

    EXPECT_TRUE(Compare(o_skeleton.roots[0].children[1].transform.translation,
                        i_skeleton.roots[0].children[1].transform.translation,
                        0.f));
    EXPECT_TRUE(Compare(o_skeleton.roots[0].children[1].transform.rotation,
                        i_skeleton.roots[0].children[1].transform.rotation,
                        0.f));
    EXPECT_TRUE(Compare(o_skeleton.roots[0].children[1].transform.scale,
                        i_skeleton.roots[0].children[1].transform.scale,
                        0.f));
  }
}

TEST(AlreadyInitialized, RawSkeletonSerialize) {

  RawSkeleton o_skeleton;
  o_skeleton.roots.resize(1);

  ozz::io::MemoryStream stream;

  // Streams out.
  ozz::io::OArchive o(&stream);
  o << o_skeleton;

  // Streams out a second time.
  o_skeleton.roots.resize(2);
  o << o_skeleton;

  // Streams in.
  stream.Seek(0, ozz::io::Stream::kSet);
  ozz::io::IArchive i(&stream);

  RawSkeleton i_skeleton;
  i >> i_skeleton;
  ASSERT_EQ(i_skeleton.num_joints(), 1);

  // A second time
  i >> i_skeleton;
  ASSERT_EQ(i_skeleton.num_joints(), 2);
}
