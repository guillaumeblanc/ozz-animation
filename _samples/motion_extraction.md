---
title: Motion extraction
layout: full
keywords: root,motion,position,rotation,bake,loop,capsule,character,track,animation,sample,
order: 47
level: 2
---

{% include links.jekyll %}
{% include link_sample_code.jekyll sample="motion_extraction" %}

Description
===========

Extracting root motion is the process of capturing character motion (translation and rotation) from an animation. This motion is re-applied at runtime to drive the character and its collision capsule.
This sample allows to tweak motion extraction settings by exposing all the parameters available to the motion extraction utility.

{% include emscripten.jekyll emscripten_path="samples/emscripten/sample_motion_extraction.js" %}

Concept
=======

The samples relies `ozz::animation::offline::MotionExtractor` utility to run motion extraction and exposes all its parameters.
The utility capture root motion from a raw animation into tracks (translation and rotation) that are use at runtime to sample character location using [`ozz::animation::offline::TrackSamplingJob`][link_track_sampling_job].

Sample usage
============

This samples allows to tweak motion extraction using all utility parameters:
  - For position and rotation, multiple settings:
    - X, Y, Z: Select which component is captured  (pitch/yaw/roll for rotations).
    - Reference: Choose extraction reference, within global, skeleton or animation option.
    - Bake: Choose to bake extracted data to output animation.
    - Loop: Makes end transformation equal to begin to make animation loop-able. Difference between end and begin is distributed all along animation duration.
  - Apply position and rotation motion at runtime.

Implementation
==============

1. Import the runtime skeleton and a raw animation objects. These are imported from files as described in the "how to load an object from a file" section. Note that the animation is still in an offline format at this stage.
2. Extract motion from the raw animation using `ozz::animation::offline::MotionExtractor` utility, into a [`ozz::animation::offline::RawFloat3Track`][link_raw_track] for the position and [`ozz::animation::offline::RawQuaternionTrack`][link_raw_track] for the rotation.
3. Optimizes the raw animation and the 2 raw tracks using [`ozz::animation::offline::AnimationOptimizer`][link_animation_optimizer] and [`ozz::animation::offline::TrackOptimizer`][link_track_optimizer].
3. Builds runtime animation and the 2 runtime tracks using [`ozz::animation::offline::AnimationBuilder`][link_animation_builder] and [`ozz::animation::offline::TrackBuilder`][link_track_builder].
4. At runtime, sample animation and convert it to model space as usual. See "playback" sample for more details.
5. Samples the 2 position and rotation tracks, using [`ozz::animation::offline::TrackSamplingJob`][link_track_sampling_job].
6. Renders character at the position and rotation sampled from the tracks.