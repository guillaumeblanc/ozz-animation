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
  {  // No argument is specified.
    const char* argv[] = {"c:/a path/test.exe"};
    const int argc = OZZ_ARRAY_SIZE(argv);

    EXPECT_EQ(ozz::options::ParseCommandLine(argc, argv, "1.0",
                                             "AutoRegistration test"),
              ozz::options::kSuccess);

    EXPECT_FALSE(OPTIONS_a_bool);
    EXPECT_EQ(OPTIONS_a_int, 46);
    EXPECT_FLOAT_EQ(OPTIONS_a_float, 46.f);
    EXPECT_STREQ(OPTIONS_a_string, "Forty six");
  }

  {  // An argument is specified.
    const char* argv[] = {"c:/a path/test.exe", "--a_bool"};
    const int argc = OZZ_ARRAY_SIZE(argv);

    EXPECT_EQ(ozz::options::ParseCommandLine(argc, argv, "1.0",
                                             "AutoRegistration test"),
              ozz::options::kSuccess);

    EXPECT_TRUE(OPTIONS_a_bool);
    OPTIONS_a_bool.RestoreDefault();

    EXPECT_EQ(OPTIONS_a_int, 46);
    EXPECT_FLOAT_EQ(OPTIONS_a_float, 46.f);
    EXPECT_STREQ(OPTIONS_a_string, "Forty six");
  }

  {  // An invalid argument is specified.
    const char* argv[] = {"c:/a path/test.exe", "--a_boolean"};
    const int argc = OZZ_ARRAY_SIZE(argv);

    EXPECT_EQ_LOG(ozz::options::ParseCommandLine(argc, argv, "1.0",
                                                 "AutoRegistration test"),
                  ozz::options::kExitFailure, std::cout, "--a_bool=<bool>");

    EXPECT_FALSE(OPTIONS_a_bool);
    EXPECT_EQ(OPTIONS_a_int, 46);
    EXPECT_FLOAT_EQ(OPTIONS_a_float, 46.f);
    EXPECT_STREQ(OPTIONS_a_string, "Forty six");
  }

  {  // An invalid argument value is specified.
    const char* argv[] = {"c:/a path/test.exe", "--a_bool=46"};
    const int argc = OZZ_ARRAY_SIZE(argv);

    EXPECT_EQ_LOG(ozz::options::ParseCommandLine(argc, argv, "1.0",
                                                 "AutoRegistration test"),
                  ozz::options::kExitFailure, std::cout, "--a_bool=<bool>");

    EXPECT_FALSE(OPTIONS_a_bool);
    EXPECT_EQ(OPTIONS_a_int, 46);
    EXPECT_FLOAT_EQ(OPTIONS_a_float, 46.f);
    EXPECT_STREQ(OPTIONS_a_string, "Forty six");
  }
}

TEST(BuiltInRegistration, Options) {
  {  // Built-in version.
    const char* argv[] = {"c:/a path/test.exe", "--version"};
    const int argc = OZZ_ARRAY_SIZE(argv);

    EXPECT_EQ_LOG(ozz::options::ParseCommandLine(argc, argv, "1.046",
                                                 "AutoRegistration test"),
                  ozz::options::kExitSuccess, std::cout, "1.046");
  }
  {  // Built-in help.
    const char* argv[] = {"c:/a path/test.exe", "--help"};
    const int argc = OZZ_ARRAY_SIZE(argv);

    EXPECT_EQ_LOG(ozz::options::ParseCommandLine(argc, argv, "1.046",
                                                 "AutoRegistration test"),
                  ozz::options::kExitSuccess, std::cout,
                  "AutoRegistration test");
  }
}

TEST(BuiltInArgv0, Options) {
  const char* argv[] = {"c:/a path/test.exe"};
  const int argc = OZZ_ARRAY_SIZE(argv);

  EXPECT_EQ(ozz::options::ParseCommandLine(argc, argv, "1.046",
                                           "AutoRegistration test"),
            ozz::options::kSuccess);

  EXPECT_STREQ(ozz::options::ParsedExecutablePath().c_str(), "c:/a path/");
  EXPECT_STREQ(ozz::options::ParsedExecutableName(), "test.exe");
}
