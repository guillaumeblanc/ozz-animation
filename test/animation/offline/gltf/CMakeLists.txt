if(NOT ozz_build_gltf)
  return()
endif()

# Creates a file with an invalid content.
file(WRITE "${ozz_temp_directory}/bad.unk" "bad content")
file(WRITE "${ozz_temp_directory}/bad.gltf" "bad content")
file(WRITE "${ozz_temp_directory}/bad.glb" "bad content")

# Run gltf2ozz generic failing tests
#----------------------------

add_test(NAME gltf2ozz_version COMMAND gltf2ozz "--version")
add_test(NAME gltf2ozz_bad_content COMMAND gltf2ozz "--file=${ozz_temp_directory}/bad.unk" )
set_tests_properties(gltf2ozz_bad_content PROPERTIES PASS_REGULAR_EXPRESSION "Failed to import file \"${ozz_temp_directory}/bad.unk\".")
add_test(NAME gltf2ozz_bad_content_gltf COMMAND gltf2ozz "--file=${ozz_temp_directory}/bad.gltf" )
set_tests_properties(gltf2ozz_bad_content_gltf PROPERTIES PASS_REGULAR_EXPRESSION "Failed to import file \"${ozz_temp_directory}/bad.gltf\".")
add_test(NAME gltf2ozz_bad_content_glb COMMAND gltf2ozz "--file=${ozz_temp_directory}/bad.glb" )
set_tests_properties(gltf2ozz_bad_content_glb PROPERTIES PASS_REGULAR_EXPRESSION "Failed to import file \"${ozz_temp_directory}/bad.glb\".")

# Run gltf2ozz skeleton failing tests
#----------------------------

add_test(NAME gltf2ozz_skel_no_animation COMMAND gltf2ozz "--file=${ozz_media_directory}/gltf/khronos/triangle.gltf" "--config={\"skeleton\":{\"filename\":\"${ozz_temp_directory}/gltf_skeleton_${CMAKE_CURRENT_LIST_LINE}.ozz\",\"import\":{\"enable\":true}},\"animations\":[{\"filename\":\"${ozz_temp_directory}/gltf_should_not_exist.ozz\",\"clip\":\"*\"}]}")
set_tests_properties(gltf2ozz_skel_no_animation PROPERTIES PASS_REGULAR_EXPRESSION "No animation found.")

add_test(NAME gltf2ozz_skel_no_matching_animation COMMAND gltf2ozz "--file=${ozz_media_directory}/gltf/khronos/interpolation_test.gltf" "--config={\"skeleton\":{\"filename\":\"${ozz_temp_directory}/gltf_skeleton_${CMAKE_CURRENT_LIST_LINE}.ozz\",\"import\":{\"enable\":true}},\"animations\":[{\"filename\":\"${ozz_temp_directory}/gltf_should_not_exist.ozz\",\"clip\":\"won_t_match\"}]}")
set_tests_properties(gltf2ozz_skel_no_matching_animation PROPERTIES PASS_REGULAR_EXPRESSION "No matching animation found")

# Ensures nothing unexpected was outputted
#-----------------------------------------

add_test(NAME gltf2ozz_skel_output COMMAND ${CMAKE_COMMAND} -E copy "${ozz_temp_directory}/gltf_should_not_exist.ozz" "${ozz_temp_directory}/gltf_should_not_exist_too.ozz")
set_tests_properties(gltf2ozz_skel_output PROPERTIES WILL_FAIL true)
set_tests_properties(gltf2ozz_skel_output PROPERTIES
  DEPENDS "gltf2ozz_skel_no_skeleton")

# Run gltf2ozz skeleton passing tests
#----------------------------

add_test(NAME gltf2ozz_skel_simple COMMAND gltf2ozz "--file=${ozz_media_directory}/gltf/khronos/interpolation_test.gltf" "--config={\"skeleton\":{\"filename\":\"${ozz_temp_directory}/gltf_interpolation_test_skeleton.ozz\",\"import\":{\"enable\":true}}}")

