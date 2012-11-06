add_executable(test_animation_builder
  animation_builder_tests.cc)
target_link_libraries(test_animation_builder
  ozz_animation_offline
  ozz_animation
  ozz_base
  gtest)
set_target_properties(test_animation_builder PROPERTIES FOLDER "ozz/tests/animation_offline")
add_test(NAME test_animation_builder COMMAND test_animation_builder)

add_executable(test_animation_optimizer animation_optimizer_tests.cc)
target_link_libraries(test_animation_optimizer
  ozz_animation_offline
  ozz_animation
  ozz_base
  gtest)
set_target_properties(test_animation_optimizer PROPERTIES FOLDER "ozz/tests/animation_offline")
add_test(NAME test_animation_optimizer COMMAND test_animation_optimizer)

add_executable(test_skeleton_builder
  skeleton_builder_tests.cc)
target_link_libraries(test_skeleton_builder
  ozz_animation_offline
  ozz_animation
  ozz_base
  gtest)
set_target_properties(test_skeleton_builder PROPERTIES FOLDER "ozz/tests/animation_offline")
add_test(NAME test_skeleton_builder COMMAND test_skeleton_builder)

add_executable(test_animation_serialize
  animation_serialize_tests.cc)
target_link_libraries(test_animation_serialize
  ozz_animation_offline
  ozz_animation
  ozz_base
  gtest)
set_target_properties(test_animation_serialize PROPERTIES FOLDER "ozz/tests/animation_offline")
add_test(NAME test_animation_serialize COMMAND test_animation_serialize)

add_executable(test_skeleton_serialize
  skeleton_serialize_tests.cc)
target_link_libraries(test_skeleton_serialize
  ozz_animation_offline
  ozz_animation
  ozz_base
  gtest)
set_target_properties(test_skeleton_serialize PROPERTIES FOLDER "ozz/tests/animation_offline")
add_test(NAME test_skeleton_serialize COMMAND test_skeleton_serialize)

add_subdirectory(collada)