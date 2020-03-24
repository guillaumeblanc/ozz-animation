add_subdirectory(containers)
add_subdirectory(io)
add_subdirectory(maths)
add_subdirectory(memory)

add_executable(test_endianness endianness_tests.cc)
target_link_libraries(test_endianness
  ozz_base
  gtest)
add_test(NAME test_endianness COMMAND test_endianness)
set_target_properties(test_endianness PROPERTIES FOLDER "ozz/tests/base")

add_executable(test_log log_tests.cc)
target_link_libraries(test_log
  ozz_base
  gtest)
add_test(NAME test_log COMMAND test_log)
set_target_properties(test_log PROPERTIES FOLDER "ozz/tests/base")

add_executable(test_platform
  platform_tests.cc
  span_tests.cc)
target_link_libraries(test_platform
  ozz_base
  gtest)
add_test(NAME test_platform COMMAND test_platform)
set_target_properties(test_platform PROPERTIES FOLDER "ozz/tests/base")

# ozz_base fuse tests
set_source_files_properties(${PROJECT_BINARY_DIR}/src_fused/ozz_base.cc PROPERTIES GENERATED 1)
add_executable(test_fuse_base
  log_tests.cc
  platform_tests.cc
  ${PROJECT_BINARY_DIR}/src_fused/ozz_base.cc)
add_dependencies(test_fuse_base BUILD_FUSE_ozz_base)
target_include_directories(test_fuse_base
  PUBLIC "${PROJECT_SOURCE_DIR}/include")
target_link_libraries(test_fuse_base
  gtest)
add_test(NAME test_fuse_base COMMAND test_fuse_base)
set_target_properties(test_fuse_base PROPERTIES FOLDER "ozz/tests/base")
