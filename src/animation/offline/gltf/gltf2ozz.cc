/*
 *
 * Ozz glTF importer
 * Author: Alexander Dzhoganov
 * Licensed under the MIT License
 *
 */

#include "ozz/animation/offline/tools/import2ozz.h"
#include "ozz/animation/runtime/skeleton.h"
#include "ozz/base/log.h"

#include <cassert>
#include <set>
#include <unordered_map>

#define TINYGLTF_IMPLEMENTATION

// No support for image loading or writing
#define TINYGLTF_NO_STB_IMAGE
#define TINYGLTF_NO_STB_IMAGE_WRITE
#define TINYGLTF_NO_EXTERNAL_IMAGE

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4702)  // unreachable code
#pragma warning(disable : 4267)  // conversion from 'size_t' to 'type'
#endif                           // _MSC_VER

#include "extern/tiny_gltf.h"

#ifdef _MSC_VER
#pragma warning(pop)
#endif  // _MSC_VER

using string = std::string;
template <typename T>
using vector = std::vector<T>;
template <typename T>
using set = std::set<T>;
template <typename K, typename V>
using unordered_map = std::unordered_map<K, V>;

class GltfImporter : public ozz::animation::offline::OzzImporter {
 public:
  GltfImporter() {
    // We don't care about image data but we have to provide this callback
    // because we're not loading the stb library
    auto image_loader = [](tinygltf::Image*, const int, std::string*,
                           std::string*, int, int, const unsigned char*, int,
                           void*) { return true; };
    m_loader.SetImageLoader(image_loader, NULL);
  }

 private:
  bool Load(const char* filename) override {
    string ext = GetFileExtension(filename);
    bool success = false;

    std::string errors;
    std::string warnings;

    // try to guess whether the input is a gltf json or a glb binary based on
    // the file extension
    if (ext == "glb") {
      success =
          m_loader.LoadBinaryFromFile(&m_model, &errors, &warnings, filename);
    } else {
      if (ext != "gltf") {
        ozz::log::Log() << "Unknown file extension '" << ext
                        << "', assuming a JSON-formatted gltf." << std::endl;
      }

      success =
          m_loader.LoadASCIIFromFile(&m_model, &errors, &warnings, filename);
    }

    // Print any errors or warnings emitted by the loader
    if (!warnings.empty()) {
      ozz::log::Log() << "glTF parsing warnings: " << warnings << std::endl;
    }

    if (!errors.empty()) {
      ozz::log::Err() << "glTF parsing errors: " << errors << std::endl;
    }

    if (success) {
      ozz::log::Log() << "glTF parsed successfully." << std::endl;
    }

    return success;
  }

  bool Import(ozz::animation::offline::RawSkeleton* skeleton,
              const NodeType& types) override {
    (void)types;

    if (m_model.scenes.empty()) {
      ozz::log::Err() << "No scenes found." << std::endl;
      return false;
    }

    if (m_model.skins.empty()) {
      ozz::log::Err() << "No skins found." << std::endl;
      return false;
    }

    // if no default scene has been set then take the first one
    // spec does not disallow gltfs without a default scene
    // but it makes more sense to keep going instead of throwing an error here
    int defaultScene = m_model.defaultScene;
    if (defaultScene == -1) {
      defaultScene = 0;
    }

    tinygltf::Scene& scene = m_model.scenes[defaultScene];
    ozz::log::Log() << "Importing from scene #" << defaultScene << " ("
                    << scene.name << ")." << std::endl;

    if (scene.nodes.empty()) {
      ozz::log::Err() << "Scene has no node." << std::endl;
      return false;
    }

    // get all the skins belonging to this scene
    auto skins = GetSkinsForScene(scene);
    if (skins.empty()) {
      ozz::log::Err() << "No skin exist in the scene." << std::endl;
      return false;
    }

    // first find the skeleton roots for each skin
    set<int> rootJoints;
    for (auto& skin : skins) {
      int rootJointIndex = FindSkinRootJointIndex(skin);
      if (rootJointIndex == -1) continue;

      rootJoints.insert(rootJointIndex);
    }

    // traverse the scene graph and record all joints starting from the roots
    for (int rootJointIndex : rootJoints) {
      ozz::animation::offline::RawSkeleton::Joint rootJoint;

      auto& rootBone = m_model.nodes[rootJointIndex];
      rootJoint.name = CreateJointName(rootJointIndex).c_str();

      if (!CreateNodeTransform(rootBone, rootJoint.transform)) {
        return false;
      }

      if (!ImportChildren(rootBone, rootJoint)) {
        return false;
      }

      skeleton->roots.push_back(std::move(rootJoint));
    }

    ozz::log::LogV() << "Printing joint hierarchy:" << std::endl;
    for (auto& root : skeleton->roots) {
      PrintSkeletonInfo(root);
    }

    if (!skeleton->Validate()) {
      ozz::log::Err()
          << "Output skeleton failed validation. This is likely a bug."
          << std::endl;
      return false;
    }

    return true;
  }

