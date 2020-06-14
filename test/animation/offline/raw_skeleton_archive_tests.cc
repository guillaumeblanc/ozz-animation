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
                        i_skeleton.roots[0].transform.translation, 0.f));
    EXPECT_TRUE(Compare(o_skeleton.roots[0].transform.rotation,
                        i_skeleton.roots[0].transform.rotation, 0.f));
    EXPECT_TRUE(Compare(o_skeleton.roots[0].transform.scale,
                        i_skeleton.roots[0].transform.scale, 0.f));

    EXPECT_TRUE(Compare(o_skeleton.roots[0].children[0].transform.translation,
                        i_skeleton.roots[0].children[0].transform.translation,
                        0.f));
    EXPECT_TRUE(Compare(o_skeleton.roots[0].children[0].transform.rotation,
                        i_skeleton.roots[0].children[0].transform.rotation,
                        0.f));
    EXPECT_TRUE(Compare(o_skeleton.roots[0].children[0].transform.scale,
                        i_skeleton.roots[0].children[0].transform.scale, 0.f));

    EXPECT_TRUE(Compare(
        o_skeleton.roots[0].children[0].children[0].transform.translation,
        i_skeleton.roots[0].children[0].children[0].transform.translation,
        0.f));
    EXPECT_TRUE(Compare(
        o_skeleton.roots[0].children[0].children[0].transform.rotation,
        i_skeleton.roots[0].children[0].children[0].transform.rotation, 0.f));
    EXPECT_TRUE(Compare(
        o_skeleton.roots[0].children[0].children[0].transform.scale,
        i_skeleton.roots[0].children[0].children[0].transform.scale, 0.f));

    EXPECT_TRUE(Compare(o_skeleton.roots[0].children[1].transform.translation,
                        i_skeleton.roots[0].children[1].transform.translation,
                        0.f));
    EXPECT_TRUE(Compare(o_skeleton.roots[0].children[1].transform.rotation,
                        i_skeleton.roots[0].children[1].transform.rotation,
                        0.f));
    EXPECT_TRUE(Compare(o_skeleton.roots[0].children[1].transform.scale,
                        i_skeleton.roots[0].children[1].transform.scale, 0.f));
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
