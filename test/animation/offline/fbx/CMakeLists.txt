if(NOT ozz_build_fbx)
  return()
endif()

# Creates a file with an invalid content.
file(WRITE "${ozz_temp_directory}/bad.fbx" "bad content")

# Run fbx2ozz generic failing tests
#----------------------------

add_test(NAME fbx2ozz_noarg COMMAND fbx2ozz)
add_test(NAME fbx2ozz_version COMMAND fbx2ozz "--version")
set_tests_properties(fbx2ozz_noarg PROPERTIES PASS_REGULAR_EXPRESSION "Required option \"file\" is not specified.")
add_test(NAME fbx2ozz_bad_content COMMAND fbx2ozz "--file=${ozz_temp_directory}/bad.fbx" )
set_tests_properties(fbx2ozz_bad_content PROPERTIES PASS_REGULAR_EXPRESSION "Failed to import file \"${ozz_temp_directory}/bad.fbx\".")

# Run fbx2ozz skeleton failing tests
#----------------------------

add_test(NAME fbx2ozz_skel_no_skeleton COMMAND fbx2ozz "--file=${ozz_media_directory}/fbx/cube.fbx" "--config={\"skeleton\":{\"output\":\"${ozz_temp_directory}/fbx_should_not_exist.ozz\"}}")
set_tests_properties(fbx2ozz_skel_no_skeleton PROPERTIES PASS_REGULAR_EXPRESSION "Failed to import skeleton.")

# Run fbx2ozz collada failing tests
#----------------------------

add_test(NAME fbx2ozz_skel_no_skeleton_dae COMMAND fbx2ozz "--file=${ozz_media_directory}/collada/cube.dae" "--config={\"skeleton\":{\"output\":\"${ozz_temp_directory}/fbx_should_not_exist.ozz\"}}")
set_tests_properties(fbx2ozz_skel_no_skeleton_dae PROPERTIES PASS_REGULAR_EXPRESSION "Failed to import skeleton.")

