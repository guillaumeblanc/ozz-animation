---
title: FAQ
layout: full
keywords: faq
order: 85
---

{% include links.jekyll %}

Frequently asked questions
==========================

{% assign faqs = site.faqs | sort:"order" %}
{% for faq in faqs %}
  <div class="w3-card-2 w3-margin">
    <button onclick="OpenClose('{{faq.id}}')" class="w3-btn-block w3-theme w3-left-align">{{faq.title}}</button>
    <div id="{{faq.id}}" class="w3-container w3-hide w3-border w3-padding">
      {{ faq.content | markdownify }}
    </div>
  </div>
{% endfor %}

<script>
  function OpenClose(id) {
    var x = document.getElementById(id);
    if (x.className.indexOf("w3-show") == -1) {
      x.classList.add("w3-show");
    } else {
      x.classList.remove("w3-show");
    }
  }
</script>