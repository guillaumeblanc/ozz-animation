---
title: How does ozz-animation deal with Multi-threading?
layout: full
order: 10
---

{% include links.jekyll %}

ozz-animation proposes a thread safe API, but doesn't implement or impose a multithreading solution. It's up to the user to distribute ozz job's workload to different threads, if needed. The [mutlithread sample][link_multithread_sample] uses OpenMp for example, but it could be any threading API really: pthread, c++11 threads, Intel TBB...
