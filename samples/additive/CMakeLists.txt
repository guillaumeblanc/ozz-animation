
add_custom_command(
  DEPENDS $<$<BOOL:${ozz_build_fbx}>:BUILD_DATA>
          $<$<BOOL:${ozz_build_fbx}>:BUILD_DATA_SAMPLE>
          "${CMAKE_CURRENT_LIST_DIR}/README.md"
          "${ozz_media_directory}/bin/pab_skeleton.ozz"
          "${ozz_media_directory}/bin/pab_walk.ozz"
          "${ozz_media_directory}/bin/pab_crackhead_additive.ozz"
  OUTPUT  "${CMAKE_CURRENT_BINARY_DIR}/README.md"
          "${CMAKE_CURRENT_BINARY_DIR}/media/skeleton.ozz"
          "${CMAKE_CURRENT_BINARY_DIR}/media/animation_base.ozz"
          "${CMAKE_CURRENT_BINARY_DIR}/media/animation_additive.ozz"
  COMMAND ${CMAKE_COMMAND} -E make_directory media
  COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_LIST_DIR}/README.md" .
  COMMAND ${CMAKE_COMMAND} -E copy "${ozz_media_directory}/bin/pab_skeleton.ozz" "./media/skeleton.ozz"
  COMMAND ${CMAKE_COMMAND} -E copy "${ozz_media_directory}/bin/pab_walk.ozz" "./media/animation_base.ozz"
  COMMAND ${CMAKE_COMMAND} -E copy "${ozz_media_directory}/bin/pab_crackhead_additive.ozz" "./media/animation_additive.ozz"
  VERBATIM)

add_executable(sample_additive
  sample_additive.cc
  "${CMAKE_CURRENT_BINARY_DIR}/README.md"
  "${CMAKE_CURRENT_BINARY_DIR}/media/skeleton.ozz"
  "${CMAKE_CURRENT_BINARY_DIR}/media/animation_base.ozz"
  "${CMAKE_CURRENT_BINARY_DIR}/media/animation_additive.ozz")
target_link_libraries(sample_additive
  sample_framework)

set_target_properties(sample_additive
  PROPERTIES FOLDER "samples")

if(EMSCRIPTEN)
  # Resource files are embedded to the output file with emscripten
  set_target_properties(sample_additive
    PROPERTIES LINK_FLAGS "--embed-file media --embed-file README.md --memory-init-file 0")

  install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/sample_additive.html
    ${CMAKE_CURRENT_BINARY_DIR}/sample_additive.js
    ${CMAKE_CURRENT_BINARY_DIR}/sample_additive.wasm
    DESTINATION bin/samples/additive)
else()
  install(TARGETS sample_additive DESTINATION bin/samples/additive)
  install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/media DESTINATION bin/samples/additive)
  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/README.md DESTINATION bin/samples/additive)
endif(EMSCRIPTEN)

add_test(NAME sample_additive COMMAND sample_additive "--max_idle_loops=${ozz_sample_testing_loops}" $<$<BOOL:${ozz_run_tests_headless}>:--norender>)
add_test(NAME sample_additive_path COMMAND sample_additive "--skeleton=media/skeleton.ozz" "--animation=media/animation_base.ozz" "--additive_animation=media/animation_additive.ozz" "--max_idle_loops=${ozz_sample_testing_loops}" $<$<BOOL:${ozz_run_tests_headless}>:--norender>)
add_test(NAME sample_additive_invalid_skeleton_path COMMAND sample_additive "--skeleton=media/bad_skeleton.ozz" $<$<BOOL:${ozz_run_tests_headless}>:--norender>)
set_tests_properties(sample_additive_invalid_skeleton_path PROPERTIES WILL_FAIL true)
add_test(NAME sample_additive_invalid_animation_path1 COMMAND sample_additive "--animation=media/bad_animation.ozz" $<$<BOOL:${ozz_run_tests_headless}>:--norender>)
set_tests_properties(sample_additive_invalid_animation_path1 PROPERTIES WILL_FAIL true)
add_test(NAME sample_additive_invalid_animation_path2 COMMAND sample_additive "--additive_animation=media/bad_animation.ozz" $<$<BOOL:${ozz_run_tests_headless}>:--norender>)
set_tests_properties(sample_additive_invalid_animation_path2 PROPERTIES WILL_FAIL true)
