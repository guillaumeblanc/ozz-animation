add_executable(test_options options_tests.cc)
target_link_libraries(test_options
  ozz_base
  ozz_options
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
