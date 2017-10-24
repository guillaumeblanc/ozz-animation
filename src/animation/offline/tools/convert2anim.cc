//----------------------------------------------------------------------------//
//                                                                            //
// ozz-animation is hosted at http://github.com/guillaumeblanc/ozz-animation  //
// and distributed under the MIT License (MIT).                               //
//                                                                            //
// Copyright (c) 2017 Guillaume Blanc                                         //
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

#include "ozz/animation/offline/tools/convert2anim.h"

#include <cstdlib>
#include <cstring>

#include "animation/offline/tools/configuration.h"

#include "ozz/animation/offline/additive_animation_builder.h"
#include "ozz/animation/offline/animation_builder.h"
#include "ozz/animation/offline/animation_optimizer.h"
#include "ozz/animation/offline/raw_animation.h"
#include "ozz/animation/offline/raw_skeleton.h"
#include "ozz/animation/offline/raw_track.h"
#include "ozz/animation/offline/skeleton_builder.h"
#include "ozz/animation/offline/track_builder.h"
#include "ozz/animation/offline/track_optimizer.h"

#include "ozz/animation/runtime/animation.h"
#include "ozz/animation/runtime/skeleton.h"
#include "ozz/animation/runtime/track.h"

#include "ozz/base/io/archive.h"
#include "ozz/base/io/stream.h"

#include "ozz/base/log.h"

#include "ozz/options/options.h"

// Declares command line options.
OZZ_OPTIONS_DECLARE_STRING(file, "Specifies input file", "", true)
OZZ_OPTIONS_DECLARE_STRING(skeleton,
                           "Specifies ozz skeleton (raw or runtime) input file",
                           "", true)

static bool ValidateEndianness(const ozz::options::Option& _option,
                               int /*_argc*/) {
  const ozz::options::StringOption& option =
      static_cast<const ozz::options::StringOption&>(_option);
  bool valid = std::strcmp(option.value(), "native") == 0 ||
               std::strcmp(option.value(), "little") == 0 ||
               std::strcmp(option.value(), "big") == 0;
  if (!valid) {
    ozz::log::Err() << "Invalid endianess option." << std::endl;
  }
  return valid;
}

OZZ_OPTIONS_DECLARE_STRING_FN(
    endian,
    "Selects output endianness mode. Can be \"native\" (same as current "
    "platform), \"little\" or \"big\".",
    "native", false, &ValidateEndianness)

ozz::Endianness Endianness() {
  // Initializes output endianness from options.
  ozz::Endianness endianness = ozz::GetNativeEndianness();
  if (std::strcmp(OPTIONS_endian, "little")) {
    endianness = ozz::kLittleEndian;
  } else if (std::strcmp(OPTIONS_endian, "big")) {
    endianness = ozz::kBigEndian;
  }
  ozz::log::LogV() << (endianness == ozz::kLittleEndian ? "Little" : "Big")
                   << " Endian output binary format selected." << std::endl;
  return endianness;
}

static bool ValidateLogLevel(const ozz::options::Option& _option,
                             int /*_argc*/) {
  const ozz::options::StringOption& option =
      static_cast<const ozz::options::StringOption&>(_option);
  bool valid = std::strcmp(option.value(), "verbose") == 0 ||
               std::strcmp(option.value(), "standard") == 0 ||
               std::strcmp(option.value(), "silent") == 0;
  if (!valid) {
    ozz::log::Err() << "Invalid log level option." << std::endl;
  }
  return valid;
}

OZZ_OPTIONS_DECLARE_STRING_FN(
    log_level,
    "Selects log level. Can be \"silent\", \"standard\" or \"verbose\".",
    "standard", false, &ValidateLogLevel)

