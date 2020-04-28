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

#include "ozz/base/containers/string.h"
#include "ozz/base/containers/vector.h"

#include "gtest/gtest.h"
#include "ozz/base/gtest_helper.h"

TEST(Registration, Options) {
  {  // Parser construction/destruction.
    ozz::options::Parser parser;
  }

  {  // Automatic un-registration
    ozz::options::BoolOption b1("b1", "1st option", false, false);
    ozz::options::Parser parser;
    ozz::options::BoolOption b2("b2", "2nd option", false, false);
    EXPECT_TRUE(parser.RegisterOption(&b1));
    EXPECT_TRUE(parser.RegisterOption(&b2));
  }

  {  // Registration/un-registration.
    ozz::options::Parser parser;

    ozz::options::BoolOption b1("b1", "1st option", false, false);
    EXPECT_TRUE(parser.RegisterOption(&b1));

    ozz::options::BoolOption b2("b2", nullptr, false, false);
    EXPECT_TRUE(parser.RegisterOption(&b2));

    EXPECT_FALSE(parser.UnregisterOption(&b2));
    EXPECT_TRUE(parser.UnregisterOption(&b1));
  }

  {  // Invalid registration.
    ozz::options::Parser parser;
    EXPECT_FALSE(parser.RegisterOption(nullptr));

    ozz::options::BoolOption bnull(nullptr, "A option", false, false);
    EXPECT_FALSE(parser.RegisterOption(&bnull));

    ozz::options::BoolOption bversion("version", nullptr, false, false);
    EXPECT_FALSE(parser.RegisterOption(&bversion));

    ozz::options::BoolOption bhelp("help", nullptr, false, false);
    EXPECT_FALSE(parser.RegisterOption(&bhelp));
  }

  {  // Invalid registration (too much options).
    ozz::options::Parser parser;

    // Needs to pre-allocate vectors to avoid objects to move in memory.
    ozz::vector<ozz::options::BoolOption> options;
    options.reserve(parser.max_options() + 1);
    ozz::vector<ozz::string> names;
    names.reserve(parser.max_options());

    // Registers the maximum allowed options.
    for (int i = 0; i < parser.max_options(); ++i) {
      std::stringstream str;
      str << "option " << i << std::ends;
      names.push_back(str.str().c_str());
      options.push_back(
          ozz::options::BoolOption(names[i].c_str(), nullptr, false, false));
      EXPECT_TRUE(parser.RegisterOption(&options.back()));
    }

    // Registers "too much" options.
    options.push_back(ozz::options::BoolOption("too much", nullptr, false, false));
    EXPECT_FALSE(parser.RegisterOption(&options.back()));

    // Unregisters all options.
    for (int i = parser.max_options(); i > 0; --i) {
      EXPECT_EQ(parser.UnregisterOption(&options[i]), i == 0);
    }
  }

  {  // Invalid registration (registers twice).
    ozz::options::BoolOption b("b", "A option", false, false);
    ozz::options::Parser parser;
    EXPECT_TRUE(parser.RegisterOption(&b));
    EXPECT_FALSE(parser.RegisterOption(&b));
    EXPECT_TRUE(parser.UnregisterOption(&b));
  }

  {  // Invalid un-registration (un-registers twice).
    ozz::options::BoolOption b("b", "A option", false, false);
    ozz::options::Parser parser;
    EXPECT_TRUE(parser.RegisterOption(&b));
    EXPECT_TRUE(parser.UnregisterOption(&b));
    EXPECT_FALSE(parser.UnregisterOption(&b));
  }

  {  // Invalid un-registration (nullptr option).
    ozz::options::Parser parser;
    EXPECT_FALSE(parser.UnregisterOption(nullptr));
  }

  {  // Duplicated option names.
    ozz::options::Parser parser;
    ozz::options::BoolOption b1("boolean", nullptr, false, false);
    EXPECT_TRUE(parser.RegisterOption(&b1));

    ozz::options::BoolOption b2("boolean", nullptr, false, false);
    EXPECT_FALSE(parser.RegisterOption(&b2));

    EXPECT_FALSE(parser.UnregisterOption(&b2));
    EXPECT_TRUE(parser.UnregisterOption(&b1));
    EXPECT_FALSE(parser.UnregisterOption(&b2));
  }
}

TEST(ParseErrors, Options) {
  const char* argv[] = {""};

  ozz::options::Parser parser;
  ozz::options::BoolOption bool_option("boolean", "", true, false);
  EXPECT_TRUE(bool_option);
  EXPECT_TRUE(parser.RegisterOption(&bool_option));

  EXPECT_EQ(parser.Parse(0, nullptr), ozz::options::kExitFailure);
  EXPECT_EQ(parser.Parse(0, argv), ozz::options::kExitFailure);

  // Expects empty path and name by default.
  EXPECT_STREQ(parser.executable_path().c_str(), "");
  EXPECT_STREQ(parser.executable_name(), "");

  EXPECT_TRUE(parser.UnregisterOption(&bool_option));
}

