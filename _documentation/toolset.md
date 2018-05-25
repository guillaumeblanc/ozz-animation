---
title: Toolset
layout: full
keywords: toolset,pipeline,dae,obj,dcc,fbx,3ds,max,maya,blender,offline,load,import,build,convert,optimize,export,import
order: 40
---

{% include links.jekyll %}

Pipeline description
====================

ozz-animation provides full support for major Digital Content Creation formats, including Fbx, Collada... Those formats are heavily used by the animation industry and supported by all major DCC tools (Maya, Max, MotionBuilder, Blender...). ozz-animation offline pipeline aims to convert from these DCC offline formats (or any proprietary format) to ozz internal runtime optimized format, as illustrated below:

<img src="{{site.baseurl}}/images/documentation/pipeline.png" alt="ozz-animation offline pipeline" class="w3-image">

In a way or an other, the aim of the pipeline and importer tools is to end up with runtime data structures (`ozz::animation::Skeleton`, `ozz::animation::Animation`, ...) that can be used with the runtime libraries, on any platform. Run-time libraries provide jobs and data structures to process runtime operations like sampling, blending...

ozz-animation provides a standard pipeline to import skeletons and animations from Fbx files, and all the other formats [supported by the fxsdk][link_fbxsdk_formats]. This pipeline is based on fbx2ozz command-line tool to import (aka convert) from DCC to ozz format.

fbx2ozz
-------

fbx2ozz command line tool can be used to import data from DCC file (fbx, dae or other supported file formats) into ozz runtime format. The DCC file is specified using \-\-file argument.

{% highlight bash %}
fbx2ozz --file="path_to_source_skeleton.fbx" 
{% endhighlight %}

By default fbx2ozz will import the skeleton, all animations, and write them to disk. A specific configuration can be provided in order to customize import behavior. This configuration, in the form of a a json document, can be provided in 2 ways:
- \-\-config option, which expects a valid json configuration string.
{% highlight bash %}
fbx2ozz --file="path_to_source_skeleton.fbx" --config="{\"skeleton\":{\"filename\":\"skeleton.ozz\"},\"animations\":[{\"filename\":\"*.ozz\"}]}"
{% endhighlight %}
Note that json configuration string must be properly escaped (see `\"` characters instead of `"` in the configuration string above).
- \-\-config_file option, which expects a path to a valid json configuration file.
{% highlight bash %}
fbx2ozz --file="path_to_source_skeleton.fbx" --config_file="path_to_config_file.json".  
{% endhighlight %}

As an example, here's a configuration that produces fbx2ozz default behavior:

{% highlight c %}
{
  //  Skeleton to import
  "skeleton" : 
  {
    "filename" : "skeleton.ozz", //  Specifies skeleton input/output filename. The file will be outputted if import is true. It will also be used as an input reference during animations import.
    "enable" : true //  Imports (from source data file) and writes skeleton output file.
  },
  //  Animations to import.
  "animations" : 
  [
    {
      "clip" : "*", //  Specifies clip name (take) of the animation to import from the source file. Wildcard characters '*' and '?' are supported
      "filename" : "*.ozz" //  Specifies animation output filename. Use a '*' character to specify part(s) of the filename that should be replaced by the clip name.
    }
  ]
}
{% endhighlight %}

