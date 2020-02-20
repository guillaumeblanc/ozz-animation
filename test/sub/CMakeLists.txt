cmake_minimum_required (VERSION 3.0)

# Defines the project's name
project(ozz_sub)

# ozz requires C++11
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Activates unit tests
enable_testing()

# Includes ozz-animation as a sub directory, using an arbitrary "ozz-animation/" binary output folder.
add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/../.." ozz-animation/)

# Then link with ozz libraries as any other cmake target.
add_executable(test_sub_project
  test_sub_project.cc)
target_link_libraries(test_sub_project
  ozz_animation)

# Adds some tests
add_test(NAME test_sub_project COMMAND test_sub_project)
if(TARGET fbx2ozz)
  add_test(NAME test_sub_fbx2ozz COMMAND fbx2ozz "--version")
endif()