TEST(ParseInvalid, Options) {
  const char* argv[] = {"c:/a path/test.exe", "--bool"};
  const int argc = OZZ_ARRAY_SIZE(argv);

  ozz::options::Parser parser;
  ozz::options::BoolOption bool_option("boolean", "", true, false);
  EXPECT_TRUE(bool_option);
  EXPECT_TRUE(parser.RegisterOption(&bool_option));

  EXPECT_EQ_LOG(parser.Parse(argc, argv), ozz::options::kExitFailure, std::cout,
                "Usage");

  // Expects empty path and name by default
  EXPECT_STREQ(parser.executable_path().c_str(), "c:/a path/");
  EXPECT_STREQ(parser.executable_name(), "test.exe");

  EXPECT_TRUE(parser.UnregisterOption(&bool_option));
}

TEST(OptionDefault, Options) {
  {  // First argument is mandatory
    ozz::options::Parser parser;
    const char* argv[] = {"c:/a path/test.exe"};
    const int argc = OZZ_ARRAY_SIZE(argv);
    EXPECT_EQ(parser.Parse(argc, argv), ozz::options::kSuccess);
  }

  {
    ozz::options::BoolOption boption("bt", "A option", true, false);
    EXPECT_TRUE(boption);
    ozz::options::IntOption ioption("int_option", "A option", 46, false);
    EXPECT_EQ(ioption, 46);
    ozz::options::FloatOption foption("float_option", "A option", 46.f, false);
    EXPECT_FLOAT_EQ(foption, 46.f);
    ozz::options::StringOption soption("string_option", "A option", "forty six",
                                       false);
    EXPECT_STREQ(soption, "forty six");

    ozz::options::Parser parser;
    EXPECT_TRUE(parser.RegisterOption(&boption));
    EXPECT_TRUE(parser.RegisterOption(&ioption));
    EXPECT_TRUE(parser.RegisterOption(&foption));
    EXPECT_TRUE(parser.RegisterOption(&soption));

    const char* argv[] = {"c:/a path/test.exe"};  // No valid option is set.
    const int argc = OZZ_ARRAY_SIZE(argv);
    EXPECT_EQ(parser.Parse(argc, argv), ozz::options::kSuccess);

    EXPECT_TRUE(boption);
    EXPECT_FALSE(parser.UnregisterOption(&boption));
    EXPECT_EQ(ioption, 46);
    EXPECT_EQ(ioption, ioption.default_value());
    EXPECT_FALSE(parser.UnregisterOption(&ioption));
    EXPECT_FLOAT_EQ(foption, 46.f);
    EXPECT_FLOAT_EQ(foption, foption.default_value());
    EXPECT_FALSE(parser.UnregisterOption(&foption));
    EXPECT_STREQ(soption, "forty six");
    EXPECT_EQ(soption, soption.default_value());
    EXPECT_TRUE(parser.UnregisterOption(&soption));
  }
}

TEST(Parsing, Options) {
  ozz::options::BoolOption bool_option("bool", "", false, false);
  EXPECT_FALSE(bool_option);
  ozz::options::IntOption int_option("int", "", 27, false);
  EXPECT_EQ(int_option, 27);
  ozz::options::IntOption int_sic_option("sic", "rip", 58, false);
  EXPECT_EQ(int_sic_option, 58);
  ozz::options::FloatOption float_option("float", "", 99.f, false);
  EXPECT_FLOAT_EQ(float_option, 99.f);
  ozz::options::StringOption string_option("string", "", "twenty six", false);
  EXPECT_STREQ(string_option, "twenty six");

  ozz::options::Parser parser;
  EXPECT_TRUE(parser.RegisterOption(&bool_option));
  EXPECT_TRUE(parser.RegisterOption(&int_option));
  EXPECT_TRUE(parser.RegisterOption(&int_sic_option));
  EXPECT_TRUE(parser.RegisterOption(&float_option));
  EXPECT_TRUE(parser.RegisterOption(&string_option));

  const char* argv[] = {"c:/a path/test.exe",
                        "--bool",
                        "",  // empty argument isn't a error
                        "--float=46.00000",
                        "",  // empty argument isn't a error
                        "--string=forty six",
                        "--int=46",
                        "",    // empty argument isn't a error
                        "--",  // "--" hides all further options.
                        "--sic=0"};
  const int argc = OZZ_ARRAY_SIZE(argv);
  EXPECT_EQ(parser.Parse(argc, argv), ozz::options::kSuccess);
  EXPECT_TRUE(bool_option);
  EXPECT_EQ(int_option, 46);
  EXPECT_EQ(int_sic_option, 58);
  EXPECT_FLOAT_EQ(float_option, 46.f);
  EXPECT_STREQ(string_option, "forty six");

  EXPECT_FALSE(parser.UnregisterOption(&bool_option));
  EXPECT_FALSE(parser.UnregisterOption(&float_option));
  EXPECT_FALSE(parser.UnregisterOption(&string_option));
  EXPECT_FALSE(parser.UnregisterOption(&int_sic_option));
  EXPECT_TRUE(parser.UnregisterOption(&int_option));
}

