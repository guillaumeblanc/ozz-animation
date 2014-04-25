add_executable(sample_optimize
  sample_optimize.cc
  README)
target_link_libraries(sample_optimize
  sample_framework
  ozz_animation_collada
  ozz_animation_offline
  ozz_animation
  ozz_options
  ozz_base)
add_custom_command(TARGET sample_optimize POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E make_directory media
  COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_LIST_DIR}/README .
  COMMAND ${CMAKE_COMMAND} -E copy "${ozz_media_directory}/collada/alain/skeleton.dae" "media/skeleton.dae"
  COMMAND ${CMAKE_COMMAND} -E copy "${ozz_media_directory}/collada/alain/atlas.dae" "media/animation.dae")

set_target_properties(sample_optimize
  PROPERTIES FOLDER "samples")

install(TARGETS sample_optimize DESTINATION bin/samples/optimize)
install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/media DESTINATION bin/samples/optimize)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/README DESTINATION bin/samples/optimize)

add_test(NAME sample_optimize_default COMMAND sample_optimize "--max_idle_loops=${SAMPLE_TESTING_LOOPS}" ${SAMPLE_RENDER_ARGUMENT})
add_test(NAME sample_optimize_path1 COMMAND sample_optimize "--skeleton=${ozz_media_directory}/collada/alain/skeleton.dae" "--animation=${ozz_media_directory}/collada/alain/atlas.dae" "--max_idle_loops=${SAMPLE_TESTING_LOOPS}" ${SAMPLE_RENDER_ARGUMENT})
add_test(NAME sample_optimize_path2 COMMAND sample_optimize "--skeleton=${ozz_media_directory}/collada/alain/skeleton_zup.dae" "--animation=${ozz_media_directory}/collada/alain/atlas.dae" "--max_idle_loops=${SAMPLE_TESTING_LOOPS}" ${SAMPLE_RENDER_ARGUMENT})
add_test(NAME sample_optimize_path3 COMMAND sample_optimize "--skeleton=${ozz_media_directory}/collada/alain/atlas.dae" "--animation=${ozz_media_directory}/collada/alain/atlas.dae" "--max_idle_loops=${SAMPLE_TESTING_LOOPS}" ${SAMPLE_RENDER_ARGUMENT})
add_test(NAME sample_optimize_path4 COMMAND sample_optimize "--skeleton=${ozz_media_directory}/collada/seymour.dae" "--animation=${ozz_media_directory}/collada/seymour.dae" "--max_idle_loops=${SAMPLE_TESTING_LOOPS}" ${SAMPLE_RENDER_ARGUMENT})
add_test(NAME sample_optimize_path5 COMMAND sample_optimize "--skeleton=${ozz_media_directory}/collada/astro_max.dae" "--animation=${ozz_media_directory}/collada/astro_max.dae" "--max_idle_loops=${SAMPLE_TESTING_LOOPS}" ${SAMPLE_RENDER_ARGUMENT})
add_test(NAME sample_optimize_path6 COMMAND sample_optimize "--skeleton=${ozz_media_directory}/collada/astro_maya.dae" "--animation=${ozz_media_directory}/collada/astro_maya.dae" "--max_idle_loops=${SAMPLE_TESTING_LOOPS}" ${SAMPLE_RENDER_ARGUMENT})
add_test(NAME sample_optimize_path7 COMMAND sample_optimize "--skeleton=${ozz_media_directory}/collada/astro_max.dae" "--animation=${ozz_media_directory}/collada/astro_maya.dae" "--max_idle_loops=${SAMPLE_TESTING_LOOPS}" ${SAMPLE_RENDER_ARGUMENT})
add_test(NAME sample_optimize_path8 COMMAND sample_optimize "--skeleton=${ozz_media_directory}/collada/astro_maya.dae" "--animation=${ozz_media_directory}/collada/astro_max.dae" "--max_idle_loops=${SAMPLE_TESTING_LOOPS}" ${SAMPLE_RENDER_ARGUMENT})
add_test(NAME sample_optimize_invalid_skeleton_path COMMAND sample_optimize "--skeleton=${ozz_media_directory}/collada/alain/skeleton.dae" "--animation=${ozz_media_directory}/collada/invalide_filename.dae" "--max_idle_loops=${SAMPLE_TESTING_LOOPS}" ${SAMPLE_RENDER_ARGUMENT})
set_tests_properties(sample_optimize_invalid_skeleton_path PROPERTIES WILL_FAIL true)
add_test(NAME sample_optimize_invalid_animation_path COMMAND sample_optimize "--skeleton=${ozz_media_directory}/collada/invalide_filename.dae" "--animation=${ozz_media_directory}/collada/animation.dae" "--max_idle_loops=${SAMPLE_TESTING_LOOPS}" ${SAMPLE_RENDER_ARGUMENT})
set_tests_properties(sample_optimize_invalid_animation_path PROPERTIES WILL_FAIL true)
