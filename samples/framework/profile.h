//----------------------------------------------------------------------------//
//                                                                            //
// ozz-animation is hosted at http://github.com/guillaumeblanc/ozz-animation  //
// and distributed under the MIT License (MIT).                               //
//                                                                            //
// Copyright (c) Guillaume Blanc                                              //
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

#ifndef OZZ_SAMPLES_FRAMEWORK_PROFILE_H_
#define OZZ_SAMPLES_FRAMEWORK_PROFILE_H_

namespace ozz {
namespace sample {
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
  const float* cursor() const { return cursor_; }

  // Returns the beginning of the recorded values.
  const float* record_begin() const { return records_begin_; }

  // Returns the end of the recorded values.
  const float* record_end() const { return records_end_; }

  // Statistics returned by GetStatistics function.
  struct Statistics {
    // Minimum value of the recorded range.
    float min;
    // Maximum value of the recorded range.
    float max;
    // Mean value of the recorded range.
    float mean;
    // Latest value of the recorded range.
    float latest;
  };

  // Builds statistics of the current record state.
  Statistics GetStatistics();

 private:
  // Disables assignment and copy.
  Record(const Record& _record);
  void operator=(const Record& _record);

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
  void operator=(const Profiler& _profiler);

  // The time at which profiling began.
  float begin_;

  // Profiling result is pushed in the record_ object.
  Record* record_;
};
}  // namespace sample
}  // namespace ozz
#endif  // OZZ_SAMPLES_FRAMEWORK_PROFILE_H_
