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

#define OZZ_INCLUDE_PRIVATE_HEADER  // Allows to include private headers.

#include "framework/profile.h"
#include "framework/internal/renderer_impl.h"

#include <cfloat>
#include <cmath>

#include "ozz/base/memory/allocator.h"

namespace ozz {
namespace sample {

Profiler::Profiler(Record* _record)
    : begin_(static_cast<float>(glfwGetTime())), record_(_record) {}

Profiler::~Profiler() {
  if (record_) {
    float end = static_cast<float>(glfwGetTime());
    record_->Push((end - begin_) * 1000.f);
  }
}

Record::Record(int _max_records)
    : max_records_(_max_records < 1 ? 1 : _max_records),
      records_end_(
          reinterpret_cast<float*>(memory::default_allocator()->Allocate(
              _max_records * sizeof(float), alignof(float))) +
          max_records_),
      records_begin_(records_end_),
      cursor_(records_end_) {}

Record::~Record() {
  memory::default_allocator()->Deallocate(records_end_ - max_records_);
}

void Record::Push(float _value) {
  if (records_begin_ + max_records_ == records_end_) {
    if (cursor_ == records_begin_) {  // Looping...
      cursor_ = records_begin_ + max_records_;
    }
  } else {
    // The buffer is not full yet.
    records_begin_--;
  }
  --cursor_;
  *cursor_ = _value;
}

Record::Statistics Record::GetStatistics() {
  Statistics statistics = {FLT_MAX, -FLT_MAX, 0.f, 0.f};
  if (records_begin_ == records_end_) {  // No record.
    return statistics;
  }

  // Computes statistics.
  float sum = 0.f;

  const float* current = cursor_;
  const float* end = records_end_;
  while (current < end) {
    if (*current < statistics.min) {
      statistics.min = *current;
    }
    if (*current > statistics.max) {
      statistics.max = *current;
    }
    sum += *current;
    ++current;

    if (current == records_end_) {  // Looping...
      end = cursor_;
      current = records_begin_;
    }
  }

  // Stores outputs.
  /*
  int exponent;
  std::frexp(_f, &exponent);
  float upper = pow(2.f, exponent);
  statistics.max = func(statistics.max);*/

  statistics.latest = *cursor_;
  statistics.mean = sum / (records_end_ - records_begin_);

  return statistics;
}
}  // namespace sample
}  // namespace ozz
