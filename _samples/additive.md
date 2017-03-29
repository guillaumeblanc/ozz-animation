---
title: Additive blending
layout: full
keywords: blend,additive,partial,blending,sample,tutorial
order: 35
level: 2
---

{% include links.jekyll %}
{% include link_sample_code.jekyll sample="additive" %}

Description
===========
Additive blending is a key concept in run-time animation. By superimposing a movement on top of a playing animation, it allows to add variety while lessening animation count and complexity.
In this sample, a "cracking head" animation is added to a "walk" cycle. Note that no synchronization is required between the two animations.

{% include emscripten.jekyll emscripten_path="samples/emscripten/sample_additive.js" %}

Concept
=======
Additive blending is different to normal blending because it does not interpolate from a pose to an other. Instead it combines poses, meaning you can see the two animations at the same time. In this example the walk cycle is never altered. The cracking head and shoulder movements from the additive animation are simply (visually speaking) added.

Additive blending is performed by ozz::animation::BlendingJob. ozz::animation::BlendingJob exposes additive layers with the same input as normal blending layers: per joint local transforms, a global layer weight and optional per-joint weights. The main differences are that additive blending is done at the end of the normal blending pass, with a different equation.

The additive (or delta) animation is created by subtracting a reference pose from a source animation. ozz proposes a ozz::animation::offline::AdditiveAnimationBuilder utility to build additive animations. It uses the first frame of the animation as the reference pose. fbx2anim supports --additive option to export delta animations from a source file.

Additionally, the sample uses an optional mask to blend the additive animation on the character upper body only. See partial blending sample for more details.

Sample usage
============
The sample proposes to modify main and additive layers weights:
- Normal blending layer weight.
- Additive blending layer weight.
- Upper body masking activation and base joint selection.

Both main and additive animation playback parameters are exposed.

Implementation
==============
1. Loads main, additive animations, and their skeleton. See "playback" sample for more details.
2. Samples each animation to get local-space transformations. Sampling an additive animation is not different from a standard one.
3. Setups ozz::animation::BlendingJob object. BlendingJob object takes as input two arrays of BlendingJob::Layer, one for standard blending, the other for additive blending. Each layer is setup with its weights and the local-space transforms outputted from the sampling stage.
4. Convert local-space transformations to model-space matrices using ozz::animation::LocalToModelJob. It takes as input the skeleton (to know about joint's hierarchy) and local-space transforms outputted from the blending pass. Output is model-space matrices array.
5. Model-space matrices array is then used for rendering the skinned mesh.