---
title: Build
layout: full
keywords: build,cmake,gcc,clang,msvc,emscripten,opengl,glfw,opengl,directx,unit-tests,package
order: 30
---

{% include links.jekyll %}

Building ozz librairies, tools and samples
==========================================

The build process relies on [cmake](http://www.cmake.org) which provides a portable build system. It is setup to build ozz-animation libraries, tools and samples (along with their data), and also run unit-tests and package sources/binary distributions. See [the feature-map page][link_features] for a list of supported OS and compilers.

You can run CMake as usual from ozz-animation root directory. It is recommended to build out-of-source though (creating "build" directory and running CMake from there):

{% highlight bash %}
mkdir build
cd build
cmake ..
cmake --build ./ --config Release
{% endhighlight %}

ozz-animation libraries and samples will be build by default. Fbx tools will be automatically built if a compatible FbxSdk is found. Unit tests are disabled by default, which can be changed by setting ozz_build_tests to ON. All build options are listed in the project root [CMakeLists.txt file][link_src_build_options].  

An optional python script `build-helper.py` is available in the root directory to help performing all operations:

- Configure CMake, generate vcproj, makefiles...
- Build from sources.
- Enable and run un unit-tests.
- Package sources and binaries.
- Clean build directory.
- ...

Integrating ozz in your project
===============================

To use ozz-animation in an application, you'll need to add ozz `include/` path to your project's header search path so that your compiler can include ozz files (paths starting with `ozz/...`).

Then you'll have to setup the project to link with ozz libraries:

- `ozz_base`: Required for any other library. Integrates io, math...
- `ozz_animation`: Integrates all animation runtime jobs, sampling, blending...
- `ozz_animation_offline`: Integrates ozz offline libraries, animation builder, optimizer...
- `ozz_animation_offline_fbx`: Integrates ozz fbx import libraries.
- `ozz_geometry`: Integrates geometry runtime jobs, skinning.

> Integrating Fbx related libraries will require to also link with Fbx sdk.

Pre-built libraries, tools and samples can be found on the [download][link_download] section.

Integrating ozz sources in your build process
=============================================

Offline and runtime sources can also be integrated to your own build process. ozz does not rely on any configuration file, so you'll only need to add sources files to your build system and add ozz `include/` path as an include directory. It should be straightforward and compatible with all modern c++ compilers.

This latest solution is interesting for ozz runtime features as it ensures compilation options compatibility and allows to trace into ozz code easily. It works for offline libraries as well.

Using amalgamated sources
=========================
Ozz also embeds automatically generated [fused / amalgamated sources][link_src_fused]. These amalgamated sources are single .cc files (one per ozz library) that can be added to your project sources. This aims to simplify project maintenance and further updates of ozz libraries.

> Ozz include path remains the same with amalgamated sources.
