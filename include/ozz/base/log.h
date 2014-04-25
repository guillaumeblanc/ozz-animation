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

#ifndef OZZ_OZZ_BASE_LOG_H_
#define OZZ_OZZ_BASE_LOG_H_

#include <iostream>

// MSVC includes <cstring> from <iostream> but not gcc.
// So it is included here to ensure a portable behavior.
#include <cstring>

// Proposes a logging interface that redirects logs to std::cout, clog and cerr
// output streams. This interface adds a logging level functionality (Silent,
// Standard, Verbose) to the std API, which can be set using
// ozz::log::GetLevel function.
// Usage conforms to std stream usage: ozz::log::OUT() << "something to log."...

namespace ozz {
namespace log {

enum Level {
  Silent,  // No output at all, even errors are muted.
  Standard,  // Default output level.
  Verbose,  // Most verbose output level.
};

// Sets the global logging level.
Level SetLevel(Level _level);

// Gets the global logging level.
Level GetLevel();

namespace internal {

// Implements logging base class.
// This class is not intended to be used publicly, it is derived by user
// classes LogV, Log, Out, Err...
// Forwards ostream::operator << to a standard ostream or a silent
// ostringstream according to the logging level at construction time.
class Logger {
public:
  // Forwards ostream::operator << for any type.
  template<typename _T>
  std::ostream& operator << (const _T& _t) {
    return stream_ << _t;
  }
  // Forwards ostream::operator << for modifiers.
  std::ostream& operator << (std::ostream& (*_Pfn)(std::ostream&)) {
    return ((*_Pfn)(stream_));
  }
  // Cast operator.
  operator std::ostream& () {
    return stream_;
  }

  // Cast operator.
  std::ostream& stream() {
    return stream_;
  }

protected:
  // Specifies the global stream and the output level.
  // Logging levels allows to select _stream or a "silent" stream according to
  // the current global logging level.
  Logger(std::ostream& _stream, Level _level);

  // Destructor, deletes the internal "silent" stream.
  ~Logger();

private:
  // Disables copy and assignment.
  Logger(const Logger&);
  void operator = (Logger&);

  // Selected output stream.
  std::ostream& stream_;

  // Stores whether the stream is local one, in which case it must be deleted
  // in the destructor.
  bool local_stream_;
};
}  // internal

// Logs verbose output to the standard error stream (std::clog).
// Enabled if logging level is Verbose.
class LogV : public internal::Logger {
public:
  LogV();
};

// Logs output to the standard error stream (std::clog).
// Enabled if logging level is not Silent.
class Log : public internal::Logger {
public:
  Log();
};

// Logs output to the standard output (std::cout).
// Enabled if logging level is not Silent.
class Out : public internal::Logger {
public:
  Out();
};

// Logs error to the standard error stream (std::cerr).
// Enabled if logging level is not Silent.
class Err : public internal::Logger {
public:
  Err();
};
} // log
} // ozz
#endif  // OZZ_OZZ_BASE_LOG_H_