TEST(BuiltPath, Options) {
  ozz::options::Parser parser;

  // Expects empty path and name by default.
  EXPECT_STREQ(parser.executable_path().c_str(), "");
  EXPECT_STREQ(parser.executable_name(), "");

  {  // Empty path.
    const char* argv[] = {""};
    const int argc = OZZ_ARRAY_SIZE(argv);

    EXPECT_EQ(parser.Parse(argc, argv), ozz::options::kSuccess);
    EXPECT_STREQ(parser.executable_path().c_str(), "");
    EXPECT_STREQ(parser.executable_name(), "");
  }

  {  // Executable name only.
    const char* argv[] = {"test.exe"};
    const int argc = OZZ_ARRAY_SIZE(argv);

    EXPECT_EQ(parser.Parse(argc, argv), ozz::options::kSuccess);
    EXPECT_STREQ(parser.executable_path().c_str(), "");
    EXPECT_STREQ(parser.executable_name(), "test.exe");
  }

  {  // Executable path only.
    const char* argv[] = {"path/"};
    const int argc = OZZ_ARRAY_SIZE(argv);

    EXPECT_EQ(parser.Parse(argc, argv), ozz::options::kSuccess);
    EXPECT_STREQ(parser.executable_path().c_str(), "path/");
    EXPECT_STREQ(parser.executable_name(), "");
  }

  {  // Full path.
    const char* argv[] = {"dir1/dir2/test.exe"};
    const int argc = OZZ_ARRAY_SIZE(argv);

    EXPECT_EQ(parser.Parse(argc, argv), ozz::options::kSuccess);
    EXPECT_STREQ(parser.executable_path().c_str(), "dir1/dir2/");
    EXPECT_STREQ(parser.executable_name(), "test.exe");
  }

  {  // Full path strting with /.
    const char* argv[] = {"/dir1/dir2/test.exe"};
    const int argc = OZZ_ARRAY_SIZE(argv);

    EXPECT_EQ(parser.Parse(argc, argv), ozz::options::kSuccess);
    EXPECT_STREQ(parser.executable_path().c_str(), "/dir1/dir2/");
    EXPECT_STREQ(parser.executable_name(), "test.exe");
  }

  {  // Full path with \ separator.
    const char* argv[] = {"dir1\\dir2\\test.exe"};
    const int argc = OZZ_ARRAY_SIZE(argv);

    EXPECT_EQ(parser.Parse(argc, argv), ozz::options::kSuccess);
    EXPECT_STREQ(parser.executable_path().c_str(), "dir1\\dir2\\");
    EXPECT_STREQ(parser.executable_name(), "test.exe");
  }

  {  // Full path with spaces.
    const char* argv[] = {"dir 1\\dir 2\\test.exe"};
    const int argc = OZZ_ARRAY_SIZE(argv);

    EXPECT_EQ(parser.Parse(argc, argv), ozz::options::kSuccess);
    EXPECT_STREQ(parser.executable_path().c_str(), "dir 1\\dir 2\\");
    EXPECT_STREQ(parser.executable_name(), "test.exe");
  }

  {  // Full path with mixed / and \ separator.
    const char* argv[] = {"dir1/dir2\\test.exe"};
    const int argc = OZZ_ARRAY_SIZE(argv);

    EXPECT_EQ(parser.Parse(argc, argv), ozz::options::kSuccess);
    EXPECT_STREQ(parser.executable_path().c_str(), "dir1/dir2\\");
    EXPECT_STREQ(parser.executable_name(), "test.exe");
  }

  {  // Full path with mixed / and \ separator.
    const char* argv[] = {"dir1\\dir2/test.exe"};
    const int argc = OZZ_ARRAY_SIZE(argv);

    EXPECT_EQ(parser.Parse(argc, argv), ozz::options::kSuccess);
    EXPECT_STREQ(parser.executable_path().c_str(), "dir1\\dir2/");
    EXPECT_STREQ(parser.executable_name(), "test.exe");
  }
}

