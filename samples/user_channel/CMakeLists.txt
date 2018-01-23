
add_custom_command(
  DEPENDS $<$<BOOL:${ozz_build_fbx}>:BUILD_DATA>
          "${CMAKE_CURRENT_LIST_DIR}/README.md"
          "${ozz_media_directory}/bin/robot_skeleton.ozz"
          "${ozz_media_directory}/bin/robot_animation.ozz"
          "${ozz_media_directory}/bin/robot_track_grasp.ozz"
  OUTPUT  "${CMAKE_CURRENT_BINARY_DIR}/README.md"
          "${CMAKE_CURRENT_BINARY_DIR}/media/skeleton.ozz"
          "${CMAKE_CURRENT_BINARY_DIR}/media/animation.ozz"
          "${CMAKE_CURRENT_BINARY_DIR}/media/track.ozz"
  COMMAND ${CMAKE_COMMAND} -E make_directory media
  COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_LIST_DIR}/README.md" .
  COMMAND ${CMAKE_COMMAND} -E copy "${ozz_media_directory}/bin/robot_skeleton.ozz" "./media/skeleton.ozz"
  COMMAND ${CMAKE_COMMAND} -E copy "${ozz_media_directory}/bin/robot_animation.ozz" "./media/animation.ozz"
  COMMAND ${CMAKE_COMMAND} -E copy "${ozz_media_directory}/bin/robot_track_grasp.ozz" "./media/track.ozz")

add_executable(sample_user_channel
  sample_user_channel.cc
  "${CMAKE_CURRENT_BINARY_DIR}/README.md"
  "${CMAKE_CURRENT_BINARY_DIR}/media/skeleton.ozz"
  "${CMAKE_CURRENT_BINARY_DIR}/media/animation.ozz"
  "${CMAKE_CURRENT_BINARY_DIR}/media/track.ozz")
target_link_libraries(sample_user_channel
  sample_framework)

set_target_properties(sample_user_channel
  PROPERTIES FOLDER "samples")

if(EMSCRIPTEN)
  # Resource files are embedded to the output file with emscripten
  set_target_properties(sample_user_channel
    PROPERTIES LINK_FLAGS "--embed-file media --embed-file README.md --memory-init-file 0")

  install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/sample_user_channel.html
    ${CMAKE_CURRENT_BINARY_DIR}/sample_user_channel.js
    DESTINATION bin/samples/user_channel)
else()
  install(TARGETS sample_user_channel DESTINATION bin/samples/user_channel)
  install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/media DESTINATION bin/samples/user_channel)
  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/README.md DESTINATION bin/samples/user_channel)
endif(EMSCRIPTEN)

add_test(NAME sample_user_channel COMMAND sample_user_channel "--max_idle_loops=${ozz_sample_testing_loops}" $<$<BOOL:${ozz_run_tests_headless}>:--norender>)
add_test(NAME sample_user_channel_path1 COMMAND sample_user_channel "--skeleton=media/skeleton.ozz" "--animation=media/animation.ozz" "--max_idle_loops=${ozz_sample_testing_loops}" $<$<BOOL:${ozz_run_tests_headless}>:--norender>)
add_test(NAME sample_user_channel_invalid_skeleton_path COMMAND sample_user_channel "--skeleton=media/bad_skeleton.ozz" $<$<BOOL:${ozz_run_tests_headless}>:--norender>)
set_tests_properties(sample_user_channel_invalid_skeleton_path PROPERTIES WILL_FAIL true)
add_test(NAME sample_user_channel_invalid_animation_path COMMAND sample_user_channel "--animation=media/bad_animation.ozz" $<$<BOOL:${ozz_run_tests_headless}>:--norender>)
set_tests_properties(sample_user_channel_invalid_animation_path PROPERTIES WILL_FAIL true)
add_test(NAME sample_user_channel_invalid_track_path COMMAND sample_user_channel "--track=media/bad_animation.ozz" $<$<BOOL:${ozz_run_tests_headless}>:--norender>)
set_tests_properties(sample_user_channel_invalid_track_path PROPERTIES WILL_FAIL true)

