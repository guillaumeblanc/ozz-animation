---
title: Offline libraries and tools
layout: full
keywords: offline, load, import, runtime, build, convert, optimize
---

{% include references.jekyll %}

Pipeline description
====================

ozz-animation provides full support for Collada and Fbx formats. Those formats are heavily used by the animation industry and supported by all major Digital Content Creation tools (Maya, Max, MotionBuilder, Blender...). ozz-animation offline pipeline aims to convert from these DCC offline formats (or any proprietary format) to ozz internal runtime format, as illustrated below:

![ozz-animation offline pipeline]({{site.baseurl}}/images/offline_pipeline.png)

In a way or another, the aim of this pipeline and importer tools is to end up with runtime data structures (`ozz::animation::Skeleton`, `ozz::animation::Animation`) that can be used with the run-time libraries, on any platform. Run-time libraries provide jobs and data structures to process runtime operations like sampling, blending...

Standard pipeline
-----------------

ozz-animation provides a standard pipeline to import skeletons and animations from Collada documents and Fbx files. This pipeline is based on command-line tools: dae2skel and dae2anim respectively for Collada skeleton and animation, fbx2skel and fbx2anim for Fbx.

dae2skel and fbx2skel command line tools require as input a file containing the skeleton to import (dae or fbx file), using \-\-file argument. Output filename is specified with \-\-skeleton argument.

{% highlight cpp %}
dae2skel --file="path_to_source_skeleton.dae" --skeleton="path_to_output_skeleton.ozz"  
{% endhighlight %}

The extension of the output file is not important though.

dae2anim and fbx2anim command line tools require two inputs:

- A dae or fbx file containing the animation to import, specified with \-\-file argument.
- A file containing an ozz runtime skeleton (usually outputted by dae2skel or fbx2skel), specified with \-\-skeleton argument.

Output filename is specified with \-\-animation argument. Like for skeleton, the extension of the output file is not important.

{% highlight cpp %}
dae2anim --file="path_to_source_animation.dae" --skeleton="path_to_skeleton.ozz" --animation="path_to_output_animation.ozz"
{% endhighlight %}

Some other optional arguments can be provided to the animation importer tools, to setup keyframe optimization ratios, sampling rate... The detailed usage for all the tools can be obtained with \-\-help argument.

Collada and Fbx importer c++ sources and libraries (ozz_animation_collada and ozz_animation_fbx) are also provided to integrate their features to any application.

Custom pipeline
---------------

Offline ozz-animation c++ libraries (`ozz_animation_offline`) are available, in order to implement custom format importers. This library exposes offline skeleton and animation data structures. They are then converted to runtime data structures using respectively `ozz::animation::offline::SkeletonBuilder` and `ozz::animation::offline::AnimationBuilder` utilities. Animations can also be optimized (removing redundant keyframes) using `ozz::animation::offline::AnimationOptimizer`.
Usage of `ozz_animation_offline` library is covered in the howto sections [write a custom skeleton importer][link_howto_custom_skeleton_importer] and [write a custom animation importer][link_howto_custom_animation_importer]. It is also demonstrated by [millipede sample][link_millipede_sample] and [optimize sample][link_optimize_sample] samples.

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