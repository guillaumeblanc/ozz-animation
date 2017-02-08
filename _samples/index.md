---
title: Overview
layout: full
keywords: sample, tutorial
permalink: /:collection/index.html
---

{% include links.jekyll %}

ozz-animation provides many samples to help with this first integration steps.

{% assign samples = site.samples | sort:"order" %}
{% for sample in samples %}
  {% if sample.title == "Overview" %}
    {% continue %}
  {% endif %}  
+ [{{sample.title}}]({{site.baseurl}}{{sample.url}}) {% include level_tag.jekyll level=sample.level %}
{% endfor %}

Some [How-To][link_how_to] are also available to help with more advanced API understanding.