  // creates a unique name for each joint
  // ozz requires all joint names to be non-empty and unique
  string CreateJointName(int nodeIndex) {
    static unordered_map<string, int> existingNames;
    auto& node = m_model.nodes[nodeIndex];

    string name(node.name.c_str());
    if (name.length() == 0) {
      std::stringstream s;
      s << "gltf_node_" << nodeIndex;
      name = s.str();
      node.name = name;

      ozz::log::LogV() << "Joint at node #" << nodeIndex
                       << " has no name. Setting name to \"" << name << "\"."
                       << std::endl;
    }

    auto it = existingNames.find(name);
    if (it != existingNames.end()) {
      std::stringstream s;
      s << name << "_" << nodeIndex;
      name = s.str().c_str();
      node.name = name;

      ozz::log::LogV()
          << "Joint at node #" << nodeIndex << " has the same name as node #"
          << it->second
          << "This is unsupported by ozz and the joint will be renamed to \""
          << name << "\".";
    }

    existingNames.insert(std::make_pair(name, nodeIndex));
    m_nodeNames.insert(std::make_pair(nodeIndex, name));
    return name;
  }

  // given a skin find which of its joints is the skeleton root and return it
  // returns -1 if the skin has no associated joints
  int FindSkinRootJointIndex(const tinygltf::Skin& skin) {
    if (skin.joints.empty()) {
      return -1;
    }

    unordered_map<int, int> parents;
    for (int nodeIndex : skin.joints) {
      for (int childIndex : m_model.nodes[nodeIndex].children) {
        parents.insert(std::make_pair(childIndex, nodeIndex));
      }
    }

    int rootBoneIndex = skin.joints[0];
    while (parents.find(rootBoneIndex) != parents.end()) {
      rootBoneIndex = parents[rootBoneIndex];
    }

    return rootBoneIndex;
  }

  // recursively import a node's children
  bool ImportChildren(const tinygltf::Node& node,
                      ozz::animation::offline::RawSkeleton::Joint& parent) {
    for (int childIndex : node.children) {
      tinygltf::Node& childNode = m_model.nodes[childIndex];

      ozz::animation::offline::RawSkeleton::Joint joint;
      joint.name = CreateJointName(childIndex).c_str();
      if (!CreateNodeTransform(childNode, joint.transform)) {
        return false;
      }

      if (!ImportChildren(childNode, joint)) {
        return false;
      }

      parent.children.push_back(std::move(joint));
    }

    return true;
  }

  // returns all animations in the gltf
  AnimationNames GetAnimationNames() override {
    AnimationNames animNames;

    for (auto& animation : m_model.animations) {
      if (animation.name.length() == 0) {
        ozz::log::LogV() << "Found an animation without a name. All animations "
                            "must have valid and unique names. The animation "
                            "will be skipped."
                         << std::endl;
        continue;
      }

      animNames.push_back(animation.name.c_str());
    }

    return animNames;
  }

