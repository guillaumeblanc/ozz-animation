
add_custom_command(
  DEPENDS $<$<BOOL:${ozz_build_fbx}>:BUILD_DATA>
          $<$<BOOL:${ozz_build_fbx}>:BUILD_DATA_SAMPLE>
          "${CMAKE_CURRENT_LIST_DIR}/README.md"
          "${ozz_media_directory}/bin/pab_skeleton.ozz"
          "${ozz_media_directory}/bin/pab_crossarms.ozz"
          "${ozz_media_directory}/bin/arnaud_mesh.ozz"
  OUTPUT  "${CMAKE_CURRENT_BINARY_DIR}/README.md"
          "${CMAKE_CURRENT_BINARY_DIR}/media/skeleton.ozz"
          "${CMAKE_CURRENT_BINARY_DIR}/media/animation.ozz"
          "${CMAKE_CURRENT_BINARY_DIR}/media/mesh.ozz"
  COMMAND ${CMAKE_COMMAND} -E make_directory media
  COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_LIST_DIR}/README.md" .
  COMMAND ${CMAKE_COMMAND} -E copy "${ozz_media_directory}/bin/pab_skeleton.ozz" "./media/skeleton.ozz"
  COMMAND ${CMAKE_COMMAND} -E copy "${ozz_media_directory}/bin/pab_crossarms.ozz" "./media/animation.ozz"
  COMMAND ${CMAKE_COMMAND} -E copy "${ozz_media_directory}/bin/arnaud_mesh.ozz" "./media/mesh.ozz"
  VERBATIM)

add_executable(sample_look_at
  sample_look_at.cc
  "${CMAKE_CURRENT_BINARY_DIR}/README.md"
  "${CMAKE_CURRENT_BINARY_DIR}/media/skeleton.ozz"
  "${CMAKE_CURRENT_BINARY_DIR}/media/animation.ozz"
  "${CMAKE_CURRENT_BINARY_DIR}/media/mesh.ozz")
target_link_libraries(sample_look_at
  sample_framework)

set_target_properties(sample_look_at
  PROPERTIES FOLDER "samples")

if(EMSCRIPTEN)
  # Resource files are embedded to the output file with emscripten
  set_target_properties(sample_look_at
    PROPERTIES LINK_FLAGS "--embed-file media --embed-file README.md --memory-init-file 0")

  install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/sample_look_at.html
    ${CMAKE_CURRENT_BINARY_DIR}/sample_look_at.js
    ${CMAKE_CURRENT_BINARY_DIR}/sample_look_at.wasm
    DESTINATION bin/samples/look_at)
else()
  install(TARGETS sample_look_at DESTINATION bin/samples/look_at)
  install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/media DESTINATION bin/samples/look_at)
  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/README.md DESTINATION bin/samples/look_at)
endif(EMSCRIPTEN)

add_test(NAME sample_look_at COMMAND sample_look_at "--max_idle_loops=${ozz_sample_testing_loops}" $<$<BOOL:${ozz_run_tests_headless}>:--norender>)
add_test(NAME sample_look_at_path1 COMMAND sample_look_at "--skeleton=media/skeleton.ozz" "--animation=media/animation.ozz" "--mesh=media/mesh.ozz" "--max_idle_loops=${ozz_sample_testing_loops}" $<$<BOOL:${ozz_run_tests_headless}>:--norender>)
add_test(NAME sample_look_at_invalid_skeleton_path COMMAND sample_look_at "--skeleton=media/bad_skeleton.ozz" $<$<BOOL:${ozz_run_tests_headless}>:--norender>)
set_tests_properties(sample_look_at_invalid_skeleton_path PROPERTIES WILL_FAIL true)
add_test(NAME sample_look_at_invalid_animation_path COMMAND sample_look_at "--animation=media/bad_animation.ozz" $<$<BOOL:${ozz_run_tests_headless}>:--norender>)
set_tests_properties(sample_look_at_invalid_animation_path PROPERTIES WILL_FAIL true)
add_test(NAME sample_look_at_invalid_mesh_path COMMAND sample_look_at "--mesh=media/bad_mesh.ozz" $<$<BOOL:${ozz_run_tests_headless}>:--norender>)
set_tests_properties(sample_look_at_invalid_mesh_path PROPERTIES WILL_FAIL true)