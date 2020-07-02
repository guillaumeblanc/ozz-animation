//----------------------------------------------------------------------------//
//                                                                            //
// ozz-animation is hosted at http://github.com/guillaumeblanc/ozz-animation  //
// and distributed under the MIT License (MIT).                               //
//                                                                            //
// Copyright (c) 2019 Guillaume Blanc                                         //
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

#include "ozz2csv.h"

#include <json/json.h>

#include <algorithm>
#include <mutex>

#include "ozz/animation/offline/raw_animation.h"
#include "ozz/animation/offline/raw_skeleton.h"
#include "ozz/animation/offline/skeleton_builder.h"
#include "ozz/animation/runtime/skeleton.h"
#include "ozz/base/containers/unordered_map.h"
#include "ozz/base/io/archive.h"
#include "ozz/base/io/stream.h"
#include "ozz/base/log.h"
#include "ozz/base/maths/simd_math.h"
#include "ozz/options/options.h"
#include "ozz2csv_chrono.h"
#include "ozz2csv_csv.h"

// Concept is to read a non optimized animation, build and sample it, compute
// model space transforms and output everything to a csv file for analysis.

OZZ_OPTIONS_DECLARE_STRING(skeleton, "Skeleton input file", "", true)
OZZ_OPTIONS_DECLARE_STRING(animation, "Raw animation input file", "", true)

OZZ_OPTIONS_DECLARE_STRING(path, "csv output path", ".", false)
OZZ_OPTIONS_DECLARE_STRING(experience, "Experience name", "experience", false)

OZZ_OPTIONS_DECLARE_STRING(
    generator,
    "Selects generator. Can be \"passtrhough\", \"optimize\" or \"runtime\"...",
    "passthrough", false)

OZZ_OPTIONS_DECLARE_STRING(config, "Generator specific config", "", false)

namespace {
template <typename _Type>
bool Load(const char* _filename, _Type* _object) {
  assert(_filename && _object);
  ozz::log::Out() << "Loading archive " << _filename << "." << std::endl;
  ozz::io::File file(_filename, "rb");
  if (!file.opened()) {
    ozz::log::Err() << "Failed to open file " << _filename << "." << std::endl;
    return false;
  }
  ozz::io::IArchive archive(&file);
  if (!archive.TestTag<_Type>()) {
    ozz::log::Err() << "Failed to load instance from file " << _filename << "."
                    << std::endl;
    return false;
  }

  // Once the tag is validated, reading cannot fail.
  archive >> *_object;

  return true;
}

ozz::string CsvFileName(const char* _name) {
  ozz::string filename = OPTIONS_path.value();
  filename += '/';
  filename += OPTIONS_experience.value();
  filename += "_";
  filename += _name;
  filename += ".csv";
  return filename;
}

class TrackedAllocator : public ozz::memory::Allocator {
 public:
  TrackedAllocator() : default_allocator_(ozz::memory::default_allocator()) {}
  ~TrackedAllocator() {  // assert(allocations_.size() == 0);
  }

  template <typename _Functor>
  void Iterate(_Functor _functor) const {
    std::lock_guard<std::mutex> guard(mutex_);
    for (const auto& entry : allocations_) {
      _functor(entry.first, entry.second);
    }
  }

 private:
  virtual void* Allocate(size_t _size, size_t _alignment) {
    void* alloc = default_allocator_->Allocate(_size, _alignment);
    if (alloc) {
      std::lock_guard<std::mutex> guard(mutex_);
      allocations_.insert({alloc, _size});
    }
    return alloc;
  }
  virtual void Deallocate(void* _block) {
    if (_block) {
      std::lock_guard<std::mutex> guard(mutex_);
      allocations_.erase(_block);
    }

    default_allocator_->Deallocate(_block);
  }

