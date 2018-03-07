//----------------------------------------------------------------------------//
//                                                                            //
// ozz-animation is hosted at http://github.com/guillaumeblanc/ozz-animation  //
// and distributed under the MIT License (MIT).                               //
//                                                                            //
// Copyright (c) 2017 Guillaume Blanc                                         //
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

#include "animation/offline/tools/convert2ozz_skel.h"

#include <cstdlib>
#include <cstring>

#include "animation/offline/tools/convert2ozz_config.h"

#include "ozz/animation/offline/tools/convert2ozz.h"

#include "ozz/animation/offline/raw_skeleton.h"
#include "ozz/animation/offline/skeleton_builder.h"

#include "ozz/animation/runtime/skeleton.h"

#include "ozz/base/io/archive.h"
#include "ozz/base/io/stream.h"

#include "ozz/base/log.h"

namespace ozz {
namespace animation {
namespace offline {

bool ProcessSkeleton(const Json::Value& _config, AnimationConverter* _converter,
                     const ozz::Endianness _endianness) {
  // First check that there's a skeleton actually expected.
  const char* output_name = _config["output"].asCString();
  if (*output_name == 0) {
    ozz::log::Log()
        << "No skeleton output name provided. Skeleton import will be skipped."
        << std::endl;
    return true;
  }

  ozz::animation::offline::RawSkeleton raw_skeleton;
  if (!_converter->Import(&raw_skeleton, _config["all_nodes"].asBool())) {
    ozz::log::Err() << "Failed to import skeleton." << std::endl;
    return false;
  }

  // Needs to be done before opening the output file, so that if it fails then
  // there's no invalid file outputted.
  ozz::animation::Skeleton* skeleton = NULL;
  if (!_config["raw"].asBool()) {
    // Builds runtime skeleton.
    ozz::log::Log() << "Builds runtime skeleton." << std::endl;
    ozz::animation::offline::SkeletonBuilder builder;
    skeleton = builder(raw_skeleton);
    if (!skeleton) {
      ozz::log::Err() << "Failed to build runtime skeleton." << std::endl;
      return false;
    }
  }

  // Prepares output stream. File is a RAII so it will close automatically at
  // the end of this scope.
  // Once the file is opened, nothing should fail as it would leave an invalid
  // file on the disk.
  {
    ozz::log::Log() << "Opens output file: " << _config["output"].asCString()
                    << std::endl;
    ozz::io::File file(_config["output"].asCString(), "wb");
    if (!file.opened()) {
      ozz::log::Err() << "Failed to open output file: \""
                      << _config["output"].asCString() << "\"." << std::endl;
      ozz::memory::default_allocator()->Delete(skeleton);
      return false;
    }

    // Initializes output archive.
    ozz::io::OArchive archive(&file, _endianness);

    // Fills output archive with the skeleton.
    if (_config["raw"].asBool()) {
      ozz::log::Log() << "Outputs RawSkeleton to binary archive." << std::endl;
      archive << raw_skeleton;
    } else {
      ozz::log::Log() << "Outputs Skeleton to binary archive." << std::endl;
      archive << *skeleton;
    }
    ozz::log::Log() << "Skeleton binary archive successfully outputted."
                    << std::endl;
  }

  // Delete local objects.
  ozz::memory::default_allocator()->Delete(skeleton);

  return true;
}
}  // namespace offline
}  // namespace animation
}  // namespace ozz
