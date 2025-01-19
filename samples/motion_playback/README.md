# Ozz-animation sample: Motion playback

## Description

Extracting root motion is the process of capturing character motion (translation and rotation) from an animation. The purpose is to re-apply motion at runtime to drive the character and its collision capsule. This allows the capsule (responsible for physical collisions) to follow precisely the character, even if it moves during the animation. Furthermore it also allows to animate motion with a speed that matches with the animation (compared to the code driven approach).

## Concept

The sample uses animation and motion tracks converted using *2ozz tools, relying itself on `ozz::animation::offline::MotionExtractor`. It uses animation.tracks.motion settings as described in the reference configuration.
The motion track for this sample is composed of:
- A translation track, built from the xz projection of the root joint.
- A rotation track, built from the yaw only movement of the root joint. Motion extractor loop option is also activated to ensure animation begin and end rotation matches.

Note also that *2ozz baked (ie: removed/extracted) the motion from the animation, so it can be re-applied at runtime.

At runtime, the position and rotation are sampled from the motion tracks each frame. They are accumulated to move the character transform (by delta motion each frame), allowing to continue moving when the animation loops.
The sample uses `ozz::sample::MotionAccumulator` and `ozz::sample::MotionSampler` utilities to help with the complexity of managing loops.

## Sample usage

The sample allows to activate / deactivate usage of motion tracks. Deactivating motion tracks shows the baked animation only.

The sample exposes `ozz::sample::MotionAccumulator` capability of applying a rotation to the path.

Finally, different options allows to render the motion track/path, as well as character trace.

## Implementation

1. Load the skeleton and the animation with root motion extracted. See "playback" sample for more details.
2. Load position (ozz::animation::Float3Track) and rotation (ozz::animation::QuaternionTrack) tracks from the file exported by *2ozz tool.
3. Sample the animation local-space transforms and convert them to model-space.
4. Uses animation sampling time/ratio to sample also the motion tracks.
5. Accumulate motion using `ozz::sample::MotionAccumulator` utility, which handles animation loops.
6. The rotation to apply this frame is computed from the angular velocity and frame duration.
6. Get current transform from the accumulator. Convert it to a Float4x4 matrix to render the character at this location.