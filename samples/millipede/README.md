# Ozz-animation sample: Millipede, offline libraries usage.

## Description

Demonstrates usage of ozz offline data structures and utilities.

## Concept

Procedurally creates an offline skeleton and animation, whose are then converted to runtime structures for sampling and rendering. The procedural skeleton aims to look like a millipede, made of slices of a leg pair and a vertebra. It makes it easy to tweak the number of joints by adding/removing slices. Both the skeleton and the animation are rebuilt from scratch when the number of joints is changed.
This sample does not intend to demonstrate how to do procedural runtime animations, which would rather require to work directly on local-transforms (the output of animation sampling stage, input of the blending stage).

## Sample usage

The sample provides the GUI to tweak the number of joints, from 7 (1 slice) up to the maximum number of joints supported by ozz (currently 1023). 
Some other playback parameters can be tuned:
- Play/pause animation.
- Fix animation time.
- Set playback speed, which can be negative to go backward.

## Implementation

1. Create the skeleton object:
   1. Instantiates a RawSkeleton and fills the hierarchy with as many slices (a vertebra and two legs) as needed to reach the requested number of joints. The RawSkeleton object is an offline suitable format, meaning it can be easily programmatically modified: Add/remove/rename joints...
   2. Convert the RawSkeleton (aka offline) object to a runtime `ozz::animation::offline::Skeleton`, using `ozz::animation::offline::SkeletonBuilder`. This offline utility does the conversion to the runtime format, which can then be serialized or used by the runtime API.
2. Create the animation object:
   1. Instantiates a `ozz::animation::offline::RawAnimation` object and fills it with keyframes for every joint of the skeleton, simulating walk cycles. The RawAnimation is an offline suitable format, meaning it is easily programmatically modified: Add/remove keys, add/remove tracks, change duration...
   2. Convert the offline animation to a runtime format, using `ozz::animation::offline::AnimationBuilder`. This utility takes as input a RawAnimation and outputs a runtime `ozz::animation::Animation`. The runtime format is the one used for sampling. In opposition with the offline one, it cannot be edited/modified. It is optimized for runtime usage in term of memory layout (context coherence for sampling) and footprint (compression scheme).
3. The remaining code of the sample is to allocate runtime buffers and samples animation every frame. See playback sample for more details.
