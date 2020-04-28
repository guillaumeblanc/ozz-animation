# Generates sample data

add_custom_command(
  DEPENDS $<$<BOOL:${ozz_build_fbx}>:BUILD_DATA>
          "${CMAKE_CURRENT_LIST_DIR}/README.md"
          "${ozz_media_directory}/bin/pab_skeleton.ozz"
          "${ozz_media_directory}/bin/pab_walk.ozz"
          "${ozz_media_directory}/bin/pab_jog.ozz"
          "${ozz_media_directory}/bin/pab_run.ozz"
  OUTPUT  "${CMAKE_CURRENT_BINARY_DIR}/README.md"
          "${CMAKE_CURRENT_BINARY_DIR}/media/skeleton.ozz"
          "${CMAKE_CURRENT_BINARY_DIR}/media/animation1.ozz"
          "${CMAKE_CURRENT_BINARY_DIR}/media/animation2.ozz"
          "${CMAKE_CURRENT_BINARY_DIR}/media/animation3.ozz"
  COMMAND ${CMAKE_COMMAND} -E make_directory media
  COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_LIST_DIR}/README.md" .
  COMMAND ${CMAKE_COMMAND} -E copy "${ozz_media_directory}/bin/pab_skeleton.ozz" "./media/skeleton.ozz"
  COMMAND ${CMAKE_COMMAND} -E copy "${ozz_media_directory}/bin/pab_walk.ozz" "./media/animation1.ozz"
  COMMAND ${CMAKE_COMMAND} -E copy "${ozz_media_directory}/bin/pab_jog.ozz" "./media/animation2.ozz"
  COMMAND ${CMAKE_COMMAND} -E copy "${ozz_media_directory}/bin/pab_run.ozz" "./media/animation3.ozz"
  VERBATIM)

# Adds sample executable
add_executable(sample_blend
  sample_blend.cc
  "${CMAKE_CURRENT_BINARY_DIR}/README.md"
  "${CMAKE_CURRENT_BINARY_DIR}/media/skeleton.ozz"
  "${CMAKE_CURRENT_BINARY_DIR}/media/animation1.ozz"
  "${CMAKE_CURRENT_BINARY_DIR}/media/animation2.ozz"
  "${CMAKE_CURRENT_BINARY_DIR}/media/animation3.ozz")
  
target_link_libraries(sample_blend
  sample_framework)

set_target_properties(sample_blend
  PROPERTIES FOLDER "samples")

if(EMSCRIPTEN)
  # Resource files are embedded to the output file with emscripten
  set_target_properties(sample_blend
    PROPERTIES LINK_FLAGS "--embed-file media --embed-file README.md --memory-init-file 0")

  install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/sample_blend.html
    ${CMAKE_CURRENT_BINARY_DIR}/sample_blend.js
    ${CMAKE_CURRENT_BINARY_DIR}/sample_blend.wasm
    DESTINATION bin/samples/blend)
else()
  install(TARGETS sample_blend DESTINATION bin/samples/blend)
  install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/media DESTINATION bin/samples/blend)
  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/README.md DESTINATION bin/samples/blend)
endif(EMSCRIPTEN)

add_test(NAME sample_blend COMMAND sample_blend "--max_idle_loops=${ozz_sample_testing_loops}" $<$<BOOL:${ozz_run_tests_headless}>:--norender>)
add_test(NAME sample_blend_path COMMAND sample_blend "--skeleton=media/skeleton.ozz" "--animation1=media/animation1.ozz" "--animation2=media/animation2.ozz"  "--animation3=media/animation3.ozz" "--max_idle_loops=${ozz_sample_testing_loops}" $<$<BOOL:${ozz_run_tests_headless}>:--norender>)
add_test(NAME sample_blend_invalid_skeleton_path COMMAND sample_blend "--skeleton=media/bad_skeleton.ozz" $<$<BOOL:${ozz_run_tests_headless}>:--norender>)
set_tests_properties(sample_blend_invalid_skeleton_path PROPERTIES WILL_FAIL true)
add_test(NAME sample_blend_invalid_animation_path1 COMMAND sample_blend "--animation1=media/bad_animation.ozz" $<$<BOOL:${ozz_run_tests_headless}>:--norender>)
set_tests_properties(sample_blend_invalid_animation_path1 PROPERTIES WILL_FAIL true)
add_test(NAME sample_blend_invalid_animation_path2 COMMAND sample_blend "--animation2=media/bad_animation.ozz" $<$<BOOL:${ozz_run_tests_headless}>:--norender>)
set_tests_properties(sample_blend_invalid_animation_path2 PROPERTIES WILL_FAIL true)
add_test(NAME sample_blend_invalid_animation_path3 COMMAND sample_blend "--animation3=media/bad_animation.ozz" $<$<BOOL:${ozz_run_tests_headless}>:--norender>)
set_tests_properties(sample_blend_invalid_animation_path3 PROPERTIES WILL_FAIL true)
