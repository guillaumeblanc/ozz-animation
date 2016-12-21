cmake_minimum_required (VERSION 3.0)

# Defines the project's name
project(ozz)

# Current version
set(CPACK_PACKAGE_VERSION_MAJOR 0)
set(CPACK_PACKAGE_VERSION_MINOR 9)
set(CPACK_PACKAGE_VERSION_PATCH 0)

# Add project build options
option(ozz_build_fbx "Build Fbx pipeline (Requires Fbx SDK)" ON)
option(ozz_build_samples "Build samples" ON)
option(ozz_build_howtos "Build howtos" ON)
option(ozz_build_tests "Build unit tests" OFF)
option(ozz_build_simd_ref "Forces SIMD math reference implementation" OFF)
option(ozz_build_cpp11 "Enable c++11" OFF)
option(ozz_build_coverage "Enable gcov build coverage" OFF)

# Outputs selected options (can be modified by the command line)
message("-- ---------------------------------------------------------")
message("-- Selected options:")
message("-- - ozz_build_fbx: " ${ozz_build_fbx})
message("-- - ozz_build_samples: " ${ozz_build_samples})
message("-- - ozz_build_howtos: " ${ozz_build_howtos})
message("-- - ozz_build_tests: " ${ozz_build_tests})
message("-- - ozz_build_simd_ref: " ${ozz_build_simd_ref})
message("-- - ozz_build_cpp11: " ${ozz_build_cpp11})
message("-- - ozz_build_coverage: " ${ozz_build_coverage})

# Add project execution options
option(ozz_run_tests_headless "Run samples without rendering (used for unit tests)" ON)
set(ozz_sample_testing_loops 20 CACHE INT "Number of loops while running sample tests (used for unit tests)")

# Include ozz cmake parameters and scripts
include(build-utils/cmake/compiler_settings.cmake)
include(build-utils/cmake/package_settings.cmake)
include(build-utils/cmake/fuse_target.cmake)
include(build-utils/cmake/clang_format.cmake)

# Enables unit tests only
if(ozz_build_tests)
  enable_testing()
endif()

# Configure CMake module path
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/build-utils/cmake/modules/")

# Detects Fbx SDK, required to build Fbx pipeline.
if(ozz_build_fbx)
  find_package(Fbx 2016)
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
if(ozz_build_howtos AND NOT EMSCRIPTEN)
  add_subdirectory(howtos)
endif()

# Continues with samples
if(ozz_build_samples)
  add_subdirectory(samples)
endif()

# Continues with the tests tree
if(ozz_build_tests AND NOT EMSCRIPTEN)
  add_subdirectory(test)
endif()
