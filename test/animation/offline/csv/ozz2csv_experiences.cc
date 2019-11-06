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

#include "ozz2csv_experiences.h"

#include "ozz2csv_csv.h"

#include "ozz/animation/offline/raw_animation.h"

#include "ozz/animation/runtime/skeleton.h"
#include "ozz/animation/runtime/skeleton_utils.h"

#include "ozz/base/log.h"

#include "ozz/base/containers/vector.h"

#include "ozz/base/maths/math_ex.h"
#include "ozz/base/maths/soa_transform.h"
#include "ozz/base/maths/transform.h"

#include <chrono>
#include <random>

#include "ozz/options/options.h"

OZZ_OPTIONS_DECLARE_FLOAT(rate, "Sampling rate", 30, false)

namespace {

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

  LTMIterator it(_locals, _models);
  ozz::animation::IterateJointsDF(_skeleton, it);

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

bool MemoryExperience(CsvFile* _csv,
                      const ozz::animation::offline::RawAnimation&,
                      const ozz::animation::Skeleton&, Generator* _generator) {
  bool success = true;
  success &= _csv->Push("size");
  success &= _csv->LineEnd();
  success &= _csv->Push(static_cast<int>(_generator->Size()));
  success &= _csv->LineEnd();
  return success;
}

bool TracksExperience(CsvFile* _csv,
                      const ozz::animation::offline::RawAnimation&,
                      const ozz::animation::Skeleton& _skeleton,
                      Generator* _generator) {
  bool success = true;
  success &= _csv->Push("joint,translations,rotations,scales");
  success &= _csv->LineEnd();

  const int num_joints = _skeleton.num_joints();
  for (int i = 0; success && i < num_joints; ++i) {
    success &= _csv->Push(i);
    success &=
        _csv->Push(_generator->GetKeyframesCount(Generator::kTranslation, i));
    success &=
        _csv->Push(_generator->GetKeyframesCount(Generator::kRotation, i));
    success &= _csv->Push(_generator->GetKeyframesCount(Generator::kScale, i));
    success &= _csv->LineEnd();
  }
  return success;
}

bool SkeletonExperience(CsvFile* _csv,
                        const ozz::animation::offline::RawAnimation&,
                        const ozz::animation::Skeleton& _skeleton, Generator*) {
  bool success = true;
  success &= _csv->Push("joint,name,parent,depth");
  success &= _csv->LineEnd();

  // Skeleton info
  const int num_joints = _skeleton.num_joints();
  for (int i = 0; success && i < num_joints; ++i) {
    success &= _csv->Push(i);
    success &= _csv->Push(_skeleton.joint_names()[i]);
    success &= _csv->Push(_skeleton.joint_parents()[i]);
    int depth = 0;
    for (int parent = _skeleton.joint_parents()[i];
         parent != ozz::animation::Skeleton::kNoParent;
         ++depth, parent = _skeleton.joint_parents()[parent]) {
    }
    success &= _csv->Push(depth);
    success &= _csv->LineEnd();
  }

  return success;
}

bool TransformsExperience(CsvFile* _csv,
                          const ozz::animation::offline::RawAnimation&,
                          const ozz::animation::Skeleton& _skeleton,
                          Generator* _generator) {
  bool success = true;
  success &= _csv->Push(
      "joint,time,lt.x,lt.y,lt.z,lr.x,lr.y,lr.z,lr.w,ls.x,ls.y,ls.z,"
      "mt.x,mt.y,mt.z,mr.x,mr.y,mr.z,mr.w,ms.x,ms.y,ms.z");
  success &= _csv->LineEnd();

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
      success &= _csv->Push(i);
      success &= _csv->Push(t);
      success &= _csv->Push(locals[i]);
      success &= _csv->Push(models[i]);
      success &= _csv->LineEnd();
    }
  }
  return success;
}

bool Profile(const char* _mode, Generator* _generator, float _time,
             float _delta, bool _reset, CsvFile* _csv) {
  bool success = true;

  float execution;
  Timer timer;
  {
    timer.Reset();

    success &= _generator->Sample(_time, _reset);

    execution = timer.Elapsed();
  }

  success &= _csv->Push(_mode);
  success &= _csv->Push(_time);
  success &= _csv->Push(_delta);
  success &= _csv->Push(execution);
  success &= _csv->LineEnd();

  return success;
}

bool PerformanceExperience(
    CsvFile* _csv, const ozz::animation::offline::RawAnimation& _animation,
    const ozz::animation::Skeleton&, Generator* _generator) {
  bool success = true;
  success &= _csv->Push("mode,time,delta,execution");
  success &= _csv->LineEnd();

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
      success &= Profile("forward", _generator, t, step, t == 0.f, _csv);
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

      success &= Profile("backward", _generator, t, -step, t == duration, _csv);
    }
  }

  // Samples random
  {
    std::mt19937 gen(0);
    std::uniform_real_distribution<float> dist(0.f, duration);

    float prev = 0.f;
    for (size_t i = 0; success && i < 200; ++i) {
      const float time = dist(gen);
      success &= Profile("random", _generator, time, time - prev, false, _csv);
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
      success &= Profile("reset", _generator, time, time - prev, true, _csv);
      prev = time;
    }
  }

  return success;
}
}  // namespace

bool RegisterDefaultExperiences(Ozz2Csv* _ozz2csv) {
  bool success = true;
  success &= _ozz2csv->RegisterExperience(&MemoryExperience, "memory");
  success &= _ozz2csv->RegisterExperience(&TracksExperience, "tracks");
  success &= _ozz2csv->RegisterExperience(&TransformsExperience, "transforms");
  success &= _ozz2csv->RegisterExperience(&SkeletonExperience, "skeleton");
  success &=
      _ozz2csv->RegisterExperience(&PerformanceExperience, "performance");

  return success;
}
