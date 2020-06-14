add_custom_command(
  DEPENDS $<$<BOOL:${ozz_build_fbx}>:BUILD_DATA>
          "${CMAKE_CURRENT_LIST_DIR}/README.md"
          "${ozz_media_directory}/bin/pab_skeleton.ozz"
          "${ozz_media_directory}/bin/pab_atlas_raw.ozz"
  OUTPUT  "${CMAKE_CURRENT_BINARY_DIR}/README.md"
          "${CMAKE_CURRENT_BINARY_DIR}/media/skeleton.ozz"
          "${CMAKE_CURRENT_BINARY_DIR}/media/animation_raw.ozz"
  COMMAND ${CMAKE_COMMAND} -E make_directory media
  COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_LIST_DIR}/README.md" .
  COMMAND ${CMAKE_COMMAND} -E copy "${ozz_media_directory}/bin/pab_skeleton.ozz" "./media/skeleton.ozz"
  COMMAND ${CMAKE_COMMAND} -E copy "${ozz_media_directory}/bin/pab_atlas_raw.ozz" "./media/animation_raw.ozz"
  VERBATIM)

add_executable(sample_optimize
  sample_optimize.cc
  "${CMAKE_CURRENT_BINARY_DIR}/README.md"
  "${CMAKE_CURRENT_BINARY_DIR}/media/skeleton.ozz"
  "${CMAKE_CURRENT_BINARY_DIR}/media/animation_raw.ozz")

target_link_libraries(sample_optimize
  ozz_animation_offline
  sample_framework)

set_target_properties(sample_optimize
  PROPERTIES FOLDER "samples")

if(EMSCRIPTEN)
  # Resource files are embedded to the output file with emscripten.
  set_target_properties(sample_optimize
    PROPERTIES LINK_FLAGS "--embed-file media --embed-file README.md --memory-init-file 0")

  install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/sample_optimize.html
    ${CMAKE_CURRENT_BINARY_DIR}/sample_optimize.js
    ${CMAKE_CURRENT_BINARY_DIR}/sample_optimize.wasm
    DESTINATION bin/samples/optimize)
else()
  install(TARGETS sample_optimize DESTINATION bin/samples/optimize)
  install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/media DESTINATION bin/samples/optimize)
  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/README.md DESTINATION bin/samples/optimize)
endif(EMSCRIPTEN)

add_test(NAME sample_optimize_default COMMAND sample_optimize "--max_idle_loops=${ozz_sample_testing_loops}" $<$<BOOL:${ozz_run_tests_headless}>:--norender>)
add_test(NAME sample_optimize_path1 COMMAND sample_optimize "--skeleton=media/skeleton.ozz" "--animation=media/animation_raw.ozz" "--max_idle_loops=${ozz_sample_testing_loops}" $<$<BOOL:${ozz_run_tests_headless}>:--norender>)
add_test(NAME sample_optimize_invalid_skeleton_path COMMAND sample_optimize "--skeleton=media/unexisting.ozz" "--max_idle_loops=${ozz_sample_testing_loops}" $<$<BOOL:${ozz_run_tests_headless}>:--norender>)
set_tests_properties(sample_optimize_invalid_skeleton_path PROPERTIES WILL_FAIL true)
add_test(NAME sample_optimize_invalid_animation_path COMMAND sample_optimize "--animation=media/unexisting.ozz" "--max_idle_loops=${ozz_sample_testing_loops}" $<$<BOOL:${ozz_run_tests_headless}>:--norender>)
set_tests_properties(sample_optimize_invalid_animation_path PROPERTIES WILL_FAIL true)
