if(NOT ozz_build_tools)
  return()
endif()

add_executable(test2ozz
  test2ozz.cc)
target_link_libraries(test2ozz
  ozz_animation_tools
  ozz_options)

set_target_properties(test2ozz
  PROPERTIES FOLDER "ozz/tests/animation_offline")

# Creates content test files
file(WRITE "${ozz_temp_directory}/bad.content" "bad content")
file(WRITE "${ozz_temp_directory}/good.content0" "good content 0")
file(WRITE "${ozz_temp_directory}/good.content1" "good content 1")
file(WRITE "${ozz_temp_directory}/good.content2" "good content 2")
file(WRITE "${ozz_temp_directory}/good.content_renamed" "good content renamed")
file(WRITE "${ozz_temp_directory}/non_unique_names.content" "good content but not unique joint names")

# Creates config test files.
file(WRITE "${ozz_temp_directory}/valid_config.json" "{\"skeleton\":{\"filename\":\"${ozz_temp_directory}/skeleton.ozz\",\"import\":{\"enable\":false}},\"animations\":[{\"filename\":\"${ozz_temp_directory}/animation_valid_config.ozz\"}]}")
file(WRITE "${ozz_temp_directory}/no_output_config.json" "{\"skeleton\":{\"import\":{\"enable\":false}},\"animations\":[]}")
file(WRITE "${ozz_temp_directory}/invalid_config.json" "invalid json content")

# Run test2ozz generic failing tests
#----------------------------

add_test(NAME test2ozz_no_arg COMMAND test2ozz)
set_tests_properties(test2ozz_no_arg PROPERTIES PASS_REGULAR_EXPRESSION "Required option \"file\" is not specified.")

add_test(NAME test2ozz_bad_argument COMMAND test2ozz "--file=${ozz_temp_directory}/good.content1" "--bad")
set_tests_properties(test2ozz_bad_argument PROPERTIES PASS_REGULAR_EXPRESSION "Invalid command line argument:\"--bad\".")

add_test(NAME test2ozz_bad_endian COMMAND test2ozz "--file=${ozz_temp_directory}/good.content1" "--endian=fat")
set_tests_properties(test2ozz_bad_endian PROPERTIES PASS_REGULAR_EXPRESSION "Invalid endianness option \"fat\".")

add_test(NAME test2ozz_bad_log COMMAND test2ozz "--file=${ozz_temp_directory}/good.content1" "--log_level=loud")
set_tests_properties(test2ozz_bad_log PROPERTIES PASS_REGULAR_EXPRESSION "Invalid log level option \"loud\".")

add_test(NAME test2ozz_bad_content COMMAND test2ozz "--file=${ozz_temp_directory}/bad.content")
set_tests_properties(test2ozz_bad_content PROPERTIES PASS_REGULAR_EXPRESSION "Failed to import file \"${ozz_temp_directory}/bad.content\".")

add_test(NAME test2ozz_unexisting_test COMMAND test2ozz "--file=${ozz_temp_directory}/file_doesn_t_exist")
set_tests_properties(test2ozz_unexisting_test PROPERTIES PASS_REGULAR_EXPRESSION "File \"${ozz_temp_directory}/file_doesn_t_exist\" doesn't exist.")

add_test(NAME test2ozz_exclusive_config COMMAND test2ozz "--file=${ozz_temp_directory}/good.content1" "--config={}" "--config_file=${ozz_temp_directory}/valid_config.json")
set_tests_properties(test2ozz_exclusive_config PROPERTIES PASS_REGULAR_EXPRESSION "--config and --config_file are exclusive options.")

add_test(NAME test2ozz_unexisting_config_file COMMAND test2ozz "--file=${ozz_temp_directory}/good.content1" "--config_file=${ozz_temp_directory}/unexisting_config.json")
set_tests_properties(test2ozz_unexisting_config_file PROPERTIES PASS_REGULAR_EXPRESSION "Failed to open config file: \"${ozz_temp_directory}/unexisting_config.json\".")

add_test(NAME test2ozz_invalid_config_file COMMAND test2ozz "--file=${ozz_temp_directory}/good.content1" "--config_file=${ozz_temp_directory}/invalid_config.json")
set_tests_properties(test2ozz_invalid_config_file PROPERTIES PASS_REGULAR_EXPRESSION "Error while parsing configuration string")

add_test(NAME test2ozz_dump_config_bad_file COMMAND test2ozz "--file=${ozz_temp_directory}/good.content1" "--config_dump_reference=${ozz_temp_directory}/unexisting/config.json")
set_tests_properties(test2ozz_dump_config_bad_file PROPERTIES PASS_REGULAR_EXPRESSION "Failed to open config file to dump" DEPENDS test2ozz_skel_simple)

add_test(NAME test2ozz_bad_type COMMAND test2ozz "--file=${ozz_temp_directory}/good.content1" "--config={\"animations\":[{\"filename\":\"${ozz_temp_directory}/anim_should_not_exist.ozz\",\"sampling_rate\":\"1\"}]}")
set_tests_properties(test2ozz_bad_type PROPERTIES PASS_REGULAR_EXPRESSION "Invalid type \"UTF-8 string\" for json member \"root.animations\\\[0\\\].sampling_rate\". \"float\" expected." DEPENDS test2ozz_skel_simple)

add_test(NAME test2ozz_bad_array COMMAND test2ozz "--file=${ozz_temp_directory}/good.content1" "--config={\"animations\":{\"sampling_rate\":true}}")
set_tests_properties(test2ozz_bad_array PROPERTIES PASS_REGULAR_EXPRESSION "Invalid type \"object\" for json member \"root.animations\". \"array\" expected." DEPENDS test2ozz_skel_simple)

add_test(NAME test2ozz_bad_type_in_array COMMAND test2ozz "--file=${ozz_temp_directory}/good.content1" "--config={\"animations\":[{},{\"sampling_rate\":true}]}")
set_tests_properties(test2ozz_bad_type_in_array PROPERTIES PASS_REGULAR_EXPRESSION "Invalid type \"boolean\" for json member \"root.animations\\\[1\\\].sampling_rate\". \"float\" expected." DEPENDS test2ozz_skel_simple)

add_test(NAME test2ozz_wrong_config_unexpected_member2 COMMAND test2ozz "--file=${ozz_temp_directory}/good.content1" "--config={\"animations\":[{},{\"foo\":true}]}")
set_tests_properties(test2ozz_wrong_config_unexpected_member2 PROPERTIES PASS_REGULAR_EXPRESSION "Invalid json member \"root.animations\\\[1\\\].foo\"." DEPENDS test2ozz_skel_simple)

add_test(NAME test2ozz_wrong_optimize COMMAND test2ozz "--file=${ozz_temp_directory}/good.content1" "--config={\"animations\":[{\"filename\":\"${ozz_temp_directory}/anim_should_not_exist.ozz\",\"optimize\":99}]}")
set_tests_properties(test2ozz_wrong_optimize PROPERTIES PASS_REGULAR_EXPRESSION "Invalid type \"integer\" for json member \"root.animations\\\[0\\\].optimize\". \"boolean\" expected." DEPENDS test2ozz_skel_simple)

add_test(NAME test2ozz_wrong_config_unexpected_member1 COMMAND test2ozz "--file=${ozz_temp_directory}/good.content1" "--config={\"skeleton\":\"${ozz_temp_directory}/skeleton.ozz\",\"filename\":\"${ozz_temp_directory}/anim_should_not_exist.ozz\"}")
set_tests_properties(test2ozz_wrong_config_unexpected_member1 PROPERTIES PASS_REGULAR_EXPRESSION "Invalid json member \"root.filename\"." DEPENDS test2ozz_skel_simple)

