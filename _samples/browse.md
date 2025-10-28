---
title: Animations set browsing
layout: full
keywords: browse,load,playback,skeleton,skinning
order: 86
level: 3
---

{% include links.jekyll %}
{% include link_sample_code.jekyll sample="browse" %}

Description
===========

This sample lists all animations from set and allows to select which animation to playback.
It loads a skeleton, an animation and skinned meshes from ozz binary archives. It playbacks animation every frame and uses model-space matrices to build skinning matrices and render a skinned mesh.

{% include emscripten.jekyll emscripten_path="samples/emscripten/sample_browse.js" %}

Concept
=======

This sample is based on skinning sample for the most part, reading and sampling an animation, loading an ozz binary mesh file which was generated with sample_fbx2mesh tool.
Every time a new animation from the set is selected through UI, the previous one is unloaded and the new one loaded. 

Sample usage
============

Animation selection.

Animation playback parameters can be tuned from sample UI:
- Play/pause animation.
- Fix animation time.
- Set playback speed, which can be negative to go backward.

Rendering options are also exposed:
- Enabling skeleton display.
- Enabling mesh display.
- Enabling skinning stage.
- Display normals, tangent and binormals.

## Implementation

1. Load animation and skeleton, sample animation to get local-space transformations, and finally convert local-space transformations to model-space matrices. See Playback sample for more details about these steps.
2. Load meshes from ozz archive. There can be multiple meshes as import utility (aka fbx2mesh) maintains dcc file meshes split.
3. Computes and allocates skinning matrices. Number of skinning matrices might be less from the number of joints, as a mesh might be skinned by a subset of all skeleton joints only. Mesh::joint_remaps is used to know how to order skinning matrices, hence is size defines their number.   
4. Skinning matrices array is updated before rendering each mesh. A skinning matrix is the multiplication of the model-space and the mesh inverse bind pose matrix for a joint. Mesh::joint_remaps is used to index skeleton joints, so they match with the mesh.
5. Skinning is performed by ozz::geometry::SkinningJob. Please check sample framework DrawSkinnedMesh function for more details about how to setup that job.