TEST(BuiltInOptions, Options) {
  {  // Default built-in values.
    const char* argv[] = {"c:/a path/test.exe"};
    const int argc = OZZ_ARRAY_SIZE(argv);

    ozz::options::Parser parser;
    EXPECT_EQ(parser.Parse(argc, argv), ozz::options::kSuccess);
  }

  {  // Default built-in command line error.
    const char* argv[] = {"c:/a path/test.exe", "--option", "--version"};
    const int argc = OZZ_ARRAY_SIZE(argv);
    ozz::options::Parser parser;
    EXPECT_EQ(parser.Parse(argc, argv), ozz::options::kExitFailure);
  }

  {  // Other built-in command line error.
    const char* argv[] = {"c:/a path/test.exe", "--version", "--option"};
    const int argc = OZZ_ARRAY_SIZE(argv);
    ozz::options::Parser parser;
    EXPECT_EQ(parser.Parse(argc, argv), ozz::options::kExitFailure);
  }

  {  // Multiple built-in options.
    const char* argv[] = {"c:/a path/test.exe",
                          "--help"
                          "--version"};
    const int argc = OZZ_ARRAY_SIZE(argv);
    ozz::options::Parser parser;
    EXPECT_EQ(parser.Parse(argc, argv), ozz::options::kExitFailure);
  }

  {  // Hidden built-in options are ignored.
    const char* argv[] = {"c:/a path/test.exe", "--", "--version"};
    const int argc = OZZ_ARRAY_SIZE(argv);
    ozz::options::Parser parser;
    EXPECT_EQ(parser.Parse(argc, argv), ozz::options::kSuccess);
  }

  {  // Valid built-in "version" option.
    const char* argv[] = {"c:/a path/test.exe", "--version"};
    const int argc = OZZ_ARRAY_SIZE(argv);
    ozz::options::Parser parser;
    parser.set_version("1.2.3");
    EXPECT_EQ_LOG(parser.Parse(argc, argv), ozz::options::kExitSuccess,
                  std::cout, "version 1.2.3");
  }

  {  // Valid built-in "version" option, with required arguments.
    const char* argv[] = {"c:/a path/test.exe", "--version"};
    const int argc = OZZ_ARRAY_SIZE(argv);
    ozz::options::Parser parser;
    ozz::options::BoolOption bool_option("bool", "", false, true);
    EXPECT_FALSE(bool_option);
    EXPECT_TRUE(parser.RegisterOption(&bool_option));
    parser.set_version("1.2.3");
    EXPECT_EQ_LOG(parser.Parse(argc, argv), ozz::options::kExitSuccess,
                  std::cout, "version 1.2.3");
    EXPECT_TRUE(parser.UnregisterOption(&bool_option));
  }

  {  // Valid built-in "version" option.
    const char* argv[] = {"c:/a path/test.exe", "--version=true"};
    const int argc = OZZ_ARRAY_SIZE(argv);
    ozz::options::Parser parser;
    parser.set_version("1.2.3");
    EXPECT_EQ_LOG(parser.Parse(argc, argv), ozz::options::kExitSuccess,
                  std::cout, "version 1.2.3");
  }

  {  // Valid negative built-in "version" option.
    const char* argv[] = {"c:/a path/test.exe", "--noversion"};
    const int argc = OZZ_ARRAY_SIZE(argv);
    ozz::options::Parser parser;
    parser.set_version("1.2.3");
    EXPECT_EQ(parser.Parse(argc, argv), ozz::options::kSuccess);
  }

  {  // -- End option scanning after the built in argument.
    const char* argv[] = {"c:/a path/test.exe", "--version", "--", "something"};
    const int argc = OZZ_ARRAY_SIZE(argv);
    ozz::options::Parser parser;
    parser.set_version("1.2.3");
    EXPECT_EQ_LOG(parser.Parse(argc, argv), ozz::options::kExitSuccess,
                  std::cout, "version 1.2.3");
  }

  {  // Built-in help version option is exclusive.
    const char* argv[] = {"c:/a path/test.exe", "--version", "--something"};
    const int argc = OZZ_ARRAY_SIZE(argv);
    ozz::options::Parser parser;
    ozz::options::BoolOption something("something", nullptr, false, false);
    EXPECT_TRUE(parser.RegisterOption(&something));
    EXPECT_EQ_LOG(parser.Parse(argc, argv), ozz::options::kExitFailure,
                  std::cout, "exclusive");
  }

  {  // Valid built-in "help" option.
    const char* argv[] = {"c:/a path/test.exe", "--help"};
    const int argc = OZZ_ARRAY_SIZE(argv);
    ozz::options::Parser parser;
    EXPECT_EQ_LOG(parser.Parse(argc, argv), ozz::options::kExitSuccess,
                  std::cout, "Usage");
  }

  {  // Built-in help option is exclusive.
    const char* argv[] = {"c:/a path/test.exe", "--help", "--something"};
    const int argc = OZZ_ARRAY_SIZE(argv);
    ozz::options::Parser parser;
    ozz::options::BoolOption something("something", nullptr, false, false);
    EXPECT_TRUE(parser.RegisterOption(&something));
    EXPECT_EQ_LOG(parser.Parse(argc, argv), ozz::options::kExitFailure,
                  std::cout, "exclusive");
  }
}

