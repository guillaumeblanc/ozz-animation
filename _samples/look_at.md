---
title: Look-at
layout: full
keywords: sample,inverse,kinematic,skeleton,hierarchy,joint,look,at,aim,ik,weapon,head,neck,spine,target,soa,local,model,space
order: 71
level: 3
---

{% include links.jekyll %}
{% include link_sample_code.jekyll sample="look_at" %}

Description
===========
Procedural look-at objective is to orientate character head in a direction (at a target position), without using any animation data. This comes as a correction on (some) joint(s) rotation, on top of current animation. The character could still use animation as a base look-at depending on target position, and use the procedural correction for the final/precise adjustment.

{% include emscripten.jekyll emscripten_path="samples/emscripten/sample_look_at.js" %}

Concept
=======
The sample relies on ozz::animation::IKAimJob which allows a single joint to aim at a targeted position. It extends the basic IKAimJob usage, by iteratively applying IK to head's ancestor joints, allowing to spread the rotation over more joints for a more realistic result. ozz::animation::IKAimJob::offset member (offset position from the joint in local-space) is a key component to achieve this result. It's used at first to tune eyes position compared to the head joint. During later iterations (for head ancestors), offset becomes even more relevant as the distance to the joint increases.

This same concept can be applied to aim a weapon at a precise target.

Sample usage
============
The sample allows to move target position or change its animation settings. If also exposes important setup aspects of the IK chain:
- Chain length: Number of joints (from the predefined hierarchy) that are corrected by IK.
- Joint weight: Weight given to IK for each joint of the chain (except for the last joint which is 1). This directly affect correction spreading.
- Chain weight: Weight given to the whole look-at process, allowing bending in and out.

Debug settings are exposed to help understanding:
- Displaying character forward vector is interesting to confirm algorithm precision.
- Displaying joints part of the look-at chain helps understand how weight and correction are spread.

Implementation
==============
1. At initialization time:
   1. Loads skeleton, animation and mesh. See "playback" sample for more details.
   2. Locates head joint and some of its ancestors, searching them by name.
2. At run time:
   1. Updates base animation and skeleton joints model-space matrices. See "playback" sample for more details.
   2. Iteratively updates from the first joint (closer to the head) to the last (the further ancestor, closer to the pelvis). For the first joint, aim IK is applied with the global forward and offset, so the forward vector aligns in direction of the target.
   3. If a weight lower that 1 is provided to the first joint, then it will not fully align to the target. In this case further joint will need to be updated:
      1. Rotates forward and offset position based on the result of the previous joint IK.
      2. Brings forward and offset back in next joint local-space.
      3. Applies IKAimJob.
   4. Aim is iteratively applied up to the last selected joint of the hierarchy. A weight of 1 is given to the last joint so we can guarantee target is reached.
   5. Updates model-space transforms. Note that they don't need to be updated between each pass, as joints are ordered from child to parent.