  bool Import(const char* animationName,
              const ozz::animation::Skeleton& skeleton, float samplingRate,
              ozz::animation::offline::RawAnimation* animation) override {
    if (samplingRate == 0.0f) {
      samplingRate = 60.0f;

      static bool samplingRateWarn = false;
      if (!samplingRateWarn) {
        ozz::log::Log() << "The animation sampling rate is set to 0 "
                           "(automatic) but glTF does not carry scene frame "
                           "rate information. Assuming a sampling rate of "
                        << samplingRate << "hz." << std::endl;

        samplingRateWarn = true;
      }
    }

    // find the corresponding gltf animation
    auto animationIt =
        std::find_if(begin(m_model.animations), end(m_model.animations),
                     [animationName](const tinygltf::Animation& animation) {
                       return animation.name == animationName;
                     });

    // this shouldn't be possible but check anyway
    if (animationIt == end(m_model.animations)) {
      ozz::log::Err() << "Animation '" << animationName
                      << "' requested not found in glTF." << std::endl;
      return false;
    }

    auto& gltfAnimation = *animationIt;

    animation->name = gltfAnimation.name.c_str();

    // animation duration is determined during sampling from the duration of the
    // longest channel
    animation->duration = 0.0f;

    int numJoints = skeleton.num_joints();
    animation->tracks.resize(numJoints);

    // gltf stores animations by splitting them in channels
    // where each channel targets a node's property i.e. translation, rotation
    // or scale ozz expects animations to be stored per joint we create a map
    // where we record the associated channels for each joint
    unordered_map<string, vector<tinygltf::AnimationChannel>> channelsPerJoint;
    for (auto& channel : gltfAnimation.channels) {
      if (channel.target_node == -1) {
        continue;
      }

      auto& targetNode = m_model.nodes[channel.target_node];
      channelsPerJoint[targetNode.name].push_back(channel);
    }

    auto jointNames = skeleton.joint_names();

    // for each joint get all its associated channels, sample them and record
    // the samples in the joint track
    for (int i = 0; i < numJoints; i++) {
      auto& channels = channelsPerJoint[jointNames[i]];
      auto& track = animation->tracks[i];

      for (auto& channel : channels) {
        auto& sampler = gltfAnimation.samplers[channel.sampler];
        if (!SampleAnimationChannel(sampler, channel.target_path,
                                    animation->duration, track, samplingRate)) {
          return false;
        }
      }

      auto node = FindNodeByName(jointNames[i]);
      assert(node != nullptr);

      // pad the bind pose transform for any joints which do not have an
      // associated channel for this animation
      if (track.translations.empty()) {
        track.translations.push_back(CreateTranslationBindPoseKey(*node));
      }
      if (track.rotations.empty()) {
        track.rotations.push_back(CreateRotationBindPoseKey(*node));
      }
      if (track.scales.empty()) {
        track.scales.push_back(CreateScaleBindPoseKey(*node));
      }
    }

    ozz::log::LogV() << "Processed animation '" << animation->name
                     << "' (tracks: " << animation->tracks.size()
                     << ", duration: " << animation->duration << "s)."
                     << std::endl;

    if (!animation->Validate()) {
      ozz::log::Err() << "Animation '" << animation->name
                      << "' failed validation." << std::endl;
      return false;
    }

    return true;
  }

  bool SampleAnimationChannel(
      const tinygltf::AnimationSampler& sampler, const string& targetPath,
      float& outDuration,
      ozz::animation::offline::RawAnimation::JointTrack& track,
      float samplingRate) {
    auto& input = m_model.accessors[sampler.input];
    assert(input.maxValues.size() == 1);

    // the max[0] property of the input accessor is the animation duration
    // this is required to be present by the spec:
    // "Animation Sampler's input accessor must have min and max properties
    // defined."
    const float duration = static_cast<float>(input.maxValues[0]);

    // if this channel's duration is larger than the animation's duration
    // then increase the animation duration to match
    if (duration > outDuration) {
      outDuration = duration;
    }

    assert(input.type == TINYGLTF_TYPE_SCALAR);
    auto& output = m_model.accessors[sampler.output];
    assert(output.type == TINYGLTF_TYPE_VEC3 ||
           output.type == TINYGLTF_TYPE_VEC4);

    const float* timestamps = BufferView<float>(input);
    if (timestamps == nullptr) {
      return false;
    }

    if (sampler.interpolation.empty()) {
      ozz::log::Err() << "Invalid sampler interpolation." << std::endl;
      return false;
    } else if (sampler.interpolation == "LINEAR") {
      assert(input.count == output.count);

      if (targetPath == "translation") {
        return SampleLinearChannel(output, timestamps, track.translations);
      } else if (targetPath == "rotation") {
        return SampleLinearChannel(output, timestamps, track.rotations);
      } else if (targetPath == "scale") {
        return SampleLinearChannel(output, timestamps, track.scales);
      }
      ozz::log::Err() << "Invalid or unknown channel target path '"
                      << targetPath << "'." << std::endl;
      return false;
    } else if (sampler.interpolation == "STEP") {
      assert(input.count == output.count);

      if (targetPath == "translation") {
        return SampleStepChannel(output, timestamps, track.translations);
      } else if (targetPath == "rotation") {
        return SampleStepChannel(output, timestamps, track.rotations);
      } else if (targetPath == "scale") {
        return SampleStepChannel(output, timestamps, track.scales);
      }

      ozz::log::Err() << "Invalid or unknown channel target path '"
                      << targetPath << "'." << std::endl;
      return false;
    } else if (sampler.interpolation == "CUBICSPLINE") {
      assert(input.count * 3 == output.count);

      if (targetPath == "translation") {
        return SampleCubicSplineChannel(output, timestamps, track.translations,
                                        samplingRate, duration);
      } else if (targetPath == "rotation") {
        if (!SampleCubicSplineChannel(output, timestamps, track.rotations,
                                      samplingRate, duration))
          return false;

        // normalize all resulting quaternions per spec
        for (auto& key : track.rotations) {
          key.value = ozz::math::Normalize(key.value);
        }

        return true;
      } else if (targetPath == "scale") {
        return SampleCubicSplineChannel(output, timestamps, track.scales,
                                        samplingRate, duration);
      }

      ozz::log::Err() << "Invalid or unknown channel target path '"
                      << targetPath << "'." << std::endl;
      return false;
    }

    ozz::log::Err() << "Invalid or unknown interpolation type '"
                    << sampler.interpolation << "'." << std::endl;
    return false;
  }

