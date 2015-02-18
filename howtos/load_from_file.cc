//============================================================================//
//                                                                            //
// ozz-animation, 3d skeletal animation libraries and tools.                  //
// https://code.google.com/p/ozz-animation/                                   //
//                                                                            //
//----------------------------------------------------------------------------//
//                                                                            //
// Copyright (c) 2012-2015 Guillaume Blanc                                    //
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

// Provides logging functionnalities.
#include "ozz/base/log.h"

// Provides files abstraction.
// See "http://code.google.com/p/ozz-animation/wiki/Advanced#File_IO_management"
#include "ozz/base/io/stream.h"

// Provides serialization/deserialization mechanism.
// See "http://code.google.com/p/ozz-animation/wiki/Advanced#Serialization_mechanics"
#include "ozz/base/io/archive.h"

// Uses the skeleton as an example of object to read.
#include "ozz/animation/runtime/skeleton.h"

#include <cstdlib>

// Code for ozz-animation HowTo: "How to load an object from a file?"
// "http://code.google.com/p/ozz-animation/wiki/HowTos#How_to_load_an_object_from_a_file?"

int main(int argc, char const *argv[]) {
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
      ozz::log::Err() << "Archive doesn't contain the expected object type." <<
        std::endl;
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

