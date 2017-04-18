add_executable(test2skel
  test2skel.cc)
target_link_libraries(test2skel
  ozz_animation_offline_skel_tools
  ozz_options)
set_target_properties(test2skel
  PROPERTIES FOLDER "ozz/tests/animation_offline")

add_executable(test2anim
  test2anim.cc)
target_link_libraries(test2anim
  ozz_animation_offline_anim_tools
  ozz_options)

set_target_properties(test2anim
  PROPERTIES FOLDER "ozz/tests/animation_offline")

# Creates a file with an invalid content.
file(WRITE "${ozz_temp_directory}/bad.content" "bad content")
file(WRITE "${ozz_temp_directory}/good.content1" "good content 1")
file(WRITE "${ozz_temp_directory}/good.content2" "good content 2")

# Run test2skel failing tests
#----------------------------
add_test(NAME test2skel_bad_argument COMMAND test2skel "--skeleton=${ozz_temp_directory}/should_not_exist.ozz" "--bad")
set_tests_properties(test2skel_bad_argument PROPERTIES WILL_FAIL true)
add_test(NAME test2skel_missing_test COMMAND test2skel "--skeleton=${ozz_temp_directory}/should_not_exist.ozz")
set_tests_properties(test2skel_missing_test PROPERTIES WILL_FAIL true)
add_test(NAME test2skel_missing_test_raw COMMAND test2skel "--raw" "--skeleton=${ozz_temp_directory}/should_not_exist.ozz")
set_tests_properties(test2skel_missing_test_raw PROPERTIES WILL_FAIL true)
add_test(NAME test2skel_missing_ozz COMMAND test2skel "--file=${ozz_media_directory}/good.content1")
set_tests_properties(test2skel_missing_ozz PROPERTIES WILL_FAIL true)
add_test(NAME test2skel_missing_ozz_raw COMMAND test2skel "--raw" "--file=${ozz_media_directory}/good.content1")
set_tests_properties(test2skel_missing_ozz_raw PROPERTIES WILL_FAIL true)
add_test(NAME test2skel_bad_endian COMMAND test2skel "--file=${ozz_temp_directory}/good.content1" "--skeleton=${ozz_temp_directory}/should_not_exist.ozz" "--endian=fat")
set_tests_properties(test2skel_bad_endian PROPERTIES WILL_FAIL true)
add_test(NAME test2skel_bad_endian_raw COMMAND test2skel "--raw" "--file=${ozz_temp_directory}/good.content1" "--skeleton=${ozz_temp_directory}/should_not_exist.ozz" "--endian=fat")
set_tests_properties(test2skel_bad_endian_raw PROPERTIES WILL_FAIL true)
add_test(NAME test2skel_bad_content COMMAND test2skel "--file=${ozz_temp_directory}/bad.content" "--skeleton=${ozz_temp_directory}/should_not_exist.ozz")
set_tests_properties(test2skel_bad_content PROPERTIES WILL_FAIL true)
add_test(NAME test2skel_bad_content_raw COMMAND test2skel "--raw" "--file=${ozz_temp_directory}/bad.content" "--skeleton=${ozz_temp_directory}/should_not_exist.ozz")
set_tests_properties(test2skel_bad_content_raw PROPERTIES WILL_FAIL true)
add_test(NAME test2skel_unexisting_test COMMAND test2skel "--file=${ozz_media_directory}/should_not_exist.test" "--skeleton=${ozz_temp_directory}/should_not_exist.ozz")
set_tests_properties(test2skel_unexisting_test PROPERTIES WILL_FAIL true)
add_test(NAME test2skel_unexisting_test_raw COMMAND test2skel "--raw" "--file=${ozz_media_directory}/should_not_exist.test" "--skeleton=${ozz_temp_directory}/should_not_exist.ozz")
set_tests_properties(test2skel_unexisting_test_raw PROPERTIES WILL_FAIL true)
add_test(NAME test2skel_invalid_output_path COMMAND test2skel "--file=${ozz_temp_directory}/good.content1" "--skeleton=${ozz_temp_directory}/invalid_path/should_not_exist.ozz")
set_tests_properties(test2skel_invalid_output_path PROPERTIES WILL_FAIL true)
add_test(NAME test2skel_invalid_output_path_raw COMMAND test2skel "--raw" "--file=${ozz_temp_directory}/good.content1" "--skeleton=${ozz_temp_directory}/invalid_path/should_not_exist.ozz")
set_tests_properties(test2skel_invalid_output_path_raw PROPERTIES WILL_FAIL true)
add_test(NAME test2skel_bad_log_level COMMAND test2skel "--file=${ozz_temp_directory}/good.content1" "--skeleton=${ozz_temp_directory}/should_not_exist.ozz" "--log_level=no_log")
set_tests_properties(test2skel_bad_log_level PROPERTIES WILL_FAIL true)
add_test(NAME test2skel_bad_log_level_raw COMMAND test2skel "--raw" "--file=${ozz_temp_directory}/good.content1" "--skeleton=${ozz_temp_directory}/should_not_exist.ozz" "--log_level=no_log")
set_tests_properties(test2skel_bad_log_level_raw PROPERTIES WILL_FAIL true)

