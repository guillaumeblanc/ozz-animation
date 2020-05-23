cmake_minimum_required (VERSION 3.3)

# Defines the project's name
project(ozz)

# Current version
set(OZZ_VERSION_MAJOR 0)
set(OZZ_VERSION_MINOR 13)
set(OZZ_VERSION_PATCH 0)
set(OZZ_VERSION ${OZZ_VERSION_MAJOR}.${OZZ_VERSION_MINOR}.${OZZ_VERSION_PATCH})

# Add project build options
option(ozz_build_tools "Build tools" ON)
option(ozz_build_fbx "Build Fbx pipeline (Requires Fbx SDK)" ON)
option(ozz_build_gltf "Build glTF importer (Requires c++11)" ON)
option(ozz_build_data "Build data on code change" OFF)
option(ozz_build_samples "Build samples" ON)
option(ozz_build_howtos "Build howtos" ON)
option(ozz_build_tests "Build unit tests" ON)
option(ozz_build_simd_ref "Force SIMD math reference implementation" OFF)
option(ozz_build_msvc_rt_dll "Select msvc DLL runtime library" ON)
option(ozz_build_postfix "Use per config postfix name" ON)

# Include ozz cmake parameters and scripts
include(CheckCXXCompilerFlag)
include(${PROJECT_SOURCE_DIR}/build-utils/cmake/compiler_settings.cmake)
include(${PROJECT_SOURCE_DIR}/build-utils/cmake/package_settings.cmake)
include(${PROJECT_SOURCE_DIR}/build-utils/cmake/fuse_target.cmake)
include(${PROJECT_SOURCE_DIR}/build-utils/cmake/clang_format.cmake)

# Add project execution options
option(ozz_run_tests_headless "Run samples without rendering (used for unit tests)" ON)
set(ozz_sample_testing_loops "20" CACHE STRING "Number of loops while running sample tests (used for unit tests)")

# Configure CMake module path
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${PROJECT_SOURCE_DIR}/build-utils/cmake/modules/")

# Detects Fbx SDK, required to build Fbx pipeline.
if(ozz_build_tools AND ozz_build_fbx)

  # Select a msvc runtime compatible with ozz_build_msvc_rt_dll
  set(FBX_MSVC_RT_DLL ${ozz_build_msvc_rt_dll})

  # Search for FBX sdk package
  find_package(Fbx 2016)

  # Disables ozz_build_fbx if Fbx SDK isn't found.
  if(NOT FBX_FOUND)
    message("Fbx SDK not found, FBX tools libraries and samples will be skipped.")
    set(ozz_build_fbx OFF)
  endif()
else()
  # Disables fbx if tools are disabled
  set(ozz_build_fbx OFF)
endif()

# gltf 
if(ozz_build_tools AND ozz_build_gltf)
else()
  set(ozz_build_gltf OFF)
endif()

# Enables unit tests.
if(ozz_build_tests)
  enable_testing()
endif()

# Locates media directory.
set(ozz_media_directory "${PROJECT_SOURCE_DIR}/media")

# Creates temporary directory for tests inputs/outputs.
set(ozz_temp_directory ${PROJECT_BINARY_DIR}/temp)
file(MAKE_DIRECTORY ${ozz_temp_directory})

# Outputs selected options (can be modified by the command line)
message("-- ---------------------------------------------------------")
message("-- Selected options:")
message("-- - ozz_build_tools: " ${ozz_build_tools})
message("-- - ozz_build_fbx: " ${ozz_build_fbx})
message("-- - ozz_build_gltf: " ${ozz_build_gltf})
message("-- - ozz_build_data: " ${ozz_build_data})
message("-- - ozz_build_samples: " ${ozz_build_samples})
message("-- - ozz_build_howtos: " ${ozz_build_howtos})
message("-- - ozz_build_tests: " ${ozz_build_tests})
message("-- - ozz_build_simd_ref: " ${ozz_build_simd_ref})
message("-- - ozz_build_msvc_rt_dll: " ${ozz_build_msvc_rt_dll})
message("-- - ozz_build_postfix: " ${ozz_build_postfix})

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


install(FILES
  ${PROJECT_SOURCE_DIR}/CHANGES.md
  ${PROJECT_SOURCE_DIR}/LICENSE.md
  ${PROJECT_SOURCE_DIR}/README.md
  DESTINATION ./)
