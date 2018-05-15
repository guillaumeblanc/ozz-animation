---
title: Baked physic simulation
layout: full
keywords: sample,tutorial,baked,physic,transformation,matrix,instanced,animated,camera
order: 55
level: 2
---

{% include links.jekyll %}
{% include link_sample_code.jekyll sample="baked" %}

Description
===========

This samples shows a physic simulation baked into an animation. This scene contains more than 1000 cuboids. Baking complex scenes offline into animations in a common technique to render cpu intensive simulations.

{% include emscripten.jekyll emscripten_path="samples/emscripten/sample_baked.js" %}

Concept
=======

This sample has two fundamental parts:
1. Extract a skeleton from the baked scene using fxb2ozz. There's no hierarchy between objects, but it's still required to define a skeleton to be able to animate it. The baked scene is made of animated meshes/cubes, which are considered as joints while importing the skeleton. See [config.json][link_baked_sample_config] for more details of this setting.
2. Animation extraction is using fbx2ozz as usual.
3. Render animated meshes. The original scene is made of cuboids of different sizes. The sample doesn't import scene meshes, but renders unit size cubes instead. Each cube is actually scaled by its animation scale track which was extracted from its corresponding mesh scale.

This sample also introduces camera animation. The camera is considered as a joint in the scene skeleton. Camera animation track is extracted in the same way meshes animations are. The sample code then forwards animated camera matrix to the renderer each frame.

Sample usage
============

The sample exposes animation time and playback speed.

Implementation
==============

1. Load animation and skeleton, sample animation to get local-space transformations, and convert local-space transformations to model-space matrices. See Playback sample for more details about these steps.
2. Render a unit size cube for each joint, transformed by each joint matrix which contains the scale to restore original mesh size. The sample uses instanced rendering to improve performances.
