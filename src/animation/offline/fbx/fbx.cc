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

#include "ozz/animation/offline/fbx/fbx.h"
#include "ozz/animation/offline/fbx/fbx_base.h"

#include "fbx_skeleton.h"
#include "fbx_animation.h"

#include "ozz/base/log.h"

#include "ozz/animation/offline/raw_animation.h"
#include "ozz/animation/offline/raw_skeleton.h"

namespace ozz {
namespace animation {
namespace offline {
namespace fbx {

bool ImportFromFile(const char* _filename, RawSkeleton* _skeleton) {
  if (!_skeleton) {
    return false;
  }
  // Reset skeleton.
  *_skeleton = RawSkeleton();

  // Import Fbx content.
  FbxManagerInstance fbx_manager;
  FbxSkeletonIOSettings settings(fbx_manager);
  FbxSceneLoader scene_loader(_filename, "", fbx_manager, settings);
  if (!scene_loader.scene()) {
    ozz::log::Err() << "Failed to import file " << _filename << "." <<
      std::endl;
    return false;
  }

  if (!ExtractSkeleton(scene_loader.scene(), _skeleton)) {
    log::Err() << "Fbx skeleton extraction failed." << std::endl;
    return false;
  }

  return true;
}

bool ImportFromFile(const char* _filename,
                    const Skeleton& _skeleton,
                    float _sampling_rate,
                    RawAnimation* _animation) {
  if (!_animation) {
    return false;
  }
  // Reset animation.
  *_animation = RawAnimation();

  // Import Fbx content.
  FbxManagerInstance fbx_manager;
  FbxAnimationIOSettings settings(fbx_manager);
  FbxSceneLoader scene_loader(_filename, "", fbx_manager, settings);
  if (!scene_loader.scene()) {
    ozz::log::Err() << "Failed to import file " << _filename << "." <<
      std::endl;
    return false;
  }

  if (!ExtractAnimation(scene_loader.scene(), _skeleton, _sampling_rate, _animation)) {
    log::Err() << "Fbx animation extraction failed." << std::endl;
    return false;
  }

  return true;
}
}  // fbx
}  // offline
}  // animation
}  // ozz
