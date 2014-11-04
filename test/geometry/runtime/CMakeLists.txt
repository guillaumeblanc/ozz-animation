# skinning_job_tests
add_executable(test_skinning_job
  skinning_job_tests.cc)
target_link_libraries(test_skinning_job
  ozz_geometry
  ozz_base
  gtest)
set_target_properties(test_skinning_job PROPERTIES FOLDER "ozz/tests/geometry")
add_test(NAME test_skinning_job COMMAND test_skinning_job)
