---
title: Animation runtime
layout: full
keywords: runtime,optimize,sample,sampling,blending,soa,sse,job,batch,cpu,cache,friendly,data,oriented
order: 50
---

{% include links.jekyll %}

ozz-animation runtime data structures
=====================================

Runtime structures are optimized, cache friendly, c++ data structures. Their data-oriented design aims to organize them (memory wise) according to the way they are consumed at runtime, maximizing cpu capabilities, trading-off memory footprint.
Ozz-animation runtime data structures cannot be modified programmatically, all data accessors are const. They are converted from offline data structures (`ozz::animation::offline::RawAnimation` and `ozz::animation::offline::RawSkeleton`), or read (de-serialized) from a stream. They are intended to be used by ozz-animation runtime jobs.

`ozz::animation::Skeleton`
--------------------------

The runtime skeleton data structure represents a skeletal hierarchy of joints, with a bind-pose (rest pose of the skeleton). This structure is filled by the `ozz::animation::SkeletonBuilder` (usually offline) and deserialized/loaded at runtime.
Joint names, joint bind-poses and hierarchical informations are all stored in separate arrays of data (as opposed to joint structures for the `ozz::animation::offline::RawSkeleton`), in order to closely match with the way runtime algorithms use them. Joint hierarchy is defined by an array of indices to each joint parent, in depth-first order. It is enough to traverse the whole joint hierarchy in indeed.

Helper functions are available from `ozz/animation/runtime/skeleton_utils.h` to traverse joint hierarchy in depth first order from the root to the leaves, or in reverse order.

`ozz::animation::Animation`
---------------------------

The runtime animation data structure stores compressed animation keyframes, for all the joints of a skeleton. Translations and scales are stored using 3 half floats (3 * 16bits) while rotations (a quaternion) are quantized on 3 16bits integers. Quaternion are normalized indeed, so the 4th component can be restored at runtime from the 3 others and a sign. Ozz restores the biggest of the 4 components, improving numerical accuracy which gets very bad for small values (√(x + ϵ) for small x). It also allows to pre-multiply each of the 3 smallest components by sqrt(2), maximizing quantization range by over 41%.

