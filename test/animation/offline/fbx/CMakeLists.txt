if(NOT ozz_build_fbx)
  return()
endif()

# Creates a file with an invalid content.
file(WRITE "${ozz_temp_directory}/content.bad" "bad content")

# Run fbx2skel failing tests
add_test(NAME fbx2skel_noarg COMMAND fbx2skel)
set_tests_properties(fbx2skel_noarg PROPERTIES WILL_FAIL true)
add_test(NAME fbx2skel_badcontent COMMAND fbx2skel "--file=${ozz_temp_directory}/content.bad" "--skeleton=${ozz_temp_directory}/fbx_should_not_exist.ozz")
set_tests_properties(fbx2skel_badcontent PROPERTIES WILL_FAIL true)
add_test(NAME fbx2skel_no_skeleton COMMAND fbx2skel "--file=${ozz_media_directory}/fbx/cube.fbx" "--skeleton=${ozz_temp_directory}/fbx_should_not_exist.ozz")
set_tests_properties(fbx2skel_no_skeleton PROPERTIES WILL_FAIL true)

# Run fbx2skel collada failing tests
add_test(NAME fbx2skel_no_skeleton_dae COMMAND fbx2skel "--file=${ozz_media_directory}/collada/cube.dae" "--skeleton=${ozz_temp_directory}/fbx_should_not_exist.ozz")
set_tests_properties(fbx2skel_no_skeleton_dae PROPERTIES WILL_FAIL true)

