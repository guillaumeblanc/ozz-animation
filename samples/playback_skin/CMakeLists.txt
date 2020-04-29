
add_custom_command(
  DEPENDS $<$<BOOL:${ozz_build_fbx}>:BUILD_DATA>
          $<$<BOOL:${ozz_build_fbx}>:BUILD_DATA_SAMPLE>
          "${CMAKE_CURRENT_LIST_DIR}/README.md"
          "${ozz_media_directory}/bin/ruby_mesh.ozz"
          "${ozz_media_directory}/bin/ruby_skeleton.ozz"
          "${ozz_media_directory}/bin/ruby_animation.ozz"
  OUTPUT  "${CMAKE_CURRENT_BINARY_DIR}/README.md"
          "${CMAKE_CURRENT_BINARY_DIR}/media/mesh.ozz"
          "${CMAKE_CURRENT_BINARY_DIR}/media/skeleton.ozz"
          "${CMAKE_CURRENT_BINARY_DIR}/media/animation.ozz"
  COMMAND ${CMAKE_COMMAND} -E make_directory media
  COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_LIST_DIR}/README.md" .
  COMMAND ${CMAKE_COMMAND} -E copy "${ozz_media_directory}/bin/ruby_mesh.ozz" "./media/mesh.ozz"
  COMMAND ${CMAKE_COMMAND} -E copy "${ozz_media_directory}/bin/ruby_skeleton.ozz" "./media/skeleton.ozz"
  COMMAND ${CMAKE_COMMAND} -E copy "${ozz_media_directory}/bin/ruby_animation.ozz" "./media/animation.ozz"
  VERBATIM)

add_executable(sample_playback_skin
  sample_playback_skin.cc
  "${CMAKE_CURRENT_BINARY_DIR}/README.md"
  "${CMAKE_CURRENT_BINARY_DIR}/media/mesh.ozz"
  "${CMAKE_CURRENT_BINARY_DIR}/media/skeleton.ozz"
  "${CMAKE_CURRENT_BINARY_DIR}/media/animation.ozz")
target_link_libraries(sample_playback_skin
  sample_framework)

set_target_properties(sample_playback_skin
  PROPERTIES FOLDER "samples")

if(EMSCRIPTEN)
  # Resource files are embedded to the output file with emscripten
  set_target_properties(sample_playback_skin
    PROPERTIES LINK_FLAGS "--embed-file media --embed-file README.md --memory-init-file 0")

  install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/sample_playback_skin.html
    ${CMAKE_CURRENT_BINARY_DIR}/sample_playback_skin.js
    ${CMAKE_CURRENT_BINARY_DIR}/sample_playback_skin.wasm
    DESTINATION bin/samples/playback_skin)
else()
  install(TARGETS sample_playback_skin DESTINATION bin/samples/playback_skin)
  install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/media DESTINATION bin/samples/playback_skin)
  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/README.md DESTINATION bin/samples/playback_skin)
endif(EMSCRIPTEN)

add_test(NAME sample_playback_skin COMMAND sample_playback_skin "--max_idle_loops=${ozz_sample_testing_loops}" $<$<BOOL:${ozz_run_tests_headless}>:--norender>)
add_test(NAME sample_playback_skin_path COMMAND sample_playback_skin "--skeleton=media/skeleton.ozz" "--animation=media/animation.ozz" "--mesh=media/mesh.ozz" "--max_idle_loops=${ozz_sample_testing_loops}" $<$<BOOL:${ozz_run_tests_headless}>:--norender>)
add_test(NAME sample_playback_skin_invalid_skeleton_path COMMAND sample_playback_skin "--skeleton=media/bad_skeleton.ozz" $<$<BOOL:${ozz_run_tests_headless}>:--norender>)
set_tests_properties(sample_playback_skin_invalid_skeleton_path PROPERTIES WILL_FAIL true)
add_test(NAME sample_playback_skin_invalid_animation_path COMMAND sample_playback_skin "--animation=media/bad_animation.ozz" $<$<BOOL:${ozz_run_tests_headless}>:--norender>)
set_tests_properties(sample_playback_skin_invalid_animation_path PROPERTIES WILL_FAIL true)
add_test(NAME sample_playback_skin_invalid_mesh_path COMMAND sample_playback_skin "--mesh=media/bad_mesh.ozz" $<$<BOOL:${ozz_run_tests_headless}>:--norender>)
set_tests_properties(sample_playback_skin_invalid_mesh_path PROPERTIES WILL_FAIL true)
