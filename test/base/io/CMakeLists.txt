add_executable(test_archive
  archive_tests.cc
  archive_tests_objects.cc
  archive_tests_objects.h)
target_link_libraries(test_archive
  ozz_base
  gtest)
target_copy_shared_libraries(test_archive)
add_test(NAME test_archive COMMAND test_archive)
set_target_properties(test_archive PROPERTIES FOLDER "ozz/tests/base")

add_executable(test_stream
  stream_tests.cc)
target_link_libraries(test_stream
  ozz_base
  gtest)
target_copy_shared_libraries(test_stream)
add_test(NAME test_stream COMMAND test_stream)
set_target_properties(test_stream PROPERTIES FOLDER "ozz/tests/base")
