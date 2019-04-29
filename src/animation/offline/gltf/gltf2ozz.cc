/*
 *
 * A (mostly) spec-compiliant glTF2ozz importer
 * Author: Alexander Dzhoganov
 * Licensed under the MIT License
 * 
 * Limitations:
 * - One skeleton per file
 * - No morphing
 * - No cubic spline interpolation
 * 
 */

#include <cassert>
#include <unordered_map>
#include <set>

#include "ozz/base/log.h"
#include "ozz/animation/offline/tools/import2ozz.h"
#include "ozz/animation/runtime/skeleton.h"

#define TINYGLTF_IMPLEMENTATION
#define TINYGLTF_NO_STB_IMAGE        //
#define TINYGLTF_NO_STB_IMAGE_WRITE  // skip loading the image libraries
#define TINYGLTF_NO_EXTERNAL_IMAGE   //

#include "tiny_gltf.h"

using namespace std;
using namespace ozz;
using namespace ozz::animation::offline;

class GltfImporter : public animation::offline::OzzImporter
{
  using RawSkeleton = ozz::animation::offline::RawSkeleton;
  using Skeleton = ozz::animation::Skeleton;

  public:
  GltfImporter()
  {
    // we don't care about image data but we have to provide this callback because we're not loading the stb library
    m_loader.SetImageLoader(&GltfImporter::LoadImageData, 0);
  }

  bool Load(const char* filename) override
  {
    auto ext = GetFileExtension(filename);
    bool success = false;

    std::string errors;
    std::string warnings;

    // try to guess whether the input is a gltf json or a glb binary based on the file extension
    if (ext == "glb")
    {
      success = m_loader.LoadBinaryFromFile(&m_model, &errors, &warnings, filename);
    }
    else
    {
      if (ext != "gltf")
        log::Log() << "\nWarning: Unknown file extension '" << ext << "', assuming .gltf";

      success = m_loader.LoadASCIIFromFile(&m_model, &errors, &warnings, filename);
    }

    // print any errors or warnings emitted by the loader
    if (!warnings.empty())
      log::Log() << "\nWarning: " << warnings << endl;

    if (!errors.empty())
      log::Err() << "\nError: " << errors << endl;

    log::Log() << "glTF parsed successfully." << endl;
    return success;
  }

  bool Import(RawSkeleton* skeleton, const NodeType& types) override
  {
    if (m_model.scenes.empty())
    {
      log::Err() << "\nError: No scenes found, bailing out." << endl;
      return false;
    }

    if (m_model.animations.empty())
    {
      log::Err() << "\nError: No animations found, bailing out." << endl;
      return false;
    }

    if (m_model.skins.empty())
    {
      log::Err() << "\nError: No skins found, bailing out." << endl;
      return false;
    }

    // if no default scene has been set just take the first one
    // spec does not disallow gltf's without a default scene
    // but it makes more sense to just keep going instead of throwing an error here
    auto defaultScene = m_model.defaultScene;
    if (defaultScene == -1)
      defaultScene = 0;

    auto& scene = m_model.scenes[defaultScene];
    log::Log() << "Importing from scene '" << scene.name << "'." << endl;

    if (scene.nodes.empty())
    {
      log::Err() << "\nError: Scene has no nodes, bailing out." << endl;
      return false;
    }

    if (scene.nodes.size() != 1)
    {
      log::Log() <<
        "\nWarning: Scene has more than one root node. Only the first one will participate in the import."
      << endl;
      log::Log() << "Listing root nodes:" << endl;

      for (auto i = 0u; i < scene.nodes.size(); i++)
        log::Log() << "* " << m_model.nodes[i].name << (i == 0 ? " [ will be imported ]" : "") << endl;
    }

    auto skins = GetSkinsForScene(scene);
    if (skins.empty())
    {
      log::Err() <<
        "\nError: No skins exist in the scene, bailing out."
      << endl;
      return false;
    }

    auto& skin = skins[0];
    unordered_map<int, int> parents;
    for (auto nodeIndex : skin.joints)
      for (auto childIndex : m_model.nodes[nodeIndex].children)
        parents.insert(make_pair(childIndex, nodeIndex));

    auto rootBoneIndex = skins[0].joints[0];
    while (parents.find(rootBoneIndex) != parents.end())
      rootBoneIndex = parents[rootBoneIndex];
  
    auto& rootBone = m_model.nodes[rootBoneIndex];

    log::Log() <<
      "Determined '" << rootBone.name << "' (id = " << rootBoneIndex << ") to be the root bone."
    << endl;

    RawSkeleton::Joint rootJoint;
    rootJoint.name = rootBone.name.c_str();

    if (!CreateTransform(rootBone, rootJoint.transform))
      return false;

    if (!ImportChildren(rootBone, rootJoint))
      return false;

    log::Log() << "Printing joint hierarchy:" << endl;

    skeleton->roots.push_back(move(rootJoint));
    PrintSkeletonInfo(skeleton->roots[0]);

    if (!skeleton->Validate())
    {
      log::Err() <<
        "\nError: Output skeleton failed validation.\n"
        "This is likely a bug."
      << endl;
      return false;
    }

    return true;
  }

