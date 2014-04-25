cmake_minimum_required (VERSION 2.8)

# Defines the project's name
project(ozz)

# Current version
set(CPACK_PACKAGE_VERSION_MAJOR 0)
set(CPACK_PACKAGE_VERSION_MINOR 4)
set(CPACK_PACKAGE_VERSION_PATCH 0)

# Add project build options
set(ozz_build_fbx ON CACHE BOOL "Buils Fbx pipeline (Requires Fbx SDK)")
set(ozz_build_samples ON CACHE BOOL "Build samples")
set(ozz_build_tests ON CACHE BOOL "Build unit tests")
set(ozz_build_sse2 ON CACHE BOOL "Enable SSE2 instructions set")
set(ozz_build_redebug_all OFF CACHE BOOL "Enable all REDEBUGing features")

# Internal variables.
set(ozz_enable_cdash OFF CACHE BOOL "Enable CDash")
set(ozz_enable_coverage OFF CACHE BOOL "Enable coverage tests")

# Internal options (cannot be modified by the GUI)
set(ozz_dashboard_build OFF CACHE BOOL INTERNAL)

# Set compiler flags and options
include(build-utils/cmake/compiler_settings.cmake)

# Set cpack flags and options
include(build-utils/cmake/package_settings.cmake)

# Enables unit tests only
if(ozz_build_tests)
  enable_testing()
endif()

# Configure CMake module path
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/build-utils/cmake/modules/")

# Detects Fbx SDK, required to build Fbx pipeline.
if(ozz_build_fbx)
  find_package(Fbx)
  # Disables ozz_build_fbx if Fbx SDK isn't found.
  if(NOT FBX_FOUND)
    message("Fbx SDK not found, FBX tools libraries and samples will be skipped.")
    set(ozz_build_fbx OFF)
  endif()
endif()

# Enables unit tests only
if(ozz_build_tests)
  enable_testing()
endif()

# Configure CMake module path
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/build-utils/cmake/modules/")

# Detects Fbx SDK, required to build Fbx pipeline.
if(ozz_build_fbx)
  find_package(Fbx)
  # Disables ozz_build_fbx if Fbx SDK isn't found.
  if(NOT FBX_FOUND)
    message("Fbx SDK not found, FBX tools libraries and samples will be skipped.")
    set(ozz_build_fbx OFF)
  endif()
endif()

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
