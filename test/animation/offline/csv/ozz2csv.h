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

#ifndef OZZ_ANIMATION_OFFLINE_CVS_OZZ2CSV_H_
#define OZZ_ANIMATION_OFFLINE_CVS_OZZ2CSV_H_

#include "ozz/base/containers/map.h"
#include "ozz/base/containers/string.h"
#include "ozz/base/io/stream.h"
#include "ozz/base/platform.h"
#include "ozz/base/span.h"

namespace ozz {
namespace animation {
class Skeleton;
namespace offline {
struct RawAnimation;
}
}  // namespace animation
namespace math {
struct Transform;
}  // namespace math
}  // namespace ozz
namespace Json {
class Value;
}

class CsvFile;

class Generator {
 public:
  virtual ~Generator() {}

  // Build sample-able animation from raw animation.
  virtual bool Build(const ozz::animation::offline::RawAnimation& _animation,
                     const ozz::animation::Skeleton& _skeleton,
                     const Json::Value& _config) = 0;

  // Built animation size in byte.
  virtual size_t Size() const = 0;

  // Get animation duration.
  virtual float Duration() const = 0;

  enum Transformation { kTranslation, kRotation, kScale };
  virtual int GetKeyframesCount(Transformation _transformation, int joint) = 0;

  // Sample animation to local samples data.
  virtual bool Sample(float _time, bool _reset = false) = 0;

  // Copy local samples data back to _transforms output.
  virtual bool ReadBack(
      const ozz::span<ozz::math::Transform>& _transforms) const = 0;
};

class Ozz2Csv {
 public:
  Ozz2Csv() {}

  // Main execution function.
  int Run(int _argc, char const* _argv[]);

  // Pushes generators to main.
  bool RegisterGenerator(Generator* _generator, const char* _name);

  // Pushes experiences
  typedef bool (*ExperienceFct)(CsvFile* _csv,
                                const ozz::animation::offline::RawAnimation&,
                                const ozz::animation::Skeleton&, Generator*);
  bool RegisterExperience(ExperienceFct _experience, const char* _name);

 private:
  bool RunExperiences(const ozz::animation::offline::RawAnimation& _animation,
                      const ozz::animation::Skeleton& _skeleton,
                      Generator* _generator);

  Generator* FindGenerator(const char* _name) const;

  // Registered generators.
  typedef ozz::cstring_map<Generator*> Generators;
  Generators generators_;

  // Registered experiences
  typedef ozz::cstring_map<ExperienceFct> Experiences;
  Experiences experiences_;
};

#endif  // OZZ_ANIMATION_OFFLINE_CVS_OZZ2CSV_H_
