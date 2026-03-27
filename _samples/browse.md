---
title: Animations set browsing
layout: full
keywords: browse,load,playback,skeleton,skinning,animation,library
order: 86
level: 3
---

{% include links.jekyll %}
{% include link_sample_code.jekyll sample="browse" %}

Description
===========

List animations and lets user browsing through and preview all animations.

{% include emscripten.jekyll emscripten_path="samples/emscripten/sample_browse.js" %}

Concept
=======
This sample lists all animations from set and allows to select which animation to playback.
It is based on skinning sample for the most part.
It loads a skeleton, an animation and skinned meshes from ozz binary archives. It playbacks animation every frame and uses model-space matrices to build skinning matrices and render a skinned mesh.

Sample usage
============

The sample UI exposes:

- A list of animations grouped by name prefix, with buttons to select the next animation or pick any animation from a group.
- Playback controls (play/pause, scrub time, speed) via the built-in playback controller.
- Rendering toggles:
  - Draw skeleton.
  - Draw mesh.
  - Show triangles, texture, vertices, normals, tangents, binormals, colors.
  - Wireframe.
  - Skip skinning.

Implementation
==============

1. Load the skeleton archive.
2. Read the animations list file and build a sorted list of animation archive filenames.
3. Randomly pick an animation at startup and load it from `media/<animation_name>`.
4. Allocate runtime buffers.
5. Every frame:
   1. Update animation time using the playback controller.
   2. Sample the current animation with `ozz::animation::SamplingJob`.
   3. Convert local-space transforms to model-space using `ozz::animation::LocalToModelJob`.
   4. Update skinning matrices and render the skeleton/mesh.
