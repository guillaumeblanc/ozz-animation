# Ozz-animation sample: Motion blending

## Description

Motion blending extends the concept of animation blending to root motion. It blends the motion resulting from the motion extraction process, which is needed to get the correct motion when animations are blent.

## Concepts

This sample is built on top of the animation blending sample. It blends three animations (walk, jog and run) and their corresponding root motions according to a single "speed" coefficient. See animation blending sample for more details.

It uses motion tracks extracted using *2ozz tools, relying itself on `ozz::animation::offline::MotionExtractor`. See animation.tracks.motion settings as described in the reference configuration.

At runtime, the motion of each animation is sampled and accumulated. See motion playback sample for more details.
The motion delta for the frame (provided by each accumulator) is used as input to `ozz::animation::MotionBlendingJob` for blending. Motion blending weights are simply the same weights as animation blending. The resulting blended motion is finally used to update character motion accumulator.

## Sample usage

The sample exposes blending parameter as a unique "blend ratio" slider that automatically controls all the other parameters to synchronize all animation layers: playback speeds, blending weights...
It's also possible to manually tweak all the sampling and blending parameters from the GUI, bypassing automatic weighting and synchronization.

An angular velocity is exposed to control the character.

> Note that predicted motion is rendered with an alpha transparency that reflects its (blending) weight.

## Implementation

1. Load animations and skeleton. See "playback" sample for more details.
2. Compute each animation time (in order to sync their duration) and samples each of them to get local-space transformations.
3. Compute each animation (layer) blend weight and fills `ozz::animation::BlendingJob` object. See "blending" sample.
4. Sample each motion track and update motion accumulator. This is implemented via `ozz::sample::MotionSampler` utility. Note that early out isn't possible, all 3 motions need to be updated to keep motion accumulator relevant.
5. Setup `ozz::animation::MotionBlendingJob` layers with the delta of motion from each animation, and the same weights as animation's ones.
6. Execute motion blending job and update character motion accumulator with the output. Character motion accumulator is implemented as `ozz::animation::MotionDeltaAccumulator` utility.
4. Convert local-space transformations to model-space matrices for rendering.