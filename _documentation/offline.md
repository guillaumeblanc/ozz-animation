---
title: Offline libraries
layout: full
keywords: offline, load, import, runtime, build, convert, optimize
order: 60
---

{% include links.jekyll %}

ozz-animation offline data structures
=====================================

Offline c++ data structure are designed to be setup and modified programmatically: add/remove joints, change animation duration and keyframes, validate data integrity... These structures define a clear public API to emphasize modifiable elements. They are intended to be used on the tool side, offline, to prepare and build runtime structures.

`ozz::animation::offline::RawSkeleton`
--------------------------------------

This structure declared in `ozz/animation/offline/skeleton_builder.h`. It is used to define the offline skeleton object that can be converted to the runtime skeleton using the SkeletonBuilder.
This skeleton structure exposes joints' hierarchy. A joint is defined with a name, a transformation (its bind pose), and its children. Children are exposed as a public std::vector of joints. This same type is used for skeleton roots, also exposed from the public API.
The public API exposed through std:vector's of joints can be used freely with the only restriction that the total number of joints does not exceed `ozz::animation::Skeleton::kMaxJoints`.

`ozz::animation::offline::RawAnimation`
---------------------------------------

This structure declared in `ozz/animation/offline/animation_builder.h`. It is used to define the offline animation object that can be converted to the runtime animation using the AnimationBuilder.
This animation structure exposes tracks of keyframes. Keyframes are defined with a time and a value which can either be a translation (3 float x, y, z), a rotation (a quaternion) or scale coefficient (3 floats x, y, z). Tracks are defined as a set of three std::vector (translation, rotation, scales). Animation structure is then a vector of tracks, along with a duration value.
Finally the RawAnimation structure exposes Validate() function to check that it is valid, meaning that all the following rules are respected:

- Animation duration is greater than 0.
- Keyframes' time are sorted in a strict ascending order.
- Keyframes' time are all within [0,animation duration] range.

Animations that would fail this validation will fail to be converted by the AnimationBuilder.
A valid RawAnimation can also be optimized (removing redundant keyframes) using AnimationOptimizer. See [optimize sample][link_optimize_sample] for more details.

ozz-animation offline utilities
===============================

ozz offline utilities are usually conversion functions, like `ozz::animation::offline::SkeletonBuilder` and `ozz::animation::offline::animationBuilder` are. Using the "builder" design approach frees the user from understanding internal details about the implementation (compression, memory packing...). It also allows to modify ozz internal implementation, without affecting existing user code.

`ozz::animation::offline::SkeletonBuilder`
------------------------------------------

The SkeletonBuilder utility class purpose is to build a [runtime skeleton][link_skeleton] from an offline raw skeleton. Raw data structures are suited for offline task, but are not optimized for runtime constraints. 

- __Inputs__
  - `ozz::animation::offline::RawSkeleton raw_skeleton`
- __Processing__
  - Validates input.
  - Builds skeleton breadth-first joints tree.
  - Packs bind poses to soa data structures.
  - Fills other runtime skeleton data structures (array of names...).
- __Outputs__
  - `ozz::animation::Skeleton skeleton`

`ozz::animation::offline::AnimationBuilder`
-------------------------------------------

The AnimationBuilder utility class purpose is to build a runtime animation from an offline raw animation. The raw animation has a simple API based on vectors of tracks and key frames, whereas the [runtime animation][link_animation] has compressed key frames structured and organized to maximize performance and cache coherency.

- __Inputs__
  - `ozz::animation::offline::RawAnimation raw_animation`
- __Processing__
  - Validates input.
  - Creates required first and last keys for all tracks (if missing).
  - Sorts keys to match runtime sampling job requirements, maximizing cache coherency.
  - Compresses translation and scales to three half floating point values, and quantizes quaternions to three 16bit integers (4th component is recomputed at runtime).
- __Outputs__
  - `ozz::animation::Animation animation`

`ozz::animation::offline::AnimationOptimizer`
---------------------------------------------

The AnimationOptimizer strips redundant/interpolable key frames from a raw animation. It doesn't actually modify input animation, but builds a second one. Tolerances are provided as input arguments, for every key frame type: translation, rotation and scale.

- __Inputs__
  - `ozz::animation::offline::RawAnimation raw_animation`
  - `float translation_tolerance`: Translation optimization tolerance, defined as the distance between two values in meters.
  - `float rotation_tolerance`: Rotation optimization tolerance, ie: the angle between two rotation values in radian.
  - `float scale_tolerance`: Scale optimization tolerance, ie: the norm of the difference of two scales.
- __Processing__
  - Validates inputs.
  - Computes the maximum length of each joint's hierarchy for the whole animation.
  - Filters (aka removes) all key frames that can be interpolated within the specified tolerance, using joint's maximum hierarchy length.
  - Builds output raw animation.
- __Outputs__
  - `ozz::animation::offline::RawAnimation raw_animation`