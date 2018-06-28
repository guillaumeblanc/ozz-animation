add_executable(test_options options_tests.cc)
target_link_libraries(test_options
  ozz_options
  ozz_base
  gtest)
add_test(NAME test_options COMMAND test_options)
set_target_properties(test_options PROPERTIES FOLDER "ozz/tests/options")

add_executable(test_options_registration options_registration_tests.cc)
target_link_libraries(test_options_registration
  ozz_options
  gtest)
add_test(NAME test_options_registration COMMAND test_options_registration)
set_target_properties(test_options_registration PROPERTIES FOLDER "ozz/tests/options")

add_executable(test_options_registration_empty options_registration_empty_tests.cc)
target_link_libraries(test_options_registration_empty
  ozz_options
  gtest)
add_test(NAME test_options_registration_empty COMMAND test_options_registration_empty)
set_target_properties(test_options_registration_empty PROPERTIES FOLDER "ozz/tests/options")

# ozz_options fuse tests
set_source_files_properties(${PROJECT_BINARY_DIR}/src_fused/ozz_options.cc PROPERTIES GENERATED 1)
add_executable(test_fuse_options
  options_registration_tests.cc
  ${PROJECT_BINARY_DIR}/src_fused/ozz_options.cc)
add_dependencies(test_fuse_options BUILD_FUSE_ozz_options)
target_include_directories(test_fuse_options
  PUBLIC "${PROJECT_SOURCE_DIR}/include")
target_link_libraries(test_fuse_options
  gtest)
add_dependencies(test_fuse_options BUILD_FUSE_ozz_options)
add_test(NAME test_fuse_options COMMAND test_fuse_options)
set_target_properties(test_fuse_options PROPERTIES FOLDER "ozz/tests/options")

