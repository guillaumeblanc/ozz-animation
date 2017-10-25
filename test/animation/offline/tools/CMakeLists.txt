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

# Creates content test files
file(WRITE "${ozz_temp_directory}/bad.content" "bad content")
file(WRITE "${ozz_temp_directory}/good.content1" "good content 1")
file(WRITE "${ozz_temp_directory}/good.content2" "good content 2")

# Creates config test files.
file(WRITE "${ozz_temp_directory}/valid_config.json" "{\"animations\":[{}]}")
file(WRITE "${ozz_temp_directory}/invalid_config.json" "invalid json content")

# Run test2skel failing tests
#----------------------------
add_test(NAME test2skel_bad_argument COMMAND test2skel "--skeleton=${ozz_temp_directory}/skel_should_not_exist.ozz" "--bad")
set_tests_properties(test2skel_bad_argument PROPERTIES WILL_FAIL true)

add_test(NAME test2skel_missing_test COMMAND test2skel "--skeleton=${ozz_temp_directory}/skel_should_not_exist.ozz")
set_tests_properties(test2skel_missing_test PROPERTIES WILL_FAIL true)

add_test(NAME test2skel_missing_test_raw COMMAND test2skel "--raw" "--skeleton=${ozz_temp_directory}/skel_should_not_exist.ozz")
set_tests_properties(test2skel_missing_test_raw PROPERTIES WILL_FAIL true)

add_test(NAME test2skel_missing_ozz COMMAND test2skel "--file=${ozz_media_directory}/good.content1")
set_tests_properties(test2skel_missing_ozz PROPERTIES WILL_FAIL true)

add_test(NAME test2skel_missing_ozz_raw COMMAND test2skel "--raw" "--file=${ozz_media_directory}/good.content1")
set_tests_properties(test2skel_missing_ozz_raw PROPERTIES WILL_FAIL true)

add_test(NAME test2skel_bad_endian COMMAND test2skel "--file=${ozz_temp_directory}/good.content1" "--skeleton=${ozz_temp_directory}/skel_should_not_exist.ozz" "--endian=fat")
set_tests_properties(test2skel_bad_endian PROPERTIES WILL_FAIL true)

add_test(NAME test2skel_bad_endian_raw COMMAND test2skel "--raw" "--file=${ozz_temp_directory}/good.content1" "--skeleton=${ozz_temp_directory}/skel_should_not_exist.ozz" "--endian=fat")
set_tests_properties(test2skel_bad_endian_raw PROPERTIES WILL_FAIL true)

add_test(NAME test2skel_bad_content COMMAND test2skel "--file=${ozz_temp_directory}/bad.content" "--skeleton=${ozz_temp_directory}/skel_should_not_exist.ozz")
set_tests_properties(test2skel_bad_content PROPERTIES WILL_FAIL true)

add_test(NAME test2skel_bad_content_raw COMMAND test2skel "--raw" "--file=${ozz_temp_directory}/bad.content" "--skeleton=${ozz_temp_directory}/skel_should_not_exist.ozz")
set_tests_properties(test2skel_bad_content_raw PROPERTIES WILL_FAIL true)

add_test(NAME test2skel_unexisting_test COMMAND test2skel "--file=${ozz_media_directory}/skel_should_not_exist.test" "--skeleton=${ozz_temp_directory}/skel_should_not_exist.ozz")
set_tests_properties(test2skel_unexisting_test PROPERTIES WILL_FAIL true)

add_test(NAME test2skel_unexisting_test_raw COMMAND test2skel "--raw" "--file=${ozz_media_directory}/skel_should_not_exist.test" "--skeleton=${ozz_temp_directory}/skel_should_not_exist.ozz")
set_tests_properties(test2skel_unexisting_test_raw PROPERTIES WILL_FAIL true)

add_test(NAME test2skel_invalid_output_path COMMAND test2skel "--file=${ozz_temp_directory}/good.content1" "--skeleton=${ozz_temp_directory}/invalid_path/skel_should_not_exist.ozz")
set_tests_properties(test2skel_invalid_output_path PROPERTIES WILL_FAIL true)

add_test(NAME test2skel_invalid_output_path_raw COMMAND test2skel "--raw" "--file=${ozz_temp_directory}/good.content1" "--skeleton=${ozz_temp_directory}/invalid_path/skel_should_not_exist.ozz")
set_tests_properties(test2skel_invalid_output_path_raw PROPERTIES WILL_FAIL true)

add_test(NAME test2skel_bad_log_level COMMAND test2skel "--file=${ozz_temp_directory}/good.content1" "--skeleton=${ozz_temp_directory}/skel_should_not_exist.ozz" "--log_level=no_log")
set_tests_properties(test2skel_bad_log_level PROPERTIES WILL_FAIL true)

add_test(NAME test2skel_bad_log_level_raw COMMAND test2skel "--raw" "--file=${ozz_temp_directory}/good.content1" "--skeleton=${ozz_temp_directory}/skel_should_not_exist.ozz" "--log_level=no_log")
set_tests_properties(test2skel_bad_log_level_raw PROPERTIES WILL_FAIL true)