add_test(NAME test2ozz_wrong_config_invalid_object COMMAND test2ozz "--file=${ozz_temp_directory}/good.content1" "--config={\"animations\":{\"filename\":\"${ozz_temp_directory}/anim_should_not_exist.ozz\"}}")
set_tests_properties(test2ozz_wrong_config_invalid_object PROPERTIES PASS_REGULAR_EXPRESSION "Invalid type \"object\" for json member \"root.animations\". \"array\" expected." DEPENDS test2ozz_skel_simple)

add_test(NAME test2ozz_wrong_config_invalid_array COMMAND test2ozz "--file=${ozz_temp_directory}/good.content1" "--config={\"animations\":[{\"optimize\":[]}]}")
set_tests_properties(test2ozz_wrong_config_invalid_array PROPERTIES PASS_REGULAR_EXPRESSION "Invalid type \"array\" for json member \"root.animations\\\[0\\\].optimize\". \"boolean\" expected." DEPENDS test2ozz_skel_simple)

# Run test2ozz generic passing tests
#----------------------------

add_test(NAME test2ozz_version COMMAND test2ozz "--version")
set_tests_properties(test2ozz_version PROPERTIES PASS_REGULAR_EXPRESSION "version 2.0")

add_test(NAME test2ozz_log_verbose COMMAND test2ozz "--file=${ozz_temp_directory}/good.content1" "--config_file=${ozz_temp_directory}/no_output_config.json" "--log_level=verbose")
set_tests_properties(test2ozz_log_verbose PROPERTIES PASS_REGULAR_EXPRESSION "Verbose log level activated.")

add_test(NAME test2ozz_native COMMAND test2ozz "--file=${ozz_temp_directory}/good.content1" "--config_file=${ozz_temp_directory}/no_output_config.json" "--endian=native")
add_test(NAME test2ozz_little COMMAND test2ozz "--file=${ozz_temp_directory}/good.content1" "--config_file=${ozz_temp_directory}/no_output_config.json" "--endian=little" "--log_level=verbose")
set_tests_properties(test2ozz_little PROPERTIES PASS_REGULAR_EXPRESSION "Little endian output binary format selected.")
add_test(NAME test2ozz_big COMMAND test2ozz "--file=${ozz_temp_directory}/good.content1" "--config_file=${ozz_temp_directory}/no_output_config.json" "--endian=big" "--log_level=verbose")
set_tests_properties(test2ozz_big PROPERTIES PASS_REGULAR_EXPRESSION "Big endian output binary format selected.")

add_test(NAME test2ozz_log_config_verbose COMMAND test2ozz "--file=${ozz_temp_directory}/good.content1" "--config_file=${ozz_temp_directory}/no_output_config.json" "--log_level=verbose")
set_tests_properties(test2ozz_log_config_verbose PROPERTIES PASS_REGULAR_EXPRESSION "//  Animations to import.")

add_test(NAME test2ozz_dump_ref_config COMMAND test2ozz "--file=${ozz_temp_directory}/good.content1" "--config_file=${ozz_temp_directory}/no_output_config.json" "--config_dump_reference=${ozz_temp_directory}/reference.json")
add_test(NAME test2ozz_dump_ref_config_output COMMAND ${CMAKE_COMMAND} -E copy "${ozz_temp_directory}/reference.json" "${ozz_temp_directory}/reference_should_exist.json")
set_tests_properties(test2ozz_dump_ref_config_output PROPERTIES DEPENDS test2ozz_dump_ref_config)

file(MAKE_DIRECTORY ${ozz_temp_directory}/ref_config_relative)
add_test(NAME test2ozz_load_ref_config COMMAND test2ozz "--file=${ozz_temp_directory}/good.content1" "--config_file=${ozz_temp_directory}/reference.json" WORKING_DIRECTORY "${ozz_temp_directory}/ref_config_relative")
set_tests_properties(test2ozz_load_ref_config PROPERTIES DEPENDS test2ozz_dump_ref_config)

file(MAKE_DIRECTORY ${ozz_temp_directory}/ref_config_relative/cp)
add_test(NAME test2ozz_load_ref_config_output COMMAND ${CMAKE_COMMAND} -E copy "${ozz_temp_directory}/ref_config_relative/skeleton.ozz" "${ozz_temp_directory}/ref_config_relative/one.ozz" "${ozz_temp_directory}/ref_config_relative/joint0-property0.ozz" "${ozz_temp_directory}/ref_config_relative/joint0-property1.ozz" "${ozz_temp_directory}/ref_config_relative/joint1-property0.ozz" "${ozz_temp_directory}/ref_config_relative/joint1-property1.ozz" "${ozz_temp_directory}/ref_config_relative/cp/")
set_tests_properties(test2ozz_load_ref_config_output PROPERTIES DEPENDS test2ozz_load_ref_config)

file(MAKE_DIRECTORY ${ozz_temp_directory}/test2ozz_no_config)
add_test(NAME test2ozz_no_config COMMAND test2ozz "--file=${ozz_temp_directory}/good.content1" WORKING_DIRECTORY "${ozz_temp_directory}/test2ozz_no_config")
set_tests_properties(test2ozz_no_config PROPERTIES PASS_REGULAR_EXPRESSION "No configuration provided, using default configuration.")

file(MAKE_DIRECTORY ${ozz_temp_directory}/test2ozz_no_config/cp)
add_test(NAME test2ozz_no_config_output COMMAND ${CMAKE_COMMAND} -E copy "${ozz_temp_directory}/test2ozz_no_config/skeleton.ozz" "${ozz_temp_directory}/test2ozz_no_config/one.ozz" "${ozz_temp_directory}/test2ozz_no_config/cp/")
set_tests_properties(test2ozz_no_config_output PROPERTIES DEPENDS test2ozz_no_config)

# Run test2ozz skeleton import failing tests
#----------------------

add_test(NAME test2ozz_skel_invalid_filename COMMAND test2ozz "--file=${ozz_temp_directory}/good.content1" "--config={\"skeleton\":{\"filename\":\"${ozz_temp_directory}/invalid_path/skel_should_not_exist.ozz\",\"import\":{\"enable\":true}},\"animations\":[]}}")
set_tests_properties(test2ozz_skel_invalid_filename PROPERTIES PASS_REGULAR_EXPRESSION "Failed to open output file: \"${ozz_temp_directory}/invalid_path/skel_should_not_exist.ozz\".")

add_test(NAME test2ozz_skel_invalid_filename_raw COMMAND test2ozz "--config={\"skeleton\":{\"filename\":\"${ozz_temp_directory}/invalid_path/skel_should_not_exist.ozz\",\"import\":{\"enable\":true,\"raw\":true}},\"animations\":[]}}")
set_tests_properties(test2ozz_skel_invalid_filename_raw PROPERTIES WILL_FAIL true)

add_test(NAME test2ozz_skel_non_unique_names COMMAND test2ozz "--file=${ozz_temp_directory}/non_unique_names.content"  "--config={\"skeleton\":{\"filename\":\"${ozz_temp_directory}/skel_should_not_exist.ozz\",\"import\":{\"enable\":true}},\"animations\":[]}}")
set_tests_properties(test2ozz_skel_non_unique_names PROPERTIES PASS_REGULAR_EXPRESSION "Skeleton contains at least one non-unique joint name \"jointx\", which is not supported.")

