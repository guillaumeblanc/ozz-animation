//============================================================================//
// Copyright (c) <2012> <Guillaume Blanc>                                     //
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
//============================================================================//

#ifndef OZZ_DEMO_FRAMEWORK_PROFILE_H_
#define OZZ_DEMO_FRAMEWORK_PROFILE_H_

namespace ozz {
namespace demo {
// Records up to a maximum number of float values. Once the maximum number is
// reached, it keeps the most recent ones and reject the oldest.
class Record {
 public:
  // Constructs and sets the maximum number of record-able values.
  // The minimum record-able number of values is 1.
  explicit Record(int _max_records);

  // Deallocate records.
  ~Record();

  // Adds _value to the records, while rejecting the oldest one if the maximum
  // number is reached.
  void Push(float _value);

  // Returns the cursor to the newest value in the circular buffer.
  // cursor() == record_begin() == record_end() if record is empty.
  // Recorded values can be accessed sequentially from the newest to the oldest
  // from [cursor():record_end()[ and then [record_begin():cursor()[
  const float* cursor() const {
    return cursor_;
  }

  // Returns the beginning of the recorded values.
  const float* record_begin() const {
    return records_begin_;
  }

  // Returns the end of the recorded values.
  const float* record_end() const {
    return records_end_;
  }

  // Builds statistics of the current record state. Any of the _min, _max, _mean
  // arguments can be NULL.
  // Returns false if statistics is not computed due to an empty record. In that
  // case none of the _min, _max and _mean argument is modified.
  bool Statistics(float* _min, float* _max, float* _mean);

 private:
  // Disables assignment and copy.
  Record(const Record& _record);
  void operator = (const Record& _record);

  // The maximum number of recorded entries.
  int max_records_;

  // Circular buffer of recorded valued, limited to max_records_ entries.
  // records_begin_ is set to records_end_ when record is empty.
  // records_begin_ then moves down to allocation begin. Therefore
  // deallocation should be done using records_end_.
  float* const records_end_;
  float* records_begin_;

  // Cursor in the circular buffer. Points to the latest pushed value.
  float* cursor_;
};

// Measures the time spent between the constructor and  the destructor (as a
// RAII object) and pushes the result to a Record.
class Profiler {
 public:
  // Starts measurement.
  explicit Profiler(Record* _record);

  // Ends measurement and pushes the result to the record.
  ~Profiler();

 private:
  // Disables assignment and copy.
  Profiler(const Profiler& _profiler);
  void operator = (const Profiler& _profiler);

  // The time at which profiling began.
  float begin_;

  // Profiling result is pushed in the record_ object.
  Record* record_;
};
}  // demo
}  // ozz
#endif  // OZZ_DEMO_FRAMEWORK_PROFILE_H_
