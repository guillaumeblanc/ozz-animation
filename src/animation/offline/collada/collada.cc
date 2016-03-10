//----------------------------------------------------------------------------//
//                                                                            //
// ozz-animation is hosted at http://github.com/guillaumeblanc/ozz-animation  //
// and distributed under the MIT License (MIT).                               //
//                                                                            //
// Copyright (c) 2015 Guillaume Blanc                                         //
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

#define OZZ_INCLUDE_PRIVATE_HEADER  // Allows to include private headers.

#include "ozz/animation/offline/collada/collada.h"

#include <cassert>

#include "tinyxml.h"

#include "ozz/base/log.h"

#include "ozz/base/memory/allocator.h"
#include "ozz/base/io/stream.h"

#include "ozz/animation/runtime/skeleton.h"
#include "ozz/animation/offline/raw_animation.h"
#include "ozz/animation/offline/raw_skeleton.h"

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
    // Allocates and read file.
    const size_t read_length = file.Size();
    content = memory::default_allocator()->Allocate<char>(read_length + 1);
    content[read_length] = '\0';
    if (file.Read(content, read_length) != read_length) {
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

bool ImportFromFile(const char* _filename,
                    const Skeleton& _skeleton,
                    float _sampling_rate,
                    RawAnimation* _animation) {
  char* xml = LoadFileToString(_filename);

  // Import xml from memory even if load from file has failed. ImportFromMemory
  // supports NULL argument and will fill output.
  bool success = ImportFromMemory(xml, _skeleton, _sampling_rate, _animation);
  memory::default_allocator()->Deallocate(xml);
  return success;
}

bool ImportFromMemory(const char* _xml,
                      const Skeleton& _skeleton,
                      float _sampling_rate,
                      RawAnimation* _animation) {
  (void)_sampling_rate;
  
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
                        skeleton_visitor,
                        _skeleton,
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
