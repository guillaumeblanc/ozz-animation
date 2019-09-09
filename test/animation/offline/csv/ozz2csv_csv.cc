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

#include "ozz2csv_csv.h"

#include "ozz/base/log.h"
#include "ozz/base/maths/transform.h"
#include "ozz/base/containers/string.h"

CsvFile::CsvFile(const char* _name, const char* _hearder)
    : ozz::io::File(_name, "wt"), first_(true) {
  if (!opened()) {
    ozz::log::Err() << "Failed opening csv file \"" << _name
                    << "\"." << std::endl;
    return;
  }
  const size_t len = strlen(_hearder);
  if (len != Write(_hearder, len)) {
    Close();
  }
}

#define F "%f"

bool CsvFile::Push(int _value) {
  char line[1024];
  sprintf(line, "%s%d", first_ ? "" : ",", _value);
  first_ = false;

  const size_t len = strlen(line);
  return len == Write(line, len);
}

bool CsvFile::Push(const char* _value) {
  char line[1024];
  sprintf(line, "%s%s", first_ ? "" : ",", _value);
  first_ = false;

  const size_t len = strlen(line);
  return len == Write(line, len);
}

bool CsvFile::Push(float _value) {
  char line[1024];
  sprintf(line, "%s" F, first_ ? "" : ",", _value);
  first_ = false;

  const size_t len = strlen(line);
  return len == Write(line, len);
}

bool CsvFile::Push(const ozz::math::Float3& _value) {
  char line[1024];
  sprintf(line, "%s" F ";" F ";" F, first_ ? "" : ",", _value.x, _value.y,
          _value.z);
  first_ = false;

  const size_t len = strlen(line);
  return len == Write(line, len);
}

bool CsvFile::Push(const ozz::math::Quaternion& _value) {
  char line[1024];
  sprintf(line, "%s" F "," F "," F "," F, first_ ? "" : ",", _value.w, _value.x,
          _value.y, _value.z);
  first_ = false;

  const size_t len = strlen(line);
  return len == Write(line, len);
}

bool CsvFile::Push(const ozz::math::Transform& _value) {
  bool success = true;
  success &= Push(_value.translation);
  success &= Push(_value.rotation);
  success &= Push(_value.scale);
  return success;
}

#undef F

bool CsvFile::LineEnd() {
  first_ = true;
  return Write("\n", 1) == 1;
}
