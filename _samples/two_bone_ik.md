---
title: Two bone IK
layout: full
keywords: sample,skeleton,hierarchy,joint,ik,two,bone,chain,fast,maya,mobu,motion,builder,rotate,plane,soa,local,model,space
order: 65
level: 2
---

{% include links.jekyll %}
{% include link_sample_code.jekyll sample="two_bone_ik" %}

Description
===========

Performs two bone IK on robot arm skeleton.

{% include emscripten.jekyll emscripten_path="samples/emscripten/sample_two_bone_ik.js" %}

Concept
=======

This sample uses ozz::animation::IKTwoBoneJob to compute two bone IK on a skeleton. Inverse kinematic allows a chain of joints to reach a target position. The job computes the transformations (rotations only) that needs to be applied to the first two joints of the chain such that the third joint reaches the provided target position (if possible). The three joints don't need to be consecutive though, they just require to be from the same chain. 

Sample usage
============

The sample exposes IKTwoBoneJob parameters:

- The pole vector which defines where the direction the middle joint should point to, allowing to control IK chain orientation.
- Twist_angle which rotates IK chain around the vector define by start-to-target vector.
- Soften ratio, allowing the chain to gradually fall behind the target position. This prevents the joint chain from snapping into the final position, softening the final degrees before the joint chain becomes flat.
- Weight given to the IK correction. This allows to blend / interpolate from zero to full IK.
  float weight;

Target position is animated by the sample, but can also be tweaked manually, as well as skeleton root transformation. Fix initial transform option select whether ik job is run from the skeleton bind pose or the last frame transforms. This allows to use IK job's weighting parameter, as well as stressing it with a wide range of input.

Implementation
==============

1. At initialization time:
  1. Loads skeleton. See "playback" sample for more details.
  2. Locates the three joint indices from the skeleton, searching them by name.
2. At run time:
  1. Updates skeleton joints model-space matrices from local-space transforms, just like it would be done at the end of animation sampling and blending stages.
  2. Target and pole vectors are converted to skeleton model space.
  3. Setup IKTwoBoneJob with sample parameters, providing the three joints model-space matrices.
  4. Once run, the job outputs two quaternions for the two first joints of the chain. The samples multiplies them to their respective local-space rotations.
  5. Model-space matrices must be updated again. Only the children of the first joint of the chain need to be updated again.