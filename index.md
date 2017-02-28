---
title: "open source c++ skeletal animation engine and tools"
layout: home
keywords: home
collection: home
---

{% include links.jekyll %}

ozz-animation provides runtime character animation functionalities (loading, sampling, blending...), with the toolchain to import from major DCC formats (Collada, Fbx, ...). It proposes a low-level renderer and game-engine agnostic implementation, focusing on performance and memory constraints with a data-oriented design.
<br />

<div class="w3-row-padding w3-center">
  <div class="w3-third">
    <div class="w3-card-2" style="min-height:460px">
      <div class="w3-container w3-margin">
        <div class="w3-xxlarge w3-wide">Features</div>
        <div class="w3-text-theme">
          <i class="fa fa-superpowers w3-padding-8" style="font-size:128px"></i>
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
    <div class="w3-card-2" style="min-height:460px">
      <div class="w3-container w3-margin">
        <div class="w3-xxlarge w3-wide">Toolset</div>
        <div class="w3-text-theme">
          <i class="fa fa-wrench w3-padding-8" style="font-size:128px"></i>
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
    <div class="w3-card-2" style="min-height:460px">
      <div class="w3-container w3-margin">
        <div class="w3-xxlarge w3-wide">Code</div>
        <div class="w3-text-theme">
          <i class="fa fa-pencil w3-padding-8" style="font-size:128px"></i>
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

<div class="w3-row w3-center">
  <div class="w3-card-2 w3-margin">
    <div class="w3-margin">
      <a class="a_reject" href="{{site.baseurl}}/samples">
        <div class="w3-center w3-xxlarge w3-wide">Samples</div>
        <p>Review samples to evaluate ozz-animation runtime features.</p>
      </a>
      {% include emscripten.jekyll emscripten_path="samples/emscripten/sample_additive.js" %}
    </div>
  </div>
</div>

<div class="w3-row-padding w3-center">
  <a class="a_reject w3-half" href="{{site.baseurl}}/documentation/getting_started">
    <div class="w3-card-2" style="min-height:260px">
      <div class="w3-container w3-margin">
        <div class="w3-xxlarge w3-wide">Getting started</div>
        <div class="w3-text-theme">
          <i class="fa fa-forward w3-padding-8" style="font-size:128px"></i>
        </div>
      </div>  
    </div>
  </a>
  <a class="a_reject w3-half" href="http://github.com/guillaumeblanc/ozz-animation" target="_blank">
    <div class="w3-card-2" style="min-height:260px">
      <div class="w3-container w3-margin">
        <div class="w3-xxlarge w3-wide">View on Github</div>
        <div class="w3-text-theme">
          <i class="fa fa-github w3-padding-8" style="font-size:128px"></i>
        </div>
      </div>
    </div>
  </a>
</div>
