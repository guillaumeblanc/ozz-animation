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

#include "ozz2csv_csv.h"

#include "ozz/animation/offline/raw_animation.h"

#include "ozz/animation/runtime/skeleton.h"
#include "ozz/animation/runtime/skeleton_utils.h"

#include "ozz/base/io/archive.h"
#include "ozz/base/io/stream.h"
#include "ozz/base/log.h"

#include "ozz/base/memory/scoped_ptr.h"

#include "ozz/base/containers/vector.h"

#include "ozz/base/maths/math_ex.h"
#include "ozz/base/maths/soa_transform.h"
#include "ozz/base/maths/transform.h"

#include "ozz/options/options.h"

#include <chrono>
#include <random>

#include <json/json.h>

// Concept is to read a non optimized animation, build and sample it, compute
// model space transforms and output everything to a csv file for analysis.

OZZ_OPTIONS_DECLARE_STRING(skeleton, "Skeleton input file", "", true)
OZZ_OPTIONS_DECLARE_STRING(animation, "Raw animation input file", "", true)

OZZ_OPTIONS_DECLARE_STRING(path, "csv output path", ".", false)
OZZ_OPTIONS_DECLARE_STRING(experience, "Experience name", "experience", false)

OZZ_OPTIONS_DECLARE_FLOAT(rate, "Sampling rate", 30, false)

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

struct LTMIterator {
  LTMIterator(const ozz::Range<const ozz::math::Transform>& _locals,
              const ozz::Range<ozz::math::Transform>& _models)
      : locals_(_locals), models_(_models) {}

  LTMIterator(const LTMIterator& _it)
      : locals_(_it.locals_), models_(_it.models_) {}

  void operator()(int _joint, int _parent) {
    if (_parent == ozz::animation::Skeleton::kNoParent) {
      models_[_joint] = locals_[_joint];
    } else {
      const ozz::math::Transform& local = locals_[_joint];
      const ozz::math::Transform& parent = models_[_parent];
      ozz::math::Transform& model = models_[_joint];

      model.translation =
          parent.translation +
          TransformVector(parent.rotation, local.translation * parent.scale);
      model.rotation = parent.rotation * local.rotation;
      model.scale = parent.scale * local.scale;
    }
  }
  const ozz::Range<const ozz::math::Transform>& locals_;
  const ozz::Range<ozz::math::Transform>& models_;

 private:
  void operator=(const LTMIterator&);
};

// Reimplement local to model-space because ozz runtime version isn't based on
// ozz::math::Transform
bool LocalToModel(const ozz::animation::Skeleton& _skeleton,
                  const ozz::Range<const ozz::math::Transform>& _locals,
                  const ozz::Range<ozz::math::Transform>& _models) {
  assert(static_cast<size_t>(_skeleton.num_joints()) == _locals.count() &&
         _locals.count() == _models.count());

  ozz::animation::IterateJointsDF(_skeleton, LTMIterator(_locals, _models));

  return true;
}

class Timer {
 public:
  Timer() { Reset(); }
  void Reset() { start_ = std::chrono::high_resolution_clock::now(); }
  float Elapsed() {
    const time_point end = std::chrono::high_resolution_clock::now();
    const float duration =
        std::chrono::duration<float, std::micro>(end - start_).count();
    start_ = end;
    return duration;
  }

 private:
  typedef std::chrono::high_resolution_clock::time_point time_point;
  time_point start_;
};

ozz::String::Std CsvFileName(const char* _name) {
  ozz::String::Std filename = OPTIONS_path.value();
  filename += '/';
  filename += OPTIONS_experience.value();
  filename += "_";
  filename += _name;
  filename += ".csv";
  return filename;
}

bool PushCsvMemory(Generator* _generator) {
  // Open csv file.
  CsvFile csv(CsvFileName("memory").c_str(), "size\n");
  if (!csv.opened()) {
    return false;
  }
  bool success = true;
  success &= csv.Push(static_cast<int>(_generator->Size()));
  success &= csv.LineEnd();
  return success;
}

bool PushCsvTracks(const ozz::animation::Skeleton& _skeleton,
                   Generator* _generator) {
  // Open csv file.
  CsvFile csv(CsvFileName("tracks").c_str(),
              "joint,translations,rotations,scales\n");
  if (!csv.opened()) {
    return false;
  }

  bool success = true;
  const int num_joints = _skeleton.num_joints();
  for (int i = 0; success && i < num_joints; ++i) {
    success &= csv.Push(i);
    success &=
        csv.Push(_generator->GetKeyframesCount(Generator::kTranslation, i));
    success &= csv.Push(_generator->GetKeyframesCount(Generator::kRotation, i));
    success &= csv.Push(_generator->GetKeyframesCount(Generator::kScale, i));
    success &= csv.LineEnd();
  }
  return success;
}

