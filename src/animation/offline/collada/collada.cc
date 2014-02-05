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

#define OZZ_INCLUDE_PRIVATE_HEADER  // Allows to include private headers.

#include "ozz/animation/offline/collada/collada.h"

#include <cassert>

#include "tinyxml.h"

#include "ozz/base/log.h"

#include "ozz/base/memory/allocator.h"
#include "ozz/base/io/stream.h"

#include "ozz/animation/runtime/skeleton.h"
#include "ozz/animation/offline/animation_builder.h"
#include "ozz/animation/offline/skeleton_builder.h"

#include "animation/offline/collada/collada_skeleton.h"
#include "animation/offline/collada/collada_animation.h"

namespace ozz {
namespace animation {
namespace offline {
namespace collada {

namespace {

// Load _filename to memory as a string. Return a valid pointer if read was
// successful, or NULL otherwise. The caller is responsible for deleting the
// return string using default_allocator().Deallocate() function.
char* LoadFileToString(const char* _filename) {
  if (!_filename) {
    return NULL;
  }
  log::Log() << "Reads Collada document " << _filename << "." << std::endl;
  // Loads file to memory.
  char* content = NULL;
  io::File file(_filename, "rb");
  if (file.opened()) {
    // Gets file size.
    file.Seek(0, io::Stream::kEnd);
    std::size_t file_length = file.Tell();
    file.Seek(0, io::Stream::kSet);

    // Allocates and read file.
    content = memory::default_allocator()->Allocate<char>(file_length + 1);
    content[file_length] = '\0';
    if (file.Read(content, file_length) != file_length) {
      log::Err() << "Failed to read file " << _filename << "." << std::endl;
      memory::default_allocator()->Deallocate(content);
      content = NULL;
    }
  } else {
    log::Err() << "Failed to open file " << _filename << "." << std::endl;
  }
  return content;
}
}  // namespace

bool ImportFromFile(const char* _filename, RawSkeleton* _skeleton) {
  char* xml = LoadFileToString(_filename);

  // Import xml from memory even if load from file has failed. ImportFromMemory
  // supports NULL argument and will fill output.
  bool success = ImportFromMemory(xml, _skeleton);
  memory::default_allocator()->Deallocate(xml);
  return success;
}

bool ParseDocument(TiXmlDocument* _doc, const char* _xml) {
  if (!_doc || !_xml) {
    return false;
  }

  _doc->Parse(_xml);
  if (_doc->Error()) {
    log::Err() << "Failed to parse xml document";
    if (_doc->ErrorRow()) {
      log::Err() << " (line " << _doc->ErrorRow();
    }
    if (_doc->ErrorCol()) {
      log::Err() << " column " << _doc->ErrorCol() << ")";
    }
    log::Err() << ":" << _doc->ErrorDesc() << std::endl;
    return false;
  }
  log::Log() << "Successfully parsed xml document." << std::endl;
  return true;
}

bool ImportFromMemory(const char* _xml, RawSkeleton* _skeleton) {
  if (!_skeleton) {
    return false;
  }
  // Reset skeleton.
  *_skeleton = RawSkeleton();

  // Opens the document.
  TiXmlDocument doc;
  if (!ParseDocument(&doc, _xml)) {
    return false;
  }

  SkeletonVisitor skeleton_visitor;
  if (!doc.Accept(&skeleton_visitor)) {
    log::Err() << "Collada skeleton parsing failed." << std::endl;
    return false;
  }

  if (!ExtractSkeleton(skeleton_visitor, _skeleton)) {
    log::Err() << "Collada skeleton extraction failed." << std::endl;
    return false;
  }

  return true;
}

bool ImportFromFile(const char* _filename, const Skeleton& _skeleton,
                   RawAnimation* _animation) {
  char* xml = LoadFileToString(_filename);

  // Import xml from memory even if load from file has failed. ImportFromMemory
  // supports NULL argument and will fill output.
  bool success = ImportFromMemory(xml, _skeleton, _animation);
  memory::default_allocator()->Deallocate(xml);
  return success;
}

bool ImportFromMemory(const char* _xml, const Skeleton& _skeleton,
                     RawAnimation* _animation) {
  if (!_animation) {
    return false;
  }
  // Reset animation.
  *_animation = RawAnimation();

  // Opens the document.
  TiXmlDocument doc;
  if (!ParseDocument(&doc, _xml)) {
    return false;
  }

  // Extracts skeletons from the Collada document.
  SkeletonVisitor skeleton_visitor;
  if (!doc.Accept(&skeleton_visitor)) {
    log::Err() << "Collada skeleton parsing failed." << std::endl;
    return false;
  }

  // Extracts animations from the Collada document.
  AnimationVisitor animation_visitor;
  if (!doc.Accept(&animation_visitor)) {
    log::Err() << "Collada animation import failed." << std::endl;
    return false;
  }

  // Allocates RawAnimation.
  if (!ExtractAnimation(animation_visitor,
                        skeleton_visitor, _skeleton,
                        _animation)) {
    log::Err() << "Collada animation extraction failed." << std::endl;
    return false;
  }

  return true;
}
}  // collada
}  // offline
}  // animation
}  // ozz
