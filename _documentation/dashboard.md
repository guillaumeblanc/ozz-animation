---
title: Dashboard
layout: full
keywords: continuous,integration,ci, travis,appveyor,sources,build,package,gcc,clang,msvc,visual studio,mac,osx,ios,windows,linux,debian,freebsd
order: 100
---

{% include links.jekyll %}

Continuous integration dashboard
================================

Use the continuous integration build dashboard to verify branches stability.

{% include table.jekyll id="dashboard_table" data=site.data.dashboard %}
This table automatically lists all existing github branches.
<script type="text/javascript" src="{{site.baseurl}}/script/populate_badges.js"></script>
<script>myTableBadges.populate("dashboard_table", "https://api.github.com/repos/guillaumeblanc/ozz-animation/branches");</script>