# Ensures nothing was outputted.
add_test(NAME fbx2skel_ouput COMMAND ${CMAKE_COMMAND} -E copy "${ozz_temp_directory}/fbx_should_not_exist.ozz" "${ozz_temp_directory}/fbx_should_not_exist_too.ozz")
set_tests_properties(fbx2skel_ouput PROPERTIES WILL_FAIL true)
set_tests_properties(fbx2skel_ouput PROPERTIES
  DEPENDS "fbx2skel_badcontent
           fbx2skel_no_skeleton
           fbx2skel_no_skeleton_dae")

# Run fbx2skel passing tests
add_test(NAME fbx2skel_version COMMAND fbx2skel "--version")
add_test(NAME fbx2skel_simple COMMAND fbx2skel "--file=${ozz_media_directory}/fbx/alain/skeleton.fbx" "--skeleton=${ozz_temp_directory}/fbx_skeleton.ozz")
add_test(NAME fbx2skel_simple_raw COMMAND fbx2skel "--raw" "--file=${ozz_media_directory}/fbx/alain/skeleton.fbx" "--skeleton=${ozz_temp_directory}/raw_fbx_skeleton.ozz")
add_test(NAME fbx2skel_little COMMAND fbx2skel "--file=${ozz_media_directory}/fbx/alain/skeleton.fbx" "--skeleton=${ozz_temp_directory}/skeleton_little.ozz" "--endian=little")
add_test(NAME fbx2skel_big COMMAND fbx2skel "--file=${ozz_media_directory}/fbx/alain/skeleton.fbx" "--skeleton=${ozz_temp_directory}/skeleton_big.ozz" "--endian=big")

# Run fbx2skel collada passing tests
add_test(NAME fbx2skel_simple_dae_astro_max COMMAND fbx2skel "--file=${ozz_media_directory}/collada/astro_max.dae" "--skeleton=${ozz_temp_directory}/astro_max_skeleton.ozz")
add_test(NAME fbx2skel_simple_dae_astro_maya COMMAND fbx2skel "--file=${ozz_media_directory}/collada/astro_maya.dae" "--skeleton=${ozz_temp_directory}/astro_maya_skeleton.ozz")
add_test(NAME fbx2skel_simple_dae_seymour COMMAND fbx2skel "--file=${ozz_media_directory}/collada/seymour.dae" "--skeleton=${ozz_temp_directory}/seymour_skeleton.ozz")

# Run fbx2anim failing tests
add_test(NAME fbx2anim_noarg COMMAND fbx2anim)
set_tests_properties(fbx2anim_noarg PROPERTIES WILL_FAIL true)
add_test(NAME fbx2anim_badcontent COMMAND fbx2anim "--file=${ozz_temp_directory}/content.bad" "--skeleton=${ozz_temp_directory}/fbx_skeleton.ozz" "--animation=${ozz_temp_directory}/fbx_should_not_exist.ozz")
set_tests_properties(fbx2anim_badcontent PROPERTIES DEPENDS fbx2skel_simple)
set_tests_properties(fbx2anim_badcontent PROPERTIES WILL_FAIL true)

# Run fbx2anim passing tests
add_test(NAME fbx2anim_version COMMAND fbx2anim "--version")
add_test(NAME fbx2anim_simple COMMAND fbx2anim "--file=${ozz_media_directory}/fbx/alain/run.fbx" "--skeleton=${ozz_temp_directory}/fbx_skeleton.ozz" "--animation=${ozz_temp_directory}/fbx_animation_${CMAKE_CURRENT_LIST_LINE}.ozz")
set_tests_properties(fbx2anim_simple PROPERTIES DEPENDS fbx2skel_simple)
add_test(NAME fbx2anim_simple_raw COMMAND fbx2anim "--raw" "--file=${ozz_media_directory}/fbx/alain/run.fbx" "--skeleton=${ozz_temp_directory}/fbx_skeleton.ozz" "--animation=${ozz_temp_directory}/fbx_raw_animation_${CMAKE_CURRENT_LIST_LINE}.ozz")
set_tests_properties(fbx2anim_simple_raw PROPERTIES DEPENDS fbx2skel_simple)
add_test(NAME fbx2anim_simple_from_raw COMMAND fbx2anim "--file=${ozz_media_directory}/fbx/alain/run.fbx" "--skeleton=${ozz_temp_directory}/raw_fbx_skeleton.ozz" "--animation=${ozz_temp_directory}/fbx_from_raw_animation_${CMAKE_CURRENT_LIST_LINE}.ozz")
set_tests_properties(fbx2anim_simple_from_raw PROPERTIES DEPENDS fbx2skel_simple_raw)
add_test(NAME fbx2anim_simple_raw_from_raw COMMAND fbx2anim "--raw" "--file=${ozz_media_directory}/fbx/alain/run.fbx" "--skeleton=${ozz_temp_directory}/raw_fbx_skeleton.ozz" "--animation=${ozz_temp_directory}/fbx_raw_from_row_animation_${CMAKE_CURRENT_LIST_LINE}.ozz")
set_tests_properties(fbx2anim_simple_raw_from_raw PROPERTIES DEPENDS fbx2skel_simple_raw)
add_test(NAME fbx2anim_little COMMAND fbx2anim "--file=${ozz_media_directory}/fbx/alain/run.fbx" "--skeleton=${ozz_temp_directory}/fbx_skeleton.ozz" "--animation=${ozz_temp_directory}/fbx_animation_little_${CMAKE_CURRENT_LIST_LINE}.ozz" "--endian=little")
set_tests_properties(fbx2anim_little PROPERTIES DEPENDS fbx2skel_simple)
add_test(NAME fbx2anim_big COMMAND fbx2anim "--file=${ozz_media_directory}/fbx/alain/run.fbx" "--skeleton=${ozz_temp_directory}/fbx_skeleton.ozz" "--animation=${ozz_temp_directory}/fbx_animation_big_${CMAKE_CURRENT_LIST_LINE}.ozz" "--endian=big")
set_tests_properties(fbx2anim_big PROPERTIES DEPENDS fbx2skel_simple)

# Run fbx2anim collada passing tests
add_test(NAME fbx2anim_simple_dae_astro_max COMMAND fbx2anim "--file=${ozz_media_directory}/collada/astro_max.dae" "--skeleton=${ozz_temp_directory}/astro_max_skeleton.ozz" "--animation=${ozz_temp_directory}/dae_animation_${CMAKE_CURRENT_LIST_LINE}.ozz")
set_tests_properties(fbx2anim_simple_dae_astro_max PROPERTIES DEPENDS fbx2skel_simple_dae_astro_max)
add_test(NAME fbx2anim_simple_dae_astro_maya COMMAND fbx2anim "--file=${ozz_media_directory}/collada/astro_maya.dae" "--skeleton=${ozz_temp_directory}/astro_maya_skeleton.ozz" "--animation=${ozz_temp_directory}/dae_animation_${CMAKE_CURRENT_LIST_LINE}.ozz")
set_tests_properties(fbx2anim_simple_dae_astro_maya PROPERTIES DEPENDS fbx2skel_simple_dae_astro_maya)
add_test(NAME fbx2anim_simple_dae_seymour COMMAND fbx2anim "--file=${ozz_media_directory}/collada/seymour.dae" "--skeleton=${ozz_temp_directory}/seymour_skeleton.ozz" "--animation=${ozz_temp_directory}/dae_animation_${CMAKE_CURRENT_LIST_LINE}.ozz")
set_tests_properties(fbx2anim_simple_dae_seymour PROPERTIES DEPENDS fbx2skel_simple_dae_seymour)

# Uses the sample playback to test other file format and special cases.
if(TARGET sample_playback)
  add_custom_command(
    DEPENDS fbx2skel
            fbx2anim
            "${ozz_media_directory}/collada/alain/skeleton.dae"
            "${ozz_media_directory}/collada/alain/skeleton_zup.dae"
            "${ozz_media_directory}/collada/alain/walk.dae"
    OUTPUT  "${ozz_temp_directory}/alain_skeleton_dae.ozz"
            "${ozz_temp_directory}/alain_skeleton_dae_zup.ozz"
            "${ozz_temp_directory}/alain_walk_dae.ozz"
    COMMAND fbx2skel
      "--file=${ozz_media_directory}/collada/alain/skeleton.dae"
      "--skeleton=${ozz_temp_directory}/alain_skeleton_dae.ozz"
    COMMAND fbx2skel
      "--file=${ozz_media_directory}/collada/alain/skeleton_zup.dae"
      "--skeleton=${ozz_temp_directory}/alain_skeleton_dae_zup.ozz"
    COMMAND fbx2anim
      "--file=${ozz_media_directory}/collada/alain/walk.dae"
      "--skeleton=${ozz_temp_directory}/alain_skeleton_dae.ozz"
      "--animation=${ozz_temp_directory}/alain_walk_dae.ozz")
      
  add_custom_target(BUILD_DATA_TEST ALL DEPENDS
    "${ozz_temp_directory}/alain_skeleton_dae.ozz"
    "${ozz_temp_directory}/alain_skeleton_dae_zup.ozz"
    "${ozz_temp_directory}/alain_walk_dae.ozz")

  add_test(NAME sample_playback_dae_zup COMMAND sample_playback  "--skeleton=${ozz_temp_directory}/alain_skeleton_dae_zup.ozz" "--animation=${ozz_temp_directory}/alain_walk_dae.ozz" "--max_idle_loops=${ozz_sample_testing_loops}" $<$<BOOL:${ozz_run_tests_headless}>:--norender>)

  add_test(NAME sample_playback_dae_seymour COMMAND sample_playback  "--skeleton=${ozz_media_directory}/bin/seymour_skeleton.ozz" "--animation=${ozz_media_directory}/bin/seymour_animation.ozz" "--max_idle_loops=${ozz_sample_testing_loops}" $<$<BOOL:${ozz_run_tests_headless}>:--norender>)
  add_test(NAME sample_playback_dae_astro_max COMMAND sample_playback  "--skeleton=${ozz_media_directory}/bin/astro_max_skeleton.ozz" "--animation=${ozz_media_directory}/bin/astro_max_animation.ozz" "--max_idle_loops=${ozz_sample_testing_loops}" $<$<BOOL:${ozz_run_tests_headless}>:--norender>)
  add_test(NAME sample_playback_dae_astro_maya COMMAND sample_playback  "--skeleton=${ozz_media_directory}/bin/astro_maya_skeleton.ozz" "--animation=${ozz_media_directory}/bin/astro_maya_animation.ozz" "--max_idle_loops=${ozz_sample_testing_loops}" $<$<BOOL:${ozz_run_tests_headless}>:--norender>)
  
  add_test(NAME sample_playback_mixed_df COMMAND sample_playback "--skeleton=${ozz_temp_directory}/alain_skeleton_dae.ozz" "--animation=${ozz_media_directory}/bin/alain_walk.ozz" "--max_idle_loops=${ozz_sample_testing_loops}" $<$<BOOL:${ozz_run_tests_headless}>:--norender>)
  add_test(NAME sample_playback_mixed_fd COMMAND sample_playback "--skeleton=${ozz_media_directory}/bin/alain_skeleton.ozz" "--animation=${ozz_temp_directory}/alain_walk_dae.ozz" "--max_idle_loops=${ozz_sample_testing_loops}" $<$<BOOL:${ozz_run_tests_headless}>:--norender>)
endif()

# ozz_animation_fbx fuse tests
add_executable(test_fuse_ozz_animation_fbx
  ${PROJECT_SOURCE_DIR}/src/animation/offline/fbx/fbx2skel.cc
  ${PROJECT_SOURCE_DIR}/src_fused/ozz_animation_fbx.cc)
target_include_directories(test_fuse_ozz_animation_fbx
  PUBLIC ${FBX_INCLUDE_DIRS})
target_link_libraries(test_fuse_ozz_animation_fbx
  debug ${FBX_LIBRARIES_DEBUG}
  optimized ${FBX_LIBRARIES}
  ozz_animation_offline_skel_tools
  ozz_options)
add_dependencies(test_fuse_ozz_animation_fbx BUILD_FUSE_ozz_animation_fbx)
set_target_properties(test_fuse_ozz_animation_fbx PROPERTIES FOLDER "ozz/tests/animation_offline")

add_test(NAME test_fuse_ozz_animation_fbx_no_arg COMMAND test_fuse_ozz_animation_fbx)
set_tests_properties(test_fuse_ozz_animation_fbx_no_arg PROPERTIES WILL_FAIL true)