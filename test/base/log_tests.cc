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

TEST(Log, Silent) {
  TestLogLevel(ozz::log::Silent);
}

TEST(Log, Standard) {
  TestLogLevel(ozz::log::Standard);
}

TEST(Log, Verbose) {
  TestLogLevel(ozz::log::Verbose);
}
