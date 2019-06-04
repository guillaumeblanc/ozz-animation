---
title: Animation blending
layout: full
keywords: sample,blend,mix,transition,weight,interpolate,lerp,slerp,nlerp
order: 20
level: 1
---

{% include links.jekyll %}
{% include link_sample_code.jekyll sample="blend" %}

Description
===========
Blending is the key concept in run-time animation:

- To create smooth transitions between animations. In this case blending is used to go from an "idle" animation to a "walk" animation for example. Blending coefficient will go from 0 (idle) to 1 (walk) from the beginning to the end of the transition.
- To compose an animation from multiple animations, like a "fast walk" animation that would result from a "walk" blent with a "run" animation. In this case the two animations must be play-backed in sync (adapt playback speed to equalize animations' duration).

This sample demonstrates the composition blending case (2). It blends three animations (walk, jog and run) according to a single "speed" coefficient: The closer the speed is from 0, the closer the result is to the walk animation. The closer the speed is from 1, the closer the result is to the run animation.

{% include emscripten.jekyll emscripten_path="samples/emscripten/sample_blend.js" %}

Concept
=======
Runtime blending is performed using a [`ozz::animation::BlendingJob`][link_blending_job] to process the blending of multiple animation layers. Each layer is assigned an animation and a weight that sets its influence in the final blended result.
The idea of this sample is to compute the blending weight of each animation layer (walk, jog and run) according to the single "speed" parameter. This can done with a simple proportional calculation. Those blending coefficients are provided as input to the `ozz::animation::BlendingJob`, along with [local-space transformations][link_coordinate_system] for the 3 layers. Output will also be in local-space.
Note that the 3 animations duration also need to be synchronized such that foot steps remain synced. This is done by adapting each animation's playback speed according to their respecting weight.

Sample usage
============
There are two ways to use the sample:
- The first one is to use the single blending parameter provided by the "Blend ratio" slider. This parameter automatically controls all the other parameters to perfectly match/synchronize all animation layers: playback speeds, blending weights...
- Setting "manual setting" option to "on" will allow to manually tweak all the sampling and blending parameters from the GUI, bypassing automatic weighting and synchronization.

Implementation
==============
1. Load animations and skeleton. See "playback" sample for more details.
2. Compute each animation time (in order to sync their duration) and samples each of them to get local-space transformations.
3. Compute each animation (layer) blend weight and fills ozz::animation::BlendingJob object. BlendingJob object takes as input an array of BlendingJob::Layer representing each layer to blend: weight and local-space transformations (as outputed from the sampling stage). It also takes as input the skeleton bind-pose, which represents the default transformation of each joint. It is used by the blending algorithm as a fall-back when the accumulated layer weight is too small (under a threshold value which is also an input) to be used. The output of the blending job is a set of local-space transformations.
4. Convert local-space transformations to model-space matrices using ozz::animation::LocalToModelJob. It takes as input the skeleton (to know about joint's hierarchy) and local-space transforms. Output is model-space matrices array.
5. Model-space matrices array can then be used for rendering (to skin a mesh) or updating the scene graph.