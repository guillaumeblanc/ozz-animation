---
title: Fingers control
layout: full
keywords: blend,additive,partial,blending,sample,tutorial,control,rig
---

{% include links.jekyll %}

Trying to see how [UE4 Control Rig fingers demo](https://twitter.com/AlanNoon/status/1256225763679965184?s=20) could work under the hood.

{% include emscripten.jekyll emscripten_path="proto/emscripten/sample_additive.js" %}

## Description

Additive blending is a key concept in run-time animation. Superimposing a movement on top of a playing animation allows to add variety while lessening animation count and management complexity. In this sample, 2 *splay* and *curl* hand animations are a added to a *walk* cycle. A weight is associated to each additive layers independently, allowing to control fingers.

## Concept

Additive blending is different from normal blending because it does not interpolate from a pose to an other. Instead it combines poses, meaning the two animations can be seen at the same time. In this example the walk cycle is never altered. Fingers curl and splay poses coming from the additive animation are simply added (visually speaking) to the base animation. For the purpose of this sample, only the first frames of the 2 additive animations are used, aka a fixed pose for each (curl and splay poses).

Additive blending is performed by ozz::animation::BlendingJob. BlendingJob exposes additive layers with the same input as normal blending layers: per joint local transforms, a global layer weight and optional per-joint weights. The main differences are that additive blending is done at the end of the normal blending pass, with a different equation.

The additive (or delta) animation is created by subtracting a reference pose from a source animation. ozz proposes a ozz::animation::offline::AdditiveAnimationBuilder utility to build additive animations. It uses the first frame of the animation as the reference pose. fbx2ozz and gltf2ozz support additive configuration option to export delta animations from a source file.

## Sample usage

The sample proposes to tune base and additive (curl & splay) layers weights. Additive layers weight are exposed via 2d slider to make it easier to control together.

## Implementation

1. Loads base, additive (curl and splay) animations, and their skeleton. See "playback" sample for more details.
2. At initialization time, the first frame of curl and splay animations are sampled in order to extract their respective local-space transformations. Sampling an additive animation is not different from a standard one. The 2 animations are deleted as only the extracted pose will be used further. 
3. At runtime, base animation is sampled to get skeleton base local-space transformations.
3. Setups ozz::animation::BlendingJob object. BlendingJob takes as input two arrays of BlendingJob::Layer, one for standard blending (base animation sampling output), the other for additive blending (the 2 curl and splay poses). Each layer is setup with its weights and the local-space transforms.
4. Convert local-space transformations to model-space matrices using ozz::animation::LocalToModelJob. It takes as input the skeleton (to know about joint's hierarchy) and local-space transforms outputted from the blending pass. Output is model-space matrices array.
5. Model-space matrices array is then used for rendering the animated skeleton pose (or the skinned mesh in a real world example).
