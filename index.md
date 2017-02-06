---
title: "open source c++ skeletal animation engine and tools"
layout: home
keywords: home
collection: home
---

{% include links.jekyll %}

ozz-animation is an **open source c++ 3d skeletal animation engine**. It provides runtime character animation functionalities (loading, sampling, blending...), with the toolchain to import from major DCC formats (Collada, Fbx, ...). It proposes a low-level renderer and game-engine agnostic implementation, focusing on performance and memory constraints with a data-oriented design.

<br />

<div class="w3-row-padding w3-center">
  <div class="w3-third">
    <div class="w3-card-2" style="min-height:400px">
      <div class="w3-container">
        <h1>Features</h1>
        <div class="w3-text-theme">
          <i class="fa fa-superpowers" style="font-size:128px"></i>
        </div>
        <p>Sampling</p>
        <p>Blending</p>
        <p>Partial blending</p>
        <p>Additive blending</p>
        <p>Software skinning</p>
      </div>  
    </div>  
  </div>
  <div class="w3-third">
    <div class="w3-card-2" style="min-height:400px">
      <div class="w3-container">
        <h1>Toolset</h1>
        <div class="w3-text-theme">
          <i class="fa fa-wrench" style="font-size:128px"></i>
        </div>
        <p>Fbx, Collada, Obj, 3ds, dxf</p>
        <p>Compression</p>
        <p>Keyframe reduction</p>
        <p>Command line tools</p>
        <p>Offline libraries</p>
      </div>  
    </div>  
  </div>
  <div class="w3-third">
    <div class="w3-card-2" style="min-height:400px">
      <div class="w3-container">
        <h1>Code</h1>
        <div class="w3-text-theme">
          <i class="fa fa-pencil" style="font-size:128px"></i>
        </div>
        <p>Engine agnostic</p>
        <p>Cache friendly</p>
        <p>Thread safety</p>
        <p>SIMD SOA math</p>
        <p>MIT license</p>
      </div>
    </div>  
  </div>
</div>

Below is a [baked animation sample][link_baked_sample], part of ozz package. It shows a physic simulation baked into an animation. This scene contains more than 1000 animated cuboids. 

{% include emscripten.jekyll emscripten_path="samples/emscripten/sample_baked.js" %}