# Ensures nothing was outputted.
add_test(NAME test2skel_ouput COMMAND ${CMAKE_COMMAND} -E copy "${ozz_temp_directory}/should_not_exist.ozz" "${ozz_temp_directory}/should_not_exist_too.ozz")
set_tests_properties(test2skel_ouput PROPERTIES WILL_FAIL true)
set_tests_properties(test2skel_ouput PROPERTIES
  DEPENDS "test2skel_bad_argument
           test2skel_missing_test
           test2skel_missing_ozz
           test2skel_missing_ozz_raw
           test2skel_bad_endian_raw
           test2skel_bad_content
           test2skel_bad_content_raw
           test2skel_unexisting_test
           test2skel_unexisting_test_raw
           test2skel_invalid_output_path")

# Run test2skel passing tests
#----------------------------
add_test(NAME test2skel_simple COMMAND test2skel "--file=${ozz_temp_directory}/good.content1" "--skeleton=${ozz_temp_directory}/skeleton.ozz")
add_test(NAME test2skel_simple_raw COMMAND test2skel "--raw" "--file=${ozz_temp_directory}/good.content1" "--skeleton=${ozz_temp_directory}/raw_skeleton.ozz")
add_test(NAME test2skel_native COMMAND test2skel "--file=${ozz_temp_directory}/good.content1" "--skeleton=${ozz_temp_directory}/skeleton_native.ozz" "--endian=native")
add_test(NAME test2skel_native_raw COMMAND test2skel "--raw" "--file=${ozz_temp_directory}/good.content1" "--skeleton=${ozz_temp_directory}/raw_skeleton_native.ozz" "--endian=native")
add_test(NAME test2skel_little COMMAND test2skel "--file=${ozz_temp_directory}/good.content1" "--skeleton=${ozz_temp_directory}/skeleton_little.ozz" "--endian=little")
add_test(NAME test2skel_big COMMAND test2skel "--file=${ozz_temp_directory}/good.content1" "--skeleton=${ozz_temp_directory}/skeleton_big.ozz" "--endian=big")
add_test(NAME test2skel_log_verbose COMMAND test2skel "--file=${ozz_temp_directory}/good.content1" "--skeleton=${ozz_temp_directory}/skeleton_verbose.ozz" "--log_level=verbose")

# Run test2anim failing tests
#----------------------------
add_test(NAME test2anim_missing_test COMMAND test2anim "--skeleton=${ozz_temp_directory}/skeleton.ozz" "--animation=${ozz_temp_directory}/should_not_exist.ozz")
set_tests_properties(test2anim_missing_test PROPERTIES WILL_FAIL true DEPENDS test2skel_simple)
add_test(NAME test2anim_missing_test_raw COMMAND test2anim "--raw" "--skeleton=${ozz_temp_directory}/raw_skeleton.ozz" "--animation=${ozz_temp_directory}/should_not_exist.ozz")
set_tests_properties(test2anim_missing_test_raw PROPERTIES WILL_FAIL true DEPENDS test2skel_simple_raw)
add_test(NAME test2anim_missing_skeleton COMMAND test2anim "--file=${ozz_temp_directory}/good.content1" "--animation=${ozz_temp_directory}/should_not_exist.ozz")
set_tests_properties(test2anim_missing_skeleton PROPERTIES WILL_FAIL true)
add_test(NAME test2anim_missing_skeleton_raw COMMAND test2anim "--raw" "--file=${ozz_temp_directory}/good.content1" "--animation=${ozz_temp_directory}/should_not_exist.ozz")
set_tests_properties(test2anim_missing_skeleton_raw PROPERTIES WILL_FAIL true)
add_test(NAME test2anim_missing_ozz COMMAND test2anim "--file=${ozz_temp_directory}/good.content1" "--skeleton=${ozz_temp_directory}/raw_skeleton.ozz")
set_tests_properties(test2anim_missing_ozz PROPERTIES WILL_FAIL true DEPENDS test2skel_simple)
add_test(NAME test2anim_missing_ozz_raw COMMAND test2anim "--raw" "--file=${ozz_temp_directory}/good.content1" "--skeleton=${ozz_temp_directory}/raw_skeleton.ozz")
set_tests_properties(test2anim_missing_ozz_raw PROPERTIES WILL_FAIL true DEPENDS test2skel_simple_raw)
add_test(NAME test2anim_bad_test COMMAND test2anim "--file=${ozz_temp_directory}/bad.content" "--skeleton=${ozz_temp_directory}/skeleton.ozz" "--animation=${ozz_temp_directory}/should_not_exist.ozz")
set_tests_properties(test2anim_bad_test PROPERTIES WILL_FAIL true DEPENDS test2skel_simple)
add_test(NAME test2anim_bad_test_raw COMMAND test2anim "--raw" "--file=${ozz_temp_directory}/bad.content" "--skeleton=${ozz_temp_directory}/raw_skeleton.ozz" "--animation=${ozz_temp_directory}/should_not_exist.ozz")
set_tests_properties(test2anim_bad_test_raw PROPERTIES WILL_FAIL true DEPENDS test2skel_simple_raw)
add_test(NAME test2anim_bad_skeleton COMMAND test2anim "--file=${ozz_temp_directory}/bad.content" "--skeleton=${ozz_temp_directory}/bad.content" "--animation=${ozz_temp_directory}/should_not_exist.ozz")
set_tests_properties(test2anim_bad_skeleton PROPERTIES WILL_FAIL true)
add_test(NAME test2anim_bad_skeleton_raw COMMAND test2anim "--raw" "--file=${ozz_temp_directory}/bad.content" "--skeleton=${ozz_temp_directory}/bad.content" "--animation=${ozz_temp_directory}/should_not_exist.ozz")
set_tests_properties(test2anim_bad_skeleton_raw PROPERTIES WILL_FAIL true)
add_test(NAME test2anim_unexisting_test COMMAND test2anim "--file=${ozz_temp_directory}/unexiting.test" "--skeleton=${ozz_temp_directory}/skeleton.ozz" "--animation=${ozz_temp_directory}/should_not_exist.ozz")
set_tests_properties(test2anim_unexisting_test PROPERTIES WILL_FAIL true DEPENDS test2skel_simple)
add_test(NAME test2anim_unexisting_test_raw COMMAND test2anim "--raw" "--file=${ozz_temp_directory}/unexiting.test" "--skeleton=${ozz_temp_directory}/raw_skeleton.ozz" "--animation=${ozz_temp_directory}/should_not_exist.ozz")
set_tests_properties(test2anim_unexisting_test_raw PROPERTIES WILL_FAIL true DEPENDS test2skel_simple_raw)
add_test(NAME test2anim_unexisting_skeleton COMMAND test2anim "--file=${ozz_temp_directory}/good.content1" "--skeleton=${ozz_temp_directory}/unexiting.ozz" "--animation=${ozz_temp_directory}/should_not_exist.ozz")
set_tests_properties(test2anim_unexisting_skeleton PROPERTIES WILL_FAIL true)
add_test(NAME test2anim_unexisting_skeleton_raw COMMAND test2anim "--raw" "--file=${ozz_temp_directory}/good.content1" "--skeleton=${ozz_temp_directory}/unexiting.ozz" "--animation=${ozz_temp_directory}/should_not_exist.ozz")
set_tests_properties(test2anim_unexisting_skeleton_raw PROPERTIES WILL_FAIL true)
add_test(NAME test2anim_invalid_output_path COMMAND test2anim "--file=${ozz_temp_directory}/good.content1" "--skeleton=${ozz_temp_directory}/skeleton.ozz" "--animation=${ozz_temp_directory}/invalid_path/should_not_exist.ozz")
set_tests_properties(test2anim_invalid_output_path PROPERTIES WILL_FAIL true DEPENDS test2skel_simple)
add_test(NAME test2anim_invalid_output_path_raw COMMAND test2anim "--raw" "--file=${ozz_temp_directory}/good.content1" "--skeleton=${ozz_temp_directory}/raw_skeleton.ozz" "--animation=${ozz_temp_directory}/invalid_path/should_not_exist.ozz")
set_tests_properties(test2anim_invalid_output_path_raw PROPERTIES WILL_FAIL true DEPENDS test2skel_simple_raw)
add_test(NAME test2anim_bad_sampling_rate COMMAND test2anim "--file=${ozz_temp_directory}/good.content1" "--skeleton=${ozz_temp_directory}/skeleton.ozz" "--animation=${ozz_temp_directory}/should_not_exist.ozz" "--sampling_rate=-1")
set_tests_properties(test2anim_bad_sampling_rate PROPERTIES WILL_FAIL true DEPENDS test2skel_simple)
add_test(NAME test2anim_bad_sampling_rate_raw COMMAND test2anim "--raw" "--file=${ozz_temp_directory}/good.content1" "--skeleton=${ozz_temp_directory}/raw_skeleton.ozz" "--animation=${ozz_temp_directory}/should_not_exist.ozz" "--sampling_rate=-1")
set_tests_properties(test2anim_bad_sampling_rate_raw PROPERTIES WILL_FAIL true DEPENDS test2skel_simple_raw)
add_test(NAME test2anim_bad_log_level COMMAND test2anim "--file=${ozz_temp_directory}/good.content1" "--skeleton=${ozz_temp_directory}/skeleton.ozz" "--animation=${ozz_temp_directory}/should_not_exist.ozz" "--log_level=no_log")
set_tests_properties(test2anim_bad_log_level PROPERTIES WILL_FAIL true DEPENDS test2skel_simple)
add_test(NAME test2anim_bad_log_level_raw COMMAND test2anim "--raw" "--file=${ozz_temp_directory}/good.content1" "--skeleton=${ozz_temp_directory}/raw_skeleton.ozz" "--animation=${ozz_temp_directory}/should_not_exist.ozz" "--log_level=no_log")
set_tests_properties(test2anim_bad_log_level_raw PROPERTIES WILL_FAIL true DEPENDS test2skel_simple_raw)
add_test(NAME test2anim_wrongoptimize COMMAND test2anim "--file=${ozz_temp_directory}/good.content1" "--skeleton=${ozz_temp_directory}/skeleton.ozz" "--animation=${ozz_temp_directory}/should_not_exist.ozz" --optimize=99)
set_tests_properties(test2anim_wrongoptimize PROPERTIES WILL_FAIL true DEPENDS test2skel_simple)
add_test(NAME test2anim_to_much_asterisks COMMAND test2anim "--file=${ozz_temp_directory}/good.content1" "--skeleton=${ozz_temp_directory}/skeleton.ozz" "--animation=${ozz_temp_directory}/*/should_not_exist_*.ozz")
set_tests_properties(test2anim_to_much_asterisks PROPERTIES WILL_FAIL true DEPENDS test2skel_simple)

# Ensures nothing was outputted.
add_test(NAME test2anim_ouput COMMAND ${CMAKE_COMMAND} -E copy "${ozz_temp_directory}/should_not_exist.ozz" "${ozz_temp_directory}/should_not_exist_too.ozz")
set_tests_properties(test2anim_ouput PROPERTIES WILL_FAIL true)
set_tests_properties(test2skel_ouput PROPERTIES
  DEPENDS "test2anim_ouput
           test2anim_missing_test
           test2anim_missing_test_raw
           test2anim_missing_skeleton
           test2anim_missing_skeleton_raw
           test2anim_missing_ozz
           test2anim_missing_ozz_raw
           test2anim_bad_test
           test2anim_bad_test_raw
           test2anim_bad_skeleton
           test2anim_bad_skeleton_raw
           test2anim_unexisting_test
           test2anim_unexisting_test_raw
           test2anim_unexisting_skeleton
           test2anim_unexisting_skeleton_raw
           test2anim_invalid_output_path
           test2anim_invalid_output_path_raw
           test2anim_bad_sampling_rate
           test2anim_bad_sampling_rate_raw
           test2anim_bad_log_level
           test2anim_bad_log_level_raw")
           
# Run test2anim passing tests
#----------------------------
add_test(NAME test2anim_simple COMMAND test2anim "--file=${ozz_temp_directory}/good.content1" "--skeleton=${ozz_temp_directory}/skeleton.ozz" "--animation=${ozz_temp_directory}/animation_${CMAKE_CURRENT_LIST_LINE}.ozz")
set_tests_properties(test2anim_simple PROPERTIES DEPENDS test2skel_simple)
add_test(NAME test2anim_optimize COMMAND test2anim "--file=${ozz_temp_directory}/good.content1" "--skeleton=${ozz_temp_directory}/skeleton.ozz" "--animation=${ozz_temp_directory}/animation_${CMAKE_CURRENT_LIST_LINE}.ozz" --optimize)
set_tests_properties(test2anim_optimize PROPERTIES DEPENDS test2skel_simple)
add_test(NAME test2anim_nooptimize COMMAND test2anim "--file=${ozz_temp_directory}/good.content1" "--skeleton=${ozz_temp_directory}/skeleton.ozz" "--animation=${ozz_temp_directory}/animation_${CMAKE_CURRENT_LIST_LINE}.ozz" --nooptimize)
set_tests_properties(test2anim_nooptimize PROPERTIES DEPENDS test2skel_simple)
add_test(NAME test2anim_simple_raw COMMAND test2anim "--raw" "--file=${ozz_temp_directory}/good.content1" "--skeleton=${ozz_temp_directory}/skeleton.ozz" "--animation=${ozz_temp_directory}/raw_animation_${CMAKE_CURRENT_LIST_LINE}.ozz")
set_tests_properties(test2anim_simple_raw PROPERTIES DEPENDS test2skel_simple)
add_test(NAME test2anim_simple_from_raw COMMAND test2anim "--file=${ozz_temp_directory}/good.content1" "--skeleton=${ozz_temp_directory}/raw_skeleton.ozz" "--animation=${ozz_temp_directory}/animation_${CMAKE_CURRENT_LIST_LINE}.ozz")
set_tests_properties(test2anim_simple_from_raw PROPERTIES DEPENDS test2skel_simple_raw)
add_test(NAME test2anim_simple_raw_from_raw COMMAND test2anim "--raw" "--file=${ozz_temp_directory}/good.content1" "--skeleton=${ozz_temp_directory}/raw_skeleton.ozz" "--animation=${ozz_temp_directory}/raw_animation_${CMAKE_CURRENT_LIST_LINE}.ozz")
set_tests_properties(test2anim_simple_raw_from_raw PROPERTIES DEPENDS test2skel_simple_raw)
add_test(NAME test2anim_additive COMMAND test2anim "--file=${ozz_temp_directory}/good.content1" "--skeleton=${ozz_temp_directory}/skeleton.ozz" "--animation=${ozz_temp_directory}/animation_${CMAKE_CURRENT_LIST_LINE}.ozz" "--additive")
set_tests_properties(test2anim_additive PROPERTIES DEPENDS test2skel_simple)
add_test(NAME test2anim_additive_raw COMMAND test2anim "--file=${ozz_temp_directory}/good.content1" "--skeleton=${ozz_temp_directory}/skeleton.ozz" "--animation=${ozz_temp_directory}/animation_${CMAKE_CURRENT_LIST_LINE}.ozz" "--additive" "--raw")
set_tests_properties(test2anim_additive_raw PROPERTIES DEPENDS test2skel_simple)
add_test(NAME test2anim_native COMMAND test2anim "--file=${ozz_temp_directory}/good.content1" "--skeleton=${ozz_temp_directory}/skeleton.ozz" "--animation=${ozz_temp_directory}/animation_native_${CMAKE_CURRENT_LIST_LINE}.ozz" "--endian=native")
set_tests_properties(test2anim_native PROPERTIES DEPENDS test2skel_simple)
add_test(NAME test2anim_little COMMAND test2anim "--file=${ozz_temp_directory}/good.content1" "--skeleton=${ozz_temp_directory}/skeleton.ozz" "--animation=${ozz_temp_directory}/animation_little_${CMAKE_CURRENT_LIST_LINE}.ozz" "--endian=little")
set_tests_properties(test2anim_little PROPERTIES DEPENDS test2skel_simple)
add_test(NAME test2anim_big COMMAND test2anim "--file=${ozz_temp_directory}/good.content1" "--skeleton=${ozz_temp_directory}/skeleton.ozz" "--animation=${ozz_temp_directory}/animation_big_${CMAKE_CURRENT_LIST_LINE}.ozz" "--endian=big")
set_tests_properties(test2anim_big PROPERTIES DEPENDS test2skel_simple)
add_test(NAME test2anim_unmatch_skeleton COMMAND test2anim "--file=${ozz_temp_directory}/good.content1" "--skeleton=${ozz_temp_directory}/skeleton.ozz" "--animation=${ozz_temp_directory}/animation_big_${CMAKE_CURRENT_LIST_LINE}.ozz" "--endian=big")
set_tests_properties(test2anim_unmatch_skeleton PROPERTIES DEPENDS test2skel_simple)
add_test(NAME test2anim_sampling_rate COMMAND test2anim "--file=${ozz_temp_directory}/good.content1" "--skeleton=${ozz_temp_directory}/skeleton.ozz" "--animation=${ozz_temp_directory}/animation_${CMAKE_CURRENT_LIST_LINE}.ozz" "--sampling_rate=10")
set_tests_properties(test2anim_sampling_rate PROPERTIES DEPENDS test2skel_simple)
add_test(NAME test2anim_log_verbose COMMAND test2anim "--file=${ozz_temp_directory}/good.content1" "--skeleton=${ozz_temp_directory}/skeleton.ozz" "--animation=${ozz_temp_directory}/animation_${CMAKE_CURRENT_LIST_LINE}.ozz" "--log_level=verbose")
set_tests_properties(test2anim_log_verbose PROPERTIES DEPENDS test2skel_simple)
add_test(NAME test2anim_multi1 COMMAND test2anim "--file=${ozz_temp_directory}/good.content1" "--skeleton=${ozz_temp_directory}/skeleton.ozz" "--animation=${ozz_temp_directory}/animation_multi1_*.ozz")
set_tests_properties(test2anim_multi1 PROPERTIES DEPENDS test2skel_simple)
add_test(NAME test2anim_multi1_2 COMMAND test2anim "--file=${ozz_temp_directory}/good.content2" "--skeleton=${ozz_temp_directory}/skeleton.ozz" "--animation=${ozz_temp_directory}/animation_multi1_2.ozz")
set_tests_properties(test2anim_multi1_2 PROPERTIES DEPENDS test2skel_simple)
add_test(NAME test2anim_multi2 COMMAND test2anim "--file=${ozz_temp_directory}/good.content2" "--skeleton=${ozz_temp_directory}/skeleton.ozz" "--animation=${ozz_temp_directory}/animation_multi2_*.ozz")
set_tests_properties(test2anim_multi2 PROPERTIES DEPENDS test2skel_simple)

# Ensures animations were outputted.
add_test(NAME test2anim_mult_ouput1_one COMMAND ${CMAKE_COMMAND} -E copy "${ozz_temp_directory}/animation_multi1_one.ozz" "${ozz_temp_directory}/animation_multi1_one_should_exist.ozz")
set_tests_properties(test2anim_mult_ouput1_one PROPERTIES DEPENDS test2anim_multi1)
add_test(NAME test2anim_mult_ouput1_2 COMMAND ${CMAKE_COMMAND} -E copy "${ozz_temp_directory}/animation_multi1_2.ozz" "${ozz_temp_directory}/animation_multi1_2_should_exist.ozz")
set_tests_properties(test2anim_mult_ouput1_2 PROPERTIES DEPENDS test2anim_multi1_2)
add_test(NAME test2anim_mult_ouput2_one COMMAND ${CMAKE_COMMAND} -E copy "${ozz_temp_directory}/animation_multi2_one.ozz" "${ozz_temp_directory}/animation_multi2_one_should_exist.ozz")
set_tests_properties(test2anim_mult_ouput2_one PROPERTIES DEPENDS test2anim_multi2)
add_test(NAME test2anim_mult_ouput2_two COMMAND ${CMAKE_COMMAND} -E copy "${ozz_temp_directory}/animation_multi2_TWO.ozz" "${ozz_temp_directory}/animation_multi2_TWO_should_exist.ozz")
set_tests_properties(test2anim_mult_ouput2_two PROPERTIES DEPENDS test2anim_multi2)

# ozz_animation_offline_skel_tools fuse tests
add_executable(test_fuse_animation_offline_skel_tools
  test2skel.cc
  ${PROJECT_SOURCE_DIR}/src_fused/ozz_animation_offline_skel_tools.cc)
add_dependencies(test_fuse_animation_offline_skel_tools BUILD_FUSE_ozz_animation_offline_skel_tools)
target_link_libraries(test_fuse_animation_offline_skel_tools
  ozz_animation_offline
  ozz_options
  gtest)
set_target_properties(test_fuse_animation_offline_skel_tools PROPERTIES FOLDER "ozz/tests/animation_offline")

add_test(NAME test_fuse_animation_offline_skel_tools_no_arg COMMAND test_fuse_animation_offline_skel_tools)
set_tests_properties(test_fuse_animation_offline_skel_tools_no_arg PROPERTIES WILL_FAIL true)

# ozz_animation_offline_anim_tools fuse tests
add_executable(test_fuse_animation_offline_anim_tools
  test2anim.cc
  ${PROJECT_SOURCE_DIR}/src_fused/ozz_animation_offline_anim_tools.cc)
add_dependencies(test_fuse_animation_offline_anim_tools BUILD_FUSE_ozz_animation_offline_anim_tools)
target_link_libraries(test_fuse_animation_offline_anim_tools
  ozz_animation_offline
  ozz_options
  gtest)

set_target_properties(test_fuse_animation_offline_anim_tools PROPERTIES FOLDER "ozz/tests/animation_offline")

add_test(NAME test_fuse_animation_offline_anim_tools_no_arg COMMAND test_fuse_animation_offline_anim_tools)
set_tests_properties(test_fuse_animation_offline_anim_tools_no_arg PROPERTIES WILL_FAIL true)
