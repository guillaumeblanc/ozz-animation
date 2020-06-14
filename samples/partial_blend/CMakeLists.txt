add_custom_command(
  DEPENDS $<$<BOOL:${ozz_build_fbx}>:BUILD_DATA>
          "${CMAKE_CURRENT_LIST_DIR}/README.md"
          "${ozz_media_directory}/bin/pab_skeleton.ozz"
          "${ozz_media_directory}/bin/pab_walk.ozz"
          "${ozz_media_directory}/bin/pab_crossarms.ozz"
  OUTPUT  "${CMAKE_CURRENT_BINARY_DIR}/README.md"
          "${CMAKE_CURRENT_BINARY_DIR}/media/skeleton.ozz"
          "${CMAKE_CURRENT_BINARY_DIR}/media/animation_base.ozz"
          "${CMAKE_CURRENT_BINARY_DIR}/media/animation_partial.ozz"
  COMMAND ${CMAKE_COMMAND} -E make_directory media
  COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_LIST_DIR}/README.md" .
  COMMAND ${CMAKE_COMMAND} -E copy "${ozz_media_directory}/bin/pab_skeleton.ozz" "./media/skeleton.ozz"
  COMMAND ${CMAKE_COMMAND} -E copy "${ozz_media_directory}/bin/pab_walk.ozz" "./media/animation_base.ozz"
  COMMAND ${CMAKE_COMMAND} -E copy "${ozz_media_directory}/bin/pab_crossarms.ozz" "./media/animation_partial.ozz"
  VERBATIM)
  
add_executable(sample_partial_blend
  sample_partial_blend.cc
  "${CMAKE_CURRENT_BINARY_DIR}/README.md"
  "${CMAKE_CURRENT_BINARY_DIR}/media/skeleton.ozz"
  "${CMAKE_CURRENT_BINARY_DIR}/media/animation_base.ozz"
  "${CMAKE_CURRENT_BINARY_DIR}/media/animation_partial.ozz")
target_link_libraries(sample_partial_blend
  sample_framework)

set_target_properties(sample_partial_blend
  PROPERTIES FOLDER "samples")

if(EMSCRIPTEN)
  # Resource files are embedded to the output file with emscripten
  set_target_properties(sample_partial_blend
    PROPERTIES LINK_FLAGS "--embed-file media --embed-file README.md --memory-init-file 0")

  install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/sample_partial_blend.html
    ${CMAKE_CURRENT_BINARY_DIR}/sample_partial_blend.js
    ${CMAKE_CURRENT_BINARY_DIR}/sample_partial_blend.wasm
    DESTINATION bin/samples/partial_blend)
else()
  install(TARGETS sample_partial_blend DESTINATION bin/samples/partial_blend)
  install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/media DESTINATION bin/samples/partial_blend)
  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/README.md DESTINATION bin/samples/partial_blend)
endif(EMSCRIPTEN)

add_test(NAME sample_partial_blend COMMAND sample_partial_blend "--max_idle_loops=${ozz_sample_testing_loops}" $<$<BOOL:${ozz_run_tests_headless}>:--norender>)
add_test(NAME sample_partial_blend_path COMMAND sample_partial_blend "--skeleton=media/skeleton.ozz" "--lower_body_animation=media/animation_base.ozz" "--upper_body_animation=media/animation_partial.ozz" "--max_idle_loops=${ozz_sample_testing_loops}" $<$<BOOL:${ozz_run_tests_headless}>:--norender>)
add_test(NAME sample_partial_blend_invalid_skeleton_path COMMAND sample_partial_blend "--skeleton=media/bad_skeleton.ozz" $<$<BOOL:${ozz_run_tests_headless}>:--norender>)
set_tests_properties(sample_partial_blend_invalid_skeleton_path PROPERTIES WILL_FAIL true)
add_test(NAME sample_partial_blend_invalid_animation_path1 COMMAND sample_partial_blend "--lower_body_animation=media/bad_animation.ozz" $<$<BOOL:${ozz_run_tests_headless}>:--norender>)
set_tests_properties(sample_partial_blend_invalid_animation_path1 PROPERTIES WILL_FAIL true)
add_test(NAME sample_partial_blend_invalid_animation_path2 COMMAND sample_partial_blend "--upper_body_animation=media/bad_animation.ozz" $<$<BOOL:${ozz_run_tests_headless}>:--norender>)
set_tests_properties(sample_partial_blend_invalid_animation_path2 PROPERTIES WILL_FAIL true)