# Ensures nothing was outputted.
add_test(NAME fbx2ozz_skel_ouput COMMAND ${CMAKE_COMMAND} -E copy "${ozz_temp_directory}/fbx_should_not_exist.ozz" "${ozz_temp_directory}/fbx_should_not_exist_too.ozz")
set_tests_properties(fbx2ozz_skel_ouput PROPERTIES WILL_FAIL true)
set_tests_properties(fbx2ozz_skel_ouput PROPERTIES
  DEPENDS "fbx2ozz_skel_no_skeleton
           fbx2ozz_skel_no_skeleton_dae")

# Run fbx2ozz skeleton passing tests
#----------------------------

add_test(NAME fbx2ozz_skel_simple COMMAND fbx2ozz "--file=${ozz_media_directory}/fbx/alain/skeleton.fbx" "--config={\"skeleton\":{\"output\":\"${ozz_temp_directory}/fbx_skeleton.ozz\"}}")
add_test(NAME fbx2ozz_skel_simple_raw COMMAND fbx2ozz "--file=${ozz_media_directory}/fbx/alain/skeleton.fbx" "--config={\"skeleton\":{\"output\":\"${ozz_temp_directory}/fbx_raw_skeleton.ozz\",\"raw\":true}}")

# Run fbx2ozz collada skeleton passing tests
#----------------------------

add_test(NAME fbx2ozz_skel_simple_dae_astro_max COMMAND fbx2ozz "--file=${ozz_media_directory}/collada/astro_max.dae" "--config={\"skeleton\":{\"output\":\"${ozz_temp_directory}/astro_max_skeleton.ozz\"}}")
add_test(NAME fbx2ozz_skel_simple_dae_astro_maya COMMAND fbx2ozz "--file=${ozz_media_directory}/collada/astro_maya.dae" "--config={\"skeleton\":{\"output\":\"${ozz_temp_directory}/astro_maya_skeleton.ozz\"}}")
add_test(NAME fbx2ozz_skel_simple_dae_seymour COMMAND fbx2ozz "--file=${ozz_media_directory}/collada/seymour.dae" "--config={\"skeleton\":{\"output\":\"${ozz_temp_directory}/seymour_skeleton.ozz\"}}")

# Run fbx2ozz animation failing tests
#----------------------------

add_test(NAME fbx2ozz_anim_noarg COMMAND fbx2ozz)
set_tests_properties(fbx2ozz_anim_noarg PROPERTIES PASS_REGULAR_EXPRESSION "Required option \"file\" is not specified.")
add_test(NAME fbx2ozz_anim_badcontent COMMAND fbx2ozz "--file=${ozz_temp_directory}/bad.fbx")
set_tests_properties(fbx2ozz_anim_badcontent PROPERTIES DEPENDS fbx2ozz_skel_simple)
set_tests_properties(fbx2ozz_anim_badcontent PROPERTIES PASS_REGULAR_EXPRESSION "FbxImporter initialization failed with error: Unexpected file type")

# Run fbx2ozz animation passing tests
#----------------------------

add_test(NAME fbx2ozz_anim_simple COMMAND fbx2ozz "--file=${ozz_media_directory}/fbx/alain/run.fbx" "--config={\"animations\":[{\"skeleton\":\"${ozz_temp_directory}/fbx_skeleton.ozz\",\"output\":\"${ozz_temp_directory}/fbx_animation_${CMAKE_CURRENT_LIST_LINE}.ozz\"}]}")
set_tests_properties(fbx2ozz_anim_simple PROPERTIES DEPENDS fbx2ozz_skel_simple)
add_test(NAME fbx2ozz_anim_simple_raw COMMAND fbx2ozz "--file=${ozz_media_directory}/fbx/alain/run.fbx" "--config={\"animations\":[{\"skeleton\":\"${ozz_temp_directory}/fbx_skeleton.ozz\",\"output\":\"${ozz_temp_directory}/fbx_animation_${CMAKE_CURRENT_LIST_LINE}.ozz\",\"raw\":true}]}")
set_tests_properties(fbx2ozz_anim_simple_raw PROPERTIES DEPENDS fbx2ozz_skel_simple)
add_test(NAME fbx2ozz_anim_simple_from_raw COMMAND fbx2ozz "--file=${ozz_media_directory}/fbx/alain/run.fbx" "--config={\"animations\":[{\"skeleton\":\"${ozz_temp_directory}/fbx_raw_skeleton.ozz\",\"output\":\"${ozz_temp_directory}/fbx_animation_${CMAKE_CURRENT_LIST_LINE}.ozz\"}]}")
set_tests_properties(fbx2ozz_anim_simple_from_raw PROPERTIES DEPENDS fbx2ozz_skel_simple_raw)
add_test(NAME fbx2ozz_anim_simple_raw_from_raw COMMAND fbx2ozz "--file=${ozz_media_directory}/fbx/alain/run.fbx" "--config={\"animations\":[{\"skeleton\":\"${ozz_temp_directory}/fbx_raw_skeleton.ozz\",\"output\":\"${ozz_temp_directory}/fbx_animation_${CMAKE_CURRENT_LIST_LINE}.ozz\",\"raw\":true}]}")
set_tests_properties(fbx2ozz_anim_simple_raw_from_raw PROPERTIES DEPENDS fbx2ozz_skel_simple_raw)
add_test(NAME fbx2ozz_anim_little COMMAND fbx2ozz "--file=${ozz_media_directory}/fbx/alain/run.fbx" "--config={\"animations\":[{\"skeleton\":\"${ozz_temp_directory}/fbx_skeleton.ozz\",\"output\":\"${ozz_temp_directory}/fbx_animation_${CMAKE_CURRENT_LIST_LINE}.ozz\"}]}" "--endian=little")
set_tests_properties(fbx2ozz_anim_little PROPERTIES DEPENDS fbx2ozz_skel_simple)
add_test(NAME fbx2ozz_anim_big COMMAND fbx2ozz "--file=${ozz_media_directory}/fbx/alain/run.fbx" "--config={\"animations\":[{\"skeleton\":\"${ozz_temp_directory}/fbx_skeleton.ozz\",\"output\":\"${ozz_temp_directory}/fbx_animation_${CMAKE_CURRENT_LIST_LINE}.ozz\"}]}" "--endian=big")
set_tests_properties(fbx2ozz_anim_big PROPERTIES DEPENDS fbx2ozz_skel_simple)

# Run fbx2ozz track passing tests
#----------------------------

configure_file("${CMAKE_CURRENT_LIST_DIR}/properties.json.cmake" "${ozz_temp_directory}/properties.json")

add_test(NAME fbx2ozz_anim_properties COMMAND fbx2ozz "--file=${ozz_media_directory}/fbx/properties.fbx" "--config_file=${ozz_temp_directory}/properties.json")
set_tests_properties(fbx2ozz_anim_properties PROPERTIES DEPENDS fbx2ozz_skel_properties)

# Ensures all tracks were outputted.
file(MAKE_DIRECTORY ${ozz_temp_directory}/fbx_properties_tracks_cp)
add_test(NAME fbx2ozz_anim_properties_ouput COMMAND ${CMAKE_COMMAND} -E copy
  "${ozz_temp_directory}/fbx_properties_track1_animated-ozz_curve_bool.ozz"
  "${ozz_temp_directory}/fbx_properties_track1_animated-ozz_curve_int.ozz"
  "${ozz_temp_directory}/fbx_properties_track1_animated-ozz_curve_number.ozz"
  "${ozz_temp_directory}/fbx_properties_track1_animated-ozz_step_bool.ozz"
  "${ozz_temp_directory}/fbx_properties_track1_animated-ozz_step_enum.ozz"
  "${ozz_temp_directory}/fbx_properties_track1_animated-ozz_step_int.ozz"
  "${ozz_temp_directory}/fbx_properties_track1_animated-ozz_step_number.ozz"
  "${ozz_temp_directory}/fbx_properties_track1_constant-ozz_bool.ozz"
  "${ozz_temp_directory}/fbx_properties_track1_constant-ozz_enum.ozz"
  "${ozz_temp_directory}/fbx_properties_track1_constant-ozz_int.ozz"
  "${ozz_temp_directory}/fbx_properties_track1_constant-ozz_number.ozz"
  "${ozz_temp_directory}/fbx_properties_track3_animated-ozz_curve_vector.ozz"
  "${ozz_temp_directory}/fbx_properties_track3_constant-ozz_vector.ozz"
  "${ozz_temp_directory}/fbx_properties_track4_animated-ozz_curve_color.ozz"
  "${ozz_temp_directory}/fbx_properties_track4_constant-ozz_color.ozz"
  "${ozz_temp_directory}/fbx_properties_tracks_cp/"
  )
set_tests_properties(fbx2ozz_anim_properties_ouput PROPERTIES
  DEPENDS fbx2ozz_anim_properties
          "${ozz_media_directory}/fbx/properties.fbx"
          "${ozz_temp_directory}/fbx_propreties_skeleton.ozz")

# Run fbx2ozz animation collada passing tests
#----------------------------

add_test(NAME fbx2ozz_anim_simple_dae_astro_max COMMAND fbx2ozz "--file=${ozz_media_directory}/collada/astro_max.dae" "--config={\"animations\":[{\"skeleton\":\"${ozz_temp_directory}/astro_max_skeleton.ozz\",\"output\":\"${ozz_temp_directory}/dae_animation_${CMAKE_CURRENT_LIST_LINE}.ozz\"}]}")
set_tests_properties(fbx2ozz_anim_simple_dae_astro_max PROPERTIES DEPENDS fbx2ozz_skel_simple_dae_astro_max)
add_test(NAME fbx2ozz_anim_simple_dae_astro_maya COMMAND fbx2ozz "--file=${ozz_media_directory}/collada/astro_maya.dae" "--config={\"animations\":[{\"skeleton\":\"${ozz_temp_directory}/astro_maya_skeleton.ozz\",\"output\":\"${ozz_temp_directory}/dae_animation_${CMAKE_CURRENT_LIST_LINE}.ozz\"}]}")
set_tests_properties(fbx2ozz_anim_simple_dae_astro_maya PROPERTIES DEPENDS fbx2ozz_skel_simple_dae_astro_maya)
add_test(NAME fbx2ozz_anim_simple_dae_seymour COMMAND fbx2ozz "--file=${ozz_media_directory}/collada/seymour.dae" "--config={\"animations\":[{\"skeleton\":\"${ozz_temp_directory}/seymour_skeleton.ozz\",\"output\":\"${ozz_temp_directory}/dae_animation_${CMAKE_CURRENT_LIST_LINE}.ozz\"}]}")
set_tests_properties(fbx2ozz_anim_simple_dae_seymour PROPERTIES DEPENDS fbx2ozz_skel_simple_dae_seymour)

# Uses the sample playback to test other file format and special cases.
if(TARGET sample_playback)

  add_custom_command(
    DEPENDS $<$<AND:$<BOOL:${ozz_build_data}>,$<BOOL:${ozz_build_fbx}>>:fbx2ozz>
            "${ozz_media_directory}/collada/alain/skeleton.dae"
            "${ozz_media_directory}/collada/alain/skeleton_zup.dae"
            "${ozz_media_directory}/collada/alain/walk.dae"
    OUTPUT  "${ozz_temp_directory}/alain_skeleton_dae.ozz"
            "${ozz_temp_directory}/alain_skeleton_dae_zup.ozz"
            "${ozz_temp_directory}/alain_walk_dae.ozz"
    COMMAND fbx2ozz
      "--file=${ozz_media_directory}/collada/alain/skeleton.dae"
      "--config={\"skeleton\":{\"output\":\"${ozz_temp_directory}/alain_skeleton_dae.ozz\"}}"
    COMMAND fbx2ozz
      "--file=${ozz_media_directory}/collada/alain/skeleton_zup.dae"
      "--config={\"skeleton\":{\"output\":\"${ozz_temp_directory}/alain_skeleton_dae_zup.ozz\"}}"
    COMMAND fbx2ozz
      "--file=${ozz_media_directory}/collada/alain/walk.dae"
      "--config={\"animations\": [{\"skeleton\":\"${ozz_temp_directory}/alain_skeleton_dae.ozz\",\"output\":\"${ozz_temp_directory}/alain_walk_dae.ozz\"}]}"
    VERBATIM)
      
  add_custom_target(BUILD_DATA_TEST ALL DEPENDS
    "${ozz_temp_directory}/alain_skeleton_dae.ozz"
    "${ozz_temp_directory}/alain_skeleton_dae_zup.ozz"
    "${ozz_temp_directory}/alain_walk_dae.ozz"
    VERBATIM)

  add_test(NAME sample_playback_dae_zup COMMAND sample_playback "--skeleton=${ozz_temp_directory}/alain_skeleton_dae_zup.ozz" "--animation=${ozz_temp_directory}/alain_walk_dae.ozz" "--max_idle_loops=${ozz_sample_testing_loops}" $<$<BOOL:${ozz_run_tests_headless}>:--norender>)

  add_test(NAME sample_playback_dae_seymour COMMAND sample_playback "--skeleton=${ozz_media_directory}/bin/seymour_skeleton.ozz" "--animation=${ozz_media_directory}/bin/seymour_animation.ozz" "--max_idle_loops=${ozz_sample_testing_loops}" $<$<BOOL:${ozz_run_tests_headless}>:--norender>)
  add_test(NAME sample_playback_dae_astro_max COMMAND sample_playback "--skeleton=${ozz_media_directory}/bin/astro_max_skeleton.ozz" "--animation=${ozz_media_directory}/bin/astro_max_animation.ozz" "--max_idle_loops=${ozz_sample_testing_loops}" $<$<BOOL:${ozz_run_tests_headless}>:--norender>)
  add_test(NAME sample_playback_dae_astro_maya COMMAND sample_playback "--skeleton=${ozz_media_directory}/bin/astro_maya_skeleton.ozz" "--animation=${ozz_media_directory}/bin/astro_maya_animation.ozz" "--max_idle_loops=${ozz_sample_testing_loops}" $<$<BOOL:${ozz_run_tests_headless}>:--norender>)
  
  add_test(NAME sample_playback_mixed_df COMMAND sample_playback "--skeleton=${ozz_temp_directory}/alain_skeleton_dae.ozz" "--animation=${ozz_media_directory}/bin/alain_walk.ozz" "--max_idle_loops=${ozz_sample_testing_loops}" $<$<BOOL:${ozz_run_tests_headless}>:--norender>)
  add_test(NAME sample_playback_mixed_fd COMMAND sample_playback "--skeleton=${ozz_media_directory}/bin/alain_skeleton.ozz" "--animation=${ozz_temp_directory}/alain_walk_dae.ozz" "--max_idle_loops=${ozz_sample_testing_loops}" $<$<BOOL:${ozz_run_tests_headless}>:--norender>)
endif()

# TODOTODOTODOTODOTODOTODOTODOTODOTODOTODOTODOTODOTODOTODOTODOTODOTODOTODOTODOTODOTODOTODOTODOTODOTODOTODOTODOTODOTODOTODOTODOTODOTODOTODOTODOTODOTODOTODO
# ozz_animation_fbx fuse tests
#set_source_files_properties(${PROJECT_BINARY_DIR}/src_fused/ozz_animation_fbx.cc PROPERTIES GENERATED 1)
#add_executable(test_fuse_ozz_animation_fbx
  #${PROJECT_SOURCE_DIR}/src/animation/offline/fbx/fbx2skel.cc
  #${PROJECT_BINARY_DIR}/src_fused/ozz_animation_fbx.cc)
#add_dependencies(test_fuse_ozz_animation_fbx BUILD_FUSE_ozz_animation_fbx)
#target_include_directories(test_fuse_ozz_animation_fbx
  #PUBLIC ${FBX_INCLUDE_DIRS})
#
#target_link_libraries(test_fuse_ozz_animation_fbx
  #debug ${FBX_LIBRARIES_DEBUG}
  #optimized ${FBX_LIBRARIES}
  #ozz_animation_offline_skel_tools
  #ozz_options)
#add_dependencies(test_fuse_ozz_animation_fbx BUILD_FUSE_ozz_animation_fbx)
#set_target_properties(test_fuse_ozz_animation_fbx PROPERTIES FOLDER "ozz/tests/animation_offline")
#
#add_test(NAME test_fuse_ozz_animation_fbx_no_arg COMMAND test_fuse_ozz_animation_fbx)
#set_tests_properties(test_fuse_ozz_animation_fbx_no_arg PROPERTIES WILL_FAIL true)