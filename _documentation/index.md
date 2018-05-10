---
title: Overview
layout: full
keywords: documentation,doc,help,howto,license,pipeline,tools,primer,introduction,definition
permalink: /:collection/index.html
---

{% include links.jekyll %}

What ozz-animation is
=====================

ozz-animation is an open source *c++ 3d skeletal animation engine*. It provides runtime *character animation playback* fonctionnalities (loading, sampling, blending...), with full support for *Fbx*, *Collada* and other major 3D formats. It proposes a low-level *renderer agnostic* and *game-engine agnostic* implementation, focusing on performance and memory constraints with a data-oriented design.

ozz-animation comes with a toolchain to convert from major Digital Content Creation formats to ozz optimized runtime structures. Offline libraries are also provided to implement the conversion from any other animation and skeleton formats.

What it's not
=============

ozz-animation is not a high level animation blend tree. It proposes the mathematical background required for playing animations and a pipeline to build/import animations. It lets you organise your data and your blending logic the way you decide.

ozz-animation doesn't do any rendering either. You're in charge of applying the output of the animation stage (one matrix per joint) to your skins, or to your scene graph nodes.

Finally ozz does not propose an official pipeline to load meshes and materials either. The sample framework embed a mesh importing tool (fbx2mesh) though.

License
=======

ozz-animation is published under the permissive MIT license, which allows to modify and redistribute sources or binaries with very few constraints. The license only has the following points to be accounted for:

- Software is used on 'as-is' basis. Authors are not liable for any damages arising from its use.
- The distribution of a modified version of the software is subject to the following restrictions:
  - The authorship of the original software must not be misrepresented,
  - Altered source versions must not be misrepresented as being the original software, and
  - The license notice must not be removed from source distributions.

The license does not require source code to be made available if distributing binary code.

Reporting issues or feature requests
====================================

[This page][link_features] lists implemented features and plans for future releases. Please use [github issues][link_github_issues] to report bugs, request features or discuss animation techniques.

Contributing
============
Everyone is welcome to contribute: code review, bug report or fix, feature request or implementation, optimization, samples...

ozz branching strategy follows [gitflow model][link_git_flow]. When submitting patches, please:

- Make pull requests to develop branch for features, to release branch for hotfixes.
- Do not include merge commits in pull requests; include only commits with the new relevant code.
- Add all relevant unit tests.
- Run all the tests and make sure they pass.