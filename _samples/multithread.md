---
title: Multi-threading
layout: full
keywords: optimize,multithreads,threading,job,openmp,intel,tbb,thread,distribute,workload
order: 90
level: 3
---

{% include links.jekyll %}
{% include link_sample_code.jekyll sample="multithread" %}

Description
===========

The sample takes advantage of ozz jobs thread-safety to distribute sampling and local-to-model jobs across multiple threads. It uses c++11 std::async API to implement a parallel-for loop over all computation tasks. 
User can tweak the number of characters and the maximum number of characters per task. Animation control is automatically handled by the sample for all characters.

![multithread sample screenshot]({{site.baseurl}}/images/samples/multithread.jpg)

Concept
=======

All ozz jobs are thread-safe: [`ozz::animation::SamplingJob`][link_sampling_job], [`ozz::animation::BlendingJob`][link_blending_job], [`ozz::animation::LocalToModelJob`][link_local_to_model_job]... This is an effect of the data-driven architecture, which makes a clear distinction between data and processes (aka jobs). Jobs' execution can thus be distributed to multiple threads safely, as long as the data provided as inputs and outputs do not create any race conditions.
As a proof of concept, this sample uses a naive strategy: All characters' update (execution of their sampling and local-to-model stages, as demonstrated in [playback sample][link_playback_sample]) are distributed using a parallel-for loop, every frame. During initialization, every character is allocated all the data required for their own update, eliminating any dependency and race condition risk.

The parallel-for recursively splits the range of characters to process into subranges, to the point such that subrange is small enough (less than a predefined number of characters). The sample also implement a small trick in order to get the number of threads (from crt thread pool) that were used during the parallel-for execution.

Sample usage
============

The sample allows to switch multi-threading on/off and set the maximum number of characters that can be updated per task.
The number of characters can also be set from the GUI.

Implementation
==============

1. This sample extends [playback sample][link_playback_sample], and uses the same procedure to load skeleton and animation objects.
2. For each character, allocates runtime buffers (local-space transforms of type ozz::math::SoaTransform, model-space matrices of type ozz::math::Float4x4) with the number of elements required for the skeleton, and a sampling cache (ozz::animation::SamplingCache). Only the skeleton and the animation are shared amongst all characters, as they are read only objects, not modified during jobs execution.
3. Update function uses a parallel-for loop to split up characters' update loop amongst std::async tasks (sampling and local-to-model jobs execution), allowing all characters' update to be executed in concurrent batches.
