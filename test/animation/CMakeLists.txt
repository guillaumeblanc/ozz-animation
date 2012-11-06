# sampling_job_tests
add_executable(test_sampling_job
  sampling_job_tests.cc)
target_link_libraries(test_sampling_job
  ozz_animation_offline
  ozz_animation
  ozz_base
  gtest)
set_target_properties(test_sampling_job PROPERTIES FOLDER "ozz/tests/animation")
add_test(NAME test_sampling_job COMMAND test_sampling_job)

# local_to_model_job_tests
add_executable(test_local_to_model_job
  local_to_model_job_tests.cc)
target_link_libraries(test_local_to_model_job
  ozz_animation_offline
  ozz_animation
  ozz_base
  gtest)
set_target_properties(test_local_to_model_job PROPERTIES FOLDER "ozz/tests/animation")
add_test(NAME test_local_to_model_job COMMAND test_local_to_model_job)

add_subdirectory(offline)
