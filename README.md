[![Travis-CI](https://travis-ci.org/guillaumeblanc/ozz-animation.svg?branch=master)](http://travis-ci.org/guillaumeblanc/ozz-animation)
[![AppVeyor](https://ci.appveyor.com/api/projects/status/github/guillaumeblanc/ozz-animation?branch=master&svg=true)](http://ci.appveyor.com/project/guillaumeblanc/ozz-animation)
<a href="http://guillaumeblanc.github.io/ozz-animation/documentation/dashboard/"><i class="fa fa-thermometer-full" aria-hidden="true"></i></a>

ozz-animation - open source c++ skeletal animation library and toolset
----------------------------------------------------------------------

[![logo](media/icon/ozz-grey-256.png)](http://guillaumeblanc.github.io/ozz-animation/)

ozz-animation is an open source c++ 3d skeletal animation library. It provides runtime character animation playback functionalities (loading, sampling, blending...). It proposes a low-level renderer agnostic and game-engine agnostic implementation, focusing on performance and memory constraints with a data-oriented design.

ozz-animation comes with the toolchain to convert from major Digital Content Creation formats (Fbx, Collada, Obj, 3ds, dxf) to ozz optimized runtime structures. Offline libraries are also provided to implement the conversion from any other animation and skeleton format.

---

Documentation is available [online](http://guillaumeblanc.github.io/ozz-animation/documentation/).

---

Contributions are welcome: code review, bug fix, feature implementation...

ozz branching strategy follows [gitflow model](http://nvie.com/posts/a-successful-git-branching-model/). When submitting patches, please:
  - Make pull requests to develop branch for features, to release branch for hotfixes.
  - Do not include merge commits in pull requests; include only commits with the new relevant code.
  - Add all relevant unit tests.
  - Run all the tests and make sure they pass.

---

ozz-animation is hosted on [github](http://github.com/guillaumeblanc/ozz-animation/) and distributed under the **MIT License** (MIT).