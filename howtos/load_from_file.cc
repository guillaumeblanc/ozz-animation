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

// Provides files abstraction.
#include "ozz/base/io/stream.h"

// Provides serialization/deserialization mechanism.
#include "ozz/base/io/archive.h"

// Uses the skeleton as an example of object to read.
#include "ozz/animation/runtime/skeleton.h"

#include <cstdlib>

// Code for ozz-animation HowTo: "How to load an object from a file?"
int main(int argc, char const* argv[]) {
  (void)argc;
  (void)argv;

  // First check that an argument was provided. We expect it to be a valid
  // filename.
  if (argc != 2) {
    ozz::log::Err() << "Invalid arguments." << std::endl;
    return EXIT_FAILURE;
  }
  // Stores filename.
  const char* filename = argv[1];

  {
    ////////////////////////////////////////////////////////////////////////////
    // The first section opens a file.
    ////////////////////////////////////////////////////////////////////////////

    // Now tries to open the file, which was provided as argument.
    // A file in ozz is a ozz::io::File, which implements ozz::io::Stream
    // interface and complies with std FILE specifications.
    // ozz::io::File follows RAII programming idiom, which ensures that the file
    // will always be closed (by ozz::io::FileStream destructor).
    ozz::io::File file(filename, "rb");

    // Checks file status, which can be closed if filename is invalid.
    if (!file.opened()) {
      ozz::log::Err() << "Cannot open file " << filename << "." << std::endl;
      return EXIT_FAILURE;
    }

    ////////////////////////////////////////////////////////////////////////////
    // The next section deserializes an object from the file.
    ////////////////////////////////////////////////////////////////////////////

    // Now the file is opened. we can actually read from it. This uses ozz
    // archive mechanism.
    // The first step is to instantiate an read-capable (ozz::io::IArchive)
    // archive object, in opposition to write-capable (ozz::io::OArchive)
    // archives.
    // Archives take as argument stream objects, which must be valid and opened.
    ozz::io::IArchive archive(&file);

    // Before actually reading the object from the file, we need to test that
    // the archive (at current seek position) contains the object type we
    // expect.
    // Archives uses a tagging system that allows to mark and detect thetype of
    // the next object to deserialize. Here we expect a skeleton, so we test for
    // a skeleton tag.
    // Tagging is not mandatory for all object types. It's usually only used for
    // high level object types (skeletons, animations...), but not low level
    // ones (math objects, native types...).
    if (!archive.TestTag<ozz::animation::Skeleton>()) {
      ozz::log::Err() << "Archive doesn't contain the expected object type."
                      << std::endl;
      return EXIT_FAILURE;
    }

    // Now the tag has been validated, the object can be read.
    // IArchive uses >> operator to read from the archive to the object.
    // Only objects that implement archive specifications can be used there,
    // along with all native types. Note that pointers aren't supported.
    ozz::animation::Skeleton skeleton;
    archive >> skeleton;

    // Getting out of this scope will destroy "file" object and close the system
    // file.
  }

  return EXIT_SUCCESS;
}
