if(NOT ozz_build_fbx)
  return()
endif()

# Creates temporary directory for tests inputs/outputs.
set(ozz_temp_directory ${CMAKE_BINARY_DIR}/temp)
file(MAKE_DIRECTORY ${ozz_temp_directory})

# Creates a file with an invalid content.
file(WRITE "${ozz_temp_directory}/content.bad" "bad content")

# Run fbx2skel passing tests
add_test(NAME fbx2skel_simple COMMAND fbx2skel "--file=${ozz_media_directory}/fbx/alain/skeleton.fbx" "--skeleton=${ozz_temp_directory}/skeleton.ozz")
add_test(NAME fbx2skel_native COMMAND fbx2skel "--file=${ozz_media_directory}/fbx/alain/skeleton.fbx" "--skeleton=${ozz_temp_directory}/skeleton_native.ozz" "--endian=native")
add_test(NAME fbx2skel_little COMMAND fbx2skel "--file=${ozz_media_directory}/fbx/alain/skeleton.fbx" "--skeleton=${ozz_temp_directory}/skeleton_little.ozz" "--endian=little")
add_test(NAME fbx2skel_big COMMAND fbx2skel "--file=${ozz_media_directory}/fbx/alain/skeleton.fbx" "--skeleton=${ozz_temp_directory}/skeleton_big.ozz" "--endian=big")

# Run fbx2anim passing tests
add_test(NAME fbx2anim_simple COMMAND fbx2anim "--file=${ozz_media_directory}/fbx/alain/walk.fbx" "--skeleton=${ozz_temp_directory}/skeleton.ozz" "--animation=${ozz_temp_directory}/animation.ozz")
set_tests_properties(fbx2anim_simple PROPERTIES DEPENDS fbx2skel_simple)
add_test(NAME fbx2anim_native COMMAND fbx2anim "--file=${ozz_media_directory}/fbx/alain/walk.fbx" "--skeleton=${ozz_temp_directory}/skeleton.ozz" "--animation=${ozz_temp_directory}/animation_native.ozz" "--endian=native")
set_tests_properties(fbx2anim_native PROPERTIES DEPENDS fbx2skel_simple)
add_test(NAME fbx2anim_little COMMAND fbx2anim "--file=${ozz_media_directory}/fbx/alain/walk.fbx" "--skeleton=${ozz_temp_directory}/skeleton.ozz" "--animation=${ozz_temp_directory}/animation_little.ozz" "--endian=little")
set_tests_properties(fbx2anim_little PROPERTIES DEPENDS fbx2skel_simple)
add_test(NAME fbx2anim_big COMMAND fbx2anim "--file=${ozz_media_directory}/fbx/alain/walk.fbx" "--skeleton=${ozz_temp_directory}/skeleton.ozz" "--animation=${ozz_temp_directory}/animation_big.ozz" "--endian=big")
set_tests_properties(fbx2anim_big PROPERTIES DEPENDS fbx2skel_simple)
