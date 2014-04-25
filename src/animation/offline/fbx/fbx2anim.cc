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

#include "ozz/animation/offline/tools/convert2anim.h"

#include "ozz/animation/offline/fbx/fbx.h"

// fbx2anim is a command line tool that converts an animation imported from a
// fbx document to ozz runtime format.
//
// dae2anim extracts animated joints from a fbx document. Only the animated
// joints whose names match those of the ozz runtime skeleton given as argument
// are selected. Keyframes are then optimized, based on command line settings,
// and serialized as a runtime animation to an ozz binary archive.
//
// Use dae2anim integrated help command (fbx2anim --help) for more details
// about available arguments.

class FbxAnimationConverter :
  public ozz::animation::offline::AnimationConverter {
private:
  // Implement SkeletonConverter::Import function.
  virtual bool Import(const char* _filename,
                      const ozz::animation::Skeleton& _skeleton,
                      float _sampling_rate,
                      ozz::animation::offline::RawAnimation* _animation) {
    return ozz::animation::offline::fbx::ImportFromFile(
      _filename, _skeleton, _sampling_rate, _animation);
  }
};

int main(int _argc, const char** _argv) {
  FbxAnimationConverter converter;
  return converter(_argc, _argv);
}
