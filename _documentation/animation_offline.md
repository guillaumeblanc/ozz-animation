---
title: Offline libraries
layout: full
keywords: offline,load,import,runtime,build,convert,optimize,reduction,compress,quantize
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

`ozz::animation::offline::Raw*Track`
---------------------------------------

This offline track data structure is meant to be used for user-channel tracks, aka animation of variables that aren't joint transformation. It is available for tracks of 1 to 4 floats (RawFloatTrack, RawFloat2Track, ..., RawFloat4Track) and quaternions (RawQuaternionTrack). Quaternions differ from float4 because of the specific interpolation and comparison treatment they require.

As all other Raw data types, they are not intended to be used in run time. They are used to define the offline track object that can be converted to the runtime one using the a [`ozz::animation::offline::TrackBuilder`][link_track_builder].

This animation structure exposes a single sequence of keyframes. Keyframes are defined with a ratio, a value and an interpolation mode:
- Ratio: A track has no duration, so it uses ratios between 0 (beginning of the track) and 1 (the end), instead of times. This allows to avoid any discrepancy between the durations of tracks and the animation they match with.
- Value: The animated value (float, ... float4, quaternion).
- Interpolation mode (`ozz::animation::offline::RawTrackInterpolation`): Defines how value is interpolated with the next key.
  - kStep: All values following this key, up to the next key, are equal.
  - kLinear: All value between this key and the next are linearly interpolated.

Track structure is then a sorted vector of keyframes.

RawTrack structure exposes a Validate() function to check that all the following rules are respected:
1. Keyframes' ratios are sorted in a strict ascending order.
2. Keyframes' ratios are all within [0,1] range.

A RawTrack that would fail this validation will fail to be converted by the [`ozz::animation::offline::TrackBuilder`][link_track_builder].

ozz-animation offline utilities
===============================

ozz offline utilities are usually conversion functions, like `ozz::animation::offline::SkeletonBuilder` and `ozz::animation::offline::AnimationBuilder` are. Using the "builder" design approach frees the user from understanding internal details about the implementation (compression, memory packing...). It also allows to modify ozz internal implementation, without affecting existing user code.

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
The optimizer also takes into account for each joint the error generated on its whole child hierarchy, with the hierarchical tolerance value. This allows for example to take into consideration the error generated on a finger when optimizing the shoulder. A small error on the shoulder can be magnified when propagated to the finger indeed.
Default optimization tolerances are set in order to favor quality over runtime performances and memory footprint.

- __Inputs__
  - `ozz::animation::offline::RawAnimation raw_animation`
  - `float translation_tolerance`: Translation optimization tolerance, defined as the distance between two values in meters.
  - `float rotation_tolerance`: Rotation optimization tolerance, ie: the angle between two rotation values in radian.
  - `float scale_tolerance`: Scale optimization tolerance, ie: the norm of the difference of two scales.
  - `float hierarchical_tolerance`: Hierarchical translation optimization tolerance, ie: the maximum error (distance) that an optimization on a joint is allowed to generate on its whole child hierarchy.
  float hierarchical_tolerance;
- __Processing__
  - Validates inputs.
  - Computes the maximum length of each joint's hierarchy for the whole animation.
  - Filters (aka removes) all key frames that can be interpolated within the specified tolerance, using joint's maximum hierarchy length.
  - Builds output raw animation.
- __Outputs__
  - `ozz::animation::offline::RawAnimation raw_animation`

`ozz::animation::offline::TrackBuilder`
---------------------------------------

Defines the class responsible for building [runtime track][link_track] instances from [offline/raw tracks][link_offline_track]. The input raw track is first validated. Runtime conversion of a validated raw track cannot fail. Note that no optimization is performed on the data at all.

`ozz::animation::offline::TrackOptimizer`
-----------------------------------------

Defines the class responsible for optimizing an [offline tracks][link_offline_track] instance. Optimization is a keyframe reduction process. Redundant and interpolable keyframes (within a tolerance value) are removed from the track.
Default optimization tolerances are set in order to favor quality over runtime performances and memory footprint.