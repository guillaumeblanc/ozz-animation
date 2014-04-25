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

#include "ozz/animation/offline/fbx/fbx.h"

// fbx2skel is a command line tool that converts a skeleton imported from a
// Fbx document to ozz runtime format.
//
// dae2skel extracts the skeleton from the Fbx document. It then builds an
// ozz runtime skeleton, from the Fbx skeleton, and serializes it to a ozz
// binary archive.
//
// Use dae2skel integrated help command (fbx2skel --help) for more details
// about available arguments.

class FbxSkeletonConverter :
  public ozz::animation::offline::SkeletonConverter {
private:
  // Implement SkeletonConverter::Import function.
  virtual bool Import(const char* _filename,
                      ozz::animation::offline::RawSkeleton* _skeleton) {
    return ozz::animation::offline::fbx::ImportFromFile(_filename,
      _skeleton);
  }
};

int main(int _argc, const char** _argv) {
  FbxSkeletonConverter converter;
  return converter(_argc, _argv);
}
