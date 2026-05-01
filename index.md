---
title: "ozz-animation - open source c++ skeletal animation library and toolset"
layout: home
keywords: home,main
collection: home
---

{% include links.jekyll %}

<div class="w3-container w3-margin">
  ozz-animation is an open source c++ 3d skeletal animation library. It provides runtime character animation functionalities (sampling, blending...), with the toolset to import major DCC formats (Collada, Fbx, glTF...). It proposes a game-engine agnostic implementation, focusing on performance and memory constraints with a data-oriented design.
</div>

<div class="w3-row-padding w3-center">
  <div class="w3-third">
    <a class="a_reject" href="{{site.baseurl}}/documentation/features/">
      <div class="w3-card-2">
        <div class="w3-container w3-margin">
          <div class="w3-xxlarge w3-wide">Features</div>
          <div class="w3-text-theme">
            <i class="fab fa-superpowers w3-padding-8" style="font-size:128px"></i>
          </div>
          <p>Sampling</p>
          <p>Blending</p>
          <p>Motion extraction</p>
          <p>Inverse kinematic</p>
          <p>Software skinning</p>
        </div>  
      </div>
    </a>
  </div>
  <div class="w3-third">
    <a class="a_reject" href="{{site.baseurl}}/documentation/toolset/">
      <div class="w3-card-2">
        <div class="w3-container w3-margin">
          <div class="w3-xxlarge w3-wide">Toolset</div>
          <div class="w3-text-theme">
            <i class="fas fa-wrench w3-padding-8" style="font-size:128px"></i>
          </div>
          <p>Fbx, Collada, glTF, 3ds...</p>
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
      <div class="w3-card-2">
        <div class="w3-container w3-margin">
          <div class="w3-xxlarge w3-wide">Code</div>
          <div class="w3-text-theme">
            <i class="fas fa-pencil-alt w3-padding-8" style="font-size:128px"></i>
          </div>
          <p>C++</p>
          <p>Engine agnostic</p>
          <p>Cache friendly</p>
          <p>SIMD SOA math</p>
          <p>MIT license</p>
        </div>
      </div>
    </a>
  </div>
</div>

<div class="w3-row w3-center">
  <div class="w3-card-2 w3-margin">
    <div class="w3-margin w3-text-theme">
      <p>This sample implements procedural look-at on an animated character, using ozz IK feature.</p>
    </div>
    {% include emscripten.jekyll emscripten_path="samples/emscripten/look_at/sample_look_at.js" %}
  </div>
</div>

<div class="w3-row w3-center">
  <a class="a_reject" href="{{site.baseurl}}/samples/">
    <div class="w3-card-2 w3-margin">
      <div class="w3-container w3-margin">
        <div class="w3-xxlarge w3-wide">Samples</div>
        <div class="w3-text-theme">
          <i class="fas fa-eye w3-padding-8" style="font-size:128px"></i>
        </div>
      </div>
    </div>
  </a>
</div>

<div class="w3-row-padding w3-center">
  <a class="a_reject w3-half" href="{{site.baseurl}}/documentation/">
    <div class="w3-card-2">
      <div class="w3-container w3-margin">
        <div class="w3-xxlarge w3-wide">Getting started</div>
        <div class="w3-text-theme">
          <i class="fas fa-forward w3-padding-8" style="font-size:128px"></i>
        </div>
      </div>  
    </div>
  </a>
  <a class="a_reject w3-half" href="https://github.com/guillaumeblanc/ozz-animation/">
    <div class="w3-card-2">
      <div class="w3-container w3-margin">
        <div class="w3-xxlarge w3-wide">View on Github</div>
        <div class="w3-text-theme">
          <i class="fab fa-github w3-padding-8" style="font-size:128px"></i>
        </div>
      </div>
    </div>
  </a>
</div>