TEST(RequiredOption, Options) {
  ozz::options::BoolOption bool_option("bool", "", false, false);
  EXPECT_FALSE(bool_option);
  ozz::options::IntOption int_required_option("int", "", 27, true);
  EXPECT_EQ(int_required_option, 27);

  ozz::options::Parser parser;
  EXPECT_TRUE(parser.RegisterOption(&bool_option));
  EXPECT_TRUE(parser.RegisterOption(&int_required_option));

  {  // Required flag missing.
    const char* argv[] = {"c:/a path/test.exe"};
    const int argc = OZZ_ARRAY_SIZE(argv);
    EXPECT_EQ_LOG(parser.Parse(argc, argv), ozz::options::kExitFailure,
                  std::cout, "Required");
    EXPECT_FALSE(bool_option);
    EXPECT_EQ(int_required_option, 27);
  }
  {  // Required flag specified.
    const char* required_argv[] = {"c:/a path/test.exe", "--int=46"};
    const int required_argc = OZZ_ARRAY_SIZE(required_argv);
    EXPECT_EQ(parser.Parse(required_argc, required_argv),
              ozz::options::kSuccess);
    EXPECT_FALSE(bool_option);
    EXPECT_EQ(int_required_option, 46);
  }
  {  // Validate help output
    EXPECT_LOG(parser.Help(), std::cout, " \\[--int\\]");
    EXPECT_LOG(parser.Help(), std::cout, " --bool");
  }

  EXPECT_FALSE(parser.UnregisterOption(&bool_option));
  EXPECT_TRUE(parser.UnregisterOption(&int_required_option));
}

TEST(DuplicatedOption, Options) {
  ozz::options::BoolOption bool_option("bool", "", false, false);
  EXPECT_FALSE(bool_option);
  ozz::options::IntOption int_option("int", "", 27, false);
  EXPECT_EQ(int_option, 27);

  ozz::options::Parser parser;
  EXPECT_TRUE(parser.RegisterOption(&bool_option));
  EXPECT_TRUE(parser.RegisterOption(&int_option));

  {  // Duplicated flags.
    const char* duplicated_argv[] = {"c:/a path/test.exe", "--int=46",
                                     "--int=47"};
    const int duplicated_argc = OZZ_ARRAY_SIZE(duplicated_argv);
    EXPECT_EQ(parser.Parse(duplicated_argc, duplicated_argv),
              ozz::options::kExitFailure);
  }

  EXPECT_FALSE(parser.UnregisterOption(&bool_option));
  EXPECT_TRUE(parser.UnregisterOption(&int_option));
}

namespace {
static bool TestFn(const ozz::options::Option& _option, int /*_argc*/) {
  const ozz::options::IntOption& option =
      static_cast<const ozz::options::IntOption&>(_option);
  bool valid = option == 46;
  if (!valid) {
    std::cout << "46 only option" << std::endl;
  }
  return valid;
}
}  // namespace

TEST(ValidateFnOption, Options) {
  ozz::options::IntOption int_option("int", "", 58, false, &TestFn);
  EXPECT_EQ(int_option, 58);
  ozz::options::IntOption int_required_option("required_int", "", 58, true,
                                              &TestFn);
  EXPECT_EQ(int_required_option, 58);

  ozz::options::Parser parser;
  EXPECT_TRUE(parser.RegisterOption(&int_option));

  {  // Validate function failed.
    const char* argv[] = {"c:/a path/test.exe", "--int=27"};
    const int argc = OZZ_ARRAY_SIZE(argv);
    EXPECT_EQ_LOG(parser.Parse(argc, argv), ozz::options::kExitFailure,
                  std::cout, "46 only option");
    EXPECT_EQ(int_option, 27);
  }
  {  // Validate function passed.
    const char* argv[] = {"c:/a path/test.exe", "--int=46"};
    const int argc = OZZ_ARRAY_SIZE(argv);
    EXPECT_EQ(parser.Parse(argc, argv), ozz::options::kSuccess);
    EXPECT_EQ(int_option, 46);
  }
  EXPECT_TRUE(parser.UnregisterOption(&int_option));

  EXPECT_TRUE(parser.RegisterOption(&int_required_option));
  {  // Validate function failed (required option).
    const char* argv[] = {"c:/a path/test.exe"};
    const int argc = OZZ_ARRAY_SIZE(argv);
    EXPECT_EQ_LOG(parser.Parse(argc, argv), ozz::options::kExitFailure,
                  std::cout, "Required");
    EXPECT_EQ(int_required_option, 58);
  }
  {  // Validate function failed (required option).
    const char* argv[] = {"c:/a path/test.exe", "--required_int=27"};
    const int argc = OZZ_ARRAY_SIZE(argv);
    EXPECT_EQ_LOG(parser.Parse(argc, argv), ozz::options::kExitFailure,
                  std::cout, "46 only option");
    EXPECT_EQ(int_required_option, 27);
  }
  {  // Validate function success (required option).
    const char* argv[] = {"c:/a path/test.exe", "--required_int=46"};
    const int argc = OZZ_ARRAY_SIZE(argv);
    EXPECT_EQ(parser.Parse(argc, argv), ozz::options::kSuccess);
    EXPECT_EQ(int_required_option, 46);
  }
  EXPECT_TRUE(parser.UnregisterOption(&int_required_option));
}