  mutable std::mutex mutex_;
  // Use another allocator to solve re-entrant allocations.
  std::unordered_map<void*, size_t> allocations_;
  ozz::memory::Allocator* default_allocator_;
};

void InvalidateCache(const TrackedAllocator& _allocator) {
  (void)_allocator;
#if defined(OZZ_SIMD_SSE2)
  auto invalidate = [](void* _alloc, size_t _size) {
    const size_t kCacheLineSize = 64;
    for (size_t i = 0; i < _size; i += kCacheLineSize) {
      _mm_clflush(ozz::PointerStride(_alloc, i));
    }
  };
  _allocator.Iterate(invalidate);
  _mm_sfence();
#endif  // OZZ_SIMD_SSE2
}

class AllocatorSetter {
 public:
  AllocatorSetter(ozz::memory::Allocator* _allocator)
      : previous_(ozz::memory::SetDefaulAllocator(_allocator)) {}
  ~AllocatorSetter() { ozz::memory::SetDefaulAllocator(previous_); }

 private:
  ozz::memory::Allocator* previous_;
};

bool FilterJoints(ozz::animation::offline::RawSkeleton& _skeleton,
                  ozz::animation::offline::RawAnimation& _animation,
                  const Json::Value& _config) {
  const Json::Value& joint_names = _config["joints"];
  if (joint_names.empty()) {
    return true;
  }

  std::function<bool(
      ozz::animation::offline::RawSkeleton::Joint::Children & _children,
      ozz::animation::offline::RawAnimation & _animation)>
      IterateRemove =
          [&IterateRemove, &joint_names](
              ozz::animation::offline::RawSkeleton::Joint::Children& _children,
              ozz::animation::offline::RawAnimation& _animation) {
            for (size_t i = 0; i < _children.size(); ++i) {
              ozz::animation::offline::RawSkeleton::Joint& current =
                  _children[i];

              bool found = false;
              for (const auto joint_name : joint_names) {
                found |= std::strstr(current.name.c_str(),
                                     joint_name.asCString()) != nullptr;
              }

              if (!found) {
                if (current.children.empty()) {
                  _children.erase(_children.begin() + i);
                  _animation.tracks.erase(_animation.tracks.begin() + i);
                  return true;
                } else {
                  ozz::log::Log()
                      << "Cannot filtering out joint \"" << current.name
                      << "\" that has children." << std::endl;
                }
              }

              // Recurse
              if (IterateRemove(current.children, _animation)) {
                return true;
              }
            };
            return false;
          };
  for (; IterateRemove(_skeleton.roots, _animation);) {
  }
  return true;
}
}  // namespace

