add_executable(sample_partial_blend
  sample_partial_blend.cc
  README)
target_link_libraries(sample_partial_blend
  sample_framework
  ozz_animation
  ozz_options
  ozz_base)
add_custom_command(TARGET sample_partial_blend POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E make_directory media
  COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_LIST_DIR}/README .
  COMMAND dae2skel "--file=${ozz_media_directory}/collada/alain/skeleton.dae" "--skeleton=media/skeleton.ozz"
  COMMAND dae2anim "--file=${ozz_media_directory}/collada/alain/walk.dae" "--skeleton=media/skeleton.ozz" "--animation=media/walk.ozz"
  COMMAND dae2anim "--file=${ozz_media_directory}/collada/alain/crossarms.dae" "--skeleton=media/skeleton.ozz" "--animation=media/crossarms.ozz")

set_target_properties(sample_partial_blend
  PROPERTIES FOLDER "samples"
  DEPENDS dae2anim
  DEPENDS dae2skel)

install(TARGETS sample_partial_blend DESTINATION bin/samples/partial_blend)
install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/media DESTINATION bin/samples/partial_blend)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/README DESTINATION bin/samples/partial_blend)

add_test(NAME sample_partial_blend COMMAND sample_partial_blend "--max_idle_loops=${SAMPLE_TESTING_LOOPS}" ${SAMPLE_RENDER_ARGUMENT})
add_test(NAME sample_partial_blend_path COMMAND sample_partial_blend "--skeleton=media/skeleton.ozz" "--lower_body_animation=media/walk.ozz" "--upper_body_animation=media/crossarms.ozz" "--max_idle_loops=${SAMPLE_TESTING_LOOPS}" ${SAMPLE_RENDER_ARGUMENT})
add_test(NAME sample_partial_blend_invalid_skeleton_path COMMAND sample_partial_blend "--skeleton=media/bad_skeleton.ozz" ${SAMPLE_RENDER_ARGUMENT})
set_tests_properties(sample_partial_blend_invalid_skeleton_path PROPERTIES WILL_FAIL true)
add_test(NAME sample_partial_blend_invalid_animation_path1 COMMAND sample_partial_blend "--lower_body_animation=media/bad_animation.ozz" ${SAMPLE_RENDER_ARGUMENT})
set_tests_properties(sample_partial_blend_invalid_animation_path1 PROPERTIES WILL_FAIL true)
add_test(NAME sample_partial_blend_invalid_animation_path2 COMMAND sample_partial_blend "--upper_body_animation=media/bad_animation.ozz" ${SAMPLE_RENDER_ARGUMENT})
set_tests_properties(sample_partial_blend_invalid_animation_path2 PROPERTIES WILL_FAIL true)
