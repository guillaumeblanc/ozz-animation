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

#ifndef OZZ_DEMO_FRAMEWORK_RENDERER_H_
#define OZZ_DEMO_FRAMEWORK_RENDERER_H_

namespace ozz {
namespace animation { class Skeleton; }
namespace math { struct Float4x4; }
namespace demo {

// Defines renderer abstract interface.
class Renderer {
 public:

  // Declares a virtual destructor to allow proper destruction.
  virtual ~Renderer() {}

  // Initializes he renderer.
  // Return true on success.
  virtual bool Initialize() = 0;

  // Renders coordinate system axes: X in red, Y in green and W in blue.
  // Axes size is given by _scale argument.
  virtual void DrawAxes(float _scale) = 0;

  // Renders a square grid of _cell_count cells width, where each square cell
  // has a size of _cell_size.
  virtual void DrawGrid(int _cell_count, float _cell_size) = 0;

  // Renders a wireframe skeleton in its bind pose.
  virtual bool DrawSkeleton(const animation::Skeleton& _skeleton, 
                            bool _render_leaf) = 0;

  // Renders a wireframe skeleton in the posture given by _begin to _end
  // matrices.
  // If _render_leaf is true, it renders the leaf joints with the length of
  // their parent.
  // Returns true on success, or false otherwise:
  // -if any input pointer is NULL.
  // -if the size of (_end - _begin) is not bigger or equal to the skeleton's
  // number of joints.
  virtual bool DrawPosture(const animation::Skeleton& _skeleton,
                          const ozz::math::Float4x4* _begin,
                          const ozz::math::Float4x4* _end,
                          bool _render_leaf) = 0;
};
}  // demo
}  // ozz
#endif  // OZZ_DEMO_FRAMEWORK_RENDERER_H_
