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

#include "framework/profile.h"

#include <cfloat>

#include "ozz/base/memory/allocator.h"

#include <GL/glfw.h>

namespace ozz {
namespace demo {

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
      records_end_(memory::default_allocator().Allocate<float>(_max_records) +
                   max_records_),
      records_begin_(records_end_),
      cursor_(records_end_) {
}

Record::~Record() {
  memory::default_allocator().Deallocate(records_end_ - max_records_);
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

bool Record::Statistics(float* _min, float* _max, float* _mean) {
  if (records_begin_ == records_end_) {  // No record.
    return false;
  }

  // Computes statistics.
  float min = FLT_MAX;
  float max = -FLT_MAX;
  float sum = 0.f;

  const float* current = cursor_;
  const float* end = records_end_;
  while (current < end) {
    if (*current < min) {
      min = *current;
    }
    if (*current > max) {
      max = *current;
    }
    sum += *current;
    current++;

    if (current == records_end_) {  // Looping...
      end = cursor_;
      current = records_begin_;
    }
  }

  // Stores outputs.
  if (_min) {
    *_min = min;
  }
  if (_max) {
    *_max = max;
  }
  if (_mean) {
    *_mean = sum / (records_end_ - records_begin_);
  }
  return true;
}
}  // demo
}  // ozz
