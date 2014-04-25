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

#include "animation/offline/fbx/fbx_base.h"

#include "ozz/base/log.h"

#include "ozz/base/maths/transform.h"

namespace ozz {
namespace animation {
namespace offline {
namespace fbx {

FbxManagerInstance::FbxManagerInstance()
    : fbx_manager_(NULL) {
  // Instantiate Fbx manager, mostly a memory manager.
  fbx_manager_ = FbxManager::Create();

  // Logs SDK version.
  ozz::log::Log() << "FBX importer version " << fbx_manager_->GetVersion() <<
    "." << std::endl;
}

FbxManagerInstance::~FbxManagerInstance() {
  // Destroy the manager and all associated objects.
  fbx_manager_->Destroy();
  fbx_manager_ = NULL;
}

FbxDefaultIOSettings::FbxDefaultIOSettings(const FbxManagerInstance& _manager)
    : io_settings_(NULL) {
  io_settings_ = FbxIOSettings::Create(_manager, IOSROOT);
  io_settings_->SetBoolProp(IMP_FBX_MATERIAL, false);
  io_settings_->SetBoolProp(EXP_FBX_TEXTURE, false);
  io_settings_->SetBoolProp(EXP_FBX_MODEL, false);
  io_settings_->SetBoolProp(EXP_FBX_SHAPE, false);
  io_settings_->SetBoolProp(IMP_FBX_LINK, false);
  io_settings_->SetBoolProp(IMP_FBX_GOBO, false);
}

FbxDefaultIOSettings::~FbxDefaultIOSettings() {
  io_settings_->Destroy();
  io_settings_ = NULL;
}

FbxAnimationIOSettings::FbxAnimationIOSettings(const FbxManagerInstance& _manager)
    : FbxDefaultIOSettings(_manager) {
}

FbxSkeletonIOSettings::FbxSkeletonIOSettings(const FbxManagerInstance& _manager)
    : FbxDefaultIOSettings(_manager) {
  settings()->SetBoolProp(IMP_FBX_ANIMATION, false);
}

FbxSceneLoader::FbxSceneLoader(const char* _filename,
                               const char* _password,
                               const FbxManagerInstance& _manager,
                               const FbxDefaultIOSettings& _io_settings)
    : scene_(NULL) {
  // Create an importer.
  FbxImporter* importer = FbxImporter::Create(_manager,"ozz importer");

  // Initialize the importer by providing a filename. Use all available plugins.
  const bool initialized = importer->Initialize(_filename, -1, _io_settings);

  // Get the version number of the FBX file format.
  int major, minor, revision;
  importer->GetFileVersion(major, minor, revision);

  if (!initialized)  // Problem with the file to be imported.
  {
    const FbxString error = importer->GetStatus().GetErrorString();
    ozz::log::Err() << "FbxImporter initialization failed with error: " <<
      error.Buffer() << std::endl;

    if (importer->GetStatus().GetCode() == FbxStatus::eInvalidFileVersion)
    {
      ozz::log::Err() << "FBX version number for " << _filename << " is " <<
        major << "." << minor<< "." << revision << "." << std::endl;
    }
  }

  if (initialized &&
      importer->IsFBX())
  {
    ozz::log::Log() << "FBX version number for " << _filename << " is " <<
      major << "." << minor<< "." << revision << "." << std::endl;

    // Load the scene.
    scene_ = FbxScene::Create(_manager,"ozz scene");
    bool imported = importer->Import(scene_);

    if(!imported &&     // The import file may have a password
       importer->GetStatus().GetCode() == FbxStatus::ePasswordError)
    {
      _io_settings.settings()->SetStringProp(IMP_FBX_PASSWORD, _password);
      _io_settings.settings()->SetBoolProp(IMP_FBX_PASSWORD_ENABLE, true);

      // Retries to import the scene.
      imported = importer->Import(scene_);

      if(!imported &&
         importer->GetStatus().GetCode() == FbxStatus::ePasswordError)
      {
        ozz::log::Err() << "Incorrect password." << std::endl;
      }
    }

    // Clear the scene if import failed.
    if (!imported) {
      scene_->Destroy();
      scene_ = NULL;
    }

    // Convert scene to ozz axis system (Right-handed, Y-up).
    FbxGlobalSettings& settings = scene_->GetGlobalSettings();
    const FbxAxisSystem scene_axis = settings.GetAxisSystem();
    const FbxAxisSystem ozz_axis(FbxAxisSystem::eYAxis,
                                 FbxAxisSystem::eParityOdd,
                                 FbxAxisSystem::eRightHanded);
    if (ozz_axis != scene_axis) {
      ozz_axis.ConvertScene(scene_);
    }

    // Convert scene to ozz unit system (meters).
    const FbxSystemUnit scene_unit = settings.GetSystemUnit();
    const FbxSystemUnit ozz_unit  = FbxSystemUnit::m;
    if (ozz_unit != scene_unit) {
      ozz_unit.ConvertScene(scene_);
    }
  }

  // Destroy the importer
  importer->Destroy();
}

FbxSceneLoader::~FbxSceneLoader() {
  if (scene_) {
    scene_->Destroy();
    scene_ = NULL;
  }
}

bool EvaluateDefaultLocalTransform(FbxNode* _node,
                                   ozz::math::Transform* _transform) {
  return FbxAMatrixToTransform(_node->EvaluateLocalTransform(), _transform);
}

bool FbxAMatrixToTransform(const FbxAMatrix& _matrix,
                           ozz::math::Transform* _transform) {

  const FbxVector4 translation = _matrix.GetT();
  _transform->translation = ozz::math::Float3(
    static_cast<float>(translation.mData[0]),
    static_cast<float>(translation.mData[1]),
    static_cast<float>(translation.mData[2]));

  const FbxQuaternion quaternion = _matrix.GetQ();
  _transform->rotation = ozz::math::Quaternion(
    static_cast<float>(quaternion.Buffer()[0]),
    static_cast<float>(quaternion.Buffer()[1]),
    static_cast<float>(quaternion.Buffer()[2]),
    static_cast<float>(quaternion.Buffer()[3]));

  const FbxVector4 scale = _matrix.GetS();
  _transform->scale = ozz::math::Float3(
    static_cast<float>(scale.mData[0]),
    static_cast<float>(scale.mData[1]),
    static_cast<float>(scale.mData[2]));

  return true;
}
}  // fbx
}  // ozz
}  // offline
}  // animation
