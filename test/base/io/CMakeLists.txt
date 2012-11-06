add_executable(test_archive
  archive_tests.cc
  archive_tests_objects.cc
  archive_tests_objects.h)
target_link_libraries(test_archive ozz_base gtest)
add_test(NAME test_archive COMMAND test_archive)
set_target_properties(test_archive PROPERTIES FOLDER "ozz/tests/base")

add_executable(test_archive_maths
  archive_tests_maths.cc
  archive_tests_simd_maths.cc
  archive_tests_soa_maths.cc)
target_link_libraries(test_archive_maths ozz_base gtest)
add_test(NAME test_archive_maths COMMAND test_archive_maths)
set_target_properties(test_archive_maths PROPERTIES FOLDER "ozz/tests/base")

add_executable(test_stream
  stream_tests.cc)
target_link_libraries(test_stream ozz_base gtest)
add_test(NAME test_stream COMMAND test_stream)
set_target_properties(test_stream PROPERTIES FOLDER "ozz/tests/base")
