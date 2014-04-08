# Samples requires OpenGL package.
find_package(OpenGL REQUIRED)

# Default time for testing samples.
set(SAMPLE_TESTING_LOOPS "12")

# Disables sample rending if testing from the dashboard.
if(ozz_enable_cdash)
  set(SAMPLE_RENDER_ARGUMENT "--norender")
endif()

# Adds glfw.
add_subdirectory(${CMAKE_SOURCE_DIR}/extern/glfw glfw)

# Adds the sample framework to the include path.
include_directories(./)

# Adds samples and helper libraries.
add_subdirectory(framework)
add_subdirectory(millipede)
add_subdirectory(optimize)
add_subdirectory(attach)
add_subdirectory(blend)
add_subdirectory(partial_blend)
add_subdirectory(multithread)
add_subdirectory(playback)
