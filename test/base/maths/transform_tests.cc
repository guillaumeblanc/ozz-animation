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

#include "ozz/base/maths/transform.h"

#include "gtest/gtest.h"

#include "ozz/base/gtest_helper.h"
#include "ozz/base/log.h"
#include "ozz/base/maths/gtest_math_helper.h"

using ozz::math::Transform;

/*
cur_x = 3 # The algorithm starts at x=3
rate = 0.01 # Learning rate
precision = 0.000001 #This tells us when to stop the algorithm
previous_step_size = 1 #
max_iters = 10000 # maximum number of iterations
iters = 0 #iteration counter
df = lambda x: 2*(x+5) #Gradient of our function

while previous_step_size > precision and iters < max_iters:
    prev_x = cur_x #Store current x value in prev_x
    cur_x = cur_x - rate * df(prev_x) #Grad descent
    previous_step_size = abs(cur_x - prev_x) #Change in x
    iters = iters+1 #iteration count
    print("Iteration",iters,"\nX value is",cur_x) #Print iterations
print("The local minimum occurs at", cur_x)
*/

// df = lambda x: 2*(x+5) #Gradient of our function

//float f(float x) { return (x + 5.f) * (x + 5.f); }
float f(float x) { return std::cos(x); }
// float df(float x) { return 2.f * (x + 5.f); }

TEST(GD, ozz_math) {
  float previous_step_size = 1;
  float cur_x = 0.f;       //  The algorithm starts at x=3
  const float rate = 0.01f;       // Learning rate
  const float precision = 1e-6f;  // This tells us when to stop the algorithm
  
  const int max_iters = 10000;  // maximum number of iterations
  int iters = 0;          // iteration counter

  float prev_y = f(cur_x);
  float prev_x = cur_x;
  float gd = 1.f;

  while (previous_step_size > precision && ++iters < max_iters) {
    float step = rate * gd;
    previous_step_size = abs(step);  // Change in x
    cur_x = cur_x - step;            //  Grad descent

    float y = f(cur_x);
    gd = (y - prev_y) / (cur_x - prev_x);
    prev_x = cur_x;
    prev_y = y;

    // ozz::log::Log() << "i x s " << iters << "\t" << cur_x << "\t" <<
    // previous_step_size << std::endl;
  }

  ozz::log::Log() << "The local minimum occurs at " << cur_x << std::endl;
}

TEST(TransformConstant, ozz_math) {
  EXPECT_FLOAT3_EQ(Transform::identity().translation, 0.f, 0.f, 0.f);
  EXPECT_QUATERNION_EQ(Transform::identity().rotation, 0.f, 0.f, 0.f, 1.f);
  EXPECT_FLOAT3_EQ(Transform::identity().scale, 1.f, 1.f, 1.f);
}
