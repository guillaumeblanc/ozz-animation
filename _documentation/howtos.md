---
title: HowTos
layout: full
keywords: documentation,doc,help,howto,how to,custom,import,animation,skeleton,fbx,load,file,serialize
order: 90
---

{% include links.jekyll %}

This page contains some how-tos for achieving different tasks with ozz-animation:

How to load an object from a file?
==================================

ozz implements serialization and deserialization through its [archive][link_serialize] library, and file io operations through the [ozz::io::Stream][link_io] interface.

So loading an object from a file means first opening the file for writing.

{% highlight cpp %}
// A file in ozz is a ozz::io::File, which implements ozz::io::Stream
// interface and complies with std FILE specifications.
// ozz::io::File follows RAII programming idiom, which ensures that
// the file will always be closed (by ozz::io::FileStream destructor).
ozz::io::File file(filename, "rb");
{% endhighlight %}

{% highlight cpp %}
// Checks file status, which can be closed if filename is invalid.
if (!file.opened()) {
  ozz::log::Err() << "Cannot open file " << filename << "." << std::endl;
  return EXIT_FAILURE;
}
{% endhighlight %}

Then use the `ozz::io::IArchive` object to deserialize the object.

{% highlight cpp %}
// Now the file is opened. we can actually read from it. This uses ozz
// archive mechanism.
// The first step is to instantiate an read-capable (ozz::io::IArchive)
// archive object, in opposition to write-capable (ozz::io::OArchive)
// archives.
// Archives take as argument stream objects, which must be valid and opened.
ozz::io::IArchive archive(&file);
{% endhighlight %}
{% highlight cpp %}
// Before actually reading the object from the file, we need to test that
// the archive (at current seek position) contains the object type we
// expect.
// Archives uses a tagging system that allows to mark and detect the type of
// the next object to deserialize. Here we expect a skeleton, so we test for
// a skeleton tag.
// Tagging is not mandatory for all object types. It's usually only used for
// high level object types (skeletons, animations...), but not low level
// ones (math, native types...).
if (!archive.TestTag<ozz::animation::Skeleton>()) {
  ozz::log::Err() << "Archive doesn't contain the expected object type." <<
    std::endl;
  return EXIT_FAILURE;
}
{% endhighlight %}
{% highlight cpp %}
// Now the tag has been validated, the object can be read.
// IArchive uses >> operator to read from the archive to the object.
// Only objects that implement archive specifications can be used there,
// along with all native types. Note that pointers aren't supported.
ozz::animation::Skeleton skeleton;
archive >> skeleton;
{% endhighlight %}

Serializing an object is very similar. The differences are that the file should be opened for writing, and the `ozz::io::OArchive` (output archive) should be used.

Full sources for this how-to are available [here][link_how_to_load_src].

How to write a custom skeleton importer?
========================================
ozz proposes all the offline [data structures][link_toolset] and [utilities][link_toolset_utilities] required to build runtime optimized skeleton data. `ozz::animation::offline::RawSkeleton` is the offline data structure for skeletons. It is defined as a hierachy of joints, with their names and default transformation. It is converted to the runtime `ozz::animation::Skeleton` with `ozz::animation::offline::SkeletonBuilder` class.

So writing a custom importer means first filling a `RawSkeleton` object. The next few code part setup a Raw skeleton with a root and 2 children (3 joints in total).

{% highlight cpp %}
// Creates a RawSkeleton
ozz::animation::offline::RawSkeleton raw_skeleton;
{% endhighlight %}

{% highlight cpp %}
// Creates the root joint. Uses std::vector API to resize the number of roots.
raw_skeleton.roots.resize(1);
ozz::animation::offline::RawSkeleton::Joint& root = raw_skeleton.roots[0];
{% endhighlight %}

{% highlight cpp %}
// Setup root joints name.
root.name = "root";
{% endhighlight %}

{% highlight cpp %}
// Setup root joints bind-pose/rest transformation. It's kind of the default
// skeleton posture (most of the time a T-pose). It's used as a fallback when
// there's no animation for a joint.
root.transform.translation = ozz::math::Float3(0.f, 1.f, 0.f);
root.transform.rotation = ozz::math::Quaternion(0.f, 0.f, 0.f, 1.f);
root.transform.scale = ozz::math::Float3(1.f, 1.f, 1.f);
{% endhighlight %}

{% highlight cpp %}
// Now adds 2 children to the root.
root.children.resize(2);
{% endhighlight %}

{% highlight cpp %}
// Setups the 1st child's name and transfomation.
ozz::animation::offline::RawSkeleton::Joint& left = root.children[0];
left.name = "left";
left.transform.translation = ozz::math::Float3(1.f, 0.f, 0.f);
left.transform.rotation...
 
//...and so on with the whole hierarchy...
{% endhighlight %}

The `RawSkeleton` object is completed once the whole joints hierarchy has been created. The next line checks its validity.

{% highlight cpp %}
// Test for skeleton validity.
// The main invalidity reason is the number of joints, which must be lower
// than ozz::animation::Skeleton::kMaxJoints.
if (!raw_skeleton.Validate()) {
  return EXIT_FAILURE;
}
{% endhighlight %}

The next step is to convert this `RawSkeleton` to a runtime Skeleton, optimized for runtime processing. This is acheived using `ozz::animation::offline::SkeletonBuilder` class:

