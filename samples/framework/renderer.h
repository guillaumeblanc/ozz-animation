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

#ifndef OZZ_SAMPLES_FRAMEWORK_RENDERER_H_
#define OZZ_SAMPLES_FRAMEWORK_RENDERER_H_

#include "ozz/base/platform.h"

namespace ozz {
namespace animation { class Skeleton; }
namespace math { struct Float4x4; struct Float3; struct Box; }
namespace sample {

// Defines renderer abstract interface.
class Renderer {
 public:

  // Defines render Color structure.
  struct Color {
    unsigned char r, g, b, a;
  };

  // Declares a virtual destructor to allow proper destruction.
  virtual ~Renderer() {}

  // Initializes he renderer.
  // Return true on success.
  virtual bool Initialize() = 0;

  // Renders coordinate system axes: X in red, Y in green and W in blue.
  // Axes size is given by _scale argument.
  virtual void DrawAxes(const ozz::math::Float4x4& _transform) = 0;

  // Renders a square grid of _cell_count cells width, where each square cell
  // has a size of _cell_size.
  virtual void DrawGrid(int _cell_count, float _cell_size) = 0;

  // Renders a skeleton in its bind pose posture.
  virtual bool DrawSkeleton(const animation::Skeleton& _skeleton,
                            const ozz::math::Float4x4& _transform,
                            bool _draw_joints = true) = 0;

  // Renders a skeleton at the specified _position in the posture given by model
  // space _matrices.
  // Returns true on success, or false if _matrices range does not match with
  // the _skeleton.
  virtual bool DrawPosture(const animation::Skeleton& _skeleton,
                           ozz::Range<const ozz::math::Float4x4> _matrices,
                           const ozz::math::Float4x4& _transform,
                           bool _draw_joints = true) = 0;

  // Renders a box at a specified location.
  // The 2 slots of _colors array respectively defines color of the filled
  // faces and color of the box outlines.
  virtual bool DrawBox(const ozz::math::Box& _box,
                       const ozz::math::Float4x4& _transform,
                       const Color _colors[2]) = 0;

  class Mesh {
   public:
    Mesh(int _vertex_count, int _index_count);
    ~Mesh();

    template<typename _Ty>
    struct Buffer {
      typedef Range<_Ty> DataRange;
      DataRange data;
      size_t stride;
    };

    // Vertices are a buffered positions and normals.
    typedef Buffer<char> Vertices;
    Vertices vertices() const;

    // Positions are a buffer of 3 consecutive floats per vertex.
    typedef Buffer<float> Positions;
    Positions positions() const;

    // Normals are a buffer of 3 float per vertex.
    typedef Buffer<float> Normals;
    Normals normals() const;

    // Normals are a buffer of 4 unsigned byte per vertex.
    struct Color {uint8_t red, green, blue, alpha;};
    typedef Buffer<Color> Colors;
    Colors colors() const;

    // Indices are a buffer of 3 consecutive uint16_t per triangle.
    typedef Buffer<uint16_t> Indices;
    Indices indices() const;

   private:
     Mesh(const Mesh&);
     void operator=(const Mesh&);

     Vertices::DataRange vertices_;
     Indices::DataRange indices_;
  };

  // Renders a mesh at a specified location.
  virtual bool DrawMesh(const ozz::math::Float4x4& _transform,
                        const Mesh& _mesh) = 0;
};
}  // sample
}  // ozz
#endif  // OZZ_SAMPLES_FRAMEWORK_RENDERER_H_