  // samples a linear animation channel
  // there is an exact mapping between gltf and ozz keyframes so we just copy
  // everything over
  template <typename KeyType>
  bool SampleLinearChannel(
      const tinygltf::Accessor& output, const float* timestamps,
      std::vector<KeyType, ozz::StdAllocator<KeyType>>& keyframes) {
    using ValueType = typename KeyType::Value;
    const ValueType* values = BufferView<ValueType>(output);
    if (values == nullptr) {
      return false;
    }

    keyframes.resize(output.count);

    for (size_t i = 0u; i < output.count; i++) {
      KeyType& key = keyframes[i];
      key.time = timestamps[i];
      key.value = values[i];
    }

    return true;
  }

  // samples a step animation channel
  // there are twice as many ozz keyframes as gltf keyframes
  template <typename KeyType>
  bool SampleStepChannel(
      const tinygltf::Accessor& output, const float* timestamps,
      std::vector<KeyType, ozz::StdAllocator<KeyType>>& keyframes) {
    using ValueType = typename KeyType::Value;
    const ValueType* values = BufferView<ValueType>(output);
    if (values == nullptr) {
      return false;
    }

    // A step is created with 2 consecutive keys. Last step is a single key.
    size_t numKeyframes = output.count * 2 - 1;
    keyframes.resize(numKeyframes);

    const float eps = 1e-6f;

    for (size_t i = 0; i < output.count; i++) {
      KeyType& key = keyframes[i * 2];
      key.time = timestamps[i];
      key.value = values[i];

      if (i < output.count - 1) {
        KeyType& nextKey = keyframes[i * 2 + 1];
        nextKey.time = timestamps[i + 1] - eps;
        nextKey.value = values[i];
      }
    }

    return true;
  }

  // samples a cubic-spline channel
  // the number of keyframes is determined from the animation duration and given
  // sample rate
  template <typename KeyType>
  bool SampleCubicSplineChannel(
      const tinygltf::Accessor& output, const float* timestamps,
      std::vector<KeyType, ozz::StdAllocator<KeyType>>& keyframes,
      float samplingRate, float duration) {
    using ValueType = typename KeyType::Value;
    const ValueType* values = BufferView<ValueType>(output);
    if (values == nullptr) {
      return false;
    }

    assert(output.count % 3 == 0);
    size_t numKeyframes = output.count / 3;

    // TODO check size matches
    keyframes.resize(static_cast<size_t>(floor(duration * samplingRate) + 1.f));
    size_t currentKey = 0;

    for (size_t i = 0; i < keyframes.size(); i++) {
      float time = (float)i / samplingRate;
      while (timestamps[currentKey] > time && currentKey < numKeyframes - 1)
        currentKey++;

      float currentTime = timestamps[currentKey];   // current keyframe time
      float nextTime = timestamps[currentKey + 1];  // next keyframe time

      float t = (time - currentTime) / (nextTime - currentTime);
      const ValueType& p0 = values[currentKey * 3 + 1];
      const ValueType m0 =
          values[currentKey * 3 + 2] * (nextTime - currentTime);
      const ValueType& p1 = values[(currentKey + 1) * 3 + 1];
      const ValueType m1 =
          values[(currentKey + 1) * 3] * (nextTime - currentTime);

      KeyType& key = keyframes[i];
      key.time = time;
      key.value = SampleHermiteSpline(t, p0, m0, p1, m1);
    }

    return true;
  }

