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

#include "ozz/options/options.h"

#include "ozz/base/platform.h"

#include "gtest/gtest.h"
#include "ozz/base/gtest_helper.h"

// Register some options.
OZZ_OPTIONS_DECLARE_BOOL(a_bool, "A bool", false, false);
OZZ_OPTIONS_DECLARE_INT(a_int, "A int", 46, false);
OZZ_OPTIONS_DECLARE_FLOAT(a_float, "A float", 46.f, false);
OZZ_OPTIONS_DECLARE_STRING(a_string, "A string", "Forty six", false);

TEST(AutoRegistration, Options) {
  { // No argument is specified.
    const char* argv[] = {"c:/a path/test.exe"};
    const int argc = OZZ_ARRAY_SIZE(argv);

    EXPECT_EQ(ozz::options::ParseCommandLine(
      argc, argv, "1.0", "AutoRegistration test"), ozz::options::kSuccess);

    EXPECT_FALSE(OPTIONS_a_bool);
    EXPECT_EQ(OPTIONS_a_int, 46);
    EXPECT_FLOAT_EQ(OPTIONS_a_float, 46.f);
    EXPECT_STREQ(OPTIONS_a_string, "Forty six");
  }

  { // An argument is specified.
    const char* argv[] = {"c:/a path/test.exe", "--a_bool"};
    const int argc = OZZ_ARRAY_SIZE(argv);

    EXPECT_EQ(ozz::options::ParseCommandLine(
      argc, argv, "1.0", "AutoRegistration test"), ozz::options::kSuccess);

    EXPECT_TRUE(OPTIONS_a_bool);
    OPTIONS_a_bool.RestoreDefault();

    EXPECT_EQ(OPTIONS_a_int, 46);
    EXPECT_FLOAT_EQ(OPTIONS_a_float, 46.f);
    EXPECT_STREQ(OPTIONS_a_string, "Forty six");
  }

  { // An invalid argument is specified.
    const char* argv[] = {"c:/a path/test.exe", "--a_boolean"};
    const int argc = OZZ_ARRAY_SIZE(argv);

    EXPECT_EQ_LOG(
      ozz::options::ParseCommandLine(
        argc, argv, "1.0", "AutoRegistration test"),
      ozz::options::kExitFailure, std::cout, "--a_bool=<bool>");

    EXPECT_FALSE(OPTIONS_a_bool);
    EXPECT_EQ(OPTIONS_a_int, 46);
    EXPECT_FLOAT_EQ(OPTIONS_a_float, 46.f);
    EXPECT_STREQ(OPTIONS_a_string, "Forty six");
  }

  { // An invalid argument value is specified.
    const char* argv[] = {"c:/a path/test.exe", "--a_bool=46"};
    const int argc = OZZ_ARRAY_SIZE(argv);

    EXPECT_EQ_LOG(
      ozz::options::ParseCommandLine(
        argc, argv, "1.0", "AutoRegistration test"),
      ozz::options::kExitFailure, std::cout, "--a_bool=<bool>");

    EXPECT_FALSE(OPTIONS_a_bool);
    EXPECT_EQ(OPTIONS_a_int, 46);
    EXPECT_FLOAT_EQ(OPTIONS_a_float, 46.f);
    EXPECT_STREQ(OPTIONS_a_string, "Forty six");
  }
}

TEST(BuiltInRegistration, Options) {
  { // Built-in version.
    const char* argv[] = {"c:/a path/test.exe", "--version"};
    const int argc = OZZ_ARRAY_SIZE(argv);

    EXPECT_EQ_LOG(
      ozz::options::ParseCommandLine(
        argc, argv, "1.046", "AutoRegistration test"),
      ozz::options::kExitSuccess, std::cout, "1.046");
  }
  { // Built-in help.
    const char* argv[] = {"c:/a path/test.exe", "--help"};
    const int argc = OZZ_ARRAY_SIZE(argv);

    EXPECT_EQ_LOG(
      ozz::options::ParseCommandLine(
        argc, argv, "1.046", "AutoRegistration test"),
      ozz::options::kExitSuccess, std::cout, "AutoRegistration test");
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
