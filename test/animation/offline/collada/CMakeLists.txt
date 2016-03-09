# Creates a file with an invalid content.
file(WRITE "${ozz_temp_directory}/content.bad" "bad content")

# Run dae2skel failing tests
add_test(NAME dae2skel_no_skeleton COMMAND dae2skel "--file=${ozz_media_directory}/collada/cube.dae" "--skeleton=${ozz_temp_directory}/fbx_should_not_exist.ozz")
set_tests_properties(dae2skel_no_skeleton PROPERTIES WILL_FAIL true)
add_test(NAME dae2skel_malformed_matrix COMMAND dae2skel "--file=${ozz_media_directory}/collada/malformed/malformed_matrix.dae" "--skeleton=${ozz_temp_directory}/fbx_should_not_exist.ozz")
set_tests_properties(dae2skel_malformed_matrix PROPERTIES WILL_FAIL true)
add_test(NAME dae2skel_malformed_translate COMMAND dae2skel "--file=${ozz_media_directory}/collada/malformed/malformed_translate.dae" "--skeleton=${ozz_temp_directory}/fbx_should_not_exist.ozz")
set_tests_properties(dae2skel_malformed_translate PROPERTIES WILL_FAIL true)
add_test(NAME dae2skel_malformed_rotate COMMAND dae2skel "--file=${ozz_media_directory}/collada/malformed/malformed_rotate.dae" "--skeleton=${ozz_temp_directory}/fbx_should_not_exist.ozz")
set_tests_properties(dae2skel_malformed_rotate PROPERTIES WILL_FAIL true)
add_test(NAME dae2skel_malformed_scale COMMAND dae2skel "--file=${ozz_media_directory}/collada/malformed/malformed_scale.dae" "--skeleton=${ozz_temp_directory}/fbx_should_not_exist.ozz")
set_tests_properties(dae2skel_malformed_scale PROPERTIES WILL_FAIL true)
add_test(NAME dae2skel_unsupported_skew COMMAND dae2skel "--file=${ozz_media_directory}/collada/malformed/unsupported_skew.dae" "--skeleton=${ozz_temp_directory}/fbx_should_not_exist.ozz")
set_tests_properties(dae2skel_unsupported_skew PROPERTIES WILL_FAIL true)

# Ensures nothing was outputted. Unfortunately there's no way to ensure this test is ran last
add_test(NAME dae2skel_ouput COMMAND ${CMAKE_COMMAND} -E copy "${ozz_temp_directory}/fbx_should_not_exist.ozz" "${ozz_temp_directory}/fbx_should_not_exist_too.ozz")
set_tests_properties(dae2skel_ouput PROPERTIES WILL_FAIL true)

# Run dae2skel passing tests
add_test(NAME dae2skel_simple COMMAND dae2skel "--file=${ozz_media_directory}/collada/alain/skeleton.dae" "--skeleton=${ozz_temp_directory}/dae_skeleton.ozz")
add_test(NAME dae2skel_simple_raw COMMAND dae2skel "--raw" "--file=${ozz_media_directory}/collada/alain/skeleton.dae" "--skeleton=${ozz_temp_directory}/raw_dae_skeleton.ozz")
add_test(NAME dae2skel_native COMMAND dae2skel "--file=${ozz_media_directory}/collada/alain/skeleton.dae" "--skeleton=${ozz_temp_directory}/skeleton_native.ozz" "--endian=native")
add_test(NAME dae2skel_little COMMAND dae2skel "--file=${ozz_media_directory}/collada/alain/skeleton.dae" "--skeleton=${ozz_temp_directory}/skeleton_little.ozz" "--endian=little")
add_test(NAME dae2skel_big COMMAND dae2skel "--file=${ozz_media_directory}/collada/alain/skeleton.dae" "--skeleton=${ozz_temp_directory}/skeleton_big.ozz" "--endian=big")


# Run fbx2anim failing tests
add_test(NAME dae2anim_badcontent COMMAND dae2anim "--file=${ozz_temp_directory}/content.bad" "--skeleton=${ozz_temp_directory}/dae_skeleton.ozz" "--animation=${ozz_temp_directory}/fbx_should_not_exist.ozz")
set_tests_properties(dae2anim_badcontent PROPERTIES DEPENDS dae2skel_simple)
set_tests_properties(dae2anim_badcontent PROPERTIES WILL_FAIL true)