  bool ImportChildren(const tinygltf::Node& node, RawSkeleton::Joint& parent)
  {
    for (auto childIndex : node.children)
    {
      auto& childNode = m_model.nodes[childIndex];

      RawSkeleton::Joint joint;
      joint.name = childNode.name.c_str();
      if (!CreateTransform(childNode, joint.transform))
        return false;

      if (!ImportChildren(childNode, joint))
        return false;

      parent.children.push_back(move(joint));
    }

    return true;
  }

  AnimationNames GetAnimationNames() override
  {
    AnimationNames animNames;

    for (auto& animation : m_model.animations)
      animNames.push_back(animation.name.c_str());

    return animNames;
  }

  bool Import(const char* animationName, const Skeleton& skeleton, float samplingRate, RawAnimation* ozzAnimation) override
  {
    tinygltf::Animation* animation = nullptr;
    for (auto& anim : m_model.animations)
    {
      if (anim.name != animationName)
        continue;

      animation = &anim;
      break;
    }

    if (animation == nullptr)
    {
      log::Err() <<
        "\nError: Animation '" << animationName << "' requested but not found in glTF.\n"
        "This is a bug."
      << endl;
      return false;
    }

    unordered_map<string, vector<tinygltf::AnimationChannel>> channelsPerJoint;
    for (auto& channel : animation->channels)
    {
      if (channel.target_node == -1)
        continue;
      
      auto& targetNode = m_model.nodes[channel.target_node];
      channelsPerJoint[targetNode.name].push_back(channel);
    }

    auto numJoints = skeleton.num_joints();
    float duration = 0.0f;

    ozz::Vector<RawAnimation::JointTrack>::Std tracks;
    tracks.resize(numJoints);

    auto jointNames = skeleton.joint_names();
    for (auto i = 0; i < numJoints; i++)
    {
      auto& channels = channelsPerJoint[jointNames[i]];
      auto& track = tracks[i];

      for (auto& channel : channels)
      {
        auto& sampler = animation->samplers[channel.sampler];
        auto& input = m_model.accessors[sampler.input];
        assert(input.maxValues.size() == 1);

        assert(input.type == TINYGLTF_TYPE_SCALAR);
        auto& output = m_model.accessors[sampler.output];
        assert(output.type == TINYGLTF_TYPE_VEC3 || output.type == TINYGLTF_TYPE_VEC4);
        
        if (sampler.interpolation == "CUBICSPLINE")
        {
          log::Err() <<
            "\nError: Cubic spline interpolation is not supported. "
            "All animations need to be baked to linear sampling.\n"
            "If you are exporting from Blender make sure to check "
            "'Always Sample Animations' in the glTF exporter settings.\n"
          << endl;

          return false;
        }

        if (input.count != output.count)
        {
          log::Err() <<
            "\nError: Mismatched accessor count. This is a bug or the glTF is invalid."
          << endl;
          return false;
        }

        auto timestamps = BufferView<float>(input);
        for (auto i = 0u; i < input.count; i++)
          if (timestamps[i] > duration)
            duration = timestamps[i];

        if (channel.target_path == "translation")
        {
          auto values = BufferView<math::Float3>(output);
          track.translations.resize(input.count);

          for (auto i = 0u; i < input.count; i++)
          {
            auto& key = track.translations[i];
            key.time = timestamps[i];
            key.value = values[i];
          }
        }
        else if (channel.target_path == "rotation")
        {
          auto values = BufferView<math::Quaternion>(output);
          track.rotations.resize(input.count);

          for (auto i = 0u; i < input.count; i++)
          {
            auto& key = track.rotations[i];
            key.time = timestamps[i];
            key.value = values[i];
          }
        }
        else if (channel.target_path == "scale")
        {
          auto values = BufferView<math::Float3>(output);
          track.scales.resize(input.count);

          for (auto i = 0u; i < input.count; i++)
          {
            auto& key = track.scales[i];
            key.time = timestamps[i];
            key.value = values[i];
          }
        }
        else if (channel.target_path == "weights")
        {
          auto& node = m_model.nodes[channel.target_node];
          log::Err() <<
            "\nError: Found 'weights' channel on node '" << node.name << "' (id = " << channel.target_node << ")\n"
            "Morphing is currently not supported."
          << endl;
          return false;
        }
        else
        {
          log::Err() <<
            "\nError: Unsupported channel target path '" << channel.target_path << "'"
          << endl;
          return false;
        }
      }
    }
    
    auto padStart = [this](auto& track, auto& node)
    {
      if (track.empty())
        track.push_back(GetDefaultKeyValue(track, node));
    };

    for (auto i = 0u; i < tracks.size(); i++)
    {
      auto& track = tracks[i];
      auto& jointName = jointNames[i];

      tinygltf::Node* node = nullptr;
      for (auto& node_ : m_model.nodes)
      {
        if (node_.name == jointName)
        {
          node = &node_;
          break;
        }
      }

      assert(node != nullptr);

      padStart(track.translations, *node);
      padStart(track.rotations, *node);
      padStart(track.scales, *node);
    }

    ozzAnimation->tracks = move(tracks);
    ozzAnimation->name = animation->name.c_str();
    ozzAnimation->duration = duration;

    if (!ozzAnimation->Validate())
    {
      log::Err() <<
        "\nError: Animation '" << ozzAnimation->name << "' failed to validate.\n"
        "This is likely a bug."
      << endl;
      return false;
    }

    log::Log() <<
      "Processed animation '" << ozzAnimation->name <<
      "' (tracks: " << ozzAnimation->tracks.size() << ", duration: " << duration << "s)."
    << endl;
    
    return true;
  }