# Ensures nothing was outputted.
add_test(NAME test2skel_output COMMAND ${CMAKE_COMMAND} -E copy "${ozz_temp_directory}/skel_should_not_exist.ozz" "${ozz_temp_directory}/skel_should_not_exist_too.ozz")
set_tests_properties(test2skel_output PROPERTIES WILL_FAIL true)
set_tests_properties(test2skel_output PROPERTIES
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
add_test(NAME test2anim_exclusive_config COMMAND test2anim "--file=${ozz_temp_directory}/good.content1" "--skeleton=${ozz_temp_directory}/skeleton.ozz" "--config={}" "--config_file=${ozz_temp_directory}/valid_config.json")
set_tests_properties(test2anim_exclusive_config PROPERTIES PASS_REGULAR_EXPRESSION "--config and --config_file are exclusive options." DEPENDS test2skel_simple)

add_test(NAME test2anim_unexisting_config_file COMMAND test2anim "--file=${ozz_temp_directory}/good.content1" "--skeleton=${ozz_temp_directory}/skeleton.ozz" "--config_file=${ozz_temp_directory}/unexisting_config.json")
set_tests_properties(test2anim_unexisting_config_file PROPERTIES PASS_REGULAR_EXPRESSION "Failed to open config file: \"${ozz_temp_directory}/unexisting_config.json\"." DEPENDS test2skel_simple)

add_test(NAME test2anim_invalid_config_file COMMAND test2anim "--file=${ozz_temp_directory}/good.content1" "--skeleton=${ozz_temp_directory}/skeleton.ozz" "--config_file=${ozz_temp_directory}/invalid_config.json")
set_tests_properties(test2anim_invalid_config_file PROPERTIES PASS_REGULAR_EXPRESSION "Error while parsing configuration string" DEPENDS test2skel_simple)

add_test(NAME test2anim_dump_config_bad_file COMMAND test2anim "--file=${ozz_temp_directory}/good.content1" "--skeleton=${ozz_temp_directory}/skeleton.ozz" "--dump_full_config=${ozz_temp_directory}/unexisting/config.json")
set_tests_properties(test2anim_dump_config_bad_file PROPERTIES PASS_REGULAR_EXPRESSION "Failed to open config file to dump" DEPENDS test2skel_simple)

add_test(NAME test2anim_missing_test COMMAND test2anim "--skeleton=${ozz_temp_directory}/skeleton.ozz" "--config={\"animations\":[{\"output\":\"${ozz_temp_directory}/anim_should_not_exist.ozz\"}]}")
set_tests_properties(test2anim_missing_test PROPERTIES PASS_REGULAR_EXPRESSION "Required option \"file\" is not specified." DEPENDS test2skel_simple)

add_test(NAME test2anim_missing_test_raw COMMAND test2anim "--skeleton=${ozz_temp_directory}/raw_skeleton.ozz" "--config={\"animations\":[{\"output\":\"${ozz_temp_directory}/anim_should_not_exist.ozz\",\"raw\":false}]}")
set_tests_properties(test2anim_missing_test_raw PROPERTIES PASS_REGULAR_EXPRESSION "Required option \"file\" is not specified."  DEPENDS test2skel_simple_raw)

add_test(NAME test2anim_missing_skeleton COMMAND test2anim "--file=${ozz_temp_directory}/good.content1" "--config={\"animations\":[{\"output\":\"${ozz_temp_directory}/anim_should_not_exist.ozz\"}]}")
set_tests_properties(test2anim_missing_skeleton PROPERTIES PASS_REGULAR_EXPRESSION "Required option \"skeleton\" is not specified." )

add_test(NAME test2anim_missing_skeleton_raw COMMAND test2anim "--file=${ozz_temp_directory}/good.content1" "--config={\"animations\":[{\"output\":\"${ozz_temp_directory}/anim_should_not_exist.ozz\",\"raw\":false}]}")
set_tests_properties(test2anim_missing_skeleton_raw PROPERTIES PASS_REGULAR_EXPRESSION "Required option \"skeleton\" is not specified." )

add_test(NAME test2anim_bad_test COMMAND test2anim "--file=${ozz_temp_directory}/bad.content" "--skeleton=${ozz_temp_directory}/skeleton.ozz" "--config={\"animations\":[{\"output\":\"${ozz_temp_directory}/anim_should_not_exist.ozz\"}]}")
set_tests_properties(test2anim_bad_test PROPERTIES PASS_REGULAR_EXPRESSION "No animation found." DEPENDS test2skel_simple)

add_test(NAME test2anim_bad_test_raw COMMAND test2anim "--file=${ozz_temp_directory}/bad.content" "--skeleton=${ozz_temp_directory}/raw_skeleton.ozz" "--config={\"animations\":[{\"output\":\"${ozz_temp_directory}/anim_should_not_exist.ozz\",\"raw\":false}]}")
set_tests_properties(test2anim_bad_test_raw PROPERTIES PASS_REGULAR_EXPRESSION "No animation found." DEPENDS test2skel_simple_raw)

add_test(NAME test2anim_bad_skeleton COMMAND test2anim "--file=${ozz_temp_directory}/good.content1" "--skeleton=${ozz_temp_directory}/bad.content" "--config={\"animations\":[{\"output\":\"${ozz_temp_directory}/anim_should_not_exist.ozz\"}]}")
set_tests_properties(test2anim_bad_skeleton PROPERTIES PASS_REGULAR_EXPRESSION "Failed to read input skeleton from binary file")

add_test(NAME test2anim_bad_skeleton_raw COMMAND test2anim "--file=${ozz_temp_directory}/good.content1" "--skeleton=${ozz_temp_directory}/bad.content" "--config={\"animations\":[{\"output\":\"${ozz_temp_directory}/anim_should_not_exist.ozz\",\"raw\":false}]}")
set_tests_properties(test2anim_bad_skeleton_raw PROPERTIES PASS_REGULAR_EXPRESSION "Failed to read input skeleton from binary file")

add_test(NAME test2anim_unexisting_test COMMAND test2anim "--file=${ozz_temp_directory}/unexiting.test" "--skeleton=${ozz_temp_directory}/skeleton.ozz" "--config={\"animations\":[{\"output\":\"${ozz_temp_directory}/anim_should_not_exist.ozz\"}]}")
set_tests_properties(test2anim_unexisting_test PROPERTIES PASS_REGULAR_EXPRESSION "File \"${ozz_temp_directory}/unexiting.test\" doesn't exist" DEPENDS test2skel_simple)

add_test(NAME test2anim_unexisting_test_raw COMMAND test2anim "--file=${ozz_temp_directory}/unexiting.test" "--skeleton=${ozz_temp_directory}/raw_skeleton.ozz" "--config={\"animations\":[{\"output\":\"${ozz_temp_directory}/anim_should_not_exist.ozz\",\"raw\":false}]}")
set_tests_properties(test2anim_unexisting_test_raw PROPERTIES PASS_REGULAR_EXPRESSION "File \"${ozz_temp_directory}/unexiting.test\" doesn't exist" DEPENDS test2skel_simple_raw)

add_test(NAME test2anim_unexisting_skeleton COMMAND test2anim "--file=${ozz_temp_directory}/good.content1" "--skeleton=${ozz_temp_directory}/unexiting.ozz" "--config={\"animations\":[{\"output\":\"${ozz_temp_directory}/anim_should_not_exist.ozz\"}]}")
set_tests_properties(test2anim_unexisting_skeleton PROPERTIES PASS_REGULAR_EXPRESSION "Failed to open input skeleton ozz binary file: \"${ozz_temp_directory}/unexiting.ozz\"")

add_test(NAME test2anim_unexisting_skeleton_raw COMMAND test2anim "--file=${ozz_temp_directory}/good.content1" "--skeleton=${ozz_temp_directory}/unexiting.ozz" "--config={\"animations\":[{\"output\":\"${ozz_temp_directory}/anim_should_not_exist.ozz\",\"raw\":false}]}")
set_tests_properties(test2anim_unexisting_skeleton_raw PROPERTIES PASS_REGULAR_EXPRESSION "Failed to open input skeleton ozz binary file: \"${ozz_temp_directory}/unexiting.ozz\"")

add_test(NAME test2anim_invalid_output_path COMMAND test2anim "--file=${ozz_temp_directory}/good.content1" "--skeleton=${ozz_temp_directory}/skeleton.ozz" "--config={\"animations\":[{\"output\":\"${ozz_temp_directory}/invalid_path/anim_should_not_exist.ozz\"}]}")
set_tests_properties(test2anim_invalid_output_path PROPERTIES PASS_REGULAR_EXPRESSION "Failed to open output file: \"${ozz_temp_directory}/invalid_path/anim_should_not_exist.ozz\"" DEPENDS test2skel_simple)

add_test(NAME test2anim_invalid_output_path_raw COMMAND test2anim "--file=${ozz_temp_directory}/good.content1" "--skeleton=${ozz_temp_directory}/raw_skeleton.ozz" "--config={\"animations\":[{\"output\":\"${ozz_temp_directory}/invalid_path/anim_should_not_exist.ozz\",\"raw\":false}]}")
set_tests_properties(test2anim_invalid_output_path_raw PROPERTIES PASS_REGULAR_EXPRESSION "Failed to open output file: \"${ozz_temp_directory}/invalid_path/anim_should_not_exist.ozz\"" DEPENDS test2skel_simple_raw)

add_test(NAME test2anim_no_match COMMAND test2anim "--file=${ozz_temp_directory}/good.content1" "--skeleton=${ozz_temp_directory}/skeleton.ozz" "--config={\"animations\":[{\"name\":\"won_t_match\",\"output\":\"${ozz_temp_directory}/invalid_path/anim_should_not_exist.ozz\"}]}")
set_tests_properties(test2anim_no_match PROPERTIES PASS_REGULAR_EXPRESSION "No matching animation found for \"won_t_match\"." DEPENDS test2skel_simple)

add_test(NAME test2anim_bad_type COMMAND test2anim "--file=${ozz_temp_directory}/good.content1" "--skeleton=${ozz_temp_directory}/raw_skeleton.ozz" "--config={\"animations\":[{\"output\":\"${ozz_temp_directory}/anim_should_not_exist.ozz\",\"sampling_rate\":\"1\"}]}")
set_tests_properties(test2anim_bad_type PROPERTIES PASS_REGULAR_EXPRESSION "Invalid type \"UTF-8 string\" for json member \"root.animations\\\[0\\\].sampling_rate\". \"float\" expected." DEPENDS test2skel_simple)

add_test(NAME test2anim_bad_type_in_array COMMAND test2anim "--file=${ozz_temp_directory}/good.content1" "--skeleton=${ozz_temp_directory}/raw_skeleton.ozz" "--config={\"animations\":[{},{\"sampling_rate\":true}]}")
set_tests_properties(test2anim_bad_type_in_array PROPERTIES PASS_REGULAR_EXPRESSION "Invalid type \"boolean\" for json member \"root.animations\\\[1\\\].sampling_rate\". \"float\" expected." DEPENDS test2skel_simple)

add_test(NAME test2anim_bad_log_level COMMAND test2anim "--file=${ozz_temp_directory}/good.content1" "--skeleton=${ozz_temp_directory}/skeleton.ozz" "--config={\"animations\":[{\"output\":\"${ozz_temp_directory}/anim_should_not_exist.ozz\"}]}" "--log_level=no_log")
set_tests_properties(test2anim_bad_log_level PROPERTIES PASS_REGULAR_EXPRESSION "Invalid log level option." DEPENDS test2skel_simple)

add_test(NAME test2anim_bad_log_level_raw COMMAND test2anim "--file=${ozz_temp_directory}/good.content1" "--skeleton=${ozz_temp_directory}/raw_skeleton.ozz" "--config={\"animations\":[{\"output\":\"${ozz_temp_directory}/anim_should_not_exist.ozz\",\"raw\":false}]}" "--log_level=no_log")
set_tests_properties(test2anim_bad_log_level_raw PROPERTIES PASS_REGULAR_EXPRESSION "Invalid log level option." DEPENDS test2skel_simple_raw)

add_test(NAME test2anim_wrong_optimize COMMAND test2anim "--file=${ozz_temp_directory}/good.content1" "--skeleton=${ozz_temp_directory}/skeleton.ozz" "--config={\"animations\":[{\"output\":\"${ozz_temp_directory}/anim_should_not_exist.ozz\",\"optimize\":99}]}")
set_tests_properties(test2anim_wrong_optimize PROPERTIES PASS_REGULAR_EXPRESSION "Invalid type \"integer\" for json member \"root.animations\\\[0\\\].optimize\". \"boolean\" expected." DEPENDS test2skel_simple)

add_test(NAME test2anim_wrong_config_unexpected_member1 COMMAND test2anim "--file=${ozz_temp_directory}/good.content1" "--skeleton=${ozz_temp_directory}/skeleton.ozz" "--config={\"output\":\"${ozz_temp_directory}/anim_should_not_exist.ozz\"}")
set_tests_properties(test2anim_wrong_config_unexpected_member1 PROPERTIES PASS_REGULAR_EXPRESSION "Invalid member \"output\"." DEPENDS test2skel_simple)

add_test(NAME test2anim_wrong_config_unexpected_member2 COMMAND test2anim "--file=${ozz_temp_directory}/good.content1" "--skeleton=${ozz_temp_directory}/skeleton.ozz" "--config={\"animations\":[{\"foo\":true}]}")
set_tests_properties(test2anim_wrong_config_unexpected_member2 PROPERTIES PASS_REGULAR_EXPRESSION "Invalid member \"foo\"." DEPENDS test2skel_simple)

add_test(NAME test2anim_wrong_config_unexpected_member3 COMMAND test2anim "--file=${ozz_temp_directory}/good.content1" "--skeleton=${ozz_temp_directory}/skeleton.ozz" "--config={\"animations\":[{\"foo\":true}]}")
set_tests_properties(test2anim_wrong_config_unexpected_member3 PROPERTIES PASS_REGULAR_EXPRESSION "Invalid member \"foo\"." DEPENDS test2skel_simple)

add_test(NAME test2anim_wrong_config_invalid_object COMMAND test2anim "--file=${ozz_temp_directory}/good.content1" "--skeleton=${ozz_temp_directory}/skeleton.ozz" "--config={\"animations\":{\"output\":\"${ozz_temp_directory}/anim_should_not_exist.ozz\"}}")
set_tests_properties(test2anim_wrong_config_invalid_object PROPERTIES PASS_REGULAR_EXPRESSION "Invalid type \"object\" for json member \"root.animations\". \"array\" expected." DEPENDS test2skel_simple)

add_test(NAME test2anim_wrong_config_invalid_array COMMAND test2anim "--file=${ozz_temp_directory}/good.content1" "--skeleton=${ozz_temp_directory}/skeleton.ozz" "--config={\"animations\":[{\"optimize\":[]}]}")
set_tests_properties(test2anim_wrong_config_invalid_array PROPERTIES PASS_REGULAR_EXPRESSION "Invalid type \"array\" for json member \"root.animations\\\[0\\\].optimize\". \"boolean\" expected." DEPENDS test2skel_simple)

add_test(NAME test2anim_track_bad_joint_name COMMAND test2anim "--file=${ozz_temp_directory}/good.content1" "--skeleton=${ozz_temp_directory}/skeleton.ozz" "--config={\"animations\":[{\"tracks\":[{\"imports\":[{\"joint_name\":\"joint3\",\"property_name\":\"property2\",\"output\":\"${ozz_temp_directory}/anim_should_not_exist.ozz\"}]}]}]}")
set_tests_properties(test2anim_track_bad_joint_name PROPERTIES PASS_REGULAR_EXPRESSION "No joint found for track import definition \"joint3\"" DEPENDS test2skel_simple)

add_test(NAME test2anim_track_bad_ppt_name COMMAND test2anim "--file=${ozz_temp_directory}/good.content1" "--skeleton=${ozz_temp_directory}/skeleton.ozz" "--config={\"animations\":[{\"tracks\":[{\"imports\":[{\"joint_name\":\"joint2\",\"property_name\":\"property2\",\"output\":\"${ozz_temp_directory}/anim_should_not_exist.ozz\"}]}]}]}")
set_tests_properties(test2anim_track_bad_ppt_name PROPERTIES PASS_REGULAR_EXPRESSION "No property found for track import definition \"joint2:property2\"" DEPENDS test2skel_simple)

# Ensures nothing was outputted.
add_test(NAME test2anim_output COMMAND ${CMAKE_COMMAND} -E copy "${ozz_temp_directory}/anim_should_not_exist.ozz" "${ozz_temp_directory}/anim_should_not_exist_too.ozz")
set_tests_properties(test2anim_output PROPERTIES WILL_FAIL true)
set_tests_properties(test2anim_output PROPERTIES
  DEPENDS "test2anim_output
           test2anim_missing_test
           test2anim_missing_test_raw
           test2anim_missing_skeleton
           test2anim_missing_skeleton_raw
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
           test2anim_bad_log_level
           test2anim_bad_log_level_raw")
           
# Run test2anim passing tests
#----------------------------
add_test(NAME test2anim_no_config COMMAND test2anim "--file=${ozz_temp_directory}/good.content1" "--skeleton=${ozz_temp_directory}/skeleton.ozz")

add_test(NAME test2anim_no_config_output COMMAND test2anim "--file=${ozz_temp_directory}/good.content1" "--skeleton=${ozz_temp_directory}/skeleton.ozz")
set_tests_properties(test2anim_no_config_output PROPERTIES PASS_REGULAR_EXPRESSION "Configuration contains no animation export definition." DEPENDS test2skel_simple)

add_test(NAME test2anim_simple COMMAND test2anim "--file=${ozz_temp_directory}/good.content1" "--skeleton=${ozz_temp_directory}/skeleton.ozz" "--config={\"animations\":[{\"output\":\"${ozz_temp_directory}/animation_simple.ozz\"}]}")
set_tests_properties(test2anim_simple PROPERTIES DEPENDS test2skel_simple)
add_test(NAME test2anim_simple_output COMMAND ${CMAKE_COMMAND} -E copy "${ozz_temp_directory}/animation_simple.ozz" "${ozz_temp_directory}/animation_simple_should_exist.ozz")
set_tests_properties(test2anim_simple_output PROPERTIES DEPENDS test2anim_simple)

file(MAKE_DIRECTORY ${ozz_temp_directory}/relative)
add_test(NAME test2anim_simple_relative COMMAND test2anim "--file=${ozz_temp_directory}/good.content1" "--skeleton=${ozz_temp_directory}/skeleton.ozz" "--config={\"animations\":[{\"output\":\"animation_simple_relative.ozz\"}]}" WORKING_DIRECTORY "${ozz_temp_directory}/relative")
set_tests_properties(test2anim_simple_relative PROPERTIES DEPENDS test2skel_simple)
add_test(NAME test2anim_simple_relative_output COMMAND ${CMAKE_COMMAND} -E copy "${ozz_temp_directory}/relative/animation_simple_relative.ozz" "${ozz_temp_directory}/relative/animation_simple_relative_should_exist.ozz")
set_tests_properties(test2anim_simple_relative_output PROPERTIES DEPENDS test2anim_simple_relative)

add_test(NAME test2anim_simple_config_file COMMAND test2anim "--file=${ozz_temp_directory}/good.content1" "--skeleton=${ozz_temp_directory}/skeleton.ozz" "--config_file=${ozz_temp_directory}/valid_config.json")
set_tests_properties(test2anim_simple_config_file PROPERTIES DEPENDS test2skel_simple)

add_test(NAME test2anim_dump_config COMMAND test2anim "--file=${ozz_temp_directory}/good.content1" "--skeleton=${ozz_temp_directory}/skeleton.ozz" "--dump_full_config=${ozz_temp_directory}/default_config.json")
set_tests_properties(test2anim_dump_config PROPERTIES DEPENDS test2skel_simple)

add_test(NAME test2anim_log_config_verbose COMMAND test2anim "--file=${ozz_temp_directory}/good.content1" "--skeleton=${ozz_temp_directory}/skeleton.ozz" --log_level=verbose)
set_tests_properties(test2anim_log_config_verbose PROPERTIES PASS_REGULAR_EXPRESSION "//  Animations to extract." DEPENDS test2skel_simple)

add_test(NAME test2anim_default_config COMMAND test2anim "--file=${ozz_temp_directory}/good.content1" "--skeleton=${ozz_temp_directory}/skeleton.ozz" "--config_file=${ozz_temp_directory}/default_config.json" WORKING_DIRECTORY "${ozz_temp_directory}")
set_tests_properties(test2anim_default_config PROPERTIES DEPENDS test2anim_dump_config)
add_test(NAME test2anim_default_config_output COMMAND ${CMAKE_COMMAND} -E copy "${ozz_temp_directory}/one.ozz" "${ozz_temp_directory}/one_should_exist.ozz")
set_tests_properties(test2anim_default_config_output PROPERTIES DEPENDS test2anim_default_config)

add_test(NAME test2anim_optimize COMMAND test2anim "--file=${ozz_temp_directory}/good.content1" "--skeleton=${ozz_temp_directory}/skeleton.ozz" "--config={\"animations\":[{\"output\":\"${ozz_temp_directory}/animation_${CMAKE_CURRENT_LIST_LINE}.ozz\",\"optimize\":true}]}")
set_tests_properties(test2anim_optimize PROPERTIES DEPENDS test2skel_simple)

add_test(NAME test2anim_optimize_tol COMMAND test2anim "--file=${ozz_temp_directory}/good.content1" "--skeleton=${ozz_temp_directory}/skeleton.ozz" "--config={\"animations\":[{\"output\":\"${ozz_temp_directory}/animation_${CMAKE_CURRENT_LIST_LINE}.ozz\",\"optimize\":true,\"optimization_tolerances\":{\"translation\":0.002,\"rotation\":0.002,\"scale\":0.002,\"hierarchical\":0.002}}]}")
set_tests_properties(test2anim_optimize_tol PROPERTIES DEPENDS test2skel_simple)

add_test(NAME test2anim_nooptimize COMMAND test2anim "--file=${ozz_temp_directory}/good.content1" "--skeleton=${ozz_temp_directory}/skeleton.ozz" "--config={\"animations\":[{\"output\":\"${ozz_temp_directory}/animation_${CMAKE_CURRENT_LIST_LINE}.ozz\"}]},\"optimize\":false}]}")
set_tests_properties(test2anim_nooptimize PROPERTIES DEPENDS test2skel_simple)

add_test(NAME test2anim_simple_raw COMMAND test2anim "--file=${ozz_temp_directory}/good.content1" "--skeleton=${ozz_temp_directory}/skeleton.ozz" "--config={\"animations\":[{\"output\":\"${ozz_temp_directory}/raw_animation_${CMAKE_CURRENT_LIST_LINE}.ozz\",\"raw\":false}]}")
set_tests_properties(test2anim_simple_raw PROPERTIES DEPENDS test2skel_simple)

add_test(NAME test2anim_simple_from_raw COMMAND test2anim "--file=${ozz_temp_directory}/good.content1" "--skeleton=${ozz_temp_directory}/raw_skeleton.ozz" "--config={\"animations\":[{\"output\":\"${ozz_temp_directory}/animation_${CMAKE_CURRENT_LIST_LINE}.ozz\"}]}")
set_tests_properties(test2anim_simple_from_raw PROPERTIES DEPENDS test2skel_simple_raw)

add_test(NAME test2anim_simple_raw_from_raw COMMAND test2anim "--file=${ozz_temp_directory}/good.content1" "--skeleton=${ozz_temp_directory}/raw_skeleton.ozz" "--config={\"animations\":[{\"output\":\"${ozz_temp_directory}/animation_${CMAKE_CURRENT_LIST_LINE}.ozz\",\"raw\":false}]}")
set_tests_properties(test2anim_simple_raw_from_raw PROPERTIES DEPENDS test2skel_simple_raw)

add_test(NAME test2anim_additive COMMAND test2anim "--file=${ozz_temp_directory}/good.content1" "--skeleton=${ozz_temp_directory}/skeleton.ozz" 
"--config={\"animations\":[{\"output\":\"${ozz_temp_directory}/animation_${CMAKE_CURRENT_LIST_LINE}.ozz\",\"additive\":true}]}")
set_tests_properties(test2anim_additive PROPERTIES DEPENDS test2skel_simple)

add_test(NAME test2anim_additive_raw COMMAND test2anim "--file=${ozz_temp_directory}/good.content1" "--skeleton=${ozz_temp_directory}/skeleton.ozz" 
"--config={\"animations\":[{\"output\":\"${ozz_temp_directory}/animation_${CMAKE_CURRENT_LIST_LINE}.ozz\",\"raw\":false,\"additive\":true}]}")
set_tests_properties(test2anim_additive_raw PROPERTIES DEPENDS test2skel_simple)

add_test(NAME test2anim_native COMMAND test2anim "--file=${ozz_temp_directory}/good.content1" "--skeleton=${ozz_temp_directory}/skeleton.ozz" 
"--config={\"animations\":[{\"output\":\"${ozz_temp_directory}/animation_native_${CMAKE_CURRENT_LIST_LINE}.ozz\"}]}" "--endian=native")
set_tests_properties(test2anim_native PROPERTIES DEPENDS test2skel_simple)

add_test(NAME test2anim_little COMMAND test2anim "--file=${ozz_temp_directory}/good.content1" "--skeleton=${ozz_temp_directory}/skeleton.ozz" 
"--config={\"animations\":[{\"output\":\"${ozz_temp_directory}/animation_little_${CMAKE_CURRENT_LIST_LINE}.ozz\"}]}" "--endian=little")
set_tests_properties(test2anim_little PROPERTIES DEPENDS test2skel_simple)

add_test(NAME test2anim_big COMMAND test2anim "--file=${ozz_temp_directory}/good.content1" "--skeleton=${ozz_temp_directory}/skeleton.ozz" 
"--config={\"animations\":[{\"output\":\"${ozz_temp_directory}/animation_big_${CMAKE_CURRENT_LIST_LINE}.ozz\"}]}" "--endian=big")
set_tests_properties(test2anim_big PROPERTIES DEPENDS test2skel_simple)

add_test(NAME test2anim_unmatch_skeleton COMMAND test2anim "--file=${ozz_temp_directory}/good.content1" "--skeleton=${ozz_temp_directory}/skeleton.ozz" "--config={\"animations\":[{\"output\":\"${ozz_temp_directory}/animation_big_${CMAKE_CURRENT_LIST_LINE}.ozz\"}]}" "--endian=big")
set_tests_properties(test2anim_unmatch_skeleton PROPERTIES DEPENDS test2skel_simple)

add_test(NAME test2anim_sampling_rate COMMAND test2anim "--file=${ozz_temp_directory}/good.content1" "--skeleton=${ozz_temp_directory}/skeleton.ozz" "--config={\"animations\":[{\"output\":\"${ozz_temp_directory}/animation_${CMAKE_CURRENT_LIST_LINE}.ozz\",\"sampling_rate\":10.}]}")
set_tests_properties(test2anim_sampling_rate PROPERTIES DEPENDS test2skel_simple)

add_test(NAME test2anim_sampling_rate_int COMMAND test2anim "--file=${ozz_temp_directory}/good.content1" "--skeleton=${ozz_temp_directory}/skeleton.ozz" "--config={\"animations\":[{\"output\":\"${ozz_temp_directory}/animation_${CMAKE_CURRENT_LIST_LINE}.ozz\",\"sampling_rate\":1}]}")
set_tests_properties(test2anim_sampling_rate_int PROPERTIES DEPENDS test2skel_simple)

add_test(NAME test2anim_sampling_rate_0 COMMAND test2anim "--file=${ozz_temp_directory}/good.content1" "--skeleton=${ozz_temp_directory}/skeleton.ozz" "--config={\"animations\":[{\"output\":\"${ozz_temp_directory}/animation_${CMAKE_CURRENT_LIST_LINE}.ozz\",\"sampling_rate\":0}]}")
set_tests_properties(test2anim_sampling_rate_0 PROPERTIES DEPENDS test2skel_simple)

add_test(NAME test2anim_sampling_rate_neg COMMAND test2anim "--file=${ozz_temp_directory}/good.content1" "--skeleton=${ozz_temp_directory}/raw_skeleton.ozz" "--config={\"animations\":[{\"output\":\"${ozz_temp_directory}/animation_${CMAKE_CURRENT_LIST_LINE}.ozz\",\"sampling_rate\":-1}]}")
set_tests_properties(test2anim_sampling_rate_neg PROPERTIES DEPENDS test2skel_simple)

add_test(NAME test2anim_log_verbose COMMAND test2anim "--file=${ozz_temp_directory}/good.content1" "--skeleton=${ozz_temp_directory}/skeleton.ozz" "--config={\"animations\":[{\"output\":\"${ozz_temp_directory}/animation_${CMAKE_CURRENT_LIST_LINE}.ozz\"}]}" "--log_level=verbose")
set_tests_properties(test2anim_log_verbose PROPERTIES DEPENDS test2skel_simple)

add_test(NAME test2anim_single1_wild COMMAND test2anim "--file=${ozz_temp_directory}/good.content1" "--skeleton=${ozz_temp_directory}/skeleton.ozz" "--config={\"animations\":[{\"output\":\"${ozz_temp_directory}/animation_single1_wild_*.ozz\"}]}")
set_tests_properties(test2anim_single1_wild PROPERTIES DEPENDS test2skel_simple)

add_test(NAME test2anim_single1_wild_ouput1 COMMAND ${CMAKE_COMMAND} -E copy "${ozz_temp_directory}/animation_single1_wild_one.ozz" "${ozz_temp_directory}/animation_single1_wild_one_should_exist.ozz")
set_tests_properties(test2anim_single1_wild_ouput1 PROPERTIES DEPENDS test2anim_single1_wild)

add_test(NAME test2anim_multi1 COMMAND test2anim "--file=${ozz_temp_directory}/good.content2" "--skeleton=${ozz_temp_directory}/skeleton.ozz" "--config={\"animations\":[{\"output\":\"${ozz_temp_directory}/animation_multi2_*.ozz\"}]}")
set_tests_properties(test2anim_multi1 PROPERTIES DEPENDS test2skel_simple)

add_test(NAME test2anim_multi2_ouput1 COMMAND ${CMAKE_COMMAND} -E copy "${ozz_temp_directory}/animation_multi2_one.ozz" "${ozz_temp_directory}/animation_multi1_one_should_exist.ozz")
set_tests_properties(test2anim_multi2_ouput1 PROPERTIES DEPENDS test2anim_multi1)

add_test(NAME test2anim_multi2_ouput2 COMMAND ${CMAKE_COMMAND} -E copy "${ozz_temp_directory}/animation_multi2_TWO.ozz" "${ozz_temp_directory}/animation_multi1_TWO_should_exist.ozz")
set_tests_properties(test2anim_multi2_ouput2 PROPERTIES DEPENDS test2anim_multi1)

add_test(NAME test2anim_multi2_single COMMAND test2anim "--file=${ozz_temp_directory}/good.content2" "--skeleton=${ozz_temp_directory}/skeleton.ozz" 
"--config={\"animations\":[{\"name\":\"T?O\",\"output\":\"${ozz_temp_directory}/animation_multi2_single.ozz\"}]}")
set_tests_properties(test2anim_multi2_single PROPERTIES DEPENDS test2skel_simple)

add_test(NAME test2anim_multi2_single_ouput1 COMMAND ${CMAKE_COMMAND} -E copy "${ozz_temp_directory}/animation_multi2_single.ozz" "${ozz_temp_directory}/animation_multi2_single_should_exist.ozz")
set_tests_properties(test2anim_multi2_single_ouput1 PROPERTIES DEPENDS test2anim_multi2_single)

add_test(NAME test2anim_multi2_multi COMMAND test2anim "--file=${ozz_temp_directory}/good.content2" "--skeleton=${ozz_temp_directory}/skeleton.ozz" "--config={\"animations\":[{\"name\":\"one\",\"output\":\"${ozz_temp_directory}/animation_multi2_multi_one.ozz\"},{\"name\":\"TWO\",\"output\":\"${ozz_temp_directory}/animation_multi2_multi_TWO.ozz\"}]}")
set_tests_properties(test2anim_multi2_multi PROPERTIES DEPENDS test2skel_simple)

add_test(NAME test2anim_multi2_multi_ouput1 COMMAND ${CMAKE_COMMAND} -E copy "${ozz_temp_directory}/animation_multi2_multi_one.ozz" "${ozz_temp_directory}/animation_multi2_multi_one_should_exist.ozz")
set_tests_properties(test2anim_multi2_multi_ouput1 PROPERTIES DEPENDS test2anim_multi2_multi)
add_test(NAME test2anim_multi2_multi_ouput2 COMMAND ${CMAKE_COMMAND} -E copy "${ozz_temp_directory}/animation_multi2_multi_TWO.ozz" "${ozz_temp_directory}/animation_multi2_multi_TWO_should_exist.ozz")
set_tests_properties(test2anim_multi2_multi_ouput2 PROPERTIES DEPENDS test2anim_multi2_multi)

add_test(NAME test2anim_track_simple COMMAND test2anim "--file=${ozz_temp_directory}/good.content1" "--skeleton=${ozz_temp_directory}/skeleton.ozz" "--config={\"animations\":[{\"tracks\":[{\"imports\":[{\"joint_name\":\"joint0\",\"property_name\":\"property0\",\"output\":\"${ozz_temp_directory}/track_simple.ozz\"}]}]}]}")
add_test(NAME test2anim_track_simple_output COMMAND ${CMAKE_COMMAND} -E copy "${ozz_temp_directory}/track_simple.ozz" "${ozz_temp_directory}/track_simple_should_exist.ozz")
set_tests_properties(test2anim_track_simple_output PROPERTIES DEPENDS test2anim_track_simple)

add_test(NAME test2anim_track_wild_joint COMMAND test2anim "--file=${ozz_temp_directory}/good.content1" "--skeleton=${ozz_temp_directory}/skeleton.ozz" "--config={\"animations\":[{\"tracks\":[{\"imports\":[{\"joint_name\":\"joint?\",\"property_name\":\"property0\",\"output\":\"${ozz_temp_directory}/track_wild_*.ozz\"}]}]}]}")
add_test(NAME test2anim_track_wild_joint_output0
  COMMAND ${CMAKE_COMMAND} -E copy "${ozz_temp_directory}/track_wild_joint0-property0.ozz" "${ozz_temp_directory}/track_wild_joint0-property0_should_exist.ozz")
set_tests_properties(test2anim_track_wild_joint_output0 PROPERTIES DEPENDS test2anim_track_wild_joint)
add_test(NAME test2anim_track_wild_joint_output1
  COMMAND ${CMAKE_COMMAND} -E copy "${ozz_temp_directory}/track_wild_joint1-property0.ozz" "${ozz_temp_directory}/track_wild_joint1-property0_should_exist.ozz")
set_tests_properties(test2anim_track_wild_joint_output1 PROPERTIES DEPENDS test2anim_track_wild_joint)

# ozz_animation_offline_skel_tools fuse tests
set_source_files_properties(${PROJECT_BINARY_DIR}/src_fused/ozz_animation_offline_skel_tools.cc PROPERTIES GENERATED 1)
add_executable(test_fuse_animation_offline_skel_tools
  test2skel.cc
  ${PROJECT_BINARY_DIR}/src_fused/ozz_animation_offline_skel_tools.cc)
add_dependencies(test_fuse_animation_offline_skel_tools BUILD_FUSE_ozz_animation_offline_skel_tools)
target_link_libraries(test_fuse_animation_offline_skel_tools
  ozz_animation_offline
  ozz_options
  gtest)
set_target_properties(test_fuse_animation_offline_skel_tools PROPERTIES FOLDER "ozz/tests/animation_offline")

add_test(NAME test_fuse_animation_offline_skel_tools_no_arg COMMAND test_fuse_animation_offline_skel_tools)
set_tests_properties(test_fuse_animation_offline_skel_tools_no_arg PROPERTIES WILL_FAIL true)

# ozz_animation_offline_anim_tools fuse tests
set_source_files_properties(${PROJECT_BINARY_DIR}/src_fused/ozz_animation_offline_anim_tools.cc PROPERTIES GENERATED 1)
add_executable(test_fuse_animation_offline_anim_tools
  test2anim.cc
  ${PROJECT_BINARY_DIR}/src_fused/ozz_animation_offline_anim_tools.cc)
add_dependencies(test_fuse_animation_offline_anim_tools BUILD_FUSE_ozz_animation_offline_anim_tools)
target_link_libraries(test_fuse_animation_offline_anim_tools
  ozz_animation_offline
  ozz_options
  json
  gtest)

set_target_properties(test_fuse_animation_offline_anim_tools PROPERTIES FOLDER "ozz/tests/animation_offline")

add_test(NAME test_fuse_animation_offline_anim_tools_no_arg COMMAND test_fuse_animation_offline_anim_tools)
set_tests_properties(test_fuse_animation_offline_anim_tools_no_arg PROPERTIES WILL_FAIL true)
