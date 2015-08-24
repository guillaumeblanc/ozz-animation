Release version 0.7.3
---------------------

* Changes license to the MIT License (MIT).

Release version 0.7.2
---------------------

* Library
  - [animation] Improves rotations accuracy during animation sampling and blending. Quaternion normalization now uses one more Newton-Raphson step when computing inverse square root.
  - [offline] Fixes fbx animation duration when take duration is different to timeline length.
  - [offline] Bakes fbx axis/unit systems to all transformations, vertices, animations (and so on...), instead of using fbx sdk ConvertScene strategy which only transform scene root. This allows to mix skeleton, animation and meshes imported from any axis/unit system.
  - [offline] Uses bind-pose transformation, instead of identity, for skeleton joints that are not animated in a fbx file.
  - [offline] Adds support to ozz fbx tools (fbx2skel and fbx2anim) for other formats: Autodesk AutoCAD DXF (.dxf), Collada DAE (.dae), 3D Studio 3DS (.3ds)  and Alias OBJ (.obj). This toolchain based on fbx sdk will replace dae toolchain (dae2skel and dae2anim) which is now deprecated.

* HowTos
  - Adds file loading how-to, which demonstrates how to open a file and deserialize an object with ozz archive library.
  - Adds custom skeleton importer how-to, which demonstrates RawSkeleton setup and conversion to runtime skleton.
  - Adds custom animation importer how-to, which demonstrates RawAnimation setup and conversion to runtime animation.

* Samples
  - [skin] Skin sample now uses the inverse bind-pose matrices imported from the mesh file, instead of the one computed from the skeleton. This is a more robust solution, which furthermore allow to share the skeleton with meshes using different bind-poses.
  - [skin] Fixes joints weight normalization when importing fbx skin meshes.
  - [framework] Optimizes dynamic vertex buffer object re-allocation strategy.

Release version 0.7.1
---------------------

* Library
  - [offline] Updates to fbx sdk 2015.1 with vs2013 support.
  
* Samples
  - Adds sample_skin_fbx2skin to binary packages.

Release version 0.7.0
---------------------

* Library
  - [geometry] Adds support for matrix palette skinning transformation in a new ozz_geometry library.
  - [offline] Returns EXIT_FAILURE from dae2skel and fbx2skel when no skeleton found in the source file.
  - [offline] Fixes fbx axis and unit system conversions.
  - [offline] Removes raw_skeleton_archive.h and raw_animation_archive.h files, moving serialization definitions back to raw_skeleton.h and raw_animation.h to simplify understanding.
  - [offline] Removes skeleton_archive.h and animation_archive.h files, moving serialization definitions back to skeleton.h and animation.h.
  - [base] Changes Range<>::Size() definition, returning range's size in bytes instead of element count.

* Samples
  - Adds a skinning sample which demonstrates new ozz_geometry SkinningJob feature and usage.

Release version 0.6.0
---------------------

* Library
  - [animation] Compresses animation key frames memory footprint. Rotation key frames are compressed from 24B to 12B (50%). 3 of the 4 components of the quaternion are quantized to 2B each, while the 4th is restored during sampling. Translation and scale components are compressed to half float, reducing their size from 20B to 12B (40%).
  - [animation] Changes runtime::Animation class serialization format to support compression. Serialization retro-compatibility for this class has not been implemented, meaning that all runtime::Animation must be rebuilt and serialized using usual dae2anim, fbx2anim or using offline::AnimationBuilder utility.
  - [base] Adds float-to-half and half-to-float conversion functions to simd math library.

Release version 0.5.0
---------------------

* Library
  - [offline] Adds --raw option to *2skel and *2anim command line tools. Allows to export raw skeleton/animation object format instead of runtime objects.
  - [offline] Moves RawAnimation and RawSkeleton from the builder files to raw_animation.h and raw_skeleton.h files.
  - [offline] Renames skeleton_serialize.h and animation_serialize.h to skeleton_archive.h and animation_archive.h for consistency.
  - [offline] Adds RawAnimation and RawSkeleton serialization support with ozz archives.
  - [options] Changes parser command line arguments type from "const char**" to "const char* const*" in order to support implicit casting from arguments of type "char**".
  - [base] Change ozz::String std redirection from typedef to struct to be coherent with all other std containers redirection.
  - [base] Moves maths archiving file from ozz/base/io to ozz/base/maths for consistency.
  - [base] Adds containers serialization support with ozz archives.
  - [base] Removes ozz fixed size integers in favor of standard types available with <stdint.h> file.

