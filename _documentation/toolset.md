---
title: Toolset
layout: full
keywords: pipeline, dae, obj, fbx, 3ds, max, maya, blender, offline, load, import, runtime, build, convert, optimize
order: 40
---

{% include links.jekyll %}

Pipeline description
====================

ozz-animation provides full support for major Digital Content Creation formats, including Fbx, Collada,. Those formats are heavily used by the animation industry and supported by all major DCC tools (Maya, Max, MotionBuilder, Blender...). ozz-animation offline pipeline aims to convert from these DCC offline formats (or any proprietary format) to ozz internal runtime format, as illustrated below:

<img src="{{site.baseurl}}/images/documentation/pipeline.png" alt="ozz-animation offline pipeline" class="w3-image">

In a way or an other, the aim of this pipeline and importer tools is to end up with runtime data structures (`ozz::animation::Skeleton`, `ozz::animation::Animation`) that can be used with the run-time libraries, on any platform. Run-time libraries provide jobs and data structures to process runtime operations like sampling, blending...

ozz-animation provides a standard pipeline to import skeletons and animations from Fbx files, and all the other formats [supported by the fxsdk][link_fbxsdk_formats]. This pipeline is based on command-line tools: fbx2skel and fbx2anim.

fbx2skel
--------

fbx2skel command line tools require as input a file containing the skeleton to import (dae, fbx or other supported files), using \-\-file argument. Output filename is specified with \-\-skeleton argument.

{% highlight bash %}
fbx2skel --file="path_to_source_skeleton.fbx" --skeleton="path_to_output_skeleton.ozz"  
{% endhighlight %}

The extension of the output file is not important though.

> One can use fbx2skel --help option to have all the details on available arguments.

fbx2anim
--------

fbx2anim command line tool requires two inputs:

- A file containing the animation to import, specified with \-\-file argument.
- A file containing an ozz runtime skeleton (usually outputted by fbx2skel), specified with \-\-skeleton argument.

Output filename is specified with \-\-animation argument. Like for skeleton, the extension of the output file is not important.

{% highlight bash %}
fbx2anim --file="path_to_source_animation.fbx" --skeleton="path_to_skeleton.ozz" --animation="path_to_output_animation.ozz"
{% endhighlight %}

Some other optional arguments can be provided to the animation importer tools, to setup keyframe optimization ratios, sampling rate... The detailed usage for all the tools can be obtained with \-\-help argument.

> Collada and Fbx importer c++ sources and libraries (ozz_animation_collada and ozz_animation_fbx) are also provided to integrate their features to any application.

Custom pipeline
===============

Offline ozz-animation c++ libraries (`ozz_animation_offline`) are available, in order to implement custom format importers. This library exposes offline skeleton and animation data structures. They are then converted to runtime data structures using respectively `ozz::animation::offline::SkeletonBuilder` and `ozz::animation::offline::AnimationBuilder` utilities. Animations can also be optimized (removing redundant keyframes) using `ozz::animation::offline::AnimationOptimizer`.

Usage of `ozz_animation_offline` library is covered in the howto sections [write a custom skeleton importer][link_howto_custom_skeleton_importer] and [write a custom animation importer][link_howto_custom_animation_importer]. It is also demonstrated by [millipede sample][link_millipede_sample] and [optimize sample][link_optimize_sample] samples.
