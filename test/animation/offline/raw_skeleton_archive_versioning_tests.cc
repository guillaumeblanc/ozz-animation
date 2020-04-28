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

#include "ozz/base/log.h"
#include "ozz/options/options.h"

OZZ_OPTIONS_DECLARE_STRING(file, "Specifies input file", "", true)
OZZ_OPTIONS_DECLARE_INT(joints, "Number of joints", 0, true)
OZZ_OPTIONS_DECLARE_STRING(root_name, "Name of the root joint", "", true)

int main(int _argc, char** _argv) {
  // Parses arguments.
  testing::InitGoogleTest(&_argc, _argv);
  ozz::options::ParseResult parse_result = ozz::options::ParseCommandLine(
      _argc, _argv, "1.0",
      "Test RawSkeleton archive versioning retrocompatibility");
  if (parse_result != ozz::options::kSuccess) {
    return parse_result == ozz::options::kExitSuccess ? EXIT_SUCCESS
                                                      : EXIT_FAILURE;
  }

  return RUN_ALL_TESTS();
}

TEST(Versioning, RawSkeletonSerialize) {
  // Open the file.
  const char* filename = OPTIONS_file;
  ozz::io::File file(filename, "rb");
  ASSERT_TRUE(file.opened());

  // Open archive and test object tag.
  ozz::io::IArchive archive(&file);
  ASSERT_TRUE(archive.TestTag<ozz::animation::offline::RawSkeleton>());

  // Read the object.
  ozz::animation::offline::RawSkeleton skeleton;
  archive >> skeleton;

  // More testing
  EXPECT_EQ(skeleton.num_joints(), OPTIONS_joints);
  if (skeleton.num_joints()) {
    EXPECT_STREQ(skeleton.roots[0].name.c_str(), OPTIONS_root_name);
  }
}
