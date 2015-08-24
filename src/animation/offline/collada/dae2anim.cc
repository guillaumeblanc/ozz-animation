//----------------------------------------------------------------------------//
//                                                                            //
// ozz-animation is hosted at http://github.com/guillaumeblanc/ozz-animation  //
// and distributed under the MIT License (MIT).                               //
//                                                                            //
// Copyright (c) 2015 Guillaume Blanc                                         //
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

#include "ozz/animation/offline/tools/convert2anim.h"

#include "ozz/animation/offline/collada/collada.h"

// dae2anim is a command line tool that converts an animation imported from a
// Collada document to ozz runtime format.
//
// dae2anim extracts animated joints from a Collada document. Only the animated
// joints whose names match those of the ozz runtime skeleton given as argument
// are selected. Keyframes are then optimized, based on command line settings,
// and serialized as a runtime animation to an ozz binary archive.
//
// Use dae2anim integrated help command (dae2anim --help) for more details
// about available arguments.

class ColladaAnimationConverter :
  public ozz::animation::offline::AnimationConverter {
private:
  // Implement SkeletonConverter::Import function.
  virtual bool Import(const char* _filename,
                      const ozz::animation::Skeleton& _skeleton,
                      float _sampling_rate,
                      ozz::animation::offline::RawAnimation* _animation) {
    return ozz::animation::offline::collada::ImportFromFile(
      _filename, _skeleton, _sampling_rate, _animation);
  }
};

int main(int _argc, const char** _argv) {
  ColladaAnimationConverter converter;
  return converter(_argc, _argv);
}
