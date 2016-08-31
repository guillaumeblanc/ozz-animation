cmake_minimum_required (VERSION 3.0)

# Defines the project's name
project(ozz)

# Current version
set(CPACK_PACKAGE_VERSION_MAJOR 0)
set(CPACK_PACKAGE_VERSION_MINOR 8)
set(CPACK_PACKAGE_VERSION_PATCH 0)

# Add project build options
set(ozz_build_fbx ON CACHE BOOL "Build Fbx pipeline (Requires Fbx SDK)")
set(ozz_build_samples ON CACHE BOOL "Build samples")
set(ozz_build_howtos ON CACHE BOOL "Build howtos")
set(ozz_build_tests ON CACHE BOOL "Build unit tests")
set(ozz_build_simd_ref OFF CACHE BOOL "Forces SIMD math reference implementation")
set(ozz_build_cpp11 OFF CACHE BOOL "Enable c++11")
set(ozz_build_coverage OFF CACHE BOOL "Enable gcov code coverage")

# Add project execution options
set(ozz_run_tests_headless ON CACHE BOOL "Run samples without rendering (used for unit tests)")
set(ozz_sample_testing_loops "20" CACHE INT "Number of loops while running sample tests (used for unit tests)")

# Include ozz cmake parameters and scripts
include(build-utils/cmake/compiler_settings.cmake)
include(build-utils/cmake/package_settings.cmake)
include(build-utils/cmake/fuse_target.cmake)

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

# Locates media directory.
set(ozz_media_directory "${CMAKE_SOURCE_DIR}/media")

# Creates temporary directory for tests inputs/outputs.
set(ozz_temp_directory ${CMAKE_BINARY_DIR}/temp)
file(MAKE_DIRECTORY ${ozz_temp_directory})

# Includes the root of ozz includes
include_directories(include)

# Starts building the sources tree
add_subdirectory(src)

# Continues with howtos
add_subdirectory(howtos)

# Continues with samples
add_subdirectory(samples)

# Continues with the tests tree
add_subdirectory(test)
