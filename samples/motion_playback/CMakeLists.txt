
add_custom_command(
  DEPENDS $<$<BOOL:${ozz_build_fbx}>:BUILD_DATA>
          "${CMAKE_CURRENT_LIST_DIR}/README.md"
          "${ozz_media_directory}/bin/pab_skeleton.ozz"
          "${ozz_media_directory}/bin/pab_jog_no_motion.ozz"
          "${ozz_media_directory}/bin/pab_jog_motion_track.ozz"
  OUTPUT  "${CMAKE_CURRENT_BINARY_DIR}/README.md"
          "${CMAKE_CURRENT_BINARY_DIR}/media/skeleton.ozz"
          "${CMAKE_CURRENT_BINARY_DIR}/media/animation.ozz"
          "${CMAKE_CURRENT_BINARY_DIR}/media/motion.ozz"
  COMMAND ${CMAKE_COMMAND} -E make_directory media
  COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_LIST_DIR}/README.md" .
  COMMAND ${CMAKE_COMMAND} -E copy "${ozz_media_directory}/bin/pab_skeleton.ozz" "./media/skeleton.ozz"
  COMMAND ${CMAKE_COMMAND} -E copy "${ozz_media_directory}/bin/pab_jog_no_motion.ozz" "./media/animation.ozz"
  COMMAND ${CMAKE_COMMAND} -E copy "${ozz_media_directory}/bin/pab_jog_motion_track.ozz" "./media/motion.ozz"
  VERBATIM)

add_executable(sample_motion_playback
sample_motion_playback.cc
  "${CMAKE_CURRENT_BINARY_DIR}/README.md"
  "${CMAKE_CURRENT_BINARY_DIR}/media/skeleton.ozz"
  "${CMAKE_CURRENT_BINARY_DIR}/media/animation.ozz"
  "${CMAKE_CURRENT_BINARY_DIR}/media/motion.ozz")
target_link_libraries(sample_motion_playback
  sample_framework)
target_copy_shared_libraries(sample_motion_playback)

set_target_properties(sample_motion_playback
  PROPERTIES FOLDER "samples")

if(EMSCRIPTEN)
  # Resource files are embedded to the output file with emscripten
  set_target_properties(sample_motion_playback
    PROPERTIES LINK_FLAGS "--embed-file media --embed-file README.md")

  install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/sample_motion_playback.html
    ${CMAKE_CURRENT_BINARY_DIR}/sample_motion_playback.js
    ${CMAKE_CURRENT_BINARY_DIR}/sample_motion_playback.wasm
    DESTINATION bin/samples/motion_playback)
else()
  install(TARGETS sample_motion_playback DESTINATION bin/samples/motion_playback)
  install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/media DESTINATION bin/samples/motion_playback)
  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/README.md DESTINATION bin/samples/motion_playback)
endif(EMSCRIPTEN)

add_test(NAME sample_motion_playback COMMAND sample_motion_playback "--max_idle_loops=${ozz_sample_testing_loops}" $<$<BOOL:${ozz_run_tests_headless}>:--norender>)
add_test(NAME sample_motion_playback_path COMMAND sample_motion_playback "--skeleton=media/skeleton.ozz" "--animation=media/animation.ozz" "--motion=media/motion.ozz" "--max_idle_loops=${ozz_sample_testing_loops}" $<$<BOOL:${ozz_run_tests_headless}>:--norender>)
add_test(NAME sample_motion_playback_invalid_skeleton_path COMMAND sample_motion_playback "--skeleton=media/bad_skeleton.ozz" $<$<BOOL:${ozz_run_tests_headless}>:--norender>)
set_tests_properties(sample_motion_playback_invalid_skeleton_path PROPERTIES WILL_FAIL true)
add_test(NAME sample_motion_playback_invalid_animation_path COMMAND sample_motion_playback "--animation=media/bad_animation.ozz" $<$<BOOL:${ozz_run_tests_headless}>:--norender>)
set_tests_properties(sample_motion_playback_invalid_animation_path PROPERTIES WILL_FAIL true)
add_test(NAME sample_motion_playback_invalid_motion_path COMMAND sample_motion_playback "--motion=media/bad_animation.ozz" $<$<BOOL:${ozz_run_tests_headless}>:--norender>)
set_tests_properties(sample_motion_playback_invalid_motion_path PROPERTIES WILL_FAIL true)

