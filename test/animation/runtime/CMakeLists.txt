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

# blending_job_tests
add_executable(test_blending_job
  blending_job_tests.cc)
target_link_libraries(test_blending_job
  ozz_animation_offline
  ozz_animation
  ozz_base
  gtest)
set_target_properties(test_blending_job PROPERTIES FOLDER "ozz/tests/animation")
add_test(NAME test_blending_job COMMAND test_blending_job)

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

add_executable(test_animation_serialize
  animation_serialize_tests.cc)
target_link_libraries(test_animation_serialize
  ozz_animation_offline
  ozz_animation
  ozz_base
  gtest)
set_target_properties(test_animation_serialize PROPERTIES FOLDER "ozz/tests/animation")
add_test(NAME test_animation_serialize COMMAND test_animation_serialize)

add_executable(test_skeleton_serialize
  skeleton_serialize_tests.cc)
target_link_libraries(test_skeleton_serialize
  ozz_animation_offline
  ozz_animation
  ozz_base
  gtest)
set_target_properties(test_skeleton_serialize PROPERTIES FOLDER "ozz/tests/animation")
add_test(NAME test_skeleton_serialize COMMAND test_skeleton_serialize)

add_executable(test_skeleton_utils
  skeleton_utils_tests.cc)
target_link_libraries(test_skeleton_utils
  ozz_animation_offline
  ozz_animation
  ozz_base
  gtest)
set_target_properties(test_skeleton_utils PROPERTIES FOLDER "ozz/tests/animation")
add_test(NAME test_skeleton_utils COMMAND test_skeleton_utils)