This structure is filled by the `ozz::animation::offline::AnimationBuilder' and deserialized/loaded at runtime.
For each transformation type (translation, rotation and scale), animation structure stores a single array of keyframes that contains all the tracks required to animate all the joints of a skeleton, matching depth-first joints order of the runtime skeleton structure. Keyframes in this array are sorted by time, then by track number. This is the comparison function "less" used be the sorting algorithm of the AnimationBuilder implementation:

{% highlight cpp %}
bool SortingKeyLess(const _Key& _left, const _Key& _right) {
  return _left.prev_key_time < _right.prev_key_time
         || (_left.prev_key_time == _right.prev_key_time
             && _left.track < _right.track);
}
{% endhighlight %}

Note that in fact "previous keyframe time" (which is only available in the offline sorting data structure) is used, instead of actual keyframe's time. This allows to optimize the search loop while sampling, as explained later.

This table shows the data layout of an animation of 1 second duration, 4 tracks (track0-3) and 17 keyframes (0-16)

<img src="{{site.baseurl}}/images/documentation/animation-tracks.png" class="w3-image">

Note that keyframes are mandatory at t=0 and t=duration, for all tracks, in order to optimize sampling algorithm. This constrain is automatically handled by the `AnimationBuilder` utility though, it does not apply to raw animations.

The color of each cell (as well as key numbers) refers to its index in the array, and thus its location in memory. This sorting strategy has 2 goals:
- This is a cache-friendly strategy that groups keyframes (for all tracks/joints) such that all keyframes needed at a time t (during sampling) are close one to each other in memory.
- Moving forward in the animation time only requires to move the time-cursor further, without traversing all tracks.

### Sampling the animation forward 

The following diagrams show "hot" keys, the one that are accessed during sampling. These keys are decompressed and stored in the `ozz::animation::SamplingJob::Context` in a SoA format suited for interpolation computations. The cache stores the current keyframe index per track.
 
- To sample at time = 0, the sampling algorithm goes through the 8 first keys and finds all the keys needed. The `SamplingJob::Context` also stores the sampling-cursor (the red arrow) at key 8, to be able to start again from there if sampling-time is moved further.

<img src="{{site.baseurl}}/images/documentation/animation-tracks-t0.png" class="w3-image">

- When sampling at time = 0.1, the cache is already filled (as shown by orange keys) with all the required data decompressed to SoA keyframes, optimized for interpolation operations. They are the same as the one used at time = 0. The sampling-cursor doesn't have to be moved either.

<img src="{{site.baseurl}}/images/documentation/animation-tracks-t0.1.png" class="w3-image">

- Sampling-time 0.3 is further than the latest sampling-cursor, so it has to be incremented up to key 10. Note that very few increments are needed, and that hot keys are still grouped. 

<img src="{{site.baseurl}}/images/documentation/animation-tracks-t0.3.png" class="w3-image">

- Moving a bit further to time = 0.7 requires to increment the sampling-cursor again. Note that hot keys are also quite grouped, despite a few holes.

<img src="{{site.baseurl}}/images/documentation/animation-tracks-t0.7.png" class="w3-image">

- Finally moving to time = 1, the cursor is pushed up to the last key, with most of the hot keys close one to each other in memory, already cached and decompressed (orange keys).

<img src="{{site.baseurl}}/images/documentation/animation-tracks-t1.png" class="w3-image">

The sampling algorithm needs to access very few keyframes when moving forward in the timeline, thanks to the way keyframes are stored.

### Sampling animation backward

The drawback of this strategy is that animation data layout is optimized for forward sequential reading. Reading an animation backward (negative delta time) requires to restart sampling from the beginning. Seeking to animation end requires to traverse the whole animation up to the end. This can have an significant performance impact to seek into the long animations.

Version 0.15 introduces changes that solve this problem. Up to this version, each keyframe would store its track index, which is needed to populate the cache (remember keyframes of all tracks are interleaved in the same array, and sorted).

Since 0.15, each keyframe stores the offset to the previous keyframe (for the same track/joint), instead of the track index directly. While it has the same memory footprint, that offset allows to find the previous frame with a O(1) complexity, which otherwise requires to search through all the keyframes up to animation begin.

> Keyframes for a track are sparse indeed, due to the decimation of interpolable keyframes. In the worst case there's only 2 keyframes, one at animation begin and one at the end. So the way keyframes are sorted, the n-1 keyframe (for a same track) can be very far ahead in memory from keyframe n.

Deducing keyframe's track index is done by searching for the keyframe in the cache instead. This search requires to iterate the cache which has an overhead. The algorithm remembers the last updated track index to shorten the loop.

Offsets are stored as 16b integers. `AnimationBuilder` is responsible for injecting a new keyframe if an offset overflows (more than 2^16 keys between 2 consecutive keyframes of a same track). This is rare, yet handled and tested.

### Seeking into the animation

Version 0.15 also introduces iframes inspired from video encoding, aka intermediate seek points in the animation. Iframes are implemented as snapshots of the sampling cache, aka a array of integers (index of keyframes). They are compressed using group varint encoding, as described by [Google](https://static.googleusercontent.com/media/research.google.com/en//people/jeff/WSDM09-keynote.pdf) and used in Protocol Buffers.

Iframes are created by `AnimationBuilder` utility. The user can decide interval in seconds between iframes, depending on how the animation is intended to be used.

`ozz::animation::SamplingJob` is responsible for deciding when an iframe shall be used at runtime (rather than sequentially reading forward or backward). The strategy is to seek to an iframe if delta time is bigger than half the interval between iframes.

> Default interval is set to 10s for importer tools, so long animations have an iframe every 10s. Smaller ones (less than 10s) will only have an iframe at the end, allowing to efficiently start playing an animation from the end, which is useful when reading backward.

`ozz::animation::*Track`
---------------------------

The runtime track data structure exists for 1 to 4 float types (`ozz::animation::FloatTrack`, ..., `ozz::animation::Float4Track`) and quaterions (`ozz::animation::QuaternionTrack`). See [`offline track`][link_raw_track] for more details on track content.
The runtime track data structure is optimized for the processing of `ozz::animation::TrackSamplingJob` and `ozz::animation::TrackTriggeringJob`. Keyframe ratios, values and interpolation mode are all store as separate buffers in order to access the cache coherently. Ratios are usually accessed/read alone from the jobs that all start by looking up the keyframes to interpolate indeed.

### `ozz::animation::TrackSamplingJob`

`ozz::animation::TrackSamplingJob` allows to sample a track at any point/time along the track. It's compatible with any track type.

### `ozz::animation::TrackTriggeringJob`

`ozz::animation::TrackTriggeringJob` detects when track curve crosses a threshold value, aka an edge, triggering dated events that can be processed as state changes. To do so, `ozz::animation::TrackTriggeringJob` considers what happens between 2 samples, not only the sampled point.

> Edge triggering wording refers to signal processing, where a signal edge is a transition from low to high or from high to low. It is called an "edge" because of the square wave which represents a signal has edges at those points. A rising edge is the transition from low to high, a falling edge is from high to low.

The job execution performs a lazy evaluation of edges. It builds an iterator that will process the next edge on each call to ++ operator.

Using dated edges allows to implement frame rate independent algorithms. Sampling a track indeed, doesn't tell what happen between the two samples, which the triggering job solves.

### Track sample

The following sample shows a use case of user channel track to drive attachment state of a box manipulated by a robot's arm. The track was edited in a DCC tool as a custom property, and imported alongside the animation using fbx2ozz.

You can experiment that using the `ozz::animation::TrackTriggeringJob`, the box remains perfectly at the correct position even if time is speed up. With the `ozz::animation::TrackSamplingJob`, a small error accumulates each loop. 

{% include emscripten.jekyll emscripten_path="samples/emscripten/sample_user_channel.js" %}

Mathematical structures
-----------------------

The runtime pipeline also refers to generic c++ mathematical structures like vectors, quaternions, matrices, `ozz:math::Transform` (translation, rotation and scale). It especially makes a heavy use of Struct-of-Array mathematical structures available in ozz for optimum memory access patterns and maximize SIMD instruction (SSE2, AVX...) throughput. This [document from Intel](https://software.intel.com/en-us/articles/how-to-manipulate-data-structure-to-optimize-memory-use-on-32-bit-intel-architecture) gives more details about the AoS/SoA layouts and their benefit.

More details about ozz-animation maths libraries are available in the [advanced][link_maths] section.

Local-space and model-space coordinate systems
==============================================

ozz-animation represents transformations as either in local or model-space coordinate systems:

- Local-space: The transformation of a joint in local space is relative to its parent.
- Model-space: The transformation of a joint in model space is relative to the root of the hierarchy.

Most of the processes in animation are done in local space (sampling, blending...). Local-space to model-space conversion is usually done by [`ozz::animation::LocalToModelJob`][link_local_to_model_job] at the end of the animation runtime pipeline, to prepare transformations for rendering. Data in ozz are structured according to the processing done on the data. Local space transforms are thus stored as separate SoA affine components (translation, rotation, scale), suitable for sampling, blending and global (whole skeleton) processes. Model space transforms are AoS matrices, suitable for rendering and per joint processes.

Runtime jobs
============

This section describes c++ runtime jobs, aka data processing units, provided by ozz-animation library.

All jobs follow the same principals: They process inputs (read-only or read-write) and fill outputs (write only). Both inputs and outputs are provided to jobs by the user. Jobs never access any memory outside of these inputs and outputs, making them intrinsically thread safe (race condition wise).

`ozz::animation::SamplingJob`
-----------------------------

The `ozz::animation::SamplingJob` is in charge of computing the local-space transformations (for all tracks) at a given time of an animation. The output could be considered as a pose, a snapshot of all the tracks of an animation at a specific time.
The SamplingJob uses a cache (aka `ozz::animation::SamplingJob::Context`) to store intermediate values (decompressed animation keyframes...) while sampling, which are likely to be used for the subsequent frames thanks to interpolation. This cache also stores pre-computed values that allows drastic optimization while playing/sampling the animation forward. Backward sampling works of course but isn't optimized through the cache though, favoring memory footprint over performance in this case.

Relying on the fact that the same set of keyframes can be relevant from an update to the next (because keyframes are interpolated), the _sampling_cache_ stores data as decompressed SoA structures. This factorizes decompression and SoA conversion cost. Furthermore, interpolating SoA structures is optimal.

See [playback sample][link_playback_sample] for a simple use case.

`ozz::animation::BlendingJob`
-----------------------------

The `ozz::animation::BlendingJob` is in charge of blending (mixing) multiple poses (the result of a sampled animation) according to their respective weight, into one output pose. The number of transforms/joints blended by the job is defined by the number of transforms of the bind-pose (note that this is a SoA format). This means that all buffers must be at least as big as the bind pose buffer.
Partial animation blending is supported through optional joint weights that can be specified for each layer's `joint_weights` buffer. Unspecified joint weights are considered as a unit weight of 1.f, allowing to mix full and partial blend operations in a single pass.

See [blending sample][link_blend_sample] that blends 3 locomotion animations (walk/jog/run) depending on a target speed factor, and [partial blending sample][link_partial_blend_sample] that blends a specific animation to the character upper-body.

`ozz::animation::LocalToModelJob`
---------------------------------

The `ozz::nimation::LocalToModelJob` is in charge of converting [local-space to model-space coordinate system][link_coordinate_system]. It uses the skeleton to define joints parent-child hierarchy. The job iterates through all joints to compute their transform relatively to the skeleton root.
Job inputs is an array of SoaTransform objects (in local-space), ordered like skeleton's joints. Job output is an array of matrices (in model-space), also ordered like skeleton's joints. Output are matrices, because the combination of affine transformations can contain shearing or complex transformation that cannot be represented as affine transform objects.

Note that this job also intrinsically unpack SoA input data (`ozz::math::SoATransform`) to standard AoS matrices (`ozz::math::Float4x4`) on output.

All samples use `ozz::animation::LocalToModelJob`. See [playback sample][link_playback_sample] for a simple use case.