  // samples a hermite spline in the form
  // p(t) = (2t^3 - 3t^2 + 1)p0 + (t^3 - 2t^2 + t)m0 + (-2t^3 + 3t^2)p1 + (t^3 -
  // t^2)m1 where t is a value between 0 and 1 p0 is the starting point at t = 0
  // m0 is the scaled starting tangent at t = 0
  // p1 is the ending point at t = 1
  // m1 is the scaled ending tangent at t = 1
  // p(t) is the resulting point value
  template <typename T>
  T SampleHermiteSpline(float t, const T& p0, const T& m0, const T& p1,
                        const T& m1) {
    float t2 = t * t;
    float t3 = t2 * t;

    // a = 2t^3 - 3t^2 + 1
    float a = 2.0f * t3 - 3.0f * t2 + 1.0f;
    // b = t^3 - 2t^2 + t
    float b = t3 - 2.0f * t2 + t;
    // c = -2t^3 + 3t^2
    float c = -2.0f * t3 + 3.0f * t2;
    // d = t^3 - t^2
    float d = t3 - t2;

    // p(t) = a * p0 + b * m0 + c * p1 + d * m1
    T pt = p0 * a + m0 * b + p1 * c + m1 * d;
    return pt;
  }

  // create the default transform for a gltf node
  bool CreateNodeTransform(const tinygltf::Node& node,
                           ozz::math::Transform& transform) {
    if (node.matrix.size() != 0) {
      // For animated nodes matrix should never be set
      // From the spec: "When a node is targeted for animation (referenced by an
      // animation.channel.target), only TRS properties may be present; matrix
      // will not be present."
      ozz::log::Err() << "Node \"" << node.name
                      << "\" transformation matrix is not empty. This is "
                         "disallowed by the glTF spec as this node is an "
                         "animation target."
                      << std::endl;
      return false;
    }

    transform = ozz::math::Transform::identity();

    if (!node.translation.empty()) {
      transform.translation =
          ozz::math::Float3(static_cast<float>(node.translation[0]),
                            static_cast<float>(node.translation[1]),
                            static_cast<float>(node.translation[2]));
    }
    if (!node.rotation.empty()) {
      transform.rotation =
          ozz::math::Quaternion(static_cast<float>(node.rotation[0]),
                                static_cast<float>(node.rotation[1]),
                                static_cast<float>(node.rotation[2]),
                                static_cast<float>(node.rotation[3]));
    }
    if (!node.scale.empty()) {
      transform.scale = ozz::math::Float3(static_cast<float>(node.scale[0]),
                                          static_cast<float>(node.scale[1]),
                                          static_cast<float>(node.scale[2]));
    }

    return true;
  }

  ozz::animation::offline::RawAnimation::TranslationKey
  CreateTranslationBindPoseKey(const tinygltf::Node& node) {
    ozz::animation::offline::RawAnimation::TranslationKey key;
    key.time = 0.0f;
    key.value = ozz::math::Float3::zero();

    if (!node.translation.empty()) {
      key.value = ozz::math::Float3(static_cast<float>(node.translation[0]),
                                    static_cast<float>(node.translation[1]),
                                    static_cast<float>(node.translation[2]));
    }

    return key;
  }

  ozz::animation::offline::RawAnimation::RotationKey CreateRotationBindPoseKey(
      const tinygltf::Node& node) {
    ozz::animation::offline::RawAnimation::RotationKey key;
    key.time = 0.0f;
    key.value = ozz::math::Quaternion::identity();

    if (!node.rotation.empty()) {
      key.value = ozz::math::Quaternion(static_cast<float>(node.rotation[0]),
                                        static_cast<float>(node.rotation[1]),
                                        static_cast<float>(node.rotation[2]),
                                        static_cast<float>(node.rotation[3]));
    }
    return key;
  }

