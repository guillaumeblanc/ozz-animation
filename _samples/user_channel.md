---
title: User channels
layout: full
keywords: track,animation,sample,float,custom,property,sample,edge,triggering,frame,rate,dependant
order: 50
level: 2
---

{% include links.jekyll %}
{% include link_sample_code.jekyll sample="playback" %}

Description
===========

The samples uses a user channel track to drive attachment state of a box manipulated by a robot's arm. The track was edited in a DCC tool as a custom property, and imported along side the animation using fbx2ozz.

{% include emscripten.jekyll emscripten_path="samples/emscripten/sample_user_channel.js" %}

Concept
=======

User-channels are the way to animate data that aren't joint transformations: float, bool... In this sample the user-data is the boolean attachment state of the box with the robot's finger. The sample attaches the box (see attachment sample) to the robot's finger when the boolean becomes true, and detaches it when it's getting false.
The user-channel track was edited in a DCC tool as a custom property and saved in the fbx file alongside the animation. fbx2ozz is used to extract this track using "animations\[\].tracks\[\].properties\[\]" parameter of fbx2ozz json configuration. See [config.json][link_user_channel_sample_config].

The sample demonstrates two ways to use the user-channel track, based on ozz API:
1. Sampling: Using ozz::animation::TrackSamplingJob, the samples queries the track value each keyframe. It will get true or false depending on if the box should be attached or detached for current frame time. If the box is detached, its position remains unchanged. If it is attached, the sample computes box position as transformed by the finger joint. The relative transformation of the box to the finger is stored when attachment state switches from off to on. Because the frame time isn't exactly the time when the state has changed, a transformation error is accumulated.
2. Edge triggering: Using ozz::animation::TrackTriggeringJob, the samples iterates all state changes that happened since the last update. This allows to compute the box relative transformation for each state change, at the exact state change time. This prevents accumulating transformation error. It even supports jumping to any point in time, because all state changes (attach / detach) are be processed. 

Sample usage
============

The sample exposes usual animation playback settings. It also exposes options to choose to from sampling or edge triggering technique (default option).

Implementation
==============

1. Loading of data is based on animation playback sample, as described in the [how to load an object from a file][link_how_to_load] section.
2. Box attachment and transformation is based on attachment sample.
3. When attachment state switches from off to on, the relative transformation of the box to the finger joint is computed (inverse wolrd-space joint matrix * box world-space matrix).
4. The sample uses 2 techniques to detect attachment state changes:  
  1. Sampling, based on [`ozz::animation::TrackSamplingJob`][link_track_sampling_job]. User-channel track is sampled at the same time t as the animation, which gives attachment state for the current frame. State changes are thus detected depending on application frame rate.
  2. Edge triggering, based on [`ozz::animation::TrackTriggeringJob`][link_track_triggering_job]. TrackTriggeringJob computes rising and falling edges for a given period of time. The sample thus queries all edges from the previous frame time to the current one. The exact edge time (aka when the track crosses threshold value) is used to compute finger joint transformation, instead of current frame time (as with sampling case). This make the algorithm frame rate independent.