  bool CreateTransform(const tinygltf::Node& node, math::Transform& transform)
  {
    transform = math::Transform::identity();

    if (!node.translation.empty())
      transform.translation = math::Float3(node.translation[0], node.translation[1], node.translation[2]);

    if (!node.rotation.empty())
      transform.rotation = math::Quaternion(node.rotation[0], node.rotation[1], node.rotation[2], node.rotation[3]);

    if (!node.scale.empty())
      transform.scale = math::Float3(node.scale[0], node.scale[1], node.scale[2]);

    if (node.matrix.size() != 0)
    {
      // For animated nodes matrix should never be set
      // From the spec: "When a node is targeted for animation (referenced by an animation.channel.target),
      // only TRS properties may be present; matrix will not be present."
      log::Err() <<
        "\nWarning: Node '" << node.name << "' transformation matrix is not empty.\n"
        "This is disallowed by the glTF spec as this node is an animation target. The matrix will be ignored."
      << endl;
    }

    return true;
  }

  RawAnimation::TranslationKey GetDefaultKeyValue(const RawAnimation::JointTrack::Translations&, const tinygltf::Node& node)
  {
    RawAnimation::TranslationKey key;
    key.time = 0.0f;
    if (node.translation.empty())
      key.value = math::Float3::zero();
    else
      key.value = math::Float3(node.translation[0], node.translation[1], node.translation[2]);
    return key;
  }

