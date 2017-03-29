---
title: Animation playback
layout: full
keywords: load,playback,sample,skeleton,hierarchy,joint,fbx,collada,3d,soa,local,model,space
order: 10
level: 1
---

{% include links.jekyll %}
{% include link_sample_code.jekyll sample="playback" %}

Description
===========
Loads a skeleton and an animation from ozz binary archives. This animation is then played-back and applied to the skeleton.
Animation time and playback speed can be tweaked.

{% include emscripten.jekyll emscripten_path="samples/emscripten/sample_playback.js" %}

Concept
=======
This sample loads ozz binary archive file, a skeleton and an animation. Ozz binary files can be produced with ozz command line tools (dae2skel tool ouputs a skeleton from a Collada document, dae2anim an animation), or with ozz serializer ([`ozz::io::OArchive`][link_io]) from your own application/converter.
At every frame the animation is sampled with [`ozz::animation::SamplingJob`][link_sampling_job]. Sampling local-space output is then converted to model-space matrices for rendering using [`ozz::animation::LocalToModelJob`][link_local_to_model_job].

Sample usage
============
Some parameters can be tuned from sample UI:
- Play/pause animation.
- Fix animation time.
- Set playback speed, which can be negative to go backward.

Implementation
==============
- Load animation and skeleton. Loading from file is covered in detail in [the howto section][link_how_to_load].
   - Open a `ozz::io::OArchive` object with a valid `ozz::io::Stream` as argument. The stream can be a `ozz::io::File`, or your custom io read capable object that inherits from `ozz::io::Stream`.
   - Check that the stream stores the expected object type using `ozz::io::OArchive::TestTag()` function. Object type is specified as a template argument.
   - Deserialize the object with `>> operator`.
- Allocates runtime buffers (local-space transforms of type `ozz::math::SoaTransform`, model-space matrices of type `ozz::math::Float4x4`) with the number of elements required for your skeleton. Note that local-space transform are Soa objects, meaning that 1 `ozz::math::SoaTransform` can store multiple (4) joints.
- Allocates sampling cache (`ozz::animation::SamplingCache`) with the number of joints required for your animation. This cache is used to store sampling local data as well as optimising key-frame lookup while reading animation forward.
- Sample animation to get local-space transformations using [`ozz::animation::SamplingJob`][link_sampling_job]. This job takes as input the animation, the cache and a time at which the animation should be sampled. Output is the local-space transformation array.
- Convert local-space transformations to model-space matrices using [`ozz::animation::LocalToModelJob`][link_local_to_model_job]. It takes as input the skeleton (to know about joint's hierarchy) and local-space transforms. Output is model-space matrices array.
- Model-space matrices array can then be used for rendering (to skin a mesh) or updating the scene graph.