* Samples
  - Adds Emscripten support to all supported samples.
  - Changes OpenGL rendering to comply with Gles2/WebGL.

* Build pipeline
  - Adds Emscripten and cross-compilation support to the builder helper python script.
  - Support for CMake 3.x.
  - Adds support for Microsoft Visual Studio 2013.
  - Drops support fot Microsoft Visual Studio 2008 and olders, as a consequence of using <stdint.h>.

Release version 0.4.0
---------------------

* Library
  - [offline] Adds Fbx import pipeline, through fbx2skel and fbx2anim command line tools.
  - [offline] Adds Fbx import and conversion library, through ozz_animation_offline_fbx. Building fbx related libraries requires fbx sdk to be installed.
  - [offline] Adds ozz_animation_offline_tools library to share the common work for Collada and Fbx import tools. This could be use to implement custom conversion command line tools.

* Samples
  - Adds Fbx resources to media path.
  - Makes use of Fbx resources with existing samples.

Release version 0.3.1
---------------------

* Samples
  - Adds keyboard camera controls.

* Build pipeline
  - Adds Mac OSX support, full offline and runtime pipeline, samples, dashboard...
  - Moves dashboard to http://ozz.qualipilote.fr/dashboard/cdash/
  - Improves dashboard configuration, using json configuration files available there: http://ozz.qualipilote.fr/dashboard/config/.

Release version 0.3.0
---------------------

* Library
  - [animation] Adds partial animation blending and masking, through per-joint-weight blending coefficients.
  - [animation] Switches all explicit [begin,end[ ranges (sequence of objects) to ozz::Range structure.
  - [animation] Moves runtime files (.h and .cc) to a separate runtime folder (ozz/animation/runtime).
  - [animation] Removes ozz/animation/utils.h and .cc
  - [options] Detects duplicated command line arguments and reports failure. 
  - [base] Adds helper functions to ozz::memory::Allocator to support allocation/reallocation/deallocation of ranges of objects through ozz::Range structure.

* Samples
  - Adds partial animation blending sample.
  - Adds multi-threading sample, using OpenMp to distribute workload.
  - Adds a sample that demonstrates how to attach an object to animated skeleton joints.
  - Improves skeleton rendering sample utility feature: includes joint rendering.
  - Adds screen-shot and video capture options from samples ui.
  - Adds a command line option (--render/--norender) to enable/disable rendering of sample, used for dashboard unit-tests.
  - Adds time management options, to dissociate (fix) update delta time from the real application time.
  - Improves camera manipulations: disables auto-framing when zooming/panning, adds mouse wheel support for zooming.
  - Fixes sample camera framing to match rendering field of view.

* Build pipeline
  - Adds CMake python helper tools (build-helper.py). Removes helper .bat files (setup, build, pack...).
  - Adds CDash support to generate nightly build reports. Default CDash server is http://my.cdash.org/index.php?project=Ozz.
  - Adds code coverage testing support using gcov.

Release version 0.2.0
---------------------

* Library
  - [animation] Adds animation blending support.
  - [animation] Sets maximum skeleton joints to 1023 (aka Skeleton::kMaxJointsNumBits) to improve packing and allow stack allocations.
  - [animation] Adds Skeleton::kRootIndex enum for parent index of a root joint.
  - [base] Adds signed/unsigned bit shift functions to simd library.
  - [base] Fixes SSE build flags for Visual Studio 64b builds.

* Samples
  - Adds blending sample.
  - Adds playback controller utility class to the sample framework.

Release version 0.1.0, initial open source release
--------------------------------------------------

* Library
  - [animation] Support for run-time animation sampling.
  - [offline] Support for building run-time animation and skeleton from a raw (aka offline/user friendly) format.
  - [offline] Full Collada import pipeline (lib and tools).
  - [offline]  Support for animation key-frame optimizations.
  - [base] Memory management redirection.
  - [base] Containers definition.
  - [base] Serialization and IO utilities implementation.
  - [base] scalar and vector maths, SIMD (SSE) and SoA implementation.
  - [options] Command line parsing utility.

* Samples
  - Playback sample, loads and samples an animation.
  - Millipede sample, offline pipeline usage.
  - Optimize sample, offline optimization pipeline.

* Build pipeline
  - CMake based build pipeline.
  - CTest/GTest based unit test framework.