  ozz::animation::offline::RawAnimation::ScaleKey CreateScaleBindPoseKey(
      const tinygltf::Node& node) {
    ozz::animation::offline::RawAnimation::ScaleKey key;
    key.time = 0.0f;
    key.value = ozz::math::Float3::one();

    if (!node.scale.empty()) {
      key.value = ozz::math::Float3(static_cast<float>(node.scale[0]),
                                    static_cast<float>(node.scale[1]),
                                    static_cast<float>(node.scale[2]));
    }
    return key;
  }

  // returns all skins belonging to a given gltf scene
  vector<tinygltf::Skin> GetSkinsForScene(const tinygltf::Scene& scene) const {
    set<int> open;
    set<int> found;

    for (int nodeIndex : scene.nodes) open.insert(nodeIndex);

    while (!open.empty()) {
      int nodeIndex = *open.begin();
      found.insert(nodeIndex);
      open.erase(nodeIndex);

      auto& node = m_model.nodes[nodeIndex];
      for (int childIndex : node.children) open.insert(childIndex);
    }

    vector<tinygltf::Skin> skins;
    for (auto& skin : m_model.skins)
      if (!skin.joints.empty() && found.find(skin.joints[0]) != found.end())
        skins.push_back(skin);

    return skins;
  }

  const tinygltf::Node* FindNodeByName(const string& name) {
    for (size_t nodeIndex = 0u; nodeIndex < m_model.nodes.size(); nodeIndex++) {
      auto it = m_nodeNames.find(nodeIndex);
      if (it == m_nodeNames.end()) {
        continue;
      }

      if (it->second == name) {
        return &m_model.nodes[nodeIndex];
      }
    }

    return nullptr;
  }

  // returns the address of a gltf buffer given an accessor
  // performs basic checks to ensure the data is in the correct format
  template <typename T>
  const T* BufferView(const tinygltf::Accessor& accessor) {
    int32_t componentSize =
        tinygltf::GetComponentSizeInBytes(accessor.componentType);
    int32_t elementSize =
        componentSize * tinygltf::GetTypeSizeInBytes(accessor.type);
    if (elementSize != sizeof(T)) {
      ozz::log::Err() << "Invalid buffer view access. Expected element size '"
                      << sizeof(T) << " got " << elementSize << " instead."
                      << std::endl;
      return nullptr;
    }

    auto& bufferView = m_model.bufferViews[accessor.bufferView];
    auto& buffer = m_model.buffers[bufferView.buffer];
    return reinterpret_cast<const T*>(
        buffer.data.data() + bufferView.byteOffset + accessor.byteOffset);
  }

  void PrintSkeletonInfo(
      const ozz::animation::offline::RawSkeleton::Joint& joint, int ident = 0) {
    for (int i = 0; i < ident; i++) {
      ozz::log::Log() << "  ";
    }
    ozz::log::Log() << joint.name << std::endl;

    for (auto& child : joint.children) {
      PrintSkeletonInfo(child, ident + 1);
    }
  }

  string GetFileExtension(const string& path) {
    if (path.find_last_of(".") != string::npos) {
      return path.substr(path.find_last_of(".") + 1);
    }
    return "";
  }

  // no support for user-defined tracks
  NodeProperties GetNodeProperties(const char*) override {
    return NodeProperties();
  }
  bool Import(const char*, const char*, const char*, NodeProperty::Type, float,
              ozz::animation::offline::RawFloatTrack*) override {
    return false;
  }
  bool Import(const char*, const char*, const char*, NodeProperty::Type, float,
              ozz::animation::offline::RawFloat2Track*) override {
    return false;
  }
  bool Import(const char*, const char*, const char*, NodeProperty::Type, float,
              ozz::animation::offline::RawFloat3Track*) override {
    return false;
  }
  bool Import(const char*, const char*, const char*, NodeProperty::Type, float,
              ozz::animation::offline::RawFloat4Track*) override {
    return false;
  }

  tinygltf::TinyGLTF m_loader;
  tinygltf::Model m_model;

  unordered_map<size_t, string> m_nodeNames;
};

int main(int _argc, const char** _argv) {
  GltfImporter converter;
  return converter(_argc, _argv);
}
