if(NOT ozz_build_fbx)
  return()
endif()

# Creates a file with an invalid content.
file(WRITE "${ozz_temp_directory}/content.bad" "bad content")

# Run fbx2skel failing tests
add_test(NAME fbx2skel_badcontent COMMAND fbx2skel "--file=${ozz_temp_directory}/content.bad" "--skeleton=${ozz_temp_directory}/should_not_exist.ozz")
set_tests_properties(fbx2skel_badcontent PROPERTIES WILL_FAIL true)
add_test(NAME fbx2skel_no_skeleton COMMAND fbx2skel "--file=${ozz_media_directory}/fbx/cube.fbx" "--skeleton=${ozz_temp_directory}/should_not_exist.ozz")
set_tests_properties(fbx2skel_no_skeleton PROPERTIES WILL_FAIL true)

# Ensures nothing was outputted. Unfortunately there's no way to ensure this test is ran last
add_test(NAME fbx2skel_ouput COMMAND ${CMAKE_COMMAND} -E copy "${ozz_temp_directory}/should_not_exist.ozz" "${ozz_temp_directory}/should_not_exist_too.ozz")
set_tests_properties(fbx2skel_ouput PROPERTIES WILL_FAIL true)

# Run fbx2skel passing tests
add_test(NAME fbx2skel_simple COMMAND fbx2skel "--file=${ozz_media_directory}/fbx/alain/skeleton.fbx" "--skeleton=${ozz_temp_directory}/skeleton.ozz")
add_test(NAME fbx2skel_simple_raw COMMAND fbx2skel "--raw" "--file=${ozz_media_directory}/fbx/alain/skeleton.fbx" "--skeleton=${ozz_temp_directory}/raw_skeleton.ozz")
add_test(NAME fbx2skel_native COMMAND fbx2skel "--file=${ozz_media_directory}/fbx/alain/skeleton.fbx" "--skeleton=${ozz_temp_directory}/skeleton_native.ozz" "--endian=native")
add_test(NAME fbx2skel_little COMMAND fbx2skel "--file=${ozz_media_directory}/fbx/alain/skeleton.fbx" "--skeleton=${ozz_temp_directory}/skeleton_little.ozz" "--endian=little")
add_test(NAME fbx2skel_big COMMAND fbx2skel "--file=${ozz_media_directory}/fbx/alain/skeleton.fbx" "--skeleton=${ozz_temp_directory}/skeleton_big.ozz" "--endian=big")

# Run fbx2anim failing tests
add_test(NAME fbx2anim_badcontent COMMAND fbx2anim "--file=${ozz_temp_directory}/content.bad" "--skeleton=${ozz_temp_directory}/skeleton.ozz" "--animation=${ozz_temp_directory}/should_not_exist.ozz")
set_tests_properties(fbx2anim_badcontent PROPERTIES DEPENDS fbx2skel_simple)
set_tests_properties(fbx2anim_badcontent PROPERTIES WILL_FAIL true)

# Ensures nothing was outputted. Unfortunately there's no way to ensure this test is ran last
add_test(NAME fbx2anim_ouput COMMAND ${CMAKE_COMMAND} -E copy "${ozz_temp_directory}/should_not_exist.ozz" "${ozz_temp_directory}/should_not_exist_too.ozz")
set_tests_properties(fbx2anim_ouput PROPERTIES WILL_FAIL true)

# Run fbx2anim passing tests
add_test(NAME fbx2anim_simple COMMAND fbx2anim "--file=${ozz_media_directory}/fbx/alain/walk.fbx" "--skeleton=${ozz_temp_directory}/skeleton.ozz" "--animation=${ozz_temp_directory}/animation.ozz")
set_tests_properties(fbx2anim_simple PROPERTIES DEPENDS fbx2skel_simple)
add_test(NAME fbx2anim_simple_raw COMMAND fbx2anim "--raw" "--file=${ozz_media_directory}/fbx/alain/walk.fbx" "--skeleton=${ozz_temp_directory}/skeleton.ozz" "--animation=${ozz_temp_directory}/raw_animation.ozz")
set_tests_properties(fbx2anim_simple_raw PROPERTIES DEPENDS fbx2skel_simple)
add_test(NAME fbx2anim_simple_from_raw COMMAND fbx2anim "--file=${ozz_media_directory}/fbx/alain/walk.fbx" "--skeleton=${ozz_temp_directory}/raw_skeleton.ozz" "--animation=${ozz_temp_directory}/from_raw_animation.ozz")
set_tests_properties(fbx2anim_simple_from_raw PROPERTIES DEPENDS fbx2skel_simple_raw)
add_test(NAME fbx2anim_simple_raw_from_raw COMMAND fbx2anim "--raw" "--file=${ozz_media_directory}/fbx/alain/walk.fbx" "--skeleton=${ozz_temp_directory}/raw_skeleton.ozz" "--animation=${ozz_temp_directory}/raw_from_row_animation.ozz")
set_tests_properties(fbx2anim_simple_raw_from_raw PROPERTIES DEPENDS fbx2skel_simple_raw)
add_test(NAME fbx2anim_native COMMAND fbx2anim "--file=${ozz_media_directory}/fbx/alain/walk.fbx" "--skeleton=${ozz_temp_directory}/skeleton.ozz" "--animation=${ozz_temp_directory}/animation_native.ozz" "--endian=native")
set_tests_properties(fbx2anim_native PROPERTIES DEPENDS fbx2skel_simple)
add_test(NAME fbx2anim_little COMMAND fbx2anim "--file=${ozz_media_directory}/fbx/alain/walk.fbx" "--skeleton=${ozz_temp_directory}/skeleton.ozz" "--animation=${ozz_temp_directory}/animation_little.ozz" "--endian=little")
set_tests_properties(fbx2anim_little PROPERTIES DEPENDS fbx2skel_simple)
add_test(NAME fbx2anim_big COMMAND fbx2anim "--file=${ozz_media_directory}/fbx/alain/walk.fbx" "--skeleton=${ozz_temp_directory}/skeleton.ozz" "--animation=${ozz_temp_directory}/animation_big.ozz" "--endian=big")
set_tests_properties(fbx2anim_big PROPERTIES DEPENDS fbx2skel_simple)
