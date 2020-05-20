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

#include "ozz/base/log.h"

#include "gtest/gtest.h"
#include "ozz/base/gtest_helper.h"

int TestFunction(std::ostream& _stream, const char* _log) {
  _stream << _log << std::endl;
  return 46;
}

void TestLogLevel(ozz::log::Level _level) {
  ozz::log::SetLevel(_level);

  EXPECT_LOG_LOGV(TestFunction(ozz::log::LogV(), "logv"), "logv");
  EXPECT_LOG_LOG(TestFunction(ozz::log::Log(), "log"), "log");
  EXPECT_LOG_OUT(TestFunction(ozz::log::Out(), "out"), "out");
  EXPECT_LOG_ERR(TestFunction(ozz::log::Err(), "err"), "err");

  EXPECT_EQ_LOG_LOGV(TestFunction(ozz::log::LogV(), "logv"), 46, "logv");
  EXPECT_EQ_LOG_LOG(TestFunction(ozz::log::Log(), "log"), 46, "log");
  EXPECT_EQ_LOG_OUT(TestFunction(ozz::log::Out(), "out"), 46, "out");
  EXPECT_EQ_LOG_ERR(TestFunction(ozz::log::Err(), "err"), 46, "err");
}

TEST(Silent, Log) { TestLogLevel(ozz::log::kSilent); }

TEST(Standard, Log) { TestLogLevel(ozz::log::kStandard); }

TEST(Verbose, Log) { TestLogLevel(ozz::log::kVerbose); }

TEST(FloatPrecision, Log) {
  const float number = 46.9352099f;
  ozz::log::Log log;

  ozz::log::FloatPrecision mod0(log, 0);
  EXPECT_LOG_LOG(log << number << '-' << std::endl, "47-");
  {
    ozz::log::FloatPrecision mod2(log, 2);
    EXPECT_LOG_LOG(log << number << '-' << std::endl, "46.94-");
  }
  EXPECT_LOG_LOG(log << number << '-' << std::endl, "47-");
}