# Ensures nothing was outputted. Unfortunately there's no way to ensure this test is ran last
add_test(NAME dae2anim_ouput COMMAND ${CMAKE_COMMAND} -E copy "${ozz_temp_directory}/fbx_should_not_exist.ozz" "${ozz_temp_directory}/fbx_should_not_exist_too.ozz")
set_tests_properties(dae2anim_ouput PROPERTIES WILL_FAIL true)

# Run dae2anim passing tests
add_test(NAME dae2anim_simple COMMAND dae2anim "--file=${ozz_media_directory}/collada/alain/atlas.dae" "--skeleton=${ozz_temp_directory}/dae_skeleton.ozz" "--animation=${ozz_temp_directory}/dae_animation_${CMAKE_CURRENT_LIST_LINE}.ozz")
set_tests_properties(dae2anim_simple PROPERTIES DEPENDS dae2skel_simple)
add_test(NAME dae2anim_simple_raw COMMAND dae2anim "--raw" "--file=${ozz_media_directory}/collada/alain/atlas.dae" "--skeleton=${ozz_temp_directory}/dae_skeleton.ozz" "--animation=${ozz_temp_directory}/dae_raw_animation_${CMAKE_CURRENT_LIST_LINE}.ozz")
set_tests_properties(dae2anim_simple_raw PROPERTIES DEPENDS dae2skel_simple)
add_test(NAME dae2anim_simple_from_raw COMMAND dae2anim "--file=${ozz_media_directory}/collada/alain/atlas.dae" "--skeleton=${ozz_temp_directory}/raw_dae_skeleton.ozz" "--animation=${ozz_temp_directory}/dae_from_raw_animation_${CMAKE_CURRENT_LIST_LINE}.ozz")
set_tests_properties(dae2anim_simple_from_raw PROPERTIES DEPENDS dae2skel_simple_raw)
add_test(NAME dae2anim_simple_raw_from_raw COMMAND dae2anim "--raw" "--file=${ozz_media_directory}/collada/alain/atlas.dae" "--skeleton=${ozz_temp_directory}/raw_dae_skeleton.ozz" "--animation=${ozz_temp_directory}/dae_raw_from_raw_animation_${CMAKE_CURRENT_LIST_LINE}.ozz")
set_tests_properties(dae2anim_simple_raw_from_raw PROPERTIES DEPENDS dae2skel_simple_raw)
add_test(NAME dae2anim_native COMMAND dae2anim "--file=${ozz_media_directory}/collada/alain/atlas.dae" "--skeleton=${ozz_temp_directory}/dae_skeleton.ozz" "--animation=${ozz_temp_directory}/dae_animation_native_${CMAKE_CURRENT_LIST_LINE}.ozz" "--endian=native")
set_tests_properties(dae2anim_native PROPERTIES DEPENDS dae2skel_simple)
add_test(NAME dae2anim_little COMMAND dae2anim "--file=${ozz_media_directory}/collada/alain/atlas.dae" "--skeleton=${ozz_temp_directory}/dae_skeleton.ozz" "--animation=${ozz_temp_directory}/dae_animation_little_${CMAKE_CURRENT_LIST_LINE}.ozz" "--endian=little")
set_tests_properties(dae2anim_little PROPERTIES DEPENDS dae2skel_simple)
add_test(NAME dae2anim_big COMMAND dae2anim "--file=${ozz_media_directory}/collada/alain/atlas.dae" "--skeleton=${ozz_temp_directory}/dae_skeleton.ozz" "--animation=${ozz_temp_directory}/dae_animation_big_${CMAKE_CURRENT_LIST_LINE}.ozz" "--endian=big")
set_tests_properties(dae2anim_big PROPERTIES DEPENDS dae2skel_simple)
add_test(NAME dae2anim_unmatch_skeleton COMMAND dae2anim "--file=${ozz_media_directory}/collada/seymour.dae" "--skeleton=${ozz_temp_directory}/dae_skeleton.ozz" "--animation=${ozz_temp_directory}/dae_animation_big_${CMAKE_CURRENT_LIST_LINE}.ozz" "--endian=big")
set_tests_properties(dae2anim_unmatch_skeleton PROPERTIES DEPENDS dae2skel_simple)
