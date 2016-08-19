---
title: Pipeline
layout: full
keywords: pipeline, dae, obj, fbx, 3ds, max, maya, blender, offline, load, import, runtime, build, convert, optimize
order: 15
---

{% include references.jekyll %}

Pipeline description
====================

ozz-animation provides full support for Collada and fbx formats. Those formats are heavily used by the animation industry and supported by all major Digital Content Creation tools (Maya, Max, MotionBuilder, Blender...). ozz-animation offline pipeline aims to convert from these DCC offline formats (or any proprietary format) to ozz internal runtime format, as illustrated below:

![ozz-animation offline pipeline]({{site.baseurl}}/images/documentation/pipeline.png)

In a way or an other, the aim of this pipeline and importer tools is to end up with runtime data structures (`ozz::animation::Skeleton`, `ozz::animation::Animation`) that can be used with the run-time libraries, on any platform. Run-time libraries provide jobs and data structures to process runtime operations like sampling, blending...

Standard pipeline
-----------------

ozz-animation provides a standard pipeline to import skeletons and animations from Collada documents and Fbx files. This pipeline is based on command-line tools: dae2skel and dae2anim respectively for Collada skeleton and animation, fbx2skel and fbx2anim for Fbx.

dae2skel and fbx2skel command line tools require as input a file containing the skeleton to import (dae or fbx file), using \-\-file argument. Output filename is specified with \-\-skeleton argument.

{% highlight cpp %}
dae2skel --file="path_to_source_skeleton.dae" --skeleton="path_to_output_skeleton.ozz"  
{% endhighlight %}

The extension of the output file is not important though.

dae2anim and fbx2anim command line tools require two inputs:

- A dae or fbx file containing the animation to import, specified with \-\-file argument.
- A file containing an ozz runtime skeleton (usually outputted by dae2skel or fbx2skel), specified with \-\-skeleton argument.

Output filename is specified with \-\-animation argument. Like for skeleton, the extension of the output file is not important.

{% highlight cpp %}
dae2anim --file="path_to_source_animation.dae" --skeleton="path_to_skeleton.ozz" --animation="path_to_output_animation.ozz"
{% endhighlight %}

Some other optional arguments can be provided to the animation importer tools, to setup keyframe optimization ratios, sampling rate... The detailed usage for all the tools can be obtained with \-\-help argument.

Collada and Fbx importer c++ sources and libraries (ozz_animation_collada and ozz_animation_fbx) are also provided to integrate their features to any application.

Custom pipeline
---------------

Offline ozz-animation c++ libraries (`ozz_animation_offline`) are available, in order to implement custom format importers. This library exposes offline skeleton and animation data structures. They are then converted to runtime data structures using respectively `ozz::animation::offline::SkeletonBuilder` and `ozz::animation::offline::AnimationBuilder` utilities. Animations can also be optimized (removing redundant keyframes) using `ozz::animation::offline::AnimationOptimizer`.

Usage of `ozz_animation_offline` library is covered in the howto sections [write a custom skeleton importer][link_howto_custom_skeleton_importer] and [write a custom animation importer][link_howto_custom_animation_importer]. It is also demonstrated by [millipede sample][link_millipede_sample] and [optimize sample][link_optimize_sample] samples.