TEST(MultipleCall, Options) {
  ozz::options::BoolOption bool_option("bool", "", false, false);
  EXPECT_FALSE(bool_option);
  ozz::options::IntOption int_required_option("int", "", 27, true);
  EXPECT_EQ(int_required_option, 27);

  ozz::options::Parser parser;
  EXPECT_TRUE(parser.RegisterOption(&bool_option));
  EXPECT_TRUE(parser.RegisterOption(&int_required_option));

  {  // Required flag.
    const char* argv[] = {"c:/a path/test.exe", "--int=46"};
    const int argc = OZZ_ARRAY_SIZE(argv);
    EXPECT_EQ(parser.Parse(argc, argv), ozz::options::kSuccess);
    EXPECT_FALSE(bool_option);
    EXPECT_EQ(int_required_option, 46);
  }
  {  // Built-in flag.
    const char* argv[] = {"c:/a path/test.exe", "--help"};
    const int argc = OZZ_ARRAY_SIZE(argv);
    EXPECT_EQ_LOG(parser.Parse(argc, argv), ozz::options::kExitSuccess,
                  std::cout, "Usage");
    EXPECT_FALSE(bool_option);
    EXPECT_EQ(int_required_option, 27);
  }
  {  // Required flag missing.
    const char* argv[] = {"c:/a path/test.exe"};
    const int argc = OZZ_ARRAY_SIZE(argv);
    EXPECT_EQ_LOG(parser.Parse(argc, argv), ozz::options::kExitFailure,
                  std::cout, "Required");
    EXPECT_FALSE(bool_option);
    EXPECT_EQ(int_required_option, 27);
  }
}

// Internal test EXPECT_ macro. Ensures parse failed, display usage and exit.
#define EXPECT_FLAG_INVALID(_parser, _arg)                               \
  do {                                                                   \
    const char* argv[] = {"c:/a path/test.exe", _arg};                   \
    const int argc = OZZ_ARRAY_SIZE(argv);                               \
    EXPECT_EQ_LOG(_parser.Parse(argc, argv), ozz::options::kExitFailure, \
                  std::cout, "Usage");                                   \
  } while (void(0), 0)

// Internal test EXPECT_ macro. Ensures parse succeed.
#define EXPECT_FLAG_VALID(_parser, _arg)                          \
  do {                                                            \
    const char* argv[] = {"c:/a path/test.exe", _arg};            \
    const int argc = OZZ_ARRAY_SIZE(argv);                        \
    EXPECT_EQ(_parser.Parse(argc, argv), ozz::options::kSuccess); \
  } while (void(0), 0)

