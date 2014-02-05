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

#include <sstream>

#include "ozz/base/memory/allocator.h"

namespace ozz {
namespace log {

// Default log level initialization.
namespace {
Level log_level = Standard;
}

Level SetLevel(Level _level) {
  const Level previous_level = log_level;
  log_level = _level;
  return previous_level;
}

Level GetLevel() {
  return log_level;
}

LogV::LogV()
    : internal::Logger(std::clog, Verbose) {
}

Log::Log()
    : internal::Logger(std::clog, Standard) {
}

Out::Out()
    : internal::Logger(std::cout, Standard) {
}

Err::Err()
    : internal::Logger(std::cerr, Standard) {
}

namespace internal {

Logger::Logger(std::ostream& _stream, Level _level)
    : stream_(_level <= GetLevel() ?
        _stream : *ozz::memory::default_allocator()->New<std::ostringstream>()),
      local_stream_(&stream_ != &_stream) {
}
Logger::~Logger() {
  if (local_stream_) {
    ozz::memory::default_allocator()->Delete(&stream_);
  }
}
}  // internal
} // log
} // ozz
