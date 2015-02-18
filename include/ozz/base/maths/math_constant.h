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

#ifndef OZZ_OZZ_BASE_MATHS_MATH_CONSTANT_H_
#define OZZ_OZZ_BASE_MATHS_MATH_CONSTANT_H_

#ifndef INCLUDE_OZZ_MATH_CONSTANT_H_
#define INCLUDE_OZZ_MATH_CONSTANT_H_

namespace ozz {
namespace math {

// Defines math trigonometric constants.
static const float k2Pi = 6.283185307179586476925286766559f;
static const float kPi = 3.1415926535897932384626433832795f;
static const float kPi_2 = 1.5707963267948966192313216916398f;
static const float kSqrt3 = 1.7320508075688772935274463415059f;
static const float kSqrt3_2 = 0.86602540378443864676372317075294f;
static const float kSqrt2 = 1.4142135623730950488016887242097f;
static const float kSqrt2_2 = 0.70710678118654752440084436210485f;

// Angle unit conversion constants.
static const float kDegreeToRadian = kPi / 180.f;
static const float kRadianToDegree = 180.f / kPi;

// Defines the square normalization tolerance value.
static const float kNormalizationTolerance = 1e-6f;
static const float kNormalizationToleranceEst = 5e-3f;
}  // math
}  // ozz

#endif // INCLUDE_OZZ_MATH_CONSTANT_H_
#endif  // OZZ_OZZ_BASE_MATHS_MATH_CONSTANT_H_
