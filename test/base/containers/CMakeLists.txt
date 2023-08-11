add_executable(test_intrusive_list intrusive_list_tests.cc)
target_include_directories(test_intrusive_list
  PUBLIC "${PROJECT_SOURCE_DIR}/include")
target_link_libraries(test_intrusive_list
  gtest)
target_compile_options(test_intrusive_list
  PRIVATE $<$<BOOL:${W_UNUSED_RESULT}>:-Wno-unused-result>)
target_copy_shared_libraries(test_intrusive_list)
add_test(NAME test_intrusive_list COMMAND test_intrusive_list)
set_target_properties(test_intrusive_list PROPERTIES FOLDER "ozz/tests/base")

add_executable(test_std_containers std_containers_tests.cc)
target_link_libraries(test_std_containers
  ozz_base
  gtest)
target_compile_options(test_std_containers
  PRIVATE $<$<BOOL:${W_UNUSED_RESULT}>:-Wno-unused-result>)
target_copy_shared_libraries(test_std_containers)
add_test(NAME test_std_containers COMMAND test_std_containers)
set_target_properties(test_std_containers PROPERTIES FOLDER "ozz/tests/base")

add_executable(test_std_containers_archive
  std_containers_archive_tests.cc)
target_link_libraries(test_std_containers_archive
  ozz_base
  gtest)
target_copy_shared_libraries(test_std_containers_archive)
add_test(NAME test_std_containers_archive COMMAND test_std_containers_archive)
set_target_properties(test_std_containers_archive PROPERTIES FOLDER "ozz/tests/base")