namespace ozz {
namespace animation {
namespace offline {
namespace {

void DisplaysOptimizationstatistics(const RawAnimation& _non_optimized,
                                    const RawAnimation& _optimized) {
  size_t opt_translations = 0, opt_rotations = 0, opt_scales = 0;
  for (size_t i = 0; i < _optimized.tracks.size(); ++i) {
    const RawAnimation::JointTrack& track = _optimized.tracks[i];
    opt_translations += track.translations.size();
    opt_rotations += track.rotations.size();
    opt_scales += track.scales.size();
  }
  size_t non_opt_translations = 0, non_opt_rotations = 0, non_opt_scales = 0;
  for (size_t i = 0; i < _non_optimized.tracks.size(); ++i) {
    const RawAnimation::JointTrack& track = _non_optimized.tracks[i];
    non_opt_translations += track.translations.size();
    non_opt_rotations += track.rotations.size();
    non_opt_scales += track.scales.size();
  }

  // Computes optimization ratios.
  float translation_ratio =
      non_opt_translations != 0
          ? 100.f * (non_opt_translations - opt_translations) /
                non_opt_translations
          : 0;
  float rotation_ratio =
      non_opt_rotations != 0
          ? 100.f * (non_opt_rotations - opt_rotations) / non_opt_rotations
          : 0;
  float scale_ratio =
      non_opt_scales != 0
          ? 100.f * (non_opt_scales - opt_scales) / non_opt_scales
          : 0;

  ozz::log::LogV() << "Optimization stage results:" << std::endl;
  ozz::log::LogV() << " - Translations key frames optimization: "
                   << translation_ratio << "%" << std::endl;
  ozz::log::LogV() << " - Rotations key frames optimization: " << rotation_ratio
                   << "%" << std::endl;
  ozz::log::LogV() << " - Scaling key frames optimization: " << scale_ratio
                   << "%" << std::endl;
}

ozz::animation::Skeleton* ImportSkeleton() {
  // Reads the skeleton from the binary ozz stream.
  ozz::animation::Skeleton* skeleton = NULL;
  {
    ozz::log::LogV() << "Opens input skeleton ozz binary file: "
                     << OPTIONS_skeleton << std::endl;
    ozz::io::File file(OPTIONS_skeleton, "rb");
    if (!file.opened()) {
      ozz::log::Err() << "Failed to open input skeleton ozz binary file: \""
                      << OPTIONS_skeleton << "\"" << std::endl;
      return NULL;
    }
    ozz::io::IArchive archive(&file);

    // File could contain a RawSkeleton or a Skeleton.
    if (archive.TestTag<ozz::animation::offline::RawSkeleton>()) {
      ozz::log::LogV() << "Reading RawSkeleton from file." << std::endl;

      // Reading the skeleton cannot file.
      ozz::animation::offline::RawSkeleton raw_skeleton;
      archive >> raw_skeleton;

      // Builds runtime skeleton.
      ozz::log::LogV() << "Builds runtime skeleton." << std::endl;
      ozz::animation::offline::SkeletonBuilder builder;
      skeleton = builder(raw_skeleton);
      if (!skeleton) {
        ozz::log::Err() << "Failed to build runtime skeleton." << std::endl;
        return NULL;
      }
    } else if (archive.TestTag<ozz::animation::Skeleton>()) {
      // Reads input archive to the runtime skeleton.
      // This operation cannot fail.
      skeleton =
          ozz::memory::default_allocator()->New<ozz::animation::Skeleton>();
      archive >> *skeleton;
    } else {
      ozz::log::Err() << "Failed to read input skeleton from binary file: "
                      << OPTIONS_skeleton << std::endl;
      return NULL;
    }
  }
  return skeleton;
}

ozz::String::Std BuildFilename(const char* _filename, const char* _data_name) {
  ozz::String::Std output(_filename);

  for (size_t asterisk = output.find('*'); asterisk != std::string::npos;
       asterisk = output.find('*')) {
    output.replace(asterisk, 1, _data_name);
  }
  return output;
}

bool Export(const ozz::animation::offline::RawAnimation& _raw_animation,
            const ozz::animation::Skeleton& _skeleton,
            const Json::Value& _config) {
  // Raw animation to build and output.
  ozz::animation::offline::RawAnimation raw_animation;

  // Make delta animation if requested.
  if (_config["additive"].asBool()) {
    ozz::log::Log() << "Makes additive animation." << std::endl;
    ozz::animation::offline::AdditiveAnimationBuilder additive_builder;
    RawAnimation raw_additive;
    if (!additive_builder(_raw_animation, &raw_additive)) {
      ozz::log::Err() << "Failed to make additive animation." << std::endl;
      return false;
    }
    // checker animation.
    raw_animation = raw_additive;
  } else {
    raw_animation = _raw_animation;
  }

  // Optimizes animation if option is enabled.
  if (_config["optimize"].asBool()) {
    ozz::log::Log() << "Optimizing animation." << std::endl;
    ozz::animation::offline::AnimationOptimizer optimizer;

    // Setup optimizer from config parameters.
    const Json::Value& tolerances = _config["optimization_tolerances"];
    optimizer.translation_tolerance = tolerances["translation"].asFloat();
    optimizer.rotation_tolerance = tolerances["rotation"].asFloat();
    optimizer.scale_tolerance = tolerances["scale"].asFloat();
    optimizer.hierarchical_tolerance = tolerances["hierarchical"].asFloat();

    ozz::animation::offline::RawAnimation raw_optimized_animation;
    if (!optimizer(raw_animation, _skeleton, &raw_optimized_animation)) {
      ozz::log::Err() << "Failed to optimize animation." << std::endl;
      return false;
    }

    // Displays optimization statistics.
    DisplaysOptimizationstatistics(raw_animation, raw_optimized_animation);

    // Brings data back to the raw animation.
    raw_animation = raw_optimized_animation;
  }

  // Builds runtime animation.
  ozz::animation::Animation* animation = NULL;
  if (!_config["raw"].asBool()) {
    ozz::log::Log() << "Builds runtime animation." << std::endl;
    ozz::animation::offline::AnimationBuilder builder;
    animation = builder(raw_animation);
    if (!animation) {
      ozz::log::Err() << "Failed to build runtime animation." << std::endl;
      return false;
    }
  }

  {
    // Prepares output stream. File is a RAII so it will close automatically
    // at the end of this scope. Once the file is opened, nothing should fail
    // as it would leave an invalid file on the disk.

    // Builds output filename.
    ozz::String::Std filename = BuildFilename(_config["output"].asCString(),
                                              _raw_animation.name.c_str());

    ozz::log::LogV() << "Opens output file: " << filename << std::endl;
    ozz::io::File file(filename.c_str(), "wb");
    if (!file.opened()) {
      ozz::log::Err() << "Failed to open output file: \"" << filename << "\""
                      << std::endl;
      ozz::memory::default_allocator()->Delete(animation);
      return false;
    }

    // Initializes output archive.
    ozz::io::OArchive archive(&file, Endianness());

    // Fills output archive with the animation.
    if (_config["raw"].asBool()) {
      ozz::log::Log() << "Outputs RawAnimation to binary archive." << std::endl;
      archive << raw_animation;
    } else {
      ozz::log::LogV() << "Outputs Animation to binary archive." << std::endl;
      archive << *animation;
    }
  }

  ozz::log::LogV() << "Animation binary archive successfully outputted."
                   << std::endl;

  // Delete local objects.
  ozz::memory::default_allocator()->Delete(animation);

  return true;
}

bool ProcessAnimation(AnimationConverter& _converter,
                      const char* _animation_name,
                      const ozz::animation::Skeleton& _skeleton,
                      const Json::Value& _config) {
  RawAnimation animation;
  if (!_converter.Import(_animation_name, _skeleton,
                         _config["sampling_rate"].asFloat(), &animation)) {
    ozz::log::Err() << "Failed to import animation \"" << _animation_name
                    << "\"" << std::endl;
    return false;
  } else {
    // Give animation a name
    animation.name = _animation_name;

    return Export(animation, _skeleton, _config);
  }
}

bool Export(const ozz::animation::offline::RawFloatTrack& _raw_track,
            const Json::Value& _config) {
  // Raw track to build and output.
  ozz::animation::offline::RawFloatTrack raw_track;

  // Optimizes track if option is enabled.
  if (_config["optimize"].asBool()) {
    ozz::log::LogV() << "Optimizing track." << std::endl;
    ozz::animation::offline::TrackOptimizer optimizer;
    // optimizer.rotation_tolerance = OPTIONS_rotation;
    ozz::animation::offline::RawFloatTrack raw_optimized_track;
    if (!optimizer(_raw_track, &raw_optimized_track)) {
      ozz::log::Err() << "Failed to optimize track." << std::endl;
      return false;
    }

    // Displays optimization statistics.
    // DisplaysOptimizationstatistics(raw_animation, raw_optimized_animation);

    // Brings data back to the raw track.
    raw_track = raw_optimized_track;
  }

  // Builds runtime track.
  ozz::animation::FloatTrack* track = NULL;
  if (!_config["raw"].asBool()) {
    ozz::log::LogV() << "Builds runtime track." << std::endl;
    ozz::animation::offline::TrackBuilder builder;
    track = builder(raw_track);
    if (!track) {
      ozz::log::Err() << "Failed to build runtime track." << std::endl;
      return false;
    }
  }

  {
    // Prepares output stream. Once the file is opened, nothing should fail as
    // it would leave an invalid file on the disk.

    // Builds output filename.
    const ozz::String::Std filename =
        BuildFilename(_config["output"].asCString(), _raw_track.name.c_str());

    ozz::log::LogV() << "Opens output file: " << filename << std::endl;
    ozz::io::File file(filename.c_str(), "wb");
    if (!file.opened()) {
      ozz::log::Err() << "Failed to open output file: " << filename
                      << std::endl;
      ozz::memory::default_allocator()->Delete(track);
      return false;
    }

    // Initializes output archive.
    ozz::io::OArchive archive(&file, Endianness());

    // Fills output archive with the track.
    if (_config["raw"].asBool()) {
      ozz::log::LogV() << "Outputs RawTrack to binary archive." << std::endl;
      archive << raw_track;
    } else {
      ozz::log::LogV() << "Outputs Track to binary archive." << std::endl;
      archive << *track;
    }
  }

  ozz::log::LogV() << "Track binary archive successfully outputted."
                   << std::endl;

  // Delete local objects.
  ozz::memory::default_allocator()->Delete(track);

  return true;
}

bool ProcessImportTrack(AnimationConverter& _converter,
                        const char* _animation_name, const Skeleton& _skeleton,
                        const Json::Value& _import) {
  // Early out if no name is specified
  const char* joint_name_match = _import["joint_name"].asCString();
  const char* ppt_name_match = _import["property_name"].asCString();

  // Process every joint that matches.
  bool success = true;
  bool joint_found = false;
  for (int s = 0; success && s < _skeleton.num_joints(); ++s) {
    const char* joint_name = _skeleton.joint_names()[s];
    if (!strmatch(joint_name, joint_name_match)) {
      continue;
    }
    joint_found = true;

    // Node found, need to find matching properties now.
    bool ppt_found = false;
    const AnimationConverter::NodeProperties properties =
        _converter.GetNodeProperties(joint_name);
    for (size_t p = 0; p < properties.size(); ++p) {
      const AnimationConverter::NodeProperty property = properties[p];
      const char* property_name = property.name.c_str();
      if (!strmatch(property.name.c_str(), ppt_name_match)) {
        continue;
      }
      ppt_found = true;

      // Property found
      RawFloatTrack track;
      success &= _converter.Import(_animation_name, joint_name, property_name,
                                   0, &track);

      if (success) {
        // Give the track a name
        track.name = joint_name;
        track.name += '-';
        track.name += property_name;

        success &= Export(track, _import);
      } else {
        ozz::log::Err() << "Failed to import track \"" << joint_name << ":"
                        << property.name << "\"" << std::endl;
      }
    }

    if (!ppt_found) {
      ozz::log::Log() << "No property found for track import definition \""
                      << joint_name_match << ":" << ppt_name_match << "\"."
                      << std::endl;
    }
  }

  if (!joint_found) {
    ozz::log::Log() << "No joint found for track import definition \""
                    << joint_name_match << "\"." << std::endl;
  }

  return success;
}

/*
bool ProcessMotionTrack(AnimationConverter& _converter,
                        const char* _animation_name, const Skeleton& _skeleton,
                        const Json::Value& _motion) {
  return true;
}*/

bool ProcessTracks(AnimationConverter& _converter, const char* _animation_name,
                   const Skeleton& _skeleton, const Json::Value& _config) {
  bool success = true;

  const Json::Value& imports = _config["imports"];
  for (Json::ArrayIndex i = 0; success && i < imports.size(); ++i) {
    success &=
        ProcessImportTrack(_converter, _animation_name, _skeleton, imports[i]);
  }

  /*
    const Json::Value& motions = _config["motions"];
    for (Json::ArrayIndex i = 0; success && i < motions.size(); ++i) {
      success &=
          ProcessMotionTrack(_converter, _animation_name, _skeleton,
    motions[i]);
    }*/

  return success;
}
}  // namespace

int AnimationConverter::operator()(int _argc, const char** _argv) {
  // Parses arguments.
  ozz::options::ParseResult parse_result = ozz::options::ParseCommandLine(
      _argc, _argv, "2.0",
      "Imports a animation from a file and converts it to ozz binary raw or "
      "runtime animation format");
  if (parse_result != ozz::options::kSuccess) {
    return parse_result == ozz::options::kExitSuccess ? EXIT_SUCCESS
                                                      : EXIT_FAILURE;
  }

  // Initializes log level from options.
  ozz::log::Level log_level = ozz::log::GetLevel();
  if (std::strcmp(OPTIONS_log_level, "silent") == 0) {
    log_level = ozz::log::kSilent;
  } else if (std::strcmp(OPTIONS_log_level, "standard") == 0) {
    log_level = ozz::log::kStandard;
  } else if (std::strcmp(OPTIONS_log_level, "verbose") == 0) {
    log_level = ozz::log::kVerbose;
  }
  ozz::log::SetLevel(log_level);

  Json::Value config;
  if (!ProcessConfiguration(&config)) {
    // Specific error message are reported during Sanitize.
    return EXIT_FAILURE;
  }

  // Ensures file to import actually exist.
  if (!ozz::io::File::Exist(OPTIONS_file)) {
    ozz::log::Err() << "File \"" << OPTIONS_file << "\" doesn't exist."
                    << std::endl;
    return EXIT_FAILURE;
  }

  // Imports animations from the document.
  ozz::log::Log() << "Importing file \"" << OPTIONS_file << "\"" << std::endl;
  if (!Load(OPTIONS_file)) {
    ozz::log::Err() << "Failed to import file \"" << OPTIONS_file << "\""
                    << std::endl;
    return EXIT_FAILURE;
  }

  // Get all available animation names.
  AnimationNames import_animation_names = GetAnimationNames();

  // Are there animations available
  if (import_animation_names.empty()) {
    ozz::log::Err() << "No animation found." << std::endl;
    return EXIT_FAILURE;
  }

  // Import skeleton instance.
  ozz::animation::Skeleton* skeleton = ImportSkeleton();
  if (!skeleton) {
    return EXIT_FAILURE;
  }

  // Iterates all imported animations, build and output them.
  bool success = true;
  const Json::Value& animations_config = config["animations"];
  if (animations_config.size() == 0) {
    ozz::log::Log() << "Configuration contains no animation export definition."
                    << std::endl;
  }
  for (Json::ArrayIndex i = 0; success && i < animations_config.size(); ++i) {
    const Json::Value& animation_config = animations_config[i];
    // Loop though all existing animations, and export those who match
    // configuration.
    for (size_t j = 0; success && j < import_animation_names.size(); ++j) {
      const char* animation_name = import_animation_names[j].c_str();
      if (!strmatch(animation_name, animation_config["name"].asCString())) {
        continue;
      }

      success =
          ProcessAnimation(*this, animation_name, *skeleton, animation_config);

      const Json::Value& tracks_config = animation_config["tracks"];
      for (Json::ArrayIndex t = 0; success && t < tracks_config.size(); ++t) {
        success =
            ProcessTracks(*this, animation_name, *skeleton, tracks_config[t]);
      }
    }
  }

  ozz::memory::default_allocator()->Delete(skeleton);

  return success ? EXIT_SUCCESS : EXIT_FAILURE;
}  // namespace animation
}  // namespace offline
}  // namespace animation
}  // namespace ozz