add_test(NAME gltf2ozz_skel_box_animated COMMAND gltf2ozz "--file=${ozz_media_directory}/gltf/khronos/box_animated.gltf" "--config={\"skeleton\":{\"filename\":\"${ozz_temp_directory}/gltf_box_animated_skeleton.ozz\",\"import\":{\"enable\":true}}}")

add_test(NAME gltf2ozz_skel_cesium COMMAND gltf2ozz "--file=${ozz_media_directory}/gltf/khronos/cesium_man.gltf" "--config={\"skeleton\":{\"filename\":\"${ozz_temp_directory}/gltf_cesium_man_skeleton.ozz\",\"import\":{\"enable\":true}}}")

# Run gltf2ozz animation passing tests
#----------------------------

add_test(NAME gltf2ozz_animation_multiple COMMAND gltf2ozz "--file=${ozz_media_directory}/gltf/khronos/interpolation_test.gltf" "--config={\"skeleton\":{\"filename\":\"${ozz_temp_directory}/gltf_interpolation_test_skeleton.ozz\",\"import\":{\"enable\":false}},\"animations\":[{\"filename\":\"${ozz_temp_directory}/gltf_interpolation_test_*.ozz\"}]}")
set_tests_properties(gltf2ozz_animation_multiple PROPERTIES DEPENDS gltf2ozz_skel_simple)

add_test(NAME gltf2ozz_box_animation COMMAND gltf2ozz "--file=${ozz_media_directory}/gltf/khronos/box_animated.gltf" "--config={\"skeleton\":{\"filename\":\"${ozz_temp_directory}/gltf_box_animated_skeleton.ozz\",\"import\":{\"enable\":false}},\"animations\":[{\"filename\":\"${ozz_temp_directory}/gltf_box_animation.ozz\"}]}")
set_tests_properties(gltf2ozz_box_animation PROPERTIES DEPENDS gltf2ozz_skel_box_animated)

add_test(NAME gltf2ozz_cesium_animation COMMAND gltf2ozz "--file=${ozz_media_directory}/gltf/khronos/cesium_man.gltf" "--config={\"skeleton\":{\"filename\":\"${ozz_temp_directory}/gltf_cesium_man_skeleton.ozz\",\"import\":{\"enable\":false}},\"animations\":[{\"filename\":\"${ozz_temp_directory}/gltf_cesium_man_animation.ozz\"}]}")
set_tests_properties(gltf2ozz_cesium_animation PROPERTIES DEPENDS gltf2ozz_skel_cesium)

# Uses the sample playback to test other file format and special cases.
if(TARGET sample_playback)

  add_custom_command(
    DEPENDS $<$<AND:$<BOOL:${ozz_build_data}>,$<BOOL:${ozz_build_gltf}>>:gltf2ozz>
            "${ozz_media_directory}/gltf/sketchfab/ruby/scene.gltf"
    OUTPUT  "${ozz_temp_directory}/gltf_ruby_skeleton.ozz"
            "${ozz_temp_directory}/gltf_ruby_animation.ozz"
    COMMAND gltf2ozz
      "--file=${ozz_media_directory}/gltf/sketchfab/ruby/scene.gltf"
      "--config={\"skeleton\":{\"filename\": \"${ozz_temp_directory}/gltf_ruby_skeleton.ozz\",\"import\":{\"enable\":true}},\"animations\":[{\"filename\":\"${ozz_temp_directory}/gltf_ruby_animation.ozz\"}]}"
    VERBATIM)
      
  add_custom_target(BUILD_DATA_GLTF_TEST ALL DEPENDS
    "${ozz_temp_directory}/gltf_ruby_skeleton.ozz"
    "${ozz_temp_directory}/gltf_ruby_animation.ozz"
    VERBATIM)

  add_test(NAME sample_playback_gltf_ruby COMMAND sample_playback "--skeleton=${ozz_temp_directory}/gltf_ruby_skeleton.ozz" "--animation=${ozz_temp_directory}/gltf_ruby_animation.ozz" "--max_idle_loops=${ozz_sample_testing_loops}" $<$<BOOL:${ozz_run_tests_headless}>:--norender>)
endif()