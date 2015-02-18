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

#include "ozz/options/options.h"

#include "ozz/base/platform.h"

#include "gtest/gtest.h"
#include "ozz/base/gtest_helper.h"

TEST(EmptyRegistration, Options) {
  { // No argument is specified.
    const char* argv[] = {testing::internal::GetArgvs()[0].c_str()};
    const int argc = OZZ_ARRAY_SIZE(argv);

    EXPECT_EQ(ozz::options::ParseCommandLine(
      argc, argv, "1.0", "AutoRegistration test"), ozz::options::kSuccess);
  }

  { // An invalid argument is specified.
    const char* argv[] = {
      testing::internal::GetArgvs()[0].c_str(), "--something"};
    const int argc = OZZ_ARRAY_SIZE(argv);

    EXPECT_EQ_LOG(
      ozz::options::ParseCommandLine(
        argc, argv, "1.0", "AutoRegistration test"), ozz::options::kExitFailure,
      std::cout, "Usage");
  }
}

TEST(BuiltInEmptyRegistration, Options) {
  { // Built-in version.
    const char* argv[] = {
      testing::internal::GetArgvs()[0].c_str(), "--version"};
    const int argc = OZZ_ARRAY_SIZE(argv);

    EXPECT_EQ_LOG(
      ozz::options::ParseCommandLine(
        argc, argv, "1.046", "AutoRegistration test"),
      ozz::options::kExitSuccess, std::cout, "1.046");
  }
  { // Built-in help.
    const char* argv[] = {testing::internal::GetArgvs()[0].c_str(), "--help"};
    const int argc = OZZ_ARRAY_SIZE(argv);

    EXPECT_EQ_LOG(
      ozz::options::ParseCommandLine(
        argc, argv, "1.046", "AutoRegistration test"),
      ozz::options::kExitSuccess, std::cout, "--version");
  }
}

TEST(BuiltInArgv0, Options) {
  const char* argv[] = {"c:/a path/test.exe"};
  const int argc = OZZ_ARRAY_SIZE(argv);

  EXPECT_EQ(ozz::options::ParseCommandLine(
    argc, argv, "1.046", "AutoRegistration test"), ozz::options::kSuccess);

  EXPECT_STREQ(ozz::options::ParsedExecutablePath().c_str(), "c:/a path/");
  EXPECT_STREQ(ozz::options::ParsedExecutableName(), "test.exe");
}
