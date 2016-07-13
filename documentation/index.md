---
title: Primer documentation for ozz-animation
layout: full
collection: documentation
keywords: documentation, doc, help, howto, license, pipeline, tools, primer, introduction, definition
---

{% include references.jekyll %}

What ozz-animation is
=====================
ozz-animation is an open source *c++ 3d skeletal animation engine*. It provides runtime *character animation playback* fonctionnalities (loading, sampling, blending...), with full support for *Collada* and *Fbx* import formats. It proposes a low-level *renderer agnostic* and *game-engine agnostic* implementation, focusing on performance and memory constraints with a data-oriented design.

ozz-animation comes with *Collada* and *Fbx* toolchains to convert from major Digital Content Creation formats to ozz optimized runtime structures. Offline libraries are also provided to implement the conversion from any other animation and skeleton formats.

What ozz-animation is not
=========================
ozz-animation is not a high level animation blend tree. It proposes the mathematical background required for playing animations and a pipeline to build/import animations. It lets you organise your data and your blending logic the way you want.

ozz-animation doesn't do any rendering either. You're in charge of applying the output of the animation stage (one matrix per joint) to your skins, or to your scene graph nodes.

Finally ozz does not propose the pipeline to load meshes and materials either.

Pipeline
========
ozz-animation comes with *Collada* and *Fbx* tool chains to convert from major Digital Content Creation tools' format to ozz optimized runtime structures. Offline libraries are also provided to implement the convertion from any other animation (and skeleton) formats.

![ozz-animation offline pipeline]({{site.baseurl}}/images/offline_pipeline.png)

Jump to [offline pipeline][link_offline_pipeline] for a description of ozz-animation offline pipeline tools and features.

Jump to [runtime pipeline][link_runtime_pipeline] for a description of ozz-animation runtime features.

License
=======
ozz-animation is published under the very permissive zlib/libpng license, which allows to modify and redistribute sources or binaries with very few constraints. The license only has the following points to be accounted for:

- Software is used on 'as-is' basis. Authors are not liable for any damages arising from its use.
- The distribution of a modified version of the software is subject to the following restrictions:
  - The authorship of the original software must not be misrepresented,
  - Altered source versions must not be misrepresented as being the original software, and
  - The license notice must not be removed from source distributions.

The license does not require source code to be made available if distributing binary code.

Reporting issues or feature requests
====================================
[This page][link_features_map] lists implemented features and plans for future releases. Please use github issues to report bugs and feature requests.

{% include feedback_form.jekyll %}

Contributing
============
Any contribution is welcome: code review, bug report or fix, feature request or implementation. Don't hesitate...
