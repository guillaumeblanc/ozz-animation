---
title: Inverse kinematic
layout: full
keywords: ik,inverse kinematic,two,bone,aim,chain,soa,sse,job,pole,vector,soft
order: 65
---

{% include links.jekyll %}

ozz has support for simple (and efficient) IK solvers. IK stage is usually applied after animation has been sampled and blended. They take as input model-space matrices and output correction quaternions that must be applied back to local-space transforms. The reason for that is that model-space matrices (actually all children of any modified joint local-space transform) from the whole chain shall be recomputed again once a joint transform has changed. Also only local-space transforms can be blended, which might be needed even after IK stage. 

`ozz::animation::IKTwoBoneJob`
==============================

Two bone IK solver can procedurally animate a chain of three joints (aka two bones) that remains on the same plane: like arms (shoulder, elbow, wrist) or legs (hip, knee, ankle). While it only cares about three joints, there can be intermediate joints between them. They will remain fixed. The only restriction is that the 3 joints should be part of the same hierarchy, aka ancestors of the last (leaf) joint.

<div class="w3-responsive">
  <table class="w3-table-all w3-centered w3-card-2">
    <tr>
      <td><img src="{{site.baseurl}}/images/documentation/two_bone_ik_a.svg" class="w3-image"></td>
      <td><img src="{{site.baseurl}}/images/documentation/two_bone_ik_b.svg" class="w3-image"></td>
      <td><img src="{{site.baseurl}}/images/documentation/two_bone_ik_c.svg" class="w3-image"></td>
    </tr>
    <tr>
      <td>A. No target/IK</td>
      <td>B. Reachable target</td>
      <td>C. Non reachable target</td>
    </tr>
  </table>
</div>

A. No target/IK: Shows a leg joint chain, at rest position, without any IK target. Start (hip), middle (knee) and end (ankle) are the three joints used by the solver. The foot in this example has a fixed right angle direction compare to the tibia.

B. Reachable target: A target in a reachable range is provided. S and M angles (rotations) are corrected so end joint can reach the target. Bones (Femur and Tibia) length are not modified. The angle between the tibia and foot isn't either.

C. Non reachable target: Target position is further than joint chain length (femur + tibia). S and M rotations are corrected so the chain (start, middle and end joints) aligns in target direction.

If the two joints have different lengths, then there can be other situations where target is not reachable.

Reachability can be obtained as an output of the the job.

The plane (defined by the three joints) can be rotated using the pole vector and twist angle. The pole vector defines a direction in which the middle joint will be pulled. Twist angles defines a rotation around start joint to end joint vector.

This animation shows how softening affects IK, and reduces the snapping effect when the chain is getting aligned / flat. Softening ratio is defined as the distance to the chain end, from which softening is starting. Note that target is considered as not reached as soon as softening starts.

`ozz::animation::IKAimJob`
==============================

Aim IK solver acts on a single joint to orientate (aim) a forward vector (in joint local-space) to a target position (in model-space): Aim a head joint so it looks at a target, aim a weapon... Up vector (in joint local-space) is also used to keep the joint oriented in the same direction as the pole vector (provided as input to the job).
The job also exposes an offset (in joint local-space) from where the forward vector should aim the target (like eyes position relative to the neck). This allows high precision aiming, or as illustrated in [look at sample][link_look_at_sample], to iteratively distribute aiming to a chain of joints.

{% include emscripten.jekyll emscripten_path="samples/emscripten/sample_look_at.js" %}
