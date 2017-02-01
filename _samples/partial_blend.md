---
title: Partial animations blending
layout: full
keywords: animation,blending,partial,lower,upper,body,arm,head,character,skeleton,additive
order: 30
level: 2
---

{% include links.jekyll %}

Description
===========

Uses partial animation blending technique to animate the lower and upper part of the skeleton with different animations.

{% include emscripten.jekyll emscripten_path="samples/emscripten/sample_partial_blend.js" %}

Concept
=======

Partial animation blending uses the same job as the full blending technique (aka [`ozz::animation::BlendingJob`][link_blending_job]). See [blend][link_blend_sample] sample for more details. Partial blending uses a coefficient per joint to weight the animation influence. This per-joint weight is modulated with layer's weight to compute the final influence of every joint. This set of weight coefficients are provided to the `BlendingJob::Layer` as an array of SoA floating point values, arranged in the same order as skeleton joints.
The sample uses `ozz::animation::IterateJointsDF` helper function to iterate all children of the upper body "root" joint, and set up per-joint weight masks.

Sample usage
============

The sample proposes two modes of interaction:
- Automatic: With this mode the GUI proposes to automatically control all blending parameters from a single "upper body weight" slider. The selected coefficient is used to set up the per-joint weights of the upper body layer, while lower body weights are set to one minus upper body weights.
- Manuel: Allows to setup all blending parameters independently.
The GUI also proposes to select the "root" joint of the upper body hierarchy, which is affected by the partially animated layer. 

Implementation
==============

1. This sample extends "blend" sample, and uses the same procedure to load skeleton and animations objects.
2. Samples each animation at their own speed, they do not need to be synchronized.
3. Prepares per-joint weight masks for the two lower and upper body layers. If interaction mode is set to automatic, then lower and upper body weight coefficients are computed from the single "Upper body weight" parameter. In this case upper body coefficient equals one minus lower body coefficient. If interaction mode is set to manual, then all parameters are set manually.
4. The sample uses `ozz::animation::IterateJointsDF` helper function to iterate all children of the upper body root joint, and set up per-joint weight masks as follows (note that weight coefficients are stored as SoA floats):
   - Upper body weight mask: Affects upper body weight coefficient to all the joints that are part of the upper body, all others are set to zero.
   - Lower body weight mask: Affects lower body weight coefficient to all the joints that are part of the lower body (ie: all the ones that are not part of the upper body), all others are set to one.
5. Sets [`ozz::animation::BlendingJob`][link_blending_job] object with the two layers for the lower and upper body. Per-joint weight masks are provided as an input to each layer. All other arguments (bind-pose, input local-space transforms, weights, output) are the same as those used by the full skeleton hierarchy blending. See "blend" sample for more details.
6. Converts [local-space transformations][link_coordinate_system], outputted from the blending stage, to model-space matrices using [`ozz::animation::LocalToModelJob`][link_local_to_model_job]. It also takes as input the skeleton (to know about joint's hierarchy). Output is model-space matrices array.
7. Model-space matrices array can then be used for rendering (to skin a mesh) or updating the scene graph.
