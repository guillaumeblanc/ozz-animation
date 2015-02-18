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

#ifndef OZZ_OZZ_ANIMATION_OFFLINE_FBX_FBX_BASE_H_
#define OZZ_OZZ_ANIMATION_OFFLINE_FBX_FBX_BASE_H_

#include <fbxsdk.h>

#include "ozz/base/maths/simd_math.h"
#include "ozz/base/maths/transform.h"

namespace ozz {
namespace math {
struct Transform;
}  // math
namespace animation {
namespace offline {
namespace fbx {

// Manages FbxManager instance.
class FbxManagerInstance {
 public:
  // Instantiates FbxManager.
  FbxManagerInstance();

  // Destroys FbxManager.
  ~FbxManagerInstance();

  // Cast operator to get internal FbxManager instance.
  operator FbxManager*() const {
    return fbx_manager_;
  }
private:
  FbxManager* fbx_manager_;
};

// Default io settings used to import a scene.
class FbxDefaultIOSettings {
 public:

   // Instantiates default settings.
  explicit FbxDefaultIOSettings(const FbxManagerInstance& _manager);

  // De-instantiates default settings.
  virtual ~FbxDefaultIOSettings();

  // Get FbxIOSettings instance.
  FbxIOSettings* settings() const {
    return io_settings_;
  }

  // Cast operator to get internal FbxIOSettings instance.
  operator FbxIOSettings*() const {
    return io_settings_;
  }
 private:

  FbxIOSettings* io_settings_;
};

// Io settings used to import an animation from a scene.
class FbxAnimationIOSettings : public FbxDefaultIOSettings {
 public:
  FbxAnimationIOSettings(const FbxManagerInstance& _manager);
};

// Io settings used to import a skeleton from a scene.
class FbxSkeletonIOSettings : public FbxDefaultIOSettings {
public:
  FbxSkeletonIOSettings(const FbxManagerInstance& _manager);
};

// Implements axis system and unit system conversion helper, from any Fbx system
// to ozz system (FbxAxisSystem::eYAxis, FbxAxisSystem::eParityOdd,
// FbxAxisSystem::eRightHanded, meter).
// While Fbx sdk FbxAxisSystem::ConvertScene and FbxSystem::ConvertScene only
// affect scene root, this class functions can be used to bake nodes, vertices,
// animations tranformations...
class FbxSystemConverter {
 public:

  // Initialize converter with fbx scene systems.
  FbxSystemConverter(const FbxAxisSystem& _from_axis,
                     const FbxSystemUnit& _from_unit);

  // Converts a fbx matrix to an ozz Float4x4 matrix, in ozz axis and unit
  // systems, using _m' = C * _m * (C-1) operation.
  math::Float4x4 ConvertMatrix(const FbxAMatrix& _m) const;

  // Convert fbx matrix to an ozz transform, in ozz axis and unit systems,
  // using _m' = C * _m * (C-1) operation.
  math::Transform ConvertTransform(const FbxAMatrix& _m) const;

  // Convert fbx FbxVector4 point to an ozz Float3, in ozz axis and unit
  // systems, using _p' = C * _p operation.
  math::Float3 ConvertPoint(const FbxVector4& _p) const;

  // Convert fbx FbxVector4 normal to an ozz Float3, in ozz axis and unit
  // systems, using _p' = ((C-1)-T) * _p operation. Normals are converted
  // using the inverse transpose matrix to support non-uniform scale
  // transformations.
  math::Float3 ConvertNormal(const FbxVector4& _p) const;

 private:

  // The matrix used to convert from "from" axis/unit to ozz coordinate system
  // base.
  math::Float4x4 convert_;

  // The inverse of convert_ matrix.
  math::Float4x4 inverse_convert_;

  // The transpose inverse of convert_ matrix.
  math::Float4x4 inverse_transpose_convert_;
};

// Loads a scene from a Fbx file.
class FbxSceneLoader {
 public:
  // Loads the scene that can then be obtained with scene() function.
   FbxSceneLoader(const char* _filename,
                  const char* _password,
                  const FbxManagerInstance& _manager,
                  const FbxDefaultIOSettings& _io_settings);
  ~FbxSceneLoader();

  // Returns a valid scene if fbx import was successful, NULL otherwise.
  FbxScene* scene() const {
    return scene_;
  }

  // Returns a valid converter if fbx import was successful, NULL otherwise.
  FbxSystemConverter* converter() {
    return converter_;
  }

private:

  // Scene instance that was loaded from the file.
  FbxScene* scene_;

  // Axis and unit conversion helper.
  FbxSystemConverter* converter_;
};
}  // fbx
}  // offline
}  // animation
}  // ozz
#endif  // OZZ_OZZ_ANIMATION_OFFLINE_FBX_FBX_BASE_H_
