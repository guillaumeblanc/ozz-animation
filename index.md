---
title: "ozz-animation - open source c++ skeletal animation library and toolset"
layout: home
keywords: home,main
collection: home
---

{% include links.jekyll %}

<div class="w3-container w3-margin">
  ozz-animation is an open source c++ 3d skeletal animation library. It provides runtime character animation functionalities (sampling, blending...), with the toolset to import major DCC formats (Collada, Fbx...). It proposes a low-level renderer and game-engine agnostic implementation, focusing on performance and memory constraints with a data-oriented design.
</div>
<br />
<div class="w3-row-padding w3-center">
  <div class="w3-third">
    <a class="a_reject" href="{{site.baseurl}}/documentation/features/">
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
    </a>
  </div>
  <div class="w3-third">
    <a class="a_reject" href="{{site.baseurl}}/documentation/toolset/">
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
    </a>
  </div>
  <div class="w3-third">
    <a class="a_reject" href="{{site.baseurl}}/documentation/animation_runtime/">
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
    </a>
  </div>
</div>

<div class="w3-row w3-center">
  <div class="w3-card-2 w3-margin">
    <div class="w3-margin">
      <a class="a_reject" href="{{site.baseurl}}/samples/">
        <div class="w3-center w3-xxlarge w3-wide">Samples</div>
        <p>Try the samples to evaluate ozz-animation runtime features.</p>
      </a>
      {% include emscripten.jekyll emscripten_path="samples/emscripten/sample_look_at.js" %}
    </div>
  </div>
</div>

<div class="w3-row-padding w3-center">
  <a class="a_reject w3-half" href="{{site.baseurl}}/documentation/">
    <div class="w3-card-2" style="min-height:260px">
      <div class="w3-container w3-margin">
        <div class="w3-xxlarge w3-wide">Getting started</div>
        <div class="w3-text-theme">
          <i class="fa fa-forward w3-padding-8" style="font-size:128px"></i>
        </div>
      </div>  
    </div>
  </a>
  <a class="a_reject w3-half" href="https://github.com/guillaumeblanc/ozz-animation/">
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
