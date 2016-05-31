if(NOT ozz_build_samples)
  return()
endif()

# dae2skel and dae2anim are cross compiled.
if(CMAKE_CROSSCOMPILING)
  find_package(dae2skel)
  find_package(dae2anim)
endif()

# Default time for testing samples.
set(SAMPLE_TESTING_LOOPS "20")

# Disables sample rending if testing from the dashboard.
if(ozz_run_tests_headless)
  set(SAMPLE_RENDER_ARGUMENT "--norender")
endif()

# Adds the sample framework to the include path.
include_directories(./)

# Adds samples and helper libraries.
add_subdirectory(additive)
add_subdirectory(attach)
#add_subdirectory(baked)
add_subdirectory(blend)
add_subdirectory(framework)
add_subdirectory(millipede)
add_subdirectory(multithread)
add_subdirectory(optimize)
add_subdirectory(partial_blend)
add_subdirectory(playback)
