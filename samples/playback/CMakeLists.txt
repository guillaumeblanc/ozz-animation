add_executable(sample_playback
  sample_playback.cc
  README)
target_link_libraries(sample_playback
  sample_framework
  ozz_animation
  ozz_options
  ozz_base)
add_custom_command(TARGET sample_playback POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E make_directory media
  COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_LIST_DIR}/README .
  COMMAND dae2skel "--file=${ozz_media_directory}/collada/alain/skeleton.dae" "--skeleton=media/skeleton.ozz"
  COMMAND dae2anim "--file=${ozz_media_directory}/collada/alain/atlas.dae" "--skeleton=media/skeleton.ozz" "--animation=media/animation.ozz")

# Also pushes fbx files if supported
if(ozz_build_fbx)
  add_custom_command(TARGET sample_playback POST_BUILD
    COMMAND fbx2skel "--file=${ozz_media_directory}/fbx/alain/skeleton.fbx" "--skeleton=media/skeleton_fbx.ozz"
    COMMAND fbx2anim "--file=${ozz_media_directory}/fbx/alain/run.fbx" "--skeleton=media/skeleton_fbx.ozz" "--animation=media/animation_fbx.ozz"
    COMMAND fbx2anim "--file=${ozz_media_directory}/fbx/alain/jog.fbx" "--skeleton=media/skeleton.ozz" "--animation=media/animation_mixed.ozz")
endif()

set_target_properties(sample_playback
  PROPERTIES FOLDER "samples"
  DEPENDS dae2anim
  DEPENDS dae2skel)

install(TARGETS sample_playback DESTINATION bin/samples/playback)
install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/media DESTINATION bin/samples/playback)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/README DESTINATION bin/samples/playback)

add_test(NAME sample_playback COMMAND sample_playback "--max_idle_loops=${SAMPLE_TESTING_LOOPS}" ${SAMPLE_RENDER_ARGUMENT})
add_test(NAME sample_playback_path COMMAND sample_playback "--skeleton=media/skeleton.ozz" "--animation=media/animation.ozz" "--max_idle_loops=${SAMPLE_TESTING_LOOPS}" ${SAMPLE_RENDER_ARGUMENT})
add_test(NAME sample_playback_invalid_skeleton_path COMMAND sample_playback "--skeleton=media/bad_skeleton.ozz" ${SAMPLE_RENDER_ARGUMENT})
set_tests_properties(sample_playback_invalid_skeleton_path PROPERTIES WILL_FAIL true)
add_test(NAME sample_playback_invalid_animation_path COMMAND sample_playback "--animation=media/bad_animation.ozz" ${SAMPLE_RENDER_ARGUMENT})
set_tests_properties(sample_playback_invalid_animation_path PROPERTIES WILL_FAIL true)

# Also pushes fbx tests if supported
if(ozz_build_fbx)
  add_test(NAME sample_playback_fbx COMMAND sample_playback "--skeleton=media/skeleton_fbx.ozz" "--animation=media/animation_fbx.ozz" "--max_idle_loops=${SAMPLE_TESTING_LOOPS}" ${SAMPLE_RENDER_ARGUMENT})
  add_test(NAME sample_playback_mixed COMMAND sample_playback "--skeleton=media/skeleton.ozz" "--animation=media/animation_mixed.ozz" "--max_idle_loops=${SAMPLE_TESTING_LOOPS}" ${SAMPLE_RENDER_ARGUMENT})
endif()
