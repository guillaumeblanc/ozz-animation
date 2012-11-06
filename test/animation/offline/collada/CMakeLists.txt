# Creates temporary directory for tests inputs/outputs.
set(ozz_temp_directory ${CMAKE_BINARY_DIR}/temp)
file(MAKE_DIRECTORY ${ozz_temp_directory})

# Creates a file with an invalid content.
file(WRITE "${ozz_temp_directory}/content.bad" "bad content")

# Run dae2skel failing tests
add_test(NAME dae2skel_missing_dae COMMAND dae2skel "--skeleton=${ozz_temp_directory}/should_not_exist.ozz")
set_tests_properties(dae2skel_missing_dae PROPERTIES WILL_FAIL true)
add_test(NAME dae2skel_missing_ozz COMMAND dae2skel "--collada=${ozz_media_directory}/collada/should_not_exist.dae")
set_tests_properties(dae2skel_missing_ozz PROPERTIES WILL_FAIL true)
add_test(NAME dae2skel_bad_endian COMMAND dae2skel "--collada=${ozz_media_directory}/collada/skeleton_yup.dae" "--skeleton=${ozz_temp_directory}/should_not_exist.ozz" "--endian==fat")
set_tests_properties(dae2skel_bad_endian PROPERTIES WILL_FAIL true)
add_test(NAME dae2skel_bad_content COMMAND dae2skel "--collada=${ozz_media_directory}/collada/content.bad" "--skeleton=${ozz_temp_directory}/should_not_exist.ozz")
set_tests_properties(dae2skel_bad_content PROPERTIES WILL_FAIL true)
add_test(NAME dae2skel_unexisting_dae COMMAND dae2skel "--collada=${ozz_media_directory}/collada/unexisting.dae" "--skeleton=${ozz_temp_directory}/skeleton_yup.ozz")
set_tests_properties(dae2skel_unexisting_dae PROPERTIES WILL_FAIL true)
add_test(NAME dae2skel_invalid_output_path COMMAND dae2skel "--collada=${ozz_media_directory}/collada/skeleton_yup.dae" "--skeleton=${ozz_temp_directory}/invalid_path/skeleton_yup.ozz")
set_tests_properties(dae2skel_invalid_output_path PROPERTIES WILL_FAIL true)

# Run dae2skel passing tests
add_test(NAME dae2skel_simple COMMAND dae2skel "--collada=${ozz_media_directory}/collada/skeleton_yup.dae" "--skeleton=${ozz_temp_directory}/skeleton_yup.ozz")
add_test(NAME dae2skel_native COMMAND dae2skel "--collada=${ozz_media_directory}/collada/skeleton_yup.dae" "--skeleton=${ozz_temp_directory}/skeleton_yup_native.ozz" "--endian=native")
add_test(NAME dae2skel_little COMMAND dae2skel "--collada=${ozz_media_directory}/collada/skeleton_yup.dae" "--skeleton=${ozz_temp_directory}/skeleton_yup_little.ozz" "--endian=little")
add_test(NAME dae2skel_big COMMAND dae2skel "--collada=${ozz_media_directory}/collada/skeleton_yup.dae" "--skeleton=${ozz_temp_directory}/skeleton_yup_big.ozz" "--endian=big")

