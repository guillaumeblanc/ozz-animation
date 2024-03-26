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

#include "gtest/gtest.h"
#include "ozz/animation/offline/motion_extractor.h"
#include "ozz/animation/offline/raw_animation.h"
#include "ozz/animation/offline/raw_skeleton.h"
#include "ozz/animation/offline/raw_track.h"
#include "ozz/animation/offline/skeleton_builder.h"
#include "ozz/animation/runtime/skeleton.h"
#include "ozz/base/maths/math_constant.h"
#include "ozz/base/memory/unique_ptr.h"

using ozz::animation::Skeleton;
using ozz::animation::offline::MotionExtractor;
using ozz::animation::offline::RawAnimation;
using ozz::animation::offline::RawFloat3Track;
using ozz::animation::offline::RawQuaternionTrack;
using ozz::animation::offline::RawSkeleton;
using ozz::animation::offline::SkeletonBuilder;

TEST(Error, MotionExtractor) {
  RawAnimation input;
  input.tracks.resize(1);

  RawSkeleton raw_skeleton;
  raw_skeleton.roots.resize(1);
  const Skeleton skeleton = std::move(*SkeletonBuilder()(raw_skeleton));

  RawFloat3Track motion_position;
  RawQuaternionTrack motion_rotation;
  RawAnimation output;

  {  // Same input and output.
    MotionExtractor extractor;
    EXPECT_FALSE(
        extractor(input, skeleton, &motion_position, &motion_rotation, &input));
  }

  {
    // Invalid animation
    RawAnimation input_invalid;
    input_invalid.tracks.resize(1);
    input_invalid.duration = -1.f;
    EXPECT_FALSE(input_invalid.Validate());

    MotionExtractor extractor;
    EXPECT_FALSE(extractor(input_invalid, skeleton, &motion_position,
                           &motion_rotation, &output));
  }

  {  // Valid
    MotionExtractor extractor;
    EXPECT_TRUE(extractor(input, skeleton, &motion_position, &motion_rotation,
                          &output));
  }
}

TEST(Extract, MotionExtractor) {}
