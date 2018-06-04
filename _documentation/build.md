---
title: Build
layout: full
keywords: download,build,libraries,amalgamated,sources,fused,make,git,gcc,clang,msvc,start,tutorial,emscripten,opengl,glfw,unit-tests,package
order: 5
---

{% include links.jekyll %}

This chapter will drive you through the steps of downloading, building and integrating ozz-animation to your project. If you find any mistake in this page, or think something should be added, please don't hesitate to [open an issue][link_github_issues].

Downloading
===========

Prebuilt built libraries, tools and samples
-------------------------------------------

Pre-compiled binaries (libraries, samples...) of the latest release for all supported platforms can be downloaded from [github releases][link_latest_release] page. These packages are available for Linux, Windows ans MacOS systems.

> Note that ozz c++ samples can also be [tested online in your web browser][link_samples], thanks to emscripten.

Downloading sources
-------------------

ozz-animation is hosted on [github][link_github]. The latest versions of the source code, including all release tags and incoming branches, are available from there.
Get a local clone of the ozz-animation git repository with this command:

{% highlight bash %}
git clone https://github.com/guillaumeblanc/ozz-animation.git
{% endhighlight %}

Alternatively, latest release sources can be downloaded as a [zip package][link_latest_sources].

Building ozz libraries, tools and samples
=========================================

Ozz build process relies on [cmake](https://cmake.org) which provides a portable build system. It is setup to build ozz-animation libraries, tools and samples (along with their data). It can also run unit-tests and package sources/binary distributions.

> See [the feature-map page][link_features_os] for a list of tested OS and compilers.

You can run CMake as usual from ozz-animation root directory. It is recommended to build out-of-source though (creating "build" directory and running CMake from there). From ozz-animation root, use the following commands from ozz sources directory:

{% highlight bash %}
mkdir build
cd build
cmake ..
cmake --build ./
{% endhighlight %}

ozz-animation libraries and samples will be built by default. Unit tests aren't built by default, to lower build time.

This will output ozz libraries:

- Runtime libraries:

  - `ozz_base`: Required for any other library. Integrates io, math, log...
  - `ozz_animation`: Integrates all animation runtime jobs, sampling, blending...
  - `ozz_geometry`: Integrates geometry runtime jobs, skinning...

- Offline libraries:

  - `ozz_animation_offline`: Integrates ozz offline libraries, animation builder, optimizer...
  - `ozz_animation_offline_fbx`: Integrates ozz fbx import libraries. Note that integrating Fbx related libraries will require to also link with Fbx sdk.

- Other libraries:

  - `ozz_options`: Cross platform command line option parser.

Build options
-------------

All build options are listed in the project root [CMakeLists.txt file][link_src_build_options]. They can be changed there, or when invoking cmake with -D argument. As an example, to setup unit tests build directives:

{% highlight bash %}
cmake -Dozz_build_tests=1 ..
{% endhighlight %}

Using build help script
-----------------------

An optional python script `build-helper.py` is available in the root directory to help performing all operations:

- Configure CMake, generate vcproj, makefiles...
- Build from sources.
- Enable and run un unit-tests.
- Package sources and binaries.
- Clean build directory.
- ...

Building Fbx tools
------------------

Ozz-animation implements a fbx toolchain (fbx2ozz command line tool), based on [Autodesk Fbx SDK][link_fbxsdk]. If a compatible Fbx SDK is installed, cmake will automatically detect it [using a custom module][link_fbxsdk_cmake_module] and build the tools.

Running unit tests
------------------

Unit tests are using ctest and gtest. Once cmake has been configured with unit tests enabled, build ozz and use the following commands to run them (from "build" directory):

{% highlight bash %}
ctest
{% endhighlight %}

Some platforms might require to specify the target configuration:
{% highlight bash %}
ctest --build-config Release
{% endhighlight %}

Integrating ozz to your build process
=====================================

There are different options to intergate ozz to your project. The recommended way is to integrate ozz as a cmake sub project.

## Integrating ozz as a cmake sub project

If you're already using cmake, then the recommended way is to include ozz as subtree or submodule within your project's source tree, and add the directory using CMake's add_subdirectory command.

{% highlight bash %}
# Includes ozz-animation as a sub directory, using an arbitrary "ozz-animation/"
# binary output folder.
add_subdirectory("your path to ozz root folder" "ozz-animation/")

# Defines your executable.
add_executable(foo foo.cc)

# Then link with ozz libraries as any other cmake target.
# This will automatically link with all ozz_animation dependencies, as well as
# add ozz include directories.
target_link_libraries(foo ozz_animation)
{% endhighlight %}

This allows to build ozz along with your project, include ozz header files and link with ozz libraries as any of your own cmake target.

## Other integration alternative

### 1. Using pre-build binaries

If using pre-build libraries, you'll need to set ozz include path and link with libraires.

#### Setting up ozz include path

If you're not using the "cmake sub project" way above, you'll need to setup ozz include path. It means adding ozz `include/` path to your project's header search path, so that you can include ozz files from your cpp file with the following syntax: `#include "ozz/...*.h"`).

With cmake, you do it this way:

{% highlight bash %}
target_include_directories(your_target "Your path to ozz include folder")
{% endhighlight %}

Without cmake, in Visual Studio for example, follow [these instructions](https://msdn.microsoft.com/en-us/library/73f9s62w.aspx).

#### Linking with ozz libraries

Then you'll have to setup the project to link with ozz libraries, which is build-system specific.

With cmake, you'd use target_link_libraries. Note that library dependencies aren't automatically deduced in this case.
{% highlight bash %}
target_link_libraries(your_target ozz_animation ozz_base)
{% endhighlight %}

Without cmake, in Visual Studio for example, follow [these instructions](https://msdn.microsoft.com/en-us/library/ba1z7822.aspx).

### 2. Integrating ozz-animation sources to your build process

Instead of linking with ozz libraries, offline and runtime sources can be integrated to your own build process. Ozz is compatible with all modern c++ compilers and does not rely on any configuration file. You'll only need to add [ozz sources files][link_src] to your build system. Of course ozz `include/` path still needs to be set.

This latest solution is interesting for ozz runtime features as it ensures compilation options compatibility. Tracing into ozz code is then straightforward also.

### 3. Integrating amalgamated sources

Ozz also allows to use fused / amalgamated sources. These amalgamated sources are single .cc files (one per ozz library) that can be added to your project sources. This aims to simplify project maintenance and further updates of ozz libraries. Again ozz include path remains the same and must be set.

Fused sources are generated during build, in a "src_fused" folder in the binrary ouput folder ("build/src_fused/"" folder by default). To generate them without building the whole library, one can use the following commands from ozz sources directory:

{% highlight bash %}
mkdir build
cd build
cmake ..
cmake --build ./ --target BUILD_FUSE_ALL
{% endhighlight %}

Fused sources can the be found in "build/src_fused/" folder.

Next step
=========

You should now be able to compile and link with ozz-animation libraries. The easiest way to start playing with ozz-animation API is to look at the [playback sample][link_playback_sample]. It implements the basics for animating a skeleton. Its documentation explains all the steps, from loading the data to updating joints position each frame. To get a deeper understanding of the runtime API and data structure, have a look to the [runtime documentation][link_runtime].

The animation data the sample uses are located in the [media/bin][link_media_bin] directory. To get a first idea about the toolset used to import these data from fbx (for example), it's recommended to read the [offline toolset][link_toolset] documentation.
