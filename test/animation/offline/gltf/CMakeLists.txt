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

add_test(NAME gltf2ozz_skel_simple COMMAND gltf2ozz "--file=${ozz_media_directory}/gltf/khronos/interpolation_test.gltf" "--config={\"skeleton\":{\"filename\":\"${ozz_temp_directory}/gltf_interpolation_test_skeleton.ozz\",\"import\":{\"enable\":true}},\"animations\":[{\"filename\":\"${ozz_temp_directory}/gltf_interpolation_test_*.ozz\"}]}")