  RawAnimation::RotationKey GetDefaultKeyValue(const RawAnimation::JointTrack::Rotations&, const tinygltf::Node& node)
  {
    RawAnimation::RotationKey key;
    key.time = 0.0f;
    if (node.rotation.empty())
      key.value = math::Quaternion::identity();
    else
      key.value = math::Quaternion(node.rotation[0], node.rotation[1], node.rotation[2], node.rotation[3]);
    return key;
  }

  RawAnimation::ScaleKey GetDefaultKeyValue(const RawAnimation::JointTrack::Scales&, const tinygltf::Node& node)
  {
    RawAnimation::ScaleKey key;
    key.time = 0.0f;
    if (node.scale.empty())
      key.value = math::Float3::one();
    else
      key.value = math::Float3(node.scale[0], node.scale[1], node.scale[2]);
    return key;
  }

  template <typename T>
  T* BufferView(const tinygltf::Accessor& accessor)
  {
    auto componentSize = tinygltf::GetComponentSizeInBytes(accessor.componentType);
    auto elementSize = componentSize * tinygltf::GetTypeSizeInBytes(accessor.type);
    if (elementSize != sizeof(T))
    {
      log::Err{} <<
        "Invalid buffer view access. Expected element size '" << sizeof(T) << 
        "but got " << elementSize << "instaead.\n"
        "This is a bug.\n"
      << endl;
      return nullptr;
    }

    auto& bufferView = m_model.bufferViews[accessor.bufferView];
    auto& buffer = m_model.buffers[bufferView.buffer];
    return (T*)(buffer.data.data() + bufferView.byteOffset + accessor.byteOffset);
  }

  vector<tinygltf::Skin> GetSkinsForScene(const tinygltf::Scene& scene) const
  {
    set<int> open;
    set<int> existsInScene;

    for (auto nodeIndex : scene.nodes)
      open.insert(nodeIndex);

    while (!open.empty())
    {
      auto nodeIndex = *open.begin();
      existsInScene.insert(nodeIndex);
      open.erase(nodeIndex);

      auto& node = m_model.nodes[nodeIndex];
      for (auto childIndex : node.children)
        open.insert(childIndex);
    }

    vector<tinygltf::Skin> skins;
    for (auto& skin : m_model.skins)
      if (!skin.joints.empty() && existsInScene.find(skin.joints[0]) != existsInScene.end())
        skins.push_back(skin);

    return skins;
  }

  void PrintSkeletonInfo(const RawSkeleton::Joint& joint, int ident = 0)
  {
    for (auto i = 0; i < ident; i++)
      log::Log() << " ";
    log::Log() << joint.name << endl;

    for (auto& child : joint.children)
      PrintSkeletonInfo(child, ident + 2);
  }

  std::string GetFileExtension(const std::string& path)
  {
    if(path.find_last_of(".") != std::string::npos)
      return path.substr(path.find_last_of(".")+1);
    return "";
  }

  // no support for user-defined tracks
  NodeProperties GetNodeProperties(const char* _node_name) override { return NodeProperties(); }
  bool Import(const char*, const char*, const char*, float, RawFloatTrack*)  override { return false; }
  bool Import(const char*, const char*, const char*, float, RawFloat2Track*) override { return false; }
  bool Import(const char*, const char*, const char*, float, RawFloat3Track*) override { return false; }
  bool Import(const char*, const char*, const char*, float, RawFloat4Track*) override { return false; }

  static bool LoadImageData(tinygltf::Image*, const int, std::string*,
    std::string*, int, int, const unsigned char*, int, void*)
  {
    return true;
  }
  
  private:
  tinygltf::TinyGLTF m_loader;
  tinygltf::Model m_model;
};

int main(int argc, const char** argv)
{
  return GltfImporter()(argc, argv);
}
