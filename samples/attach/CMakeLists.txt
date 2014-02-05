add_executable(sample_attach
  sample_attach.cc
  README)
target_link_libraries(sample_attach
  sample_framework
  ozz_animation
  ozz_options
  ozz_base)
add_custom_command(TARGET sample_attach POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E make_directory media
  COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_LIST_DIR}/README .
  COMMAND dae2skel "--collada=${ozz_media_directory}/collada/spoox/spoox.dae" "--skeleton=media/skeleton.ozz"
  COMMAND dae2anim "--collada=${ozz_media_directory}/collada/spoox/spoox_walk.dae" "--skeleton=media/skeleton.ozz" "--animation=media/animation.ozz")

set_target_properties(sample_attach
  PROPERTIES FOLDER "samples"
  DEPENDS dae2anim
  DEPENDS dae2skel)

install(TARGETS sample_attach
  DESTINATION bin/samples/attach)
install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/media
  DESTINATION bin/samples/attach)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/README
  DESTINATION bin/samples/attach)

add_test(NAME sample_attach COMMAND sample_attach "--max_idle_loops=${SAMPLE_TESTING_LOOPS}" ${SAMPLE_RENDER_ARGUMENT})
add_test(NAME sample_load_path COMMAND sample_attach "--skeleton=media/skeleton.ozz" "--animation=media/animation.ozz" "--max_idle_loops=${SAMPLE_TESTING_LOOPS}" ${SAMPLE_RENDER_ARGUMENT})
add_test(NAME sample_load_invalid_skeleton_path COMMAND sample_attach "--skeleton=media/bad_skeleton.ozz" ${SAMPLE_RENDER_ARGUMENT})
set_tests_properties(sample_load_invalid_skeleton_path PROPERTIES WILL_FAIL true)
add_test(NAME sample_load_invalid_animation_path COMMAND sample_attach "--animation=media/bad_animation.ozz" ${SAMPLE_RENDER_ARGUMENT})
set_tests_properties(sample_load_invalid_animation_path PROPERTIES WILL_FAIL true)
