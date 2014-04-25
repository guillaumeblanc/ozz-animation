# Creates temporary directory for tests inputs/outputs.
set(ozz_temp_directory ${CMAKE_BINARY_DIR}/temp)
file(MAKE_DIRECTORY ${ozz_temp_directory})

# Creates a file with an invalid content.
file(WRITE "${ozz_temp_directory}/content.bad" "bad content")

# Run dae2skel failing tests
add_test(NAME dae2skel_malformed_matrix COMMAND dae2skel "--file=${ozz_media_directory}/collada/malformed/malformed_matrix.dae" "--skeleton=${ozz_temp_directory}/malformed_matrix.ozz")
set_tests_properties(dae2skel_malformed_matrix PROPERTIES WILL_FAIL true)
add_test(NAME dae2skel_malformed_translate COMMAND dae2skel "--file=${ozz_media_directory}/collada/malformed/malformed_translate.dae" "--skeleton=${ozz_temp_directory}/malformed_translate.ozz")
set_tests_properties(dae2skel_malformed_translate PROPERTIES WILL_FAIL true)
add_test(NAME dae2skel_malformed_rotate COMMAND dae2skel "--file=${ozz_media_directory}/collada/malformed/malformed_rotate.dae" "--skeleton=${ozz_temp_directory}/malformed_rotate.ozz")
set_tests_properties(dae2skel_malformed_rotate PROPERTIES WILL_FAIL true)
add_test(NAME dae2skel_malformed_scale COMMAND dae2skel "--file=${ozz_media_directory}/collada/malformed/malformed_scale.dae" "--skeleton=${ozz_temp_directory}/malformed_scale.ozz")
set_tests_properties(dae2skel_malformed_scale PROPERTIES WILL_FAIL true)
add_test(NAME dae2skel_unsupported_skew COMMAND dae2skel "--file=${ozz_media_directory}/collada/malformed/unsupported_skew.dae" "--skeleton=${ozz_temp_directory}/unsupported_skew.ozz")
set_tests_properties(dae2skel_unsupported_skew PROPERTIES WILL_FAIL true )

# Run dae2skel passing tests
add_test(NAME dae2skel_simple COMMAND dae2skel "--file=${ozz_media_directory}/collada/alain/skeleton.dae" "--skeleton=${ozz_temp_directory}/skeleton.ozz")
add_test(NAME dae2skel_native COMMAND dae2skel "--file=${ozz_media_directory}/collada/alain/skeleton.dae" "--skeleton=${ozz_temp_directory}/skeleton_native.ozz" "--endian=native")
add_test(NAME dae2skel_little COMMAND dae2skel "--file=${ozz_media_directory}/collada/alain/skeleton.dae" "--skeleton=${ozz_temp_directory}/skeleton_little.ozz" "--endian=little")
add_test(NAME dae2skel_big COMMAND dae2skel "--file=${ozz_media_directory}/collada/alain/skeleton.dae" "--skeleton=${ozz_temp_directory}/skeleton_big.ozz" "--endian=big")

# Run dae2anim passing tests
add_test(NAME dae2anim_simple COMMAND dae2anim "--file=${ozz_media_directory}/collada/alain/atlas.dae" "--skeleton=${ozz_temp_directory}/skeleton.ozz" "--animation=${ozz_temp_directory}/animation.ozz")
set_tests_properties(dae2anim_simple PROPERTIES DEPENDS dae2skel_simple)
add_test(NAME dae2anim_native COMMAND dae2anim "--file=${ozz_media_directory}/collada/alain/atlas.dae" "--skeleton=${ozz_temp_directory}/skeleton.ozz" "--animation=${ozz_temp_directory}/animation_native.ozz" "--endian=native")
set_tests_properties(dae2anim_native PROPERTIES DEPENDS dae2skel_simple)
add_test(NAME dae2anim_little COMMAND dae2anim "--file=${ozz_media_directory}/collada/alain/atlas.dae" "--skeleton=${ozz_temp_directory}/skeleton.ozz" "--animation=${ozz_temp_directory}/animation_little.ozz" "--endian=little")
set_tests_properties(dae2anim_little PROPERTIES DEPENDS dae2skel_simple)
add_test(NAME dae2anim_big COMMAND dae2anim "--file=${ozz_media_directory}/collada/alain/atlas.dae" "--skeleton=${ozz_temp_directory}/skeleton.ozz" "--animation=${ozz_temp_directory}/animation_big.ozz" "--endian=big")
set_tests_properties(dae2anim_big PROPERTIES DEPENDS dae2skel_simple)
add_test(NAME dae2anim_unmatch_skeleton COMMAND dae2anim "--file=${ozz_media_directory}/collada/seymour.dae" "--skeleton=${ozz_temp_directory}/skeleton.ozz" "--animation=${ozz_temp_directory}/animation_big.ozz" "--endian=big")
set_tests_properties(dae2anim_unmatch_skeleton PROPERTIES DEPENDS dae2skel_simple)
