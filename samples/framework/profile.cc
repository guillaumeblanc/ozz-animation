//============================================================================//
//                                                                            //
// ozz-animation, 3d skeletal animation libraries and tools.                  //
// https://code.google.com/p/ozz-animation/                                   //
//                                                                            //
//----------------------------------------------------------------------------//
//                                                                            //
// Copyright (c) 2012-2015 Guillaume Blanc                                    //
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
//                                                                            //
//============================================================================//

#define OZZ_INCLUDE_PRIVATE_HEADER  // Allows to include private headers.

#include "framework/profile.h"
#include "framework/internal/renderer_impl.h"

#include <cfloat>
#include <cmath>

#include "ozz/base/memory/allocator.h"

namespace ozz {
namespace sample {

Profiler::Profiler(Record* _record)
    : begin_(static_cast<float>(glfwGetTime())),
      record_(_record) {
}

Profiler::~Profiler() {
  if (record_) {
    float end = static_cast<float>(glfwGetTime());
    record_->Push((end - begin_) * 1000.f);
  }
}

Record::Record(int _max_records)
    : max_records_(_max_records < 1 ? 1 : _max_records),
      records_end_(memory::default_allocator()->Allocate<float>(_max_records) +
                   max_records_),
      records_begin_(records_end_),
      cursor_(records_end_) {
}

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
    if (*current >  statistics.max) {
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
}  // sample
}  // ozz
