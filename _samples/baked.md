---
title: Baked physic simulation
layout: full
keywords: sample, tutorial, attach, baked, physic, local, model, space, transformation, matrix, instanced, camera
order: 55
level: 2
---

Description
===========

This samples shows a physic simulation baked into an animation. This scene contains more than 1000 cuboids. Baking complex scenes offline into animations in a common technique to render cpu intensive simulations.

{% include emscripten.jekyll emscripten_path="samples/emscripten/sample_baked.js" %}

Concept
=======

This sample has two fundamental parts:
1. Extract a skeleton from the baked scene (fbx2baked). The baked scene is made of animated meshes/cubes. There's no hierarchy between objects, but it's still required to define a skeleton to be able to animate it, aka assign animated tracks. Animation extraction is using fbx2anim as usual.
2. Render animated meshes. The original scene is made of cuboids of different sizes. The sample doesn't import scene meshes, but renders unit size cubes instead. Each cube is actually scaled by its animation scale track which was extracted from its corresponding mesh scale.

This sample also introduces camera animation. The camera is considered as a joint in the scene skeleton. Camera animation track is extracted in the same way meshes animations are. The sample code then forwards animated camera matrix to the renderer each frame.

Sample usage
============

The sample exposes animation time and playback speed.

Implementation
==============

1. fbx2baked implementation is based on fbx2skel. The only difference being that fbx2baked does not search for joints, but for meshes.
2. Load animation and skeleton, sample animation to get local-space transformations, and convert local-space transformations to model-space matrices. See Playback sample for more details about these steps.
3. Render a unit size cube for each joint, transformed by each joint matrix which contains the scale to restore original mesh size. The sample uses instanced rendering to improve performances.
