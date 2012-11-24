cmake_minimum_required (VERSION 2.8)

# Defines the project's name
project(ozz)

# Add project build options
option(ozz_build_tests "Build unit tests" ON)
option(ozz_build_demos "Build demos" ON)

if(ozz_build_tests)
  enable_testing()
endif()

# Set ozz compiler flags and options
include(build/cmake/compiler_flags.cmake)

# Includes the root of ozz includes
include_directories(include)

# Starts building the sources tree
add_subdirectory(src)

# Locates media directory.
set(ozz_media_directory "${CMAKE_SOURCE_DIR}/media")

# Adds test to build
if(ozz_build_tests)
  # Continues with the tests tree
  add_subdirectory(test)
endif()

# Continues with demos
if(ozz_build_demos)
  add_subdirectory(demo)
endif()

# Prepares installation
install(DIRECTORY "${CMAKE_SOURCE_DIR}/include" DESTINATION ".")

# Prepares packing
set(CPACK_PACKAGE_VERSION_MAJOR 0)
set(CPACK_PACKAGE_VERSION_MINOR 2)
set(CPACK_PACKAGE_VERSION_PATCH 0)
set(CPACK_PACKAGE_VENDOR "Guillaume Blanc")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Ozz run-time animation library and tools")
#set(CPACK_PACKAGE_ICON )
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/LICENSE")
set(CPACK_SOURCE_IGNORE_FILES
  "/build_out-of-source/"  # Ignore files from out-of-source build
  "/.git/"
  "*.kdev4"
  "*.*~")
set(CPACK_NSIS_MENU_LINKS
  "http://code.google.com/p/ozz-animation/;Ozz home"
  "bin/demo;Demo"
  "bin/tools;Tools")

include(CPack)