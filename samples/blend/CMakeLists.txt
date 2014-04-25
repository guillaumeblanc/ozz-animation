add_executable(sample_blend
  sample_blend.cc
  README)
target_link_libraries(sample_blend
  sample_framework
  ozz_animation
  ozz_options
  ozz_base)
add_custom_command(TARGET sample_blend POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E make_directory media
  COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_LIST_DIR}/README .
  COMMAND dae2skel "--file=${ozz_media_directory}/collada/alain/skeleton.dae" "--skeleton=media/skeleton.ozz"
  COMMAND dae2anim "--file=${ozz_media_directory}/collada/alain/walk.dae" "--skeleton=media/skeleton.ozz" "--animation=media/walk.ozz"
  COMMAND dae2anim "--file=${ozz_media_directory}/collada/alain/jog.dae" "--skeleton=media/skeleton.ozz" "--animation=media/jog.ozz"
  COMMAND dae2anim "--file=${ozz_media_directory}/collada/alain/run.dae" "--skeleton=media/skeleton.ozz" "--animation=media/run.ozz")

set_target_properties(sample_blend
  PROPERTIES FOLDER "samples"
  DEPENDS dae2anim
  DEPENDS dae2skel)

install(TARGETS sample_blend DESTINATION bin/samples/blend)
install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/media DESTINATION bin/samples/blend)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/README DESTINATION bin/samples/blend)

add_test(NAME sample_blend COMMAND sample_blend "--max_idle_loops=${SAMPLE_TESTING_LOOPS}" ${SAMPLE_RENDER_ARGUMENT})
add_test(NAME sample_blend_path COMMAND sample_blend "--skeleton=media/skeleton.ozz" "--animation1=media/walk.ozz" "--animation2=media/jog.ozz"  "--animation3=media/run.ozz" "--max_idle_loops=${SAMPLE_TESTING_LOOPS}" ${SAMPLE_RENDER_ARGUMENT})
add_test(NAME sample_blend_invalid_skeleton_path COMMAND sample_blend "--skeleton=media/bad_skeleton.ozz" ${SAMPLE_RENDER_ARGUMENT})
set_tests_properties(sample_blend_invalid_skeleton_path PROPERTIES WILL_FAIL true)
add_test(NAME sample_blend_invalid_animation_path1 COMMAND sample_blend "--animation1=media/bad_animation.ozz" ${SAMPLE_RENDER_ARGUMENT})
set_tests_properties(sample_blend_invalid_animation_path1 PROPERTIES WILL_FAIL true)
add_test(NAME sample_blend_invalid_animation_path2 COMMAND sample_blend "--animation2=media/bad_animation.ozz" ${SAMPLE_RENDER_ARGUMENT})
set_tests_properties(sample_blend_invalid_animation_path2 PROPERTIES WILL_FAIL true)
add_test(NAME sample_blend_invalid_animation_path3 COMMAND sample_blend "--animation3=media/bad_animation.ozz" ${SAMPLE_RENDER_ARGUMENT})
set_tests_properties(sample_blend_invalid_animation_path3 PROPERTIES WILL_FAIL true)