int Ozz2Csv::Run(int _argc, char const* _argv[]) {
  TrackedAllocator tracked_allocator;
  AllocatorSetter allocator_setter(&tracked_allocator);

  // Parses arguments.
  ozz::options::ParseResult parse_result =
      ozz::options::ParseCommandLine(_argc, _argv, "1.0", "TODO");
  if (parse_result != ozz::options::kSuccess) {
    return parse_result == ozz::options::kExitSuccess ? EXIT_SUCCESS
                                                      : EXIT_FAILURE;
  }

  // Load config
  // Use {} as a default config, otherwise take the one specified as argument.
  std::string config_string = "{}";
  // Takes config from program options.
  if (OPTIONS_config.value()[0] != 0) {
    config_string = OPTIONS_config.value();
    ozz::log::Log() << "Using configuration string: " << config_string
                    << std::endl;
  }
  Json::Value config;
  Json::Reader json_builder;
  if (!json_builder.parse(config_string, config, true)) {
    ozz::log::Err() << "Error while parsing configuration string: "
                    << json_builder.getFormattedErrorMessages() << std::endl;
    return EXIT_FAILURE;
  }

  // Load data
  ozz::animation::offline::RawSkeleton raw_skeleton;
  if (!Load(OPTIONS_skeleton, &raw_skeleton)) {
    return EXIT_FAILURE;
  }
  ozz::animation::offline::RawAnimation raw_animation;
  if (!Load(OPTIONS_animation, &raw_animation)) {
    return EXIT_FAILURE;
  }

  // Filters out unneeded joints.
  if (!FilterJoints(raw_skeleton, raw_animation, config)) {
    return EXIT_FAILURE;
  }

  // Builds runtime skeleton
  ozz::animation::offline::SkeletonBuilder builder;
  const auto skeleton = builder(raw_skeleton);

  if (!raw_animation.Validate()) {
    ozz::log::Err() << "Loaded animation is invalid." << std::endl;
    return EXIT_FAILURE;
  }
  const int num_joints = skeleton->num_joints();
  if (raw_animation.num_tracks() != num_joints) {
    ozz::log::Err() << "Animation doesn't match skeleton number of joints."
                    << std::endl;
    return EXIT_FAILURE;
  }

  // Selects and initialize generator.
  Generator* generator = FindGenerator(OPTIONS_generator);
  if (!generator) {
    ozz::log::Err() << "Failed to find generator \""
                    << OPTIONS_generator.value() << "\"." << std::endl;
    return EXIT_FAILURE;
  }

  if (!Generate(generator, raw_animation, *skeleton, config)) {
    return EXIT_FAILURE;
  }

  // Runs all experiences
  ozz::log::Log() << "Running experiences." << std::endl;
  const auto& cache_invalidator =
      std::bind(InvalidateCache, std::cref(tracked_allocator));
  if (!RunExperiences(raw_animation, *skeleton, generator, cache_invalidator)) {
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}

bool Ozz2Csv::Generate(Generator* _generator,
                       const ozz::animation::offline::RawAnimation& _animation,
                       const ozz::animation::Skeleton& _skeleton,
                       const Json::Value& _config) {
  ozz::log::Log() << "Initializing \"" << OPTIONS_generator.value()
                  << "\" generator." << std::endl;

  Chrono chrono;
  if (!_generator->Build(_animation, _skeleton, _config)) {
    ozz::log::Err() << "Failed to initialize generator \""
                    << OPTIONS_generator.value() << "\"." << std::endl;
    return false;
  }
  const float elapsed = chrono.Elapsed();

  const ozz::string filename = CsvFileName("compression");
  CsvFile csv(filename.c_str());
  if (!csv.opened()) {
    return false;
  }

  bool success = true;
  success &= csv.Push("size,time");
  success &= csv.LineEnd();
  success &= csv.Push(static_cast<int>(_generator->Size()));
  success &= csv.Push(elapsed * 1e-6f);  // to seconds
  success &= csv.LineEnd();
  return success;
}

bool Ozz2Csv::RunExperiences(
    const ozz::animation::offline::RawAnimation& _animation,
    const ozz::animation::Skeleton& _skeleton, Generator* _generator,
    const Ozz2Csv::CacheInvalidator& _cache_invalidator) {
  for (Experiences::const_iterator it = experiences_.begin();
       it != experiences_.end(); ++it) {
    // Opens csv file.
    const ozz::string filename = CsvFileName(it->first);
    CsvFile csv(filename.c_str());
    if (!csv.opened()) {
      return false;
    }
    // Run experience.
    if (!(it->second)(&csv, _animation, _skeleton, _generator,
                      _cache_invalidator)) {
      ozz::log::Err() << "Operation failed while running experience \""
                      << it->first << "\"." << std::endl;
      return false;
    }
  }
  return true;
}

Generator* Ozz2Csv::FindGenerator(const char* _name) const {
  Generators::const_iterator it = generators_.find(_name);
  if (it == generators_.end()) {
    return NULL;
  }
  return it->second;
}

bool Ozz2Csv::RegisterGenerator(Generator* _generator, const char* _name) {
  // Detects conficting keys
  if (FindGenerator(_name)) {
    return false;
  }
  generators_[_name] = _generator;
  return true;
}

bool Ozz2Csv::RegisterExperience(ExperienceFct _experience, const char* _name) {
  experiences_[_name] = _experience;
  return true;
}
