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

#ifndef OZZ_OZZ_BASE_GTEST_HELPER_H_
#define OZZ_OZZ_BASE_GTEST_HELPER_H_

#include <sstream>

// EXPECT_ASSERTION expands to real death test if assertions are enabled.
// Parameters:
//   statement -  A statement that a macro such as EXPECT_DEATH would test
//                for program termination.
//   regex     -  A regex that a macro such as EXPECT_DEATH would use to test
//                the output of statement.
#ifdef NDEBUG
// Expands to nothing if asserts aren't enabled (ie: NDEBUG is defined)
#define EXPECT_ASSERTION(_statement, _regex) do {} while (void(0), false);
#else  // NDEBUG
#ifdef _WIN32
#include <crtdbg.h>
#include <cstdlib>
namespace internal {
// Provides a hook during abort to ensure EXIT_FAILURE is returned.
inline int AbortHook(int, char*, int*) {
  exit(EXIT_FAILURE);
}
} // internal
#define EXPECT_ASSERTION(_statement, _regex) do {\
  /* During death tests executions:*/\
  /* Disables popping message boxes during crt and stl assertions*/\
  int old_mode = 0;\
  _CRT_REPORT_HOOK old_hook = NULL;\
  if (testing::internal::GTEST_FLAG(internal_run_death_test).length() > 0) {\
    old_mode = _CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_DEBUG);\
    old_hook = _CrtSetReportHook(&internal::AbortHook);\
  }\
  EXPECT_DEATH(_statement, _regex);\
  if (testing::internal::GTEST_FLAG(internal_run_death_test).length() > 0) {\
    _CrtSetReportMode(_CRT_ASSERT, old_mode);\
    _CrtSetReportHook(old_hook);\
  }\
} while (void(0), 0)
#else  // _WIN32
#define EXPECT_ASSERTION(_statement, _regex) EXPECT_DEATH(_statement, _regex)
#endif  // _WIN32
#endif  // NDEBUG

// EXPECT_EQ_LOG* executes _expression and compares its result with _eq.
// While executing _expression, EXPECT_EQ_LOG redirects _output (ex:
// std::clog) and then expects that the output matched the regular expression
// _re.
#define EXPECT_EQ_LOG(_expression, _eq, _output, _re) do{\
  internal::RedirectOuputTester tester(_output, _re);\
  EXPECT_EQ(_expression, _eq);\
}while(void(0), 0)

// There are multiple declinations EXPECT_EQ_LOG*, to match with clog, cerr and
// cout outputs, and verbose level option.

// Specialises EXPECT_EQ_LOG* for verbose clog output type.
#define EXPECT_EQ_LOG_LOGV(_expression, _eq, _re)\
  EXPECT_EQ_LOG(\
    _expression, _eq, std::clog,\
    ozz::log::Verbose <= ozz::log::GetLevel() ? _re : NULL)

// Specialises EXPECT_EQ_LOG* for standard clog output type.
#define EXPECT_EQ_LOG_LOG(_expression, _eq, _re)\
  EXPECT_EQ_LOG(\
    _expression, _eq, std::clog,\
    ozz::log::Standard <= ozz::log::GetLevel() ? _re : NULL)

// Specialises EXPECT_EQ_LOG* for standard cout output type.
#define EXPECT_EQ_LOG_OUT(_expression, _eq, _re)\
  EXPECT_EQ_LOG(\
    _expression, _eq, std::cout,\
    ozz::log::Standard <= ozz::log::GetLevel() ? _re : NULL)

// Specialises EXPECT_EQ_LOG* for standard cerr output type.
#define EXPECT_EQ_LOG_ERR(_expression, _eq, _re)\
  EXPECT_EQ_LOG(\
    _expression, _eq, std::cerr,\
    ozz::log::Standard <= ozz::log::GetLevel() ? _re : NULL)

// EXPECT_EQ_LOG* executes _expression while redirecting _output (ex:
// std::clog) and then expects that the output matched the regular expression
// _re.
#define EXPECT_LOG(_expression, _output, _re) do{\
  internal::RedirectOuputTester tester(_output, _re);\
  (_expression);\
}while(void(0), 0)

// There are multiple declinations EXPECT_LOG*, to match with clog, cerr and
// cout outputs, and verbose level option.

// Specialises EXPECT_LOG* for verbose clog output type.
#define EXPECT_LOG_LOGV(_expression, _re)\
  EXPECT_LOG(\
    _expression, std::clog,\
    ozz::log::Verbose <= ozz::log::GetLevel() ? _re : NULL)

// Specialises EXPECT_LOG* for standard clog output type.
#define EXPECT_LOG_LOG(_expression, _re)\
  EXPECT_LOG(\
    _expression, std::clog,\
    ozz::log::Standard <= ozz::log::GetLevel() ? _re : NULL)

// Specialises EXPECT_LOG* for standard cout output type.
#define EXPECT_LOG_OUT(_expression, _re)\
  EXPECT_LOG(\
    _expression, std::cout,\
    ozz::log::Standard <= ozz::log::GetLevel() ? _re : NULL)

// Specialises EXPECT_LOG* for standard cerr output type.
#define EXPECT_LOG_ERR(_expression, _re)\
  EXPECT_LOG(\
    _expression, std::cerr,\
    ozz::log::Standard <= ozz::log::GetLevel() ? _re : NULL)

namespace internal {
class RedirectOuputTester{
public:
  // Specify a NULL _regex to test an empty output
  RedirectOuputTester(std::ostream& _ostream, const char* _regex) :
    ostream_(_ostream),
    old_(_ostream.rdbuf()),
    regex_(_regex) {
    // Redirect ostream_ buffer.
    ostream_.rdbuf(redirect_.rdbuf());
  }
  ~RedirectOuputTester() {
    if (regex_) {
      EXPECT_TRUE(::testing::internal::RE::PartialMatch(
        redirect_.str().c_str(), regex_));
    } else {
      EXPECT_EQ(redirect_.str().size(), 0u);
    }
    // Restore ostream_ buffer.
    ostream_.rdbuf(old_);
    // finally outputs everything temporary redirected.
    ostream_ << redirect_.str();
  }
private:
  std::ostream& ostream_;
  std::streambuf* old_;
  const char* regex_;
  std::stringstream redirect_;
};
} // internal
#endif  // OZZ_OZZ_BASE_GTEST_HELPER_H_