# Run test2ozz skeleton passing tests
#----------------------------
add_test(NAME test2ozz_skel_simple COMMAND test2ozz "--file=${ozz_temp_directory}/good.content1" "--config={\"skeleton\":{\"filename\":\"${ozz_temp_directory}/skeleton.ozz\"},\"animations\":[]}}")
add_test(NAME test2ozz_skel_simple_raw COMMAND test2ozz "--file=${ozz_temp_directory}/good.content1" "--config={\"skeleton\":{\"filename\":\"${ozz_temp_directory}/raw_skeleton.ozz\",\"import\":{\"raw\":true}},\"animations\":[]}}")
add_test(NAME test2ozz_skel_no_import COMMAND test2ozz "--file=${ozz_temp_directory}/good.content1" "--config={\"skeleton\":{\"filename\":\"${ozz_temp_directory}/skel_should_not_exist.ozz\",\"import\":{\"enable\":false}},\"animations\":[]}}")
set_tests_properties(test2ozz_skel_no_import PROPERTIES PASS_REGULAR_EXPRESSION "Skeleton build disabled, import will be skipped")

add_test(NAME test2ozz_log_hierarchy COMMAND test2ozz "--file=${ozz_temp_directory}/good.content1" "--config={\"skeleton\":{\"filename\":\"${ozz_temp_directory}/skeleton_${CMAKE_CURRENT_LIST_LINE}.ozz\"},\"animations\":[]}}" "--log_level=verbose")
set_tests_properties(test2ozz_log_hierarchy PROPERTIES PASS_REGULAR_EXPRESSION ".joint0 t")