Import will behave in the following way:
- Generate a runtime skeleton from the scene content (because skeleton.enable is true), and write it to a file named \"skeleton.ozz\".
- Import all animations from the scene (because of animations[].clip is \"\*\"), and write each of them to \"DCC_animation_clip_name.ozz\" (because of animations[].filename is \"\*.ozz\").

Note that if skeleton.enable was false, the skeleton would be loaded from the ozz binary file named \"skeleton.ozz\" (set with the same skeleton.filename json member), instead of from the DCC file. This allows for example to reuse a skeleton to load animations of different DCC files.

The file [reference.json][link_src_reference_json] (automatically generated during build) exposes all the available configuration members:

{% highlight c %}
{
  //  Skeleton to import
  "skeleton" : 
  {
    "filename" : "skeleton.ozz", //  Specifies skeleton input/output filename. The file will be outputted if import is true. It will also be used as an input reference during animations import.
    //  Define skeleton import settings.
    "import" : 
    {
      "enable" : true, //  Imports (from source data file) and writes skeleton output file.
      "raw" : false, //  Outputs raw skeleton.
      //  Define nodes types that should be considered as skeleton joints.
      "types" : 
      {
        "skeleton" : true, //  Uses skeleton nodes as skeleton joints.
        "marker" : false, //  Uses marker nodes as skeleton joints.
        "camera" : false, //  Uses camera nodes as skeleton joints.
        "geometry" : false, //  Uses geometry nodes as skeleton joints.
        "light" : false, //  Uses light nodes as skeleton joints.
        "null" : false, //  Uses null nodes as skeleton joints.
        "any" : false //  Uses any node type as skeleton joints, including those listed above and any other.
      }
    }
  },
  //  Animations to import.
  "animations" : 
  [
    {
      "clip" : "*", //  Specifies clip name (take) of the animation to import from the source file. Wildcard characters '*' and '?' are supported
      "filename" : "*.ozz", //  Specifies animation output filename. Use a '*' character to specify part(s) of the filename that should be replaced by the clip name.
      "raw" : false, //  Outputs raw animation.
      "additive" : false, //  Creates a delta animation that can be used for additive blending.
      "sampling_rate" : 0, //  Selects animation sampling rate in hertz. Set a value <= 0 to use imported scene default frame rate.
      "optimize" : true, //  Activates keyframes reduction optimization.
      //  Optimization tolerances.
      "optimization_tolerances" : 
      {
        "translation" : 0.001, //  Translation optimization tolerance, defined as the distance between two translation values in meters.
        "rotation" : 0.001745, //  Rotation optimization tolerance, ie: the angle between two rotation values in radian.
        "scale" : 0.001, //  Scale optimization tolerance, ie: the norm of the difference of two scales.
        "hierarchical" : 0.001 //  Hierarchical translation optimization tolerance, ie: the maximum error (distance) that an optimization on a joint is allowed to generate on its whole child hierarchy.
      },
      //  Tracks to build.
      "tracks" : 
      [
        {
          //  Properties to import.
          "properties" : 
          [
            {
              "filename" : "*.ozz", //  Specifies track output filename(s). Use a '*' character to specify part(s) of the filename that should be replaced by the track (aka "joint_name-property_name") name.
              "joint_name" : "*", //  Name of the joint that contains the property to import. Wildcard characters '*' and '?' are supported.
              "property_name" : "*", //  Name of the property to import. Wildcard characters '*' and '?' are supported.
              "type" : 1, //  Type of the property, aka the number of floating point components. 1 to 4 components are supported.
              "raw" : false, //  Outputs raw track.
              "optimize" : true, //  Activates keyframes optimization.
              "optimization_tolerance" : 0.001 //  Optimization tolerance
            }
          ]
        }
      ]
    }
  ]
}
{% endhighlight %}

Some other command line options are available on fbx2ozz. One can use fbx2ozz \-\-help option to get all the details on all available options.

> Fbx importer c++ sources and libraries (ozz_animation_fbx) are also provided to integrate their features to any application.

Custom pipeline
===============

Offline ozz-animation c++ libraries (`ozz_animation_offline`) are available in order to implement custom format importers. This library exposes offline skeleton and animation data structures. They are converted to runtime data structures using respectively [`ozz::animation::offline::SkeletonBuilder`][link_skeleton_builder] and [`ozz::animation::offline::AnimationBuilder`][link_animation_builder] utilities. Animations can also be optimized (removing redundant keyframes) using [`ozz::animation::offline::AnimationOptimizer`][link_animation_optimizer].

Usage of `ozz_animation_offline` library is covered in the howto sections [write a custom skeleton importer][link_howto_custom_skeleton_importer] and [write a custom animation importer][link_howto_custom_animation_importer]. It is also demonstrated by [millipede sample][link_millipede_sample] and [optimize sample][link_optimize_sample] samples.