TEST(ParseBool, Options) {
  {  // Invalid options
    ozz::options::BoolOption bool_option("option", "", false, false);
    EXPECT_FALSE(bool_option);

    ozz::options::Parser parser;
    EXPECT_TRUE(parser.RegisterOption(&bool_option));

    EXPECT_FLAG_INVALID(parser, "option");
    EXPECT_FALSE(bool_option);
    EXPECT_FLAG_INVALID(parser, "-option");
    EXPECT_FALSE(bool_option);
    EXPECT_FLAG_INVALID(parser, "--option_value");
    EXPECT_FALSE(bool_option);
    EXPECT_FLAG_INVALID(parser, "--fla");
    EXPECT_FALSE(bool_option);
    EXPECT_FLAG_INVALID(parser, "--option_value");
    EXPECT_FALSE(bool_option);
    EXPECT_FLAG_INVALID(parser, "--option true");
    EXPECT_FALSE(bool_option);
    EXPECT_FLAG_INVALID(parser, "--nooption=");
    EXPECT_FALSE(bool_option);

    EXPECT_TRUE(parser.UnregisterOption(&bool_option));
  }

  {  // Valid true values.
    ozz::options::BoolOption bool_option("option", "", false, false);
    EXPECT_FALSE(bool_option);

    ozz::options::Parser parser;
    EXPECT_TRUE(parser.RegisterOption(&bool_option));

    EXPECT_FLAG_VALID(parser, "--option");
    EXPECT_TRUE(bool_option);
    EXPECT_FLAG_VALID(parser, "--option=yes");
    EXPECT_TRUE(bool_option);
    EXPECT_FLAG_VALID(parser, "--option=true");
    EXPECT_TRUE(bool_option);
    EXPECT_FLAG_VALID(parser, "--option=1");
    EXPECT_TRUE(bool_option);
    EXPECT_FLAG_VALID(parser, "--option=    true");
    EXPECT_TRUE(bool_option);
    EXPECT_FLAG_VALID(parser, "--Option=1");
    EXPECT_TRUE(bool_option);
    EXPECT_FLAG_VALID(parser, "--option=y");
    EXPECT_TRUE(bool_option);
    EXPECT_FLAG_VALID(parser, "--option=t");
    EXPECT_TRUE(bool_option);

    EXPECT_TRUE(parser.UnregisterOption(&bool_option));
  }

  {  // Valid false values.
    ozz::options::BoolOption bool_option("option", "", true, false);
    EXPECT_TRUE(bool_option);

    ozz::options::Parser parser;
    EXPECT_TRUE(parser.RegisterOption(&bool_option));

    EXPECT_FLAG_VALID(parser, "--nooption");
    EXPECT_FALSE(bool_option);
    EXPECT_FLAG_VALID(parser, "--option=no");
    EXPECT_FALSE(bool_option);
    EXPECT_FLAG_VALID(parser, "--option=false");
    EXPECT_FALSE(bool_option);
    EXPECT_FLAG_VALID(parser, "--option=0");
    EXPECT_FALSE(bool_option);
    EXPECT_FLAG_VALID(parser, "--option=  \tno");
    EXPECT_FALSE(bool_option);
    EXPECT_FLAG_VALID(parser, "--option=n");
    EXPECT_FALSE(bool_option);
    EXPECT_FLAG_VALID(parser, "--option=f");
    EXPECT_FALSE(bool_option);
    EXPECT_FLAG_VALID(parser, "--Nooption");
    EXPECT_FALSE(bool_option);
    EXPECT_FLAG_VALID(parser, "--nooption");
    EXPECT_FALSE(bool_option);

    EXPECT_TRUE(parser.UnregisterOption(&bool_option));
  }
}

TEST(ParseFloat, Options) {
  {  // Invalid options
    ozz::options::FloatOption float_option("option", "", 46.f, false);
    EXPECT_FLOAT_EQ(float_option, 46.f);

    ozz::options::Parser parser;
    EXPECT_TRUE(parser.RegisterOption(&float_option));

    EXPECT_FLAG_INVALID(parser, "option");
    EXPECT_FLOAT_EQ(float_option, 46.f);
    EXPECT_FLAG_INVALID(parser, "-option");
    EXPECT_FLOAT_EQ(float_option, 46.f);
    EXPECT_FLAG_INVALID(parser, "--option");
    EXPECT_FLOAT_EQ(float_option, 46.f);
    EXPECT_FLAG_INVALID(parser, "--fla");
    EXPECT_FLOAT_EQ(float_option, 46.f);
    EXPECT_FLAG_INVALID(parser, "--option=");
    EXPECT_FLOAT_EQ(float_option, 46.f);
    EXPECT_FLAG_INVALID(parser, "--option 46");
    EXPECT_FLOAT_EQ(float_option, 46.f);

    EXPECT_TRUE(parser.UnregisterOption(&float_option));
  }

  {  // Valid options
    ozz::options::FloatOption float_option("option", "", 0.f, false);
    EXPECT_FLOAT_EQ(float_option, 0.f);

    ozz::options::Parser parser;
    EXPECT_TRUE(parser.RegisterOption(&float_option));

    EXPECT_FLAG_VALID(parser, "--option=46.000");
    EXPECT_FLOAT_EQ(float_option, 46.f);
    EXPECT_FLAG_VALID(parser, "--option=0.0046");
    EXPECT_FLOAT_EQ(float_option, .0046f);
    EXPECT_FLAG_VALID(parser, "--option=.0046");
    EXPECT_FLOAT_EQ(float_option, .0046f);
    EXPECT_FLAG_VALID(parser, "--option=460e-1");
    EXPECT_FLOAT_EQ(float_option, 46.f);
    EXPECT_FLAG_VALID(parser, "--option=-46");
    EXPECT_FLOAT_EQ(float_option, -46.f);
    EXPECT_FLAG_VALID(parser, "--option= 046");
    EXPECT_FLOAT_EQ(float_option, 46.f);
    EXPECT_FLAG_VALID(parser, "--option= \t 046");
    EXPECT_FLOAT_EQ(float_option, 46.f);
    EXPECT_FLAG_VALID(parser, "--Option=46E0");
    EXPECT_FLOAT_EQ(float_option, 46.f);

    EXPECT_TRUE(parser.UnregisterOption(&float_option));
  }
}