{% highlight cpp %}
// Creates a SkeletonBuilder instance.
ozz::animation::offline::SkeletonBuilder builder;
{% endhighlight %}

{% highlight cpp %}
// Executes the builder on the previously prepared RawSkeleton, which returns
// a new runtime skeleton instance.
// This operation will fail and return NULL if the RawSkeleton isn't valid.
ozz::animation::Skeleton* skeleton = builder(raw_skeleton);
{% endhighlight %}

Now use the skeleton as you want, but don't forget to delete it in the end.

{% highlight cpp %}
// In the end the skeleton needs to be deleted.
ozz::memory::default_allocator()->Delete(skeleton);
{% endhighlight %}

If your custom importer is a command line tool like fbx2skel is, you can re-use [`ozz::animation::offline::SkeletonConverter`][link_src_convert2skel]. By overriding `SkeletonConverter::Import()` function, you'll benefit from ozz command line tools implementation, skeleton serialization and so on...

Full sources for this how-to are available [here][link_src_howto_custom_skeleton_importer].

How to write a custom animation importer?
=========================================
As for the skeleton, ozz proposes all the offline [data structures][link_toolset] and [utilities][link_toolset] required to build runtime optimized animation data. `ozz::animation::offline::RawAnimation` is the offline data structure for animations. It is defined as an array of tracks, each one containing discrete arrays of translation, rotation and scale keyframes. It is converted to the runtime `ozz::animation::Animation` with `ozz::animation::offline::AnimationBuilder` class.

So writing a custom importer means first filling a RawAnimation object. The next few code part setup a Raw animation with 3 tracks (usable with a 3 joints skeleton).

{% highlight cpp %}
// Creates a RawAnimation.
ozz::animation::offline::RawAnimation raw_animation;
{% endhighlight %}

{% highlight cpp %}
// Sets animation duration (to 1.4s).
// All the animation keyframes times must be within range [0, duration].
raw_animation.duration = 1.4f;
{% endhighlight %}

{% highlight cpp %}
// Creates 3 animation tracks.
// There should be as much tracks as there are joints in the skeleton that
// this animation targets.
raw_animation.tracks.resize(3);
{% endhighlight %}

Fills each track with keyframes. Tracks should be ordered in the same order as joints in the `ozz::animation::Skeleton`. Joint's names can be used to find joint's index in the skeleton.

{% highlight cpp %}
// Fills 1st track with 2 translation keyframes.
{
  // Create a keyframe, at t=0, with a translation value.
  const ozz::animation::offline::RawAnimation::TranslationKey key0 = {
    0.f, ozz::math::Float3(0.f, 4.6f, 0.f)};

  raw_animation.tracks[0].translations.push_back(key0);

  // Create a new keyframe, at t=0.93 (must be less than duration), with a
  // translation value.
  const ozz::animation::offline::RawAnimation::TranslationKey key1 = {
    .93f, ozz::math::Float3(0.f, 9.9f, 0.f)};
  raw_animation.tracks[0].translations.push_back(key1);
}
{% endhighlight %}

{% highlight cpp %}
// Fills 1st track with a rotation keyframe. It's not mandatory to have the
// same number of keyframes for translation, rotations and scales.
{
  // Create a keyframe, at t=.46, with a quaternion value.
  const ozz::animation::offline::RawAnimation::RotationKey key0 = {
    .46f, ozz::math::Quaternion(0.f, 1.f, 0.f, 0.f)};

  raw_animation.tracks[0].rotations.push_back(key0);
}
{% endhighlight %}

{% highlight cpp %}
// For this example, don't fill scale with any key. The default value will be
// identity, which is ozz::math::Float3(1.f, 1.f, 1.f) for scale.
{% endhighlight %}

...and so on with all other tracks...

{% highlight cpp %}
// Test for animation validity. These are the errors that could invalidate
// an animation:
//  1. Animation duration is less than 0.
//  2. Keyframes' are not sorted in a strict ascending order.
//  3. Keyframes' are not within [0, duration] range.
if (!raw_animation.Validate()) {
  return EXIT_FAILURE;
}
{% endhighlight %}

The next section converts the `RawAnimation` to a runtime Animation.
If you want to optimize your animation with [`ozz::animation::offline::AnimationOptimizer`][link_animation_optimizer], this must be done at this point, before building the runtime animation.

{% highlight cpp %}
// Creates a AnimationBuilder instance.
ozz::animation::offline::AnimationBuilder builder;
{% endhighlight %}

{% highlight cpp %}
// Executes the builder on the previously prepared RawAnimation, which returns
// a new runtime animation instance.
// This operation will fail and return NULL if the RawAnimation isn't valid.
ozz::animation::Animation* animation = builder(raw_animation);
{% endhighlight %}

... Now use the animation as you want...

{% highlight cpp %}
// In the end the animation needs to be deleted.
ozz::memory::default_allocator()->Delete(animation);
{% endhighlight %}

If your custom importer is a command line tool like fbx2anim is, you can re-use [`ozz::animation::offline::AnimationConverter`][link_src_convert2anim]. By overriding `AnimationConverter::Import()` function, you'll benefit from ozz command line tools implementation, keyframe optimisation, animation serialization and so on...

Full sources for this how-to are available [here][link_src_howto_custom_animation_importer].