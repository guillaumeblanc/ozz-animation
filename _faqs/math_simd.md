---
title: How does ozz-animation deal with Simd on non-pc platforms?
layout: null
order: 20
---

ozz automatically falls back to a scalar/fpu implementation if SSE isn't available.