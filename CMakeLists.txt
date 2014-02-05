cmake_minimum_required (VERSION 2.8)

# Defines the project's name
project(ozz)

# Add project build options
set(ozz_build_samples ON CACHE BOOL "Build samples")
set(ozz_build_tests ON CACHE BOOL "Build unit tests")
set(ozz_build_sse2 ON CACHE BOOL "Enable SSE2 instructions set")
set(ozz_redebug_all OFF CACHE BOOL "Enable all REDEBUGing features")
set(ozz_enable_cdash OFF CACHE BOOL "Enable CDash")
set(ozz_enable_coverage OFF CACHE BOOL "Enable coverage tests")

# Enables unit tests only
if(ozz_build_tests)
  enable_testing()
endif()

# Set compiler flags and options
include(build-utils/cmake/compiler_settings.cmake)

# Set cpack flags and options
include(build-utils/cmake/package_settings.cmake)

# Includes the root of ozz includes
include_directories(include)

# Starts building the sources tree
add_subdirectory(src)

# Locates media directory.
set(ozz_media_directory "${CMAKE_SOURCE_DIR}/media")

# Adds test to build if they haven't been disabled
if(ozz_build_tests OR BUILD_TESTING)
  # Continues with the tests tree
  add_subdirectory(test)
endif()

# Continues with samples
if(ozz_build_samples)
  add_subdirectory(samples)
endif()