bool PushCsvSkeleton(const ozz::animation::Skeleton& _skeleton) {
  // Open csv file.
  CsvFile csv(CsvFileName("skeleton").c_str(), "joint,name,parent,depth\n");
  if (!csv.opened()) {
    return false;
  }

  // Return value.
  bool success = true;

  // Skeleton info
  const int num_joints = _skeleton.num_joints();
  for (int i = 0; success && i < num_joints; ++i) {
    success &= csv.Push(i);
    success &= csv.Push(_skeleton.joint_names()[i]);
    success &= csv.Push(_skeleton.joint_parents()[i]);
    int depth = 0;
    for (int parent = _skeleton.joint_parents()[i];
         parent != ozz::animation::Skeleton::kNoParent;
         ++depth, parent = _skeleton.joint_parents()[parent]) {
    }
    success &= csv.Push(depth);
    success &= csv.LineEnd();
  }

  return success;
}

bool PushCsvTransforms(const ozz::animation::Skeleton& _skeleton,
                       Generator* _generator) {
  bool success = true;

  // Open csv file.
  CsvFile csv(CsvFileName("transforms").c_str(),
              "joint,time,lt.x,lt.y,lt.z,lr.x,lr.y,lr.z,lr.w,ls.x,ls.y,ls.z,"
              "mt.x,mt.y,mt.z,mr.x,mr.y,mr.z,mr.w,ms.x,ms.y,ms.z\n");
  if (!csv.opened()) {
    return EXIT_FAILURE;
  }

  // Allocates transforms
  const int num_joints = _skeleton.num_joints();
  ozz::Vector<ozz::math::Transform>::Std locals(num_joints);
  ozz::Vector<ozz::math::Transform>::Std models(num_joints);

  const float duration = _generator->Duration();
  const float step = 1.f / OPTIONS_rate.value();
  bool end = false;
  for (float t = 0.f; success && !end; t += step) {
    // Condition for last execution
    if (t >= duration) {
      t = duration;
      end = true;
    }

    // Generatorized animation sampling
    // --------------------------------------------
    success &= _generator->Sample(t);
    success &= _generator->ReadBack(make_range(locals));
    success &= LocalToModel(_skeleton, make_range(locals), make_range(models));

    // Push output values to csv
    // -------------------------
    for (int i = 0; success && i < num_joints; ++i) {
      success &= csv.Push(i);
      success &= csv.Push(t);
      success &= csv.Push(locals[i]);
      success &= csv.Push(models[i]);
      success &= csv.LineEnd();
    }
  }
  return success;
}

bool PushProfile(const char* _mode, Generator* _generator, float _time,
                 float _delta, bool _reset, CsvFile* _csv) {
  bool success = true;

  float execution;
  Timer timer;
  {
    timer.Reset();

    success &= _generator->Sample(_time, _reset);

    execution = timer.Elapsed();
  }

  _csv->Push(_mode);
  _csv->Push(_time);
  _csv->Push(_delta);
  _csv->Push(execution);
  _csv->LineEnd();

  return success;
}

bool PushCsvPerformance(const ozz::animation::offline::RawAnimation _animation,
                        Generator* _generator) {
  bool success = true;

  // Open csv file.
  CsvFile csv(CsvFileName("performance").c_str(),
              "mode,time,delta,execution\n");
  if (!csv.opened()) {
    return EXIT_FAILURE;
  }

  const float duration = _animation.duration;

  // Samples forward
  {
    const float step = 1.f / OPTIONS_rate.value();
    bool end = false;
    for (float t = 0.f; success && !end; t += step) {
      // Condition for last execution
      if (t >= duration) {
        t = duration;
        end = true;
      }
      success &= PushProfile("forward", _generator, t, step, t == 0.f, &csv);
    }
  }

  // Samples backward
  {
    const float step = 1.f / OPTIONS_rate.value();
    bool end = false;
    for (float t = duration; success && !end; t -= step) {
      // Condition for last execution
      if (t <= 0.f) {
        t = 0;
        end = true;
      }

      success &=
          PushProfile("backward", _generator, t, -step, t == duration, &csv);
    }
  }

  // Samples random
  {
    std::mt19937 gen(0);
    std::uniform_real_distribution<float> dist(0.f, duration);

    float prev = 0.f;
    for (size_t i = 0; success && i < 200; ++i) {
      const float time = dist(gen);
      success &=
          PushProfile("random", _generator, time, time - prev, false, &csv);
      prev = time;
    }
  }

  // Samples random reset
  {
    std::mt19937 gen(0);
    std::uniform_real_distribution<float> dist(0.f, duration);

    float prev = 0.f;
    for (size_t i = 0; success && i < 200; ++i) {
      const float time = dist(gen);
      success &=
          PushProfile("reset", _generator, time, time - prev, true, &csv);
      prev = time;
    }
  }

  return success;
}

}  // namespace

