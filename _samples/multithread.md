---
title: Multi-threading
layout: full
keywords: animation,sampling,blending,playback,optimize,multithreads,threading,job,openmp,intel,tbb,thread
order: 70
level: 3
---

{% include links.jekyll %}

> Note for windows users: On Windows, this sample requires "VC++ Redistribuable Packages" to be installed. OpenMp runtime libraries are only available as Dlls (vcomp`*`.dll) indeed. Latest packages are available from [Microsoft](http://support.microsoft.com/kb/2019667).

Description
===========

The sample takes advantage of ozz jobs thread-safety to distribute sampling and local-to-model jobs across multiple threads, using OpenMp.
User can tweak the number of characters and the number of threads. Animation control is automatically handled by the sample for all characters.

![multithread sample screenshot]({{site.baseurl}}/images/samples/multithread.jpg)

Concept
=======

All ozz-animation jobs are thread-safe: [`ozz::animation::SamplingJob`][link_sampling_job], [`ozz::animation::BlendingJob`][link_blending_job], [`ozz::animation::LocalToModelJob`][link_local_to_model_job]... This is an effect of the data-driven architecture, which makes a clear distinction between data and processes (aka jobs). Jobs' execution can thus be distributed to multiple threads safely, as long as the data provided as inputs and outputs do not create any race conditions.
As a proof of concept, this sample uses a naive strategy: All characters' update (execution of their sampling and local-to-model stages, as demonstrated in [`playback sample`][link_playback_sample]) are distributed using an OpenMp parallel-for, every frame. During initialization, every character is allocated with all the data required for their own update, eliminating any dependency and race condition risk.

Sample usage
============

The sample allows to switch OpenMp multi-threading on/off and set the number of threads used to distribute characters' update. The number of characters can also be set from the GUI.

Implementation
==============

1. This sample extends [`playback sample`][link_playback_sample], and uses the same procedure to load skeleton and animation objects.
2. For each character, allocates runtime buffers (local-space transforms of type ozz::math::SoaTransform, model-space matrices of type ozz::math::Float4x4) with the number of elements required for the skeleton, and a sampling cache (ozz::animation::SamplingCache). Only the skeleton and the animation are shared amongst all characters, as they are read only objects, not modified during jobs execution.
3. Update function uses an OpenMp parallel-for loop to split up characters' update loop amongst OpenMp threads (sampling and local-to-model jobs execution). It allows all characters' update to be executed concurrently.