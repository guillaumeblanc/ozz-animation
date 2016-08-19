---
title: "open source c++ skeletal animation engine and tools"
layout: full
keywords: home
collection: home
---

{% include references.jekyll %}

![logo]({{site.baseurl}}/images/logo/ozz-grey-128.png)

ozz-animation is an **open source c++ 3d skeletal animation engine**. It provides runtime character animation functionalities (loading, sampling, blending...), with the toolchain to import from major DCC formats (Collada, Fbx, ...). It proposes a low-level renderer agnostic and game-engine agnostic implementation, focusing on performance and memory constraints with a data-oriented design.

---

Below is a [runtime blending sample][link_blend_sample], part of ozz package.

{% include emscripten.jekyll emscripten_path="samples/emscripten/sample_blend.js" %}

---

Contributions are welcome: code review, bug fix, feature implementation...

ozz branching strategy follows [gitflow model](http://nvie.com/posts/a-successful-git-branching-model/). When submitting patches, please:

- Make pull requests to develop branch for features, to release branch for hotfixes.
- Do not include merge commits in pull requests; include only commits with the new relevant code.
- Add all relevant unit tests.
- Run all the tests and make sure they pass.

---

ozz-animation is hosted at [github][link_github] and distributed under the MIT License (MIT).