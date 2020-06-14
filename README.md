[![logo](media/icon/ozz-grey-256.png)](http://guillaumeblanc.github.io/ozz-animation/)

ozz-animation
=============

open source c++ 3d skeletal animation library and toolset
---------------------------------------------------------

ozz-animation provides runtime character animation playback functionalities (loading, sampling, blending...). It proposes a low-level renderer agnostic and game-engine agnostic implementation, focusing on performance and memory constraints with a data-oriented design.

ozz-animation comes with the toolchain to convert from major Digital Content Creation formats (gltf, Fbx, Collada, Obj, 3ds, dxf) to ozz optimized runtime structures. Offline libraries are also provided to implement the conversion from any other animation and skeleton format.

Documentation
-------------

Documentation and samples are available from [ozz-animation website](http://guillaumeblanc.github.io/ozz-animation/). Join [Gitter](https://gitter.im/ozz-animation/community) for further discussions.

Supported platforms
-------------------

Ozz is tested on Linux, Mac OS and Windows, for x86, x86-64 and ARM architectures. The run-time code (ozz_base, ozz_animation, ozz_geometry) depends only on c++11, the standard CRT and has no OS specific code, portability to any other platform shouldn't be an issue.

Samples, tools and tests depend on external libraries (glfw, tinygltf, Fbx SDK, jsoncpp, gtest, ...), which could limit portability.

Build status
------------

|         | Linux  | Mac OS | Windows |
| ------- | ------ | ------ | ------- |
| master  | [![Travis-CI](https://travis-ci.org/guillaumeblanc/ozz-animation.svg?branch=master)](http://travis-ci.org/guillaumeblanc/ozz-animation) | [![Travis-CI](https://travis-ci.org/guillaumeblanc/ozz-animation.svg?branch=master)](http://travis-ci.org/guillaumeblanc/ozz-animation) | [![AppVeyor](https://ci.appveyor.com/api/projects/status/github/guillaumeblanc/ozz-animation?branch=master&svg=true)](http://ci.appveyor.com/project/guillaumeblanc/ozz-animation) |
| develop | [![Travis-CI](https://travis-ci.org/guillaumeblanc/ozz-animation.svg?branch=develop)](http://travis-ci.org/guillaumeblanc/ozz-animation) | [![Travis-CI](https://travis-ci.org/guillaumeblanc/ozz-animation.svg?branch=develop)](http://travis-ci.org/guillaumeblanc/ozz-animation) | [![AppVeyor](https://ci.appveyor.com/api/projects/status/github/guillaumeblanc/ozz-animation?branch=develop&svg=true)](http://ci.appveyor.com/project/guillaumeblanc/ozz-animation) |

The dashboard for all branches is available [here](http://guillaumeblanc.github.io/ozz-animation/documentation/dashboard/).

Contributing
------------

All contributions are welcome: code reviews, bug reports, bug fixes, samples, features, platforms, testing, documentation, optimizations...

Please read [CONTRIBUTING](CONTRIBUTING.md) file for more details about how to submit bugs or contribute to the code.

License
-------

ozz-animation is hosted on [github](http://github.com/guillaumeblanc/ozz-animation/) and distributed under the **MIT License** (MIT).
