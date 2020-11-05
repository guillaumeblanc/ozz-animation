# group_varint_tests
add_executable(test_group_varint
  group_varint_tests.cc)
target_link_libraries(test_group_varint
  ozz_base
  gtest)
target_copy_shared_libraries(test_group_varint)
set_target_properties(test_group_varint PROPERTIES FOLDER "ozz/tests/base")
add_test(NAME test_group_varint COMMAND test_group_varint)
