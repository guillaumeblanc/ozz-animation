---
title: Features list and availability
layout: full
keywords: roadmap,features list,version,plan,compiler,gcc,clang,msvc,visual studio,os,mac,osx,ios,windows,linux,debian,freebsd
---

{% include references.jekyll %}

This page lists implemented features and plans for future releases.

{% include feedback_form.jekyll %}

Features
========

{% include table.jekyll data=site.data.features %}

Samples
=======

{% include table.jekyll data=site.data.samples %}

Os support
==========

The run-time code (ozz_base, ozz_animation, ozz_geometry) only refers to the standard CRT and has no OS specific code. Os support should be considered for samples, tools and tests only.

{% include table.jekyll data=site.data.os %}

Compiler support
================

{% include table.jekyll data=site.data.compilers %}
