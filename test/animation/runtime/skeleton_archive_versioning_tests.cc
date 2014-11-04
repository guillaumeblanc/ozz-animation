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

#include "gtest/gtest.h"

#include "ozz/base/io/archive.h"
#include "ozz/base/io/stream.h"

#include "ozz/options/options.h"
#include "ozz/base/log.h"

OZZ_OPTIONS_DECLARE_STRING(file, "Specifies input file", "", true)
OZZ_OPTIONS_DECLARE_INT(joints, "Number of joints", 0, true)
OZZ_OPTIONS_DECLARE_STRING(root_name, "Name of the root joint", "", true)

int main(int _argc, char** _argv) {
  // Parses arguments.
  testing::InitGoogleTest(&_argc, _argv);
  ozz::options::ParseResult parse_result = ozz::options::ParseCommandLine(
    _argc, _argv,
    "1.0",
    "Test Skeleton archive versioning retrocompatibility");
  if (parse_result != ozz::options::kSuccess) {
    return parse_result == ozz::options::kExitSuccess ?
      EXIT_SUCCESS : EXIT_FAILURE;
  }

  return RUN_ALL_TESTS();
}

TEST(Versioning, SkeletonSerialize) {
  // Open the file.
  const char* filename = OPTIONS_file;
  ozz::io::File file(filename, "rb");
  ASSERT_TRUE(file.opened());

  // Open archive and test object tag.
  ozz::io::IArchive archive(&file);
  ASSERT_TRUE(archive.TestTag<ozz::animation::Skeleton>());

  // Read the object.
  ozz::animation::Skeleton skeleton;
  archive >> skeleton;

  // More testing
  EXPECT_EQ(skeleton.num_joints(), OPTIONS_joints);
  if (skeleton.num_joints()) {
    EXPECT_STREQ(skeleton.joint_names()[0], OPTIONS_root_name);
  }
}