# Ensures nothing was outputted.
add_test(NAME test2ozz_skel_output COMMAND ${CMAKE_COMMAND} -E copy "${ozz_temp_directory}/skel_should_not_exist.ozz" "${ozz_temp_directory}/skel_should_not_exist_too.ozz")
set_tests_properties(test2ozz_skel_output PROPERTIES WILL_FAIL true)
set_tests_properties(test2ozz_skel_output PROPERTIES
  DEPENDS "test2ozz_skel_no_import
           test2ozz_skel_non_unique_names
           test2ozz_skel_invalid_filename
           test2ozz_skel_invalid_filename_raw")

# Run test2ozz animation import failing tests
#----------------------------
add_test(NAME test2ozz_anim_no_animation COMMAND test2ozz "--file=${ozz_temp_directory}/good.content1" "--config={\"skeleton\":{\"filename\":\"${ozz_temp_directory}/skeleton.ozz\",\"import\":{\"enable\":false}},\"animations\":[]}")
set_tests_properties(test2ozz_anim_no_animation PROPERTIES PASS_REGULAR_EXPRESSION "Configuration contains no animation import definition, animations import will be skipped." DEPENDS test2ozz_skel_simple)

add_test(NAME test2ozz_anim_no_clip_name COMMAND test2ozz "--file=${ozz_temp_directory}/good.content1" "--config={\"skeleton\":{\"filename\":\"${ozz_temp_directory}/skeleton.ozz\",\"import\":{\"enable\":false}},\"animations\":[{\"clip\":\"\"}]}")
set_tests_properties(test2ozz_anim_no_clip_name PROPERTIES PASS_REGULAR_EXPRESSION "No clip name provided. Animation import will be skipped." DEPENDS test2ozz_skel_simple)

add_test(NAME test2ozz_anim_no_animation_output COMMAND test2ozz "--file=${ozz_temp_directory}/good.content1" "--config={\"skeleton\":{\"filename\":\"${ozz_temp_directory}/skeleton.ozz\",\"import\":{\"enable\":false}},\"animations\":[{\"filename\":\"\"}]}")
set_tests_properties(test2ozz_anim_no_animation_output PROPERTIES PASS_REGULAR_EXPRESSION "Failed to open output file: \"\"." DEPENDS test2ozz_skel_simple)

add_test(NAME test2ozz_anim_no_anim_test COMMAND test2ozz "--file=${ozz_temp_directory}/good.content0" "--config={\"skeleton\":{\"filename\":\"${ozz_temp_directory}/skeleton.ozz\",\"import\":{\"enable\":false}},\"animations\":[{\"filename\":\"${ozz_temp_directory}/anim_should_not_exist.ozz\"}]}")
set_tests_properties(test2ozz_anim_no_anim_test PROPERTIES PASS_REGULAR_EXPRESSION "No animation found." DEPENDS test2ozz_skel_simple)

add_test(NAME test2ozz_anim_bad_skeleton COMMAND test2ozz "--file=${ozz_temp_directory}/good.content1" "--config={\"skeleton\":{\"filename\":\"${ozz_temp_directory}/bad.content\",\"import\":{\"enable\":false}},\"animations\":[{\"filename\":\"${ozz_temp_directory}/anim_should_not_exist.ozz\"}]}")
set_tests_properties(test2ozz_anim_bad_skeleton PROPERTIES PASS_REGULAR_EXPRESSION "Failed to read input skeleton from binary file" DEPENDS test2ozz_skel_simple)

add_test(NAME test2ozz_anim_unexisting_skeleton COMMAND test2ozz "--file=${ozz_temp_directory}/good.content1" "--config={\"skeleton\":{\"filename\":\"${ozz_temp_directory}/unexisting.ozz\",\"import\":{\"enable\":false}},\"animations\":[{\"filename\":\"${ozz_temp_directory}/anim_should_not_exist.ozz\"}]}")
set_tests_properties(test2ozz_anim_unexisting_skeleton PROPERTIES PASS_REGULAR_EXPRESSION "Failed to open input skeleton ozz binary file: \"${ozz_temp_directory}/unexisting.ozz\"")

add_test(NAME test2ozz_anim_invalid_filename COMMAND test2ozz "--file=${ozz_temp_directory}/good.content1" "--config={\"skeleton\":{\"filename\":\"${ozz_temp_directory}/skeleton.ozz\",\"import\":{\"enable\":false}},\"animations\":[{\"filename\":\"${ozz_temp_directory}/invalid_path/anim_should_not_exist.ozz\"}]}")
set_tests_properties(test2ozz_anim_invalid_filename PROPERTIES PASS_REGULAR_EXPRESSION "Failed to open output file: \"${ozz_temp_directory}/invalid_path/anim_should_not_exist.ozz\"." DEPENDS test2ozz_skel_simple)

add_test(NAME test2ozz_anim_no_match COMMAND test2ozz "--file=${ozz_temp_directory}/good.content1" "--config={\"skeleton\":{\"filename\":\"${ozz_temp_directory}/skeleton.ozz\",\"import\":{\"enable\":false}},\"animations\":[{\"clip\":\"won_t_match\",\"filename\":\"${ozz_temp_directory}/invalid_path/anim_should_not_exist.ozz\"}]}")
set_tests_properties(test2ozz_anim_no_match PROPERTIES PASS_REGULAR_EXPRESSION "No matching animation found for \"won_t_match\"." DEPENDS test2ozz_skel_simple)

add_test(NAME test2ozz_anim_additive_wrong_ref COMMAND test2ozz "--file=${ozz_temp_directory}/good.content1" "--config={\"skeleton\":{\"filename\":\"${ozz_temp_directory}/skeleton.ozz\",\"import\":{\"enable\":false}},\"animations\":[{\"filename\":\"${ozz_temp_directory}/animation_${CMAKE_CURRENT_LIST_LINE}.ozz\",\"additive\":true,\"additive_reference\":\"anim\"}]}")
set_tests_properties(test2ozz_anim_additive_wrong_ref PROPERTIES PASS_REGULAR_EXPRESSION "Invalid additive reference pose \"anim\"." DEPENDS test2ozz_skel_simple)

# Run test2ozz track import failing tests
#----------------------------

add_test(NAME test2ozz_anim_track_bad_joint_name COMMAND test2ozz "--file=${ozz_temp_directory}/good.content1" "--config={\"skeleton\":{\"filename\":\"${ozz_temp_directory}/skeleton.ozz\",\"import\":{\"enable\":false}},\"animations\":[{\"filename\":\"${ozz_temp_directory}/animation_${CMAKE_CURRENT_LIST_LINE}.ozz\",\"tracks\":[{\"properties\":[{\"joint_name\":\"joint3\",\"property_name\":\"property2\",\"filename\":\"${ozz_temp_directory}/anim_should_not_exist.ozz\"}]}]}]}")
set_tests_properties(test2ozz_anim_track_bad_joint_name PROPERTIES PASS_REGULAR_EXPRESSION "No joint found for track import definition \"joint3\"" DEPENDS test2ozz_skel_simple)

add_test(NAME test2ozz_anim_track_bad_ppt_name COMMAND test2ozz "--file=${ozz_temp_directory}/good.content1" "--config={\"skeleton\":{\"filename\":\"${ozz_temp_directory}/skeleton.ozz\",\"import\":{\"enable\":false}},\"animations\":[{\"filename\":\"${ozz_temp_directory}/animation_${CMAKE_CURRENT_LIST_LINE}.ozz\",\"tracks\":[{\"properties\":[{\"joint_name\":\"joint2\",\"property_name\":\"propertyx\",\"filename\":\"${ozz_temp_directory}/anim_should_not_exist.ozz\"}]}]}]}")
set_tests_properties(test2ozz_anim_track_bad_ppt_name PROPERTIES PASS_REGULAR_EXPRESSION "No property found for track import definition \"joint2:propertyx\"" DEPENDS test2ozz_skel_simple)

add_test(NAME test2ozz_anim_track_bad_type COMMAND test2ozz "--file=${ozz_temp_directory}/good.content1" "--config={\"skeleton\":{\"filename\":\"${ozz_temp_directory}/skeleton.ozz\",\"import\":{\"enable\":false}},\"animations\":[{\"filename\":\"${ozz_temp_directory}/animation_${CMAKE_CURRENT_LIST_LINE}.ozz\",\"tracks\":[{\"properties\":[{\"joint_name\":\"joint1\",\"property_name\":\"property2\",\"type\":\"float3\",\"filename\":\"${ozz_temp_directory}/anim_should_not_exist.ozz\"}]}]}]}")
set_tests_properties(test2ozz_anim_track_bad_type PROPERTIES PASS_REGULAR_EXPRESSION "Incompatible type \"float3\" for matching property \"joint1:property2\" of type \"float2\"" DEPENDS test2ozz_skel_simple)

add_test(NAME test2ozz_anim_track_invalid_type COMMAND test2ozz "--file=${ozz_temp_directory}/good.content1" "--config={\"skeleton\":{\"filename\":\"${ozz_temp_directory}/skeleton.ozz\",\"import\":{\"enable\":false}},\"animations\":[{\"filename\":\"${ozz_temp_directory}/animation_${CMAKE_CURRENT_LIST_LINE}.ozz\",\"tracks\":[{\"properties\":[{\"joint_name\":\"joint1\",\"property_name\":\"property2\",\"type\":\"1\",\"filename\":\"${ozz_temp_directory}/anim_should_not_exist.ozz\"}]}]}]}")
set_tests_properties(test2ozz_anim_track_invalid_type PROPERTIES PASS_REGULAR_EXPRESSION "Invalid value \"1\" for import track type property." DEPENDS test2ozz_skel_simple)

# Ensures nothing was outputted.
add_test(NAME test2ozz_anim_output COMMAND ${CMAKE_COMMAND} -E copy "${ozz_temp_directory}/anim_should_not_exist.ozz" "${ozz_temp_directory}/anim_should_not_exist_too.ozz")
set_tests_properties(test2ozz_anim_output PROPERTIES WILL_FAIL true)
set_tests_properties(test2ozz_anim_output PROPERTIES
  DEPENDS "test2ozz_anim_output
           test2ozz_anim_missing_test
           test2ozz_anim_bad_test
           test2ozz_anim_bad_skeleton
           test2ozz_anim_unexisting_skeleton
           test2ozz_anim_invalid_filename
           test2ozz_anim_bad_log_level
           test2ozz_anim_track_bad_joint_name
           test2ozz_anim_track_bad_ppt_name
           test2ozz_anim_track_bad_type
           test2ozz_anim_track_invalid_type")

# Run test2ozz animation import passing tests
#----------------------------

add_test(NAME test2ozz_anim_simple COMMAND test2ozz "--file=${ozz_temp_directory}/good.content1" "--config={\"skeleton\":{\"filename\":\"${ozz_temp_directory}/skeleton.ozz\",\"import\":{\"enable\":false}},\"animations\":[{\"filename\":\"${ozz_temp_directory}/animation_simple.ozz\"}]}")
set_tests_properties(test2ozz_anim_simple PROPERTIES DEPENDS test2ozz_skel_simple)
add_test(NAME test2ozz_anim_simple_output COMMAND ${CMAKE_COMMAND} -E copy "${ozz_temp_directory}/animation_simple.ozz" "${ozz_temp_directory}/animation_simple_should_exist.ozz")
set_tests_properties(test2ozz_anim_simple_output PROPERTIES DEPENDS test2ozz_anim_simple)

add_test(NAME test2ozz_anim_simple_renamed COMMAND test2ozz "--file=${ozz_temp_directory}/good.content_renamed" "--config={\"skeleton\":{\"filename\":\"${ozz_temp_directory}/skeleton.ozz\",\"import\":{\"enable\":false}},\"animations\":[{\"filename\":\"${ozz_temp_directory}/*.ozz\"}]}")
set_tests_properties(test2ozz_anim_simple_renamed PROPERTIES PASS_REGULAR_EXPRESSION "in order to be used as a valid filename" DEPENDS test2ozz_skel_simple)

file(MAKE_DIRECTORY ${ozz_temp_directory}/anim_simple_relative)
add_test(NAME test2ozz_anim_simple_relative COMMAND test2ozz "--file=${ozz_temp_directory}/good.content1" "--config={\"skeleton\":{\"filename\":\"${ozz_temp_directory}/skeleton.ozz\",\"import\":{\"enable\":false}},\"animations\":[{\"filename\":\"animation.ozz\"}]}" WORKING_DIRECTORY "${ozz_temp_directory}/anim_simple_relative")
set_tests_properties(test2ozz_anim_simple_relative PROPERTIES DEPENDS test2ozz_skel_simple)
add_test(NAME test2ozz_anim_simple_relative_output COMMAND ${CMAKE_COMMAND} -E copy "${ozz_temp_directory}/anim_simple_relative/animation.ozz" "${ozz_temp_directory}/anim_simple_relative/animation_should_exist.ozz")
set_tests_properties(test2ozz_anim_simple_relative_output PROPERTIES DEPENDS test2ozz_anim_simple_relative)

add_test(NAME test2ozz_anim_simple_config_file COMMAND test2ozz "--file=${ozz_temp_directory}/good.content1" "--config_file=${ozz_temp_directory}/valid_config.json")
set_tests_properties(test2ozz_anim_simple_config_file PROPERTIES DEPENDS test2ozz_skel_simple)

add_test(NAME test2ozz_anim_optimize COMMAND test2ozz "--file=${ozz_temp_directory}/good.content1" "--config={\"skeleton\":{\"filename\":\"${ozz_temp_directory}/skeleton.ozz\",\"import\":{\"enable\":false}},\"animations\":[{\"filename\":\"${ozz_temp_directory}/animation_${CMAKE_CURRENT_LIST_LINE}.ozz\",\"optimize\":true}]}")
set_tests_properties(test2ozz_anim_optimize PROPERTIES DEPENDS test2ozz_skel_simple)

add_test(NAME test2ozz_anim_optimize_tol COMMAND test2ozz "--file=${ozz_temp_directory}/good.content1" "--config={\"skeleton\":{\"filename\":\"${ozz_temp_directory}/skeleton.ozz\",\"import\":{\"enable\":false}},\"animations\":[{\"filename\":\"${ozz_temp_directory}/animation_${CMAKE_CURRENT_LIST_LINE}.ozz\",\"optimize\":true,\"optimization_settings\":{\"tolerance\":0.002,\"distance\":0.002}}]}")
set_tests_properties(test2ozz_anim_optimize_tol PROPERTIES DEPENDS test2ozz_skel_simple)

add_test(NAME test2ozz_anim_optimize_joints_tol COMMAND test2ozz "--file=${ozz_temp_directory}/good.content1" "--config={\"skeleton\":{\"filename\":\"${ozz_temp_directory}/skeleton.ozz\",\"import\":{\"enable\":false}},\"animations\":[{\"filename\":\"${ozz_temp_directory}/animation_${CMAKE_CURRENT_LIST_LINE}.ozz\",\"optimize\":true,\"optimization_settings\":{\"override\":[{\"name\":\"joint?\",\"tolerance\":0.002,\"distance\":0.002}]}}]}")
set_tests_properties(test2ozz_anim_optimize_joints_tol PROPERTIES DEPENDS test2ozz_skel_simple)

add_test(NAME test2ozz_anim_optimize_joints_tol_verbose COMMAND test2ozz "--file=${ozz_temp_directory}/good.content1" "--config={\"skeleton\":{\"filename\":\"${ozz_temp_directory}/skeleton.ozz\",\"import\":{\"enable\":false}},\"animations\":[{\"filename\":\"${ozz_temp_directory}/animation_${CMAKE_CURRENT_LIST_LINE}.ozz\",\"optimize\":true,\"optimization_settings\":{\"override\":[{\"name\":\"joint?\",\"tolerance\":0.002,\"distance\":0.002}]}}]}" "--log_level=verbose")
set_tests_properties(test2ozz_anim_optimize_joints_tol_verbose PROPERTIES PASS_REGULAR_EXPRESSION "Found joint \"joint2\" matching pattern" DEPENDS test2ozz_skel_simple)

add_test(NAME test2ozz_anim_optimize_joints_not_found_tol COMMAND test2ozz "--file=${ozz_temp_directory}/good.content1" "--config={\"skeleton\":{\"filename\":\"${ozz_temp_directory}/skeleton.ozz\",\"import\":{\"enable\":false}},\"animations\":[{\"filename\":\"${ozz_temp_directory}/animation_${CMAKE_CURRENT_LIST_LINE}.ozz\",\"optimize\":true,\"optimization_settings\":{\"override\":[{\"name\":\"joint_won_t_find\",\"tolerance\":0.002,\"distance\":0.002}]}}]}")
set_tests_properties(test2ozz_anim_optimize_joints_not_found_tol PROPERTIES PASS_REGULAR_EXPRESSION "No joint found for optimization setting" DEPENDS test2ozz_skel_simple)

add_test(NAME test2ozz_anim_optimize_joints_redundant_tol COMMAND test2ozz "--file=${ozz_temp_directory}/good.content1" "--config={\"skeleton\":{\"filename\":\"${ozz_temp_directory}/skeleton.ozz\",\"import\":{\"enable\":false}},\"animations\":[{\"filename\":\"${ozz_temp_directory}/animation_${CMAKE_CURRENT_LIST_LINE}.ozz\",\"optimize\":true,\"optimization_settings\":{\"override\":[{\"name\":\"joint1\"},{\"name\":\"joint?\"}]}}]}")
set_tests_properties(test2ozz_anim_optimize_joints_redundant_tol PROPERTIES PASS_REGULAR_EXPRESSION "Redundant optimization setting for pattern" DEPENDS test2ozz_skel_simple)

add_test(NAME test2ozz_anim_no_opt COMMAND test2ozz "--file=${ozz_temp_directory}/good.content1" "--config={\"skeleton\":{\"filename\":\"${ozz_temp_directory}/skeleton.ozz\",\"import\":{\"enable\":false}},\"animations\":[{\"filename\":\"${ozz_temp_directory}/animation_${CMAKE_CURRENT_LIST_LINE}.ozz\"}]},\"optimize\":false}]}")
set_tests_properties(test2ozz_anim_no_opt PROPERTIES DEPENDS test2ozz_skel_simple)

add_test(NAME test2ozz_anim_simple_raw COMMAND test2ozz "--file=${ozz_temp_directory}/good.content1" "--config={\"skeleton\":{\"filename\":\"${ozz_temp_directory}/skeleton.ozz\",\"import\":{\"enable\":false}},\"animations\":[{\"filename\":\"${ozz_temp_directory}/raw_animation_${CMAKE_CURRENT_LIST_LINE}.ozz\",\"raw\":false}]}")
set_tests_properties(test2ozz_anim_simple_raw PROPERTIES DEPENDS test2ozz_skel_simple)

add_test(NAME test2ozz_anim_simple_from_raw COMMAND test2ozz "--file=${ozz_temp_directory}/good.content1" "--config={\"skeleton\":{\"filename\":\"${ozz_temp_directory}/raw_skeleton.ozz\",\"import\":{\"enable\":false}},\"animations\":[{\"filename\":\"${ozz_temp_directory}/animation_${CMAKE_CURRENT_LIST_LINE}.ozz\"}]}")
set_tests_properties(test2ozz_anim_simple_from_raw PROPERTIES DEPENDS test2ozz_skel_simple_raw)

add_test(NAME test2ozz_anim_simple_raw_from_raw COMMAND test2ozz "--file=${ozz_temp_directory}/good.content1" "--config={\"skeleton\":{\"filename\":\"${ozz_temp_directory}/raw_skeleton.ozz\",\"import\":{\"enable\":false}},\"animations\":[{\"filename\":\"${ozz_temp_directory}/animation_${CMAKE_CURRENT_LIST_LINE}.ozz\",\"raw\":false}]}")
set_tests_properties(test2ozz_anim_simple_raw_from_raw PROPERTIES DEPENDS test2ozz_skel_simple_raw)

add_test(NAME test2ozz_anim_additive COMMAND test2ozz "--file=${ozz_temp_directory}/good.content1" "--config={\"skeleton\":{\"filename\":\"${ozz_temp_directory}/skeleton.ozz\",\"import\":{\"enable\":false}},\"animations\":[{\"filename\":\"${ozz_temp_directory}/animation_${CMAKE_CURRENT_LIST_LINE}.ozz\",\"additive\":true}]}")
set_tests_properties(test2ozz_anim_additive PROPERTIES DEPENDS test2ozz_skel_simple)

add_test(NAME test2ozz_anim_additive_raw COMMAND test2ozz "--file=${ozz_temp_directory}/good.content1" "--config={\"skeleton\":{\"filename\":\"${ozz_temp_directory}/skeleton.ozz\",\"import\":{\"enable\":false}},\"animations\":[{\"filename\":\"${ozz_temp_directory}/animation_${CMAKE_CURRENT_LIST_LINE}.ozz\",\"raw\":false,\"additive\":true}]}")
set_tests_properties(test2ozz_anim_additive_raw PROPERTIES DEPENDS test2ozz_skel_simple)

add_test(NAME test2ozz_anim_additive_ref_anim COMMAND test2ozz "--file=${ozz_temp_directory}/good.content1" "--config={\"skeleton\":{\"filename\":\"${ozz_temp_directory}/skeleton.ozz\",\"import\":{\"enable\":false}},\"animations\":[{\"filename\":\"${ozz_temp_directory}/animation_${CMAKE_CURRENT_LIST_LINE}.ozz\",\"additive\":true,\"additive_reference\":\"animation\"}]}")
set_tests_properties(test2ozz_anim_additive_ref_anim PROPERTIES DEPENDS test2ozz_skel_simple)

add_test(NAME test2ozz_anim_additive_ref_skel COMMAND test2ozz "--file=${ozz_temp_directory}/good.content1" "--config={\"skeleton\":{\"filename\":\"${ozz_temp_directory}/skeleton.ozz\",\"import\":{\"enable\":false}},\"animations\":[{\"filename\":\"${ozz_temp_directory}/animation_${CMAKE_CURRENT_LIST_LINE}.ozz\",\"additive\":true,\"additive_reference\":\"skeleton\"}]}")
set_tests_properties(test2ozz_anim_additive_ref_skel PROPERTIES DEPENDS test2ozz_skel_simple)

add_test(NAME test2ozz_anim_native COMMAND test2ozz "--file=${ozz_temp_directory}/good.content1" "--config={\"skeleton\":{\"filename\":\"${ozz_temp_directory}/skeleton.ozz\",\"import\":{\"enable\":false}},\"animations\":[{\"filename\":\"${ozz_temp_directory}/animation_native_${CMAKE_CURRENT_LIST_LINE}.ozz\"}]}" "--endian=native")
set_tests_properties(test2ozz_anim_native PROPERTIES DEPENDS test2ozz_skel_simple)

add_test(NAME test2ozz_anim_little COMMAND test2ozz "--file=${ozz_temp_directory}/good.content1" "--config={\"skeleton\":{\"filename\":\"${ozz_temp_directory}/skeleton.ozz\",\"import\":{\"enable\":false}},\"animations\":[{\"filename\":\"${ozz_temp_directory}/animation_little_${CMAKE_CURRENT_LIST_LINE}.ozz\"}]}" "--endian=little")
set_tests_properties(test2ozz_anim_little PROPERTIES DEPENDS test2ozz_skel_simple)

add_test(NAME test2ozz_anim_big COMMAND test2ozz "--file=${ozz_temp_directory}/good.content1" "--config={\"skeleton\":{\"filename\":\"${ozz_temp_directory}/skeleton.ozz\",\"import\":{\"enable\":false}},\"animations\":[{\"filename\":\"${ozz_temp_directory}/animation_big_${CMAKE_CURRENT_LIST_LINE}.ozz\"}]}" "--endian=big")
set_tests_properties(test2ozz_anim_big PROPERTIES DEPENDS test2ozz_skel_simple)

add_test(NAME test2ozz_anim_unmatch_skeleton COMMAND test2ozz "--file=${ozz_temp_directory}/good.content1" "--config={\"skeleton\":{\"filename\":\"${ozz_temp_directory}/skeleton.ozz\",\"import\":{\"enable\":false}},\"animations\":[{\"filename\":\"${ozz_temp_directory}/animation_big_${CMAKE_CURRENT_LIST_LINE}.ozz\"}]}" "--endian=big")
set_tests_properties(test2ozz_anim_unmatch_skeleton PROPERTIES DEPENDS test2ozz_skel_simple)

add_test(NAME test2ozz_anim_sampling_rate COMMAND test2ozz "--file=${ozz_temp_directory}/good.content1" "--config={\"skeleton\":{\"filename\":\"${ozz_temp_directory}/skeleton.ozz\",\"import\":{\"enable\":false}},\"animations\":[{\"filename\":\"${ozz_temp_directory}/animation_${CMAKE_CURRENT_LIST_LINE}.ozz\",\"sampling_rate\":10.}]}")
set_tests_properties(test2ozz_anim_sampling_rate PROPERTIES DEPENDS test2ozz_skel_simple)

add_test(NAME test2ozz_anim_sampling_rate_int COMMAND test2ozz "--file=${ozz_temp_directory}/good.content1" "--config={\"skeleton\":{\"filename\":\"${ozz_temp_directory}/skeleton.ozz\",\"import\":{\"enable\":false}},\"animations\":[{\"filename\":\"${ozz_temp_directory}/animation_${CMAKE_CURRENT_LIST_LINE}.ozz\",\"sampling_rate\":1}]}")
set_tests_properties(test2ozz_anim_sampling_rate_int PROPERTIES DEPENDS test2ozz_skel_simple)

add_test(NAME test2ozz_anim_sampling_rate_0 COMMAND test2ozz "--file=${ozz_temp_directory}/good.content1" "--config={\"skeleton\":{\"filename\":\"${ozz_temp_directory}/skeleton.ozz\",\"import\":{\"enable\":false}},\"animations\":[{\"filename\":\"${ozz_temp_directory}/animation_${CMAKE_CURRENT_LIST_LINE}.ozz\",\"sampling_rate\":0}]}")
set_tests_properties(test2ozz_anim_sampling_rate_0 PROPERTIES DEPENDS test2ozz_skel_simple)

add_test(NAME test2ozz_anim_sampling_rate_neg COMMAND test2ozz "--file=${ozz_temp_directory}/good.content1" "--config={\"skeleton\":{\"filename\":\"${ozz_temp_directory}/skeleton.ozz\",\"import\":{\"enable\":false}},\"animations\":[{\"filename\":\"${ozz_temp_directory}/animation_${CMAKE_CURRENT_LIST_LINE}.ozz\",\"sampling_rate\":-1}]}")
set_tests_properties(test2ozz_anim_sampling_rate_neg PROPERTIES DEPENDS test2ozz_skel_simple)

add_test(NAME test2ozz_anim_log_verbose COMMAND test2ozz "--file=${ozz_temp_directory}/good.content1" "--config={\"skeleton\":{\"filename\":\"${ozz_temp_directory}/skeleton.ozz\",\"import\":{\"enable\":false}},\"animations\":[{\"filename\":\"${ozz_temp_directory}/animation_${CMAKE_CURRENT_LIST_LINE}.ozz\"}]}" "--log_level=verbose")
set_tests_properties(test2ozz_anim_log_verbose PROPERTIES DEPENDS test2ozz_skel_simple)

add_test(NAME test2ozz_anim_single1_wild COMMAND test2ozz "--file=${ozz_temp_directory}/good.content1" "--config={\"skeleton\":{\"filename\":\"${ozz_temp_directory}/skeleton.ozz\",\"import\":{\"enable\":false}},\"animations\":[{\"filename\":\"${ozz_temp_directory}/animation_single1_wild_*.ozz\"}]}")
set_tests_properties(test2ozz_anim_single1_wild PROPERTIES DEPENDS test2ozz_skel_simple)
add_test(NAME test2ozz_anim_single1_wild_output1 COMMAND ${CMAKE_COMMAND} -E copy "${ozz_temp_directory}/animation_single1_wild_one.ozz" "${ozz_temp_directory}/animation_single1_wild_one_should_exist.ozz")
set_tests_properties(test2ozz_anim_single1_wild_output1 PROPERTIES DEPENDS test2ozz_anim_single1_wild)

add_test(NAME test2ozz_anim_multi1 COMMAND test2ozz "--file=${ozz_temp_directory}/good.content2" "--config={\"skeleton\":{\"filename\":\"${ozz_temp_directory}/skeleton.ozz\",\"import\":{\"enable\":false}},\"animations\":[{\"filename\":\"${ozz_temp_directory}/animation_multi2_*.ozz\"}]}")
set_tests_properties(test2ozz_anim_multi1 PROPERTIES DEPENDS test2ozz_skel_simple)
add_test(NAME test2ozz_anim_multi2_output1 COMMAND ${CMAKE_COMMAND} -E copy "${ozz_temp_directory}/animation_multi2_one.ozz" "${ozz_temp_directory}/animation_multi2_one_should_exist.ozz")
set_tests_properties(test2ozz_anim_multi2_output1 PROPERTIES DEPENDS test2ozz_anim_multi1)
add_test(NAME test2ozz_anim_multi2_output2 COMMAND ${CMAKE_COMMAND} -E copy "${ozz_temp_directory}/animation_multi2_TWO.ozz" "${ozz_temp_directory}/animation_multi1_TWO_should_exist.ozz")
set_tests_properties(test2ozz_anim_multi2_output2 PROPERTIES DEPENDS test2ozz_anim_multi1)

add_test(NAME test2ozz_anim_multi2_single COMMAND test2ozz "--file=${ozz_temp_directory}/good.content2" "--config={\"skeleton\":{\"filename\":\"${ozz_temp_directory}/skeleton.ozz\",\"import\":{\"enable\":false}},\"animations\":[{\"clip\":\"T?O\",\"filename\":\"${ozz_temp_directory}/animation_multi2_single.ozz\"}]}")
set_tests_properties(test2ozz_anim_multi2_single PROPERTIES DEPENDS test2ozz_skel_simple)

add_test(NAME test2ozz_anim_multi2_single_output1 COMMAND ${CMAKE_COMMAND} -E copy "${ozz_temp_directory}/animation_multi2_single.ozz" "${ozz_temp_directory}/animation_multi2_single_should_exist.ozz")
set_tests_properties(test2ozz_anim_multi2_single_output1 PROPERTIES DEPENDS test2ozz_anim_multi2_single)

add_test(NAME test2ozz_anim_multi2_multi COMMAND test2ozz "--file=${ozz_temp_directory}/good.content2" "--config={\"skeleton\":{\"filename\":\"${ozz_temp_directory}/skeleton.ozz\",\"import\":{\"enable\":false}},\"animations\":[{\"clip\":\"one\",\"filename\":\"${ozz_temp_directory}/animation_multi2_multi_one.ozz\"},{\"clip\":\"TWO\",\"filename\":\"${ozz_temp_directory}/animation_multi2_multi_TWO.ozz\"}]}")
set_tests_properties(test2ozz_anim_multi2_multi PROPERTIES DEPENDS test2ozz_skel_simple)

add_test(NAME test2ozz_anim_multi2_multi_output1 COMMAND ${CMAKE_COMMAND} -E copy "${ozz_temp_directory}/animation_multi2_multi_one.ozz" "${ozz_temp_directory}/animation_multi2_multi_one_should_exist.ozz")
set_tests_properties(test2ozz_anim_multi2_multi_output1 PROPERTIES DEPENDS test2ozz_anim_multi2_multi)
add_test(NAME test2ozz_anim_multi2_multi_output2 COMMAND ${CMAKE_COMMAND} -E copy "${ozz_temp_directory}/animation_multi2_multi_TWO.ozz" "${ozz_temp_directory}/animation_multi2_multi_TWO_should_exist.ozz")
set_tests_properties(test2ozz_anim_multi2_multi_output2 PROPERTIES DEPENDS test2ozz_anim_multi2_multi)

# Run test2ozz tracks import passing tests
#----------------------------

add_test(NAME test2ozz_anim_track_simple COMMAND test2ozz "--file=${ozz_temp_directory}/good.content1" "--config={\"skeleton\":{\"filename\":\"${ozz_temp_directory}/skeleton.ozz\",\"import\":{\"enable\":false}},\"animations\":[{\"filename\":\"${ozz_temp_directory}/animation_track_simple_*.ozz\",\"tracks\":[{\"properties\":[{\"joint_name\":\"joint0\",\"property_name\":\"property0\",\"filename\":\"${ozz_temp_directory}/track_simple.ozz\"}]}]}]}")
set_tests_properties(test2ozz_anim_track_simple PROPERTIES DEPENDS test2ozz_skel_simple)
add_test(NAME test2ozz_anim_track_simple_output COMMAND ${CMAKE_COMMAND} -E copy "${ozz_temp_directory}/track_simple.ozz" "${ozz_temp_directory}/track_simple_should_exist.ozz")
set_tests_properties(test2ozz_anim_track_simple_output PROPERTIES DEPENDS test2ozz_anim_track_simple)
add_test(NAME test2ozz_anim_track_simple_output_anim COMMAND ${CMAKE_COMMAND} -E copy "${ozz_temp_directory}/animation_track_simple_one.ozz" "${ozz_temp_directory}/animation_track_simple_one_should_exist.ozz")
set_tests_properties(test2ozz_anim_track_simple_output_anim PROPERTIES DEPENDS test2ozz_anim_track_simple)

add_test(NAME test2ozz_anim_track_simple_raw COMMAND test2ozz "--file=${ozz_temp_directory}/good.content1" "--config={\"skeleton\":{\"filename\":\"${ozz_temp_directory}/skeleton.ozz\",\"import\":{\"enable\":false}},\"animations\":[{\"filename\":\"${ozz_temp_directory}/animation_track_simple_raw_*.ozz\",\"tracks\":[{\"properties\":[{\"joint_name\":\"joint0\",\"property_name\":\"property0\",\"filename\":\"${ozz_temp_directory}/track_simple_raw.ozz\",\"raw\":true}]}]}]}")
set_tests_properties(test2ozz_anim_track_simple_raw PROPERTIES DEPENDS test2ozz_skel_simple)
add_test(NAME test2ozz_anim_track_simple_raw_output COMMAND ${CMAKE_COMMAND} -E copy "${ozz_temp_directory}/track_simple_raw.ozz" "${ozz_temp_directory}/track_simple_raw_should_exist.ozz")
set_tests_properties(test2ozz_anim_track_simple_raw_output PROPERTIES DEPENDS test2ozz_anim_track_simple_raw)
add_test(NAME test2ozz_anim_track_simple_raw_output_anim COMMAND ${CMAKE_COMMAND} -E copy "${ozz_temp_directory}/animation_track_simple_raw_one.ozz" "${ozz_temp_directory}/animation_track_simple_raw_one_should_exist.ozz")
set_tests_properties(test2ozz_anim_track_simple_raw_output_anim PROPERTIES DEPENDS test2ozz_anim_track_simple_raw)

add_test(NAME test2ozz_anim_track_simple_no_opt COMMAND test2ozz "--file=${ozz_temp_directory}/good.content1" "--config={\"skeleton\":{\"filename\":\"${ozz_temp_directory}/skeleton.ozz\",\"import\":{\"enable\":false}},\"animations\":[{\"filename\":\"${ozz_temp_directory}/animation_track_simple_no_opt_*.ozz\",\"tracks\":[{\"properties\":[{\"joint_name\":\"joint0\",\"property_name\":\"property0\",\"filename\":\"${ozz_temp_directory}/track_simple_no_opt.ozz\",\"optimize\":false}]}]}]}")
set_tests_properties(test2ozz_anim_track_simple_no_opt PROPERTIES DEPENDS test2ozz_skel_simple)
add_test(NAME test2ozz_anim_track_simple_no_opt_output COMMAND ${CMAKE_COMMAND} -E copy "${ozz_temp_directory}/track_simple_no_opt.ozz" "${ozz_temp_directory}/track_simple_no_opt_should_exist.ozz")
set_tests_properties(test2ozz_anim_track_simple_no_opt_output PROPERTIES DEPENDS test2ozz_anim_track_simple_no_opt)
add_test(NAME test2ozz_anim_track_simple_no_opt_output_anim COMMAND ${CMAKE_COMMAND} -E copy "${ozz_temp_directory}/animation_track_simple_no_opt_one.ozz" "${ozz_temp_directory}/animation_track_simple_no_opt_one_should_exist.ozz")
set_tests_properties(test2ozz_anim_track_simple_no_opt_output_anim PROPERTIES DEPENDS test2ozz_anim_track_simple_no_opt)

add_test(NAME test2ozz_anim_track_simple_no_opt_raw COMMAND test2ozz "--file=${ozz_temp_directory}/good.content1" "--config={\"skeleton\":{\"filename\":\"${ozz_temp_directory}/skeleton.ozz\",\"import\":{\"enable\":false}},\"animations\":[{\"filename\":\"${ozz_temp_directory}/animation_track_simple_no_opt_raw_*.ozz\",\"tracks\":[{\"properties\":[{\"joint_name\":\"joint0\",\"property_name\":\"property0\",\"filename\":\"${ozz_temp_directory}/track_simple_no_opt_raw.ozz\",\"optimize\":false,\"raw\":true}]}]}]}")
set_tests_properties(test2ozz_anim_track_simple_no_opt_raw PROPERTIES DEPENDS test2ozz_skel_simple)
add_test(NAME test2ozz_anim_track_simple_no_opt_raw_output COMMAND ${CMAKE_COMMAND} -E copy "${ozz_temp_directory}/track_simple_no_opt_raw.ozz" "${ozz_temp_directory}/track_simple_no_opt_raw_should_exist.ozz")
set_tests_properties(test2ozz_anim_track_simple_no_opt_raw_output PROPERTIES DEPENDS test2ozz_anim_track_simple_no_opt_raw)
add_test(NAME test2ozz_anim_track_simple_no_opt_raw_output_anim COMMAND ${CMAKE_COMMAND} -E copy "${ozz_temp_directory}/animation_track_simple_no_opt_raw_one.ozz" "${ozz_temp_directory}/animation_track_simple_no_opt_raw_one_should_exist.ozz")
set_tests_properties(test2ozz_anim_track_simple_no_opt_raw_output_anim PROPERTIES DEPENDS test2ozz_anim_track_simple_no_opt_raw)

add_test(NAME test2ozz_anim_track2_simple COMMAND test2ozz "--file=${ozz_temp_directory}/good.content1" "--config={\"skeleton\":{\"filename\":\"${ozz_temp_directory}/skeleton.ozz\",\"import\":{\"enable\":false}},\"animations\":[{\"filename\":\"${ozz_temp_directory}/animation_track2_simple_*.ozz\",\"tracks\":[{\"properties\":[{\"joint_name\":\"joint0\",\"property_name\":\"property2\",\"type\":\"float2\",\"filename\":\"${ozz_temp_directory}/track2_simple.ozz\"}]}]}]}")
set_tests_properties(test2ozz_anim_track2_simple PROPERTIES DEPENDS test2ozz_skel_simple)
add_test(NAME test2ozz_anim_track2_simple_output COMMAND ${CMAKE_COMMAND} -E copy "${ozz_temp_directory}/track2_simple.ozz" "${ozz_temp_directory}/track2_simple_should_exist.ozz")
set_tests_properties(test2ozz_anim_track2_simple_output PROPERTIES DEPENDS test2ozz_anim_track2_simple)
add_test(NAME test2ozz_anim_track2_simple_output_anim COMMAND ${CMAKE_COMMAND} -E copy "${ozz_temp_directory}/animation_track2_simple_one.ozz" "${ozz_temp_directory}/animation_track2_simple_one_should_exist.ozz")
set_tests_properties(test2ozz_anim_track2_simple_output_anim PROPERTIES DEPENDS test2ozz_anim_track2_simple)

add_test(NAME test2ozz_anim_track_wild_joint COMMAND test2ozz "--file=${ozz_temp_directory}/good.content1" "--config={\"skeleton\":{\"filename\":\"${ozz_temp_directory}/skeleton.ozz\",\"import\":{\"enable\":false}},\"animations\":[{\"filename\":\"${ozz_temp_directory}/animation_track_wild_joint_*.ozz\",\"tracks\":[{\"properties\":[{\"joint_name\":\"joint?\",\"property_name\":\"property0\",\"filename\":\"${ozz_temp_directory}/track_wild_*.ozz\"}]}]}]}")
set_tests_properties(test2ozz_anim_track_wild_joint PROPERTIES DEPENDS test2ozz_skel_simple)
add_test(NAME test2ozz_anim_track_wild_joint_output0
  COMMAND ${CMAKE_COMMAND} -E copy "${ozz_temp_directory}/track_wild_joint0-property0.ozz" "${ozz_temp_directory}/track_wild_joint0-property0_should_exist.ozz")
set_tests_properties(test2ozz_anim_track_wild_joint_output0 PROPERTIES DEPENDS test2ozz_anim_track_wild_joint)
add_test(NAME test2ozz_anim_track_wild_joint_output1
  COMMAND ${CMAKE_COMMAND} -E copy "${ozz_temp_directory}/track_wild_joint1-property0.ozz" "${ozz_temp_directory}/track_wild_joint1-property0_should_exist.ozz")
set_tests_properties(test2ozz_anim_track_wild_joint_output1 PROPERTIES DEPENDS test2ozz_anim_track_wild_joint)

# Run test2ozz both skeleton and animation import passing tests
#----------------------------

add_test(NAME test2ozz_skel_anim_simple COMMAND test2ozz "--file=${ozz_temp_directory}/good.content1" "--config={\"skeleton\":{\"filename\":\"${ozz_temp_directory}/skeleton_skel_anim.ozz\",\"import\":{\"enable\":true}},\"animations\":[{\"filename\":\"${ozz_temp_directory}/animation_skel_anim_simple.ozz\"}]}")

# Fused sources tests
#----------------------------

# ozz_animation_tools fuse tests
set_source_files_properties(${PROJECT_BINARY_DIR}/src_fused/ozz_animation_tools.cc PROPERTIES GENERATED 1)
add_executable(test_fuse_ozz_animation_tools
  test2ozz.cc
  ${PROJECT_BINARY_DIR}/src_fused/ozz_animation_tools.cc)
add_dependencies(test_fuse_ozz_animation_tools BUILD_FUSE_ozz_animation_tools)
target_link_libraries(test_fuse_ozz_animation_tools
  ozz_animation_offline
  ozz_options
  json
  gtest)

set_target_properties(test_fuse_ozz_animation_tools PROPERTIES FOLDER "ozz/tests/animation_offline")

add_test(NAME test_fuse_ozz_animation_tools_no_arg COMMAND test_fuse_ozz_animation_tools)
set_tests_properties(test_fuse_ozz_animation_tools_no_arg PROPERTIES PASS_REGULAR_EXPRESSION "Required option \"file\" is not specified.")