int Ozz2Csv::Run(int _argc, char const* _argv[]) {
  // Parses arguments.
  ozz::options::ParseResult parse_result =
      ozz::options::ParseCommandLine(_argc, _argv, "1.0", "TODO");
  if (parse_result != ozz::options::kSuccess) {
    return parse_result == ozz::options::kExitSuccess ? EXIT_SUCCESS
                                                      : EXIT_FAILURE;
  }

  // Load data
  ozz::animation::Skeleton skeleton;
  if (!Load(OPTIONS_skeleton, &skeleton)) {
    return EXIT_FAILURE;
  }
  ozz::animation::offline::RawAnimation animation;
  if (!Load(OPTIONS_animation, &animation)) {
    return EXIT_FAILURE;
  }
  if (!animation.Validate()) {
    ozz::log::Err() << "Loaded animation is invalid." << std::endl;
    return EXIT_FAILURE;
  }
  const int num_joints = skeleton.num_joints();
  if (animation.num_tracks() != num_joints) {
    ozz::log::Err() << "Animation doesn't match skeleton number of joints."
                    << std::endl;
    return EXIT_FAILURE;
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

  // Selects and initialize generator.
  Generator* generator = FindGenerator(OPTIONS_generator);
  if (!generator) {
    ozz::log::Err() << "Failed to find generator \""
                    << OPTIONS_generator.value() << "\"." << std::endl;
    return EXIT_FAILURE;
  }
  ozz::log::Log() << "Initializing \"" << OPTIONS_generator.value()
                  << "\" generator." << std::endl;
  if (!generator->Build(animation, skeleton, config)) {
    ozz::log::Err() << "Failed to initialize generator \""
                    << OPTIONS_generator.value() << "\"." << std::endl;
    return EXIT_FAILURE;
  }

  // Runs all experiences
  ozz::log::Log() << "Running experiences." << std::endl;
  if (!RunExperiences(animation, skeleton, generator)) {
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}

bool Ozz2Csv::RunExperiences(
    const ozz::animation::offline::RawAnimation& _animation,
    const ozz::animation::Skeleton& _skeleton, Generator* _generator) {
  // Skeleton info
  if (!PushCsvSkeleton(_skeleton)) {
    ozz::log::Err() << "Operation failed while writing skeleton data."
                    << std::endl;
    return false;
  }

  // Animation size stat.
  if (!PushCsvMemory(_generator)) {
    ozz::log::Err() << "Operation failed while writing memory data."
                    << std::endl;
    return false;
  }

  // Animation keyframes stat.
  if (!PushCsvTracks(_skeleton, _generator)) {
    ozz::log::Err() << "Operation failed while writing keyframe count data."
                    << std::endl;
    return false;
  }

  // Transforms stat.
  if (!PushCsvTransforms(_skeleton, _generator)) {
    ozz::log::Err() << "Operation failed while writing transform data."
                    << std::endl;
    return false;
  }

  // Performance stat.
  if (!PushCsvPerformance(_animation, _generator)) {
    ozz::log::Err() << "Operation failed while writing performance data."
                    << std::endl;
    return false;
  }

  return true;
}

Generator* Ozz2Csv::FindGenerator(const char* _name) const {
  Generators::const_iterator it = generators.find(_name);
  if (it == generators.end()) {
    return NULL;
  }
  return it->second;
}

bool Ozz2Csv::RegisterGenerator(Generator* _generator, const char* _name) {
  // Detects conficting keys
  if (FindGenerator(_name)) {
    return false;
  }

  generators[_name] = _generator;

  return true;
}
