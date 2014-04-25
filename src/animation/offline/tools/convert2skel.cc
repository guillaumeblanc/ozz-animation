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

#include "ozz/animation/offline/tools/convert2skel.h"

#include <cstdlib>
#include <cstring>

#include "ozz/animation/offline/skeleton_builder.h"

#include "ozz/animation/runtime/skeleton_serialize.h"

#include "ozz/base/io/archive.h"
#include "ozz/base/io/stream.h"

#include "ozz/base/log.h"

#include "ozz/options/options.h"

// Declares command line options.
OZZ_OPTIONS_DECLARE_STRING(file, "Specifies input file", "", true)
OZZ_OPTIONS_DECLARE_STRING(skeleton, "Specifies Ozz skeleton ouput file", "", true)

static bool ValidateEndianness(const ozz::options::Option& _option,
                               int /*_argc*/) {
  const ozz::options::StringOption& option =
    static_cast<const ozz::options::StringOption&>(_option);
  bool valid = std::strcmp(option.value(), "native") == 0 ||
               std::strcmp(option.value(), "little") == 0 ||
               std::strcmp(option.value(), "big") == 0;
  if (!valid) {
    ozz::log::Err() << "Invalid endianess option." << std::endl;
  }
  return valid;
}

OZZ_OPTIONS_DECLARE_STRING_FN(
  endian,
  "Selects output endianness mode. Can be \"native\" (same as current "\
  "platform), \"little\" or \"big\".",
  "native",
  false,
  &ValidateEndianness)

static bool ValidateLogLevel(const ozz::options::Option& _option,
                             int /*_argc*/) {
  const ozz::options::StringOption& option =
    static_cast<const ozz::options::StringOption&>(_option);
  bool valid = std::strcmp(option.value(), "verbose") == 0 ||
               std::strcmp(option.value(), "standard") == 0 ||
               std::strcmp(option.value(), "silent") == 0;
  if (!valid) {
    ozz::log::Err() << "Invalid log level option." << std::endl;
  }
  return valid;
}

OZZ_OPTIONS_DECLARE_STRING_FN(
  log_level,
  "Selects log level. Can be \"silent\", \"standard\" or \"verbose\".",
  "standard",
  false,
  &ValidateLogLevel)

namespace ozz {
namespace animation {
namespace offline {

int SkeletonConverter::operator()(int _argc, const char** _argv) {
  // Parses arguments.
  ozz::options::ParseResult parse_result = ozz::options::ParseCommandLine(
    _argc, _argv,
    "1.0",
    "Imports a skeleton from a file and converts it to ozz binary/run-time "
    "skeleton format");
  if (parse_result != ozz::options::kSuccess) {
    return parse_result == ozz::options::kExitSuccess ?
      EXIT_SUCCESS : EXIT_FAILURE;
  }

  // Initializes log level from options.
  ozz::log::Level log_level = ozz::log::GetLevel();
  if (std::strcmp(OPTIONS_log_level, "silent") == 0) {
    log_level = ozz::log::Silent;
  } else if (std::strcmp(OPTIONS_log_level, "standard") == 0) {
    log_level = ozz::log::Standard;
  } else if (std::strcmp(OPTIONS_log_level, "verbose") == 0) {
    log_level = ozz::log::Verbose;
  }
  ozz::log::SetLevel(log_level);

  // Imports skeleton from the file.
  ozz::log::Log() << "Importing file \"" << OPTIONS_file << "\"" <<
    std::endl;
  ozz::animation::offline::RawSkeleton raw_skeleton;
  if (!Import(OPTIONS_file, &raw_skeleton)) {
    ozz::log::Err() << "Failed to import file \"" << OPTIONS_file << "\"" <<
      std::endl;
    return EXIT_FAILURE;
  }

  // Builds runtime skeleton.
  ozz::log::Log() << "Builds runtime skeleton." << std::endl;
  ozz::animation::offline::SkeletonBuilder builder;
  ozz::animation::Skeleton* skeleton = builder(raw_skeleton);
  if (!skeleton) {
    ozz::log::Err() << "Failed to build runtime skeleton." << std::endl;
    return EXIT_FAILURE;
  }

  {
    // Prepares output stream. File is a RAII so it will close automatically at
    // the end of this scope.
    // Once the file is opened, nothing should fail as it would leave an invalid
    // file on the disk.
    ozz::log::Log() << "Opens output file: " << OPTIONS_skeleton << std::endl;
    ozz::io::File file(OPTIONS_skeleton, "wb");
    if (!file.opened()) {
      ozz::log::Err() << "Failed to open output file: " << OPTIONS_skeleton << std::endl;
      ozz::memory::default_allocator()->Delete(skeleton);
      return EXIT_FAILURE;
    }

    // Initializes output endianness from options.
    ozz::Endianness endianness = ozz::GetNativeEndianness();
    if (std::strcmp(OPTIONS_endian, "little")) {
      endianness = ozz::kLittleEndian;
    } else if (std::strcmp(OPTIONS_endian, "big")) {
      endianness = ozz::kBigEndian;
    }
    ozz::log::Log() << (endianness == ozz::kLittleEndian ? "Little" : "Big") <<
      " Endian output binary format selected." << std::endl;

    // Initializes output archive.
    ozz::log::Log() << "Outputs to binary archive." << std::endl;
    ozz::io::OArchive archive(&file, endianness);

    // Fills output archive with the skeleton.
    archive << *skeleton;
  }
  ozz::log::Log() << "Skeleton binary archive successfully outputed." << std::endl;

  // Delete local objects.
  ozz::memory::default_allocator()->Delete(skeleton);

  return EXIT_SUCCESS;
}
}  // offline
}  // animation
}  // ozz