TEST(ParseInt, Options) {
  {  // Invalid options
    ozz::options::IntOption int_option("option", "", 46, false);
    EXPECT_EQ(int_option, 46);

    ozz::options::Parser parser;
    EXPECT_TRUE(parser.RegisterOption(&int_option));

    EXPECT_FLAG_INVALID(parser, "option");
    EXPECT_EQ(int_option, 46);
    EXPECT_FLAG_INVALID(parser, "-option");
    EXPECT_EQ(int_option, 46);
    EXPECT_FLAG_INVALID(parser, "--option");
    EXPECT_EQ(int_option, 46);
    EXPECT_FLAG_INVALID(parser, "--fla");
    EXPECT_EQ(int_option, 46);
    EXPECT_FLAG_INVALID(parser, "--option=");
    EXPECT_EQ(int_option, 46);
    EXPECT_FLAG_INVALID(parser, "--option 99");
    EXPECT_EQ(int_option, 46);
    EXPECT_FLAG_INVALID(parser, "--option=99.0");
    EXPECT_EQ(int_option, 46);

    EXPECT_TRUE(parser.UnregisterOption(&int_option));
  }

  {  // Valid options
    ozz::options::IntOption int_option("option", "", 0, false);
    EXPECT_EQ(int_option, 0);

    ozz::options::Parser parser;
    EXPECT_TRUE(parser.RegisterOption(&int_option));

    EXPECT_FLAG_VALID(parser, "--option=46");
    EXPECT_EQ(int_option, 46);
    EXPECT_FLAG_VALID(parser, "--option=0046");
    EXPECT_EQ(int_option, 46);
    EXPECT_FLAG_VALID(parser, "--option=-46");
    EXPECT_EQ(int_option, -46);
    EXPECT_FLAG_VALID(parser, "--option=   46");
    EXPECT_EQ(int_option, 46);
    EXPECT_FLAG_VALID(parser, "--option=\t46");
    EXPECT_EQ(int_option, 46);
    EXPECT_FLAG_VALID(parser, "--Option=-46");
    EXPECT_EQ(int_option, -46);

    EXPECT_TRUE(parser.UnregisterOption(&int_option));
  }
}

TEST(ParseString, Options) {
  {  // Invalid options
    ozz::options::StringOption string_option("option", "", "default", false);
    EXPECT_STREQ(string_option, "default");

    ozz::options::Parser parser;
    EXPECT_TRUE(parser.RegisterOption(&string_option));

    EXPECT_FLAG_INVALID(parser, "option");
    EXPECT_STREQ(string_option, "default");
    EXPECT_FLAG_INVALID(parser, "-option");
    EXPECT_STREQ(string_option, "default");
    EXPECT_FLAG_INVALID(parser, "--option");
    EXPECT_STREQ(string_option, "default");
    EXPECT_FLAG_INVALID(parser, "--fla=twenty seven");
    EXPECT_STREQ(string_option, "default");
    EXPECT_FLAG_INVALID(parser, "--option forty six");
    EXPECT_STREQ(string_option, "default");

    EXPECT_TRUE(parser.UnregisterOption(&string_option));
  }

  {  // Valid options
    ozz::options::StringOption string_option("option", "", "default", false);
    EXPECT_STREQ(string_option, "default");

    ozz::options::Parser parser;
    EXPECT_TRUE(parser.RegisterOption(&string_option));

    EXPECT_FLAG_VALID(parser, "--option=");
    EXPECT_STREQ(string_option, "");
    EXPECT_FLAG_VALID(parser, "--option=forty-six");
    EXPECT_STREQ(string_option, "forty-six");
    EXPECT_FLAG_VALID(parser, "--option=forty six");
    EXPECT_STREQ(string_option, "forty six");
    EXPECT_FLAG_VALID(parser, "--option=\"forty six\"");
    EXPECT_STREQ(string_option, "\"forty six\"");
    EXPECT_FLAG_VALID(parser, "--option= forty six");
    EXPECT_STREQ(string_option, "forty six");
    EXPECT_FLAG_VALID(parser, "--option=\t forty six");
    EXPECT_STREQ(string_option, "forty six");
    EXPECT_FLAG_VALID(parser, "--option=46");
    EXPECT_STREQ(string_option, "46");
    EXPECT_FLAG_VALID(parser, "--optiOn=forty-six");
    EXPECT_STREQ(string_option, "forty-six");

    EXPECT_TRUE(parser.UnregisterOption(&string_option));
  }
}

#undef EXPECT_FLAG_INVALID
#undef EXPECT_FLAG_VALID
