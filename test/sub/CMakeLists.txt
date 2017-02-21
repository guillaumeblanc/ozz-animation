cmake_minimum_required (VERSION 3.0)

# Defines the project's name
project(ozz_sub)

# Setup a variable that points to ozz-animation folder path.
set(path_to_ozz_folder "${CMAKE_CURRENT_LIST_DIR}/../..")

# Includes ozz-animation as a sub directory, using an arbitrary "ozz-animation/" binary output folder.
add_subdirectory(${path_to_ozz_folder} "$CMAKE_BINARY_DIR/ozz-animation/")

# Setup ozz include path.
include_directories(${path_to_ozz_folder}/include)

# Then link with ozz libraries as any other cmake target.
add_executable(test_sub_project
  test_sub_project.cc)
target_link_libraries(test_sub_project
  ozz_animation
  ozz_base)