# Run dae2anim failing tests
add_test(NAME dae2anim_missing_dae COMMAND dae2anim "--skeleton=${ozz_temp_directory}/skeleton_yup.ozz" "--animation=${ozz_temp_directory}/animation.ozz")
set_tests_properties(dae2anim_missing_dae PROPERTIES WILL_FAIL true DEPENDS dae2skel_simple)
add_test(NAME dae2anim_missing_skeleton COMMAND dae2anim "--collada=${ozz_media_directory}/collada/animation.dae" "--animation=${ozz_temp_directory}/animation.ozz")
set_tests_properties(dae2anim_missing_skeleton PROPERTIES WILL_FAIL true)
add_test(NAME dae2anim_missing_ozz COMMAND dae2anim "--collada=${ozz_media_directory}/collada/animation.dae" "--skeleton=${ozz_temp_directory}/skeleton_yup.ozz")
set_tests_properties(dae2anim_missing_ozz PROPERTIES WILL_FAIL true DEPENDS dae2skel_simple)
add_test(NAME dae2anim_bad_dae COMMAND dae2anim "--collada=${ozz_media_directory}/collada/content.bad" "--skeleton=${ozz_temp_directory}/skeleton_yup.ozz")
set_tests_properties(dae2anim_bad_dae PROPERTIES WILL_FAIL true DEPENDS dae2skel_simple)
add_test(NAME dae2anim_bad_skeleton COMMAND dae2anim "--collada=${ozz_media_directory}/collada/animation.dae" "--skeleton=${ozz_temp_directory}/content.bad")
set_tests_properties(dae2anim_bad_skeleton PROPERTIES WILL_FAIL true)
add_test(NAME dae2anim_unexisting_dae COMMAND dae2anim "--collada=${ozz_media_directory}/collada/unexiting.dae" "--skeleton=${ozz_temp_directory}/skeleton_yup.ozz" "--animation=${ozz_temp_directory}/animation_big.ozz" "--endian=big")
set_tests_properties(dae2anim_unexisting_dae PROPERTIES WILL_FAIL true DEPENDS dae2skel_simple)
add_test(NAME dae2anim_unexisting_skeleton COMMAND dae2anim "--collada=${ozz_media_directory}/collada/animation.dae" "--skeleton=${ozz_temp_directory}/unexiting.ozz" "--animation=${ozz_temp_directory}/animation_big.ozz" "--endian=big")
set_tests_properties(dae2anim_unexisting_skeleton PROPERTIES WILL_FAIL true)
add_test(NAME dae2anim_invalid_output_path COMMAND dae2anim "--collada=${ozz_media_directory}/collada/animation.dae" "--skeleton=${ozz_temp_directory}/skeleton_yup.ozz" "--animation=${ozz_temp_directory}/invalid_path/animation.ozz")
set_tests_properties(dae2anim_invalid_output_path PROPERTIES WILL_FAIL true DEPENDS dae2skel_simple)

# Run dae2anim passing tests
add_test(NAME dae2anim_simple COMMAND dae2anim "--collada=${ozz_media_directory}/collada/animation.dae" "--skeleton=${ozz_temp_directory}/skeleton_yup.ozz" "--animation=${ozz_temp_directory}/animation.ozz")
set_tests_properties(dae2anim_simple PROPERTIES DEPENDS dae2skel_simple)
add_test(NAME dae2anim_native COMMAND dae2anim "--collada=${ozz_media_directory}/collada/animation.dae" "--skeleton=${ozz_temp_directory}/skeleton_yup.ozz" "--animation=${ozz_temp_directory}/animation_native.ozz" "--endian=native")
set_tests_properties(dae2anim_native PROPERTIES DEPENDS dae2skel_simple)
add_test(NAME dae2anim_little COMMAND dae2anim "--collada=${ozz_media_directory}/collada/animation.dae" "--skeleton=${ozz_temp_directory}/skeleton_yup.ozz" "--animation=${ozz_temp_directory}/animation_little.ozz" "--endian=little")
set_tests_properties(dae2anim_little PROPERTIES DEPENDS dae2skel_simple)
add_test(NAME dae2anim_big COMMAND dae2anim "--collada=${ozz_media_directory}/collada/animation.dae" "--skeleton=${ozz_temp_directory}/skeleton_yup.ozz" "--animation=${ozz_temp_directory}/animation_big.ozz" "--endian=big")
set_tests_properties(dae2anim_big PROPERTIES DEPENDS dae2skel_simple)
add_test(NAME dae2anim_unmatch_skeleton COMMAND dae2anim "--collada=${ozz_media_directory}/collada/seymour.dae" "--skeleton=${ozz_temp_directory}/skeleton_yup.ozz" "--animation=${ozz_temp_directory}/animation_big.ozz" "--endian=big")
set_tests_properties(dae2anim_unmatch_skeleton PROPERTIES DEPENDS dae2skel_simple)
