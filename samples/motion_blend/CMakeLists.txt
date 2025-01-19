# Generates sample data

add_custom_command(
  DEPENDS $<$<BOOL:${ozz_build_fbx}>:BUILD_DATA>
          "${CMAKE_CURRENT_LIST_DIR}/README.md"
          "${ozz_media_directory}/bin/pab_skeleton.ozz"
          "${ozz_media_directory}/bin/pab_walk_no_motion.ozz"
          "${ozz_media_directory}/bin/pab_jog_no_motion.ozz"
          "${ozz_media_directory}/bin/pab_jog_no_motion.ozz"
          "${ozz_media_directory}/bin/pab_walk_motion_track.ozz"
          "${ozz_media_directory}/bin/pab_jog_motion_track.ozz"
          "${ozz_media_directory}/bin/pab_jog_motion_track.ozz"
  OUTPUT  "${CMAKE_CURRENT_BINARY_DIR}/README.md"
          "${CMAKE_CURRENT_BINARY_DIR}/media/skeleton.ozz"
          "${CMAKE_CURRENT_BINARY_DIR}/media/animation1.ozz"
          "${CMAKE_CURRENT_BINARY_DIR}/media/animation2.ozz"
          "${CMAKE_CURRENT_BINARY_DIR}/media/animation3.ozz"
          "${CMAKE_CURRENT_BINARY_DIR}/media/motion1.ozz"
          "${CMAKE_CURRENT_BINARY_DIR}/media/motion2.ozz"
          "${CMAKE_CURRENT_BINARY_DIR}/media/motion3.ozz"
  COMMAND ${CMAKE_COMMAND} -E make_directory media
  COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_LIST_DIR}/README.md" .
  COMMAND ${CMAKE_COMMAND} -E copy "${ozz_media_directory}/bin/pab_skeleton.ozz" "./media/skeleton.ozz"
  COMMAND ${CMAKE_COMMAND} -E copy "${ozz_media_directory}/bin/pab_walk_no_motion.ozz" "./media/animation1.ozz"
  COMMAND ${CMAKE_COMMAND} -E copy "${ozz_media_directory}/bin/pab_jog_no_motion.ozz" "./media/animation2.ozz"
  COMMAND ${CMAKE_COMMAND} -E copy "${ozz_media_directory}/bin/pab_run_no_motion.ozz" "./media/animation3.ozz"
  COMMAND ${CMAKE_COMMAND} -E copy "${ozz_media_directory}/bin/pab_walk_motion_track.ozz" "./media/motion1.ozz"
  COMMAND ${CMAKE_COMMAND} -E copy "${ozz_media_directory}/bin/pab_jog_motion_track.ozz" "./media/motion2.ozz"
  COMMAND ${CMAKE_COMMAND} -E copy "${ozz_media_directory}/bin/pab_run_motion_track.ozz" "./media/motion3.ozz"
  VERBATIM)

# Adds sample executable
add_executable(sample_motion_blend
sample_motion_blend.cc
  "${CMAKE_CURRENT_BINARY_DIR}/README.md"
  "${CMAKE_CURRENT_BINARY_DIR}/media/skeleton.ozz"
  "${CMAKE_CURRENT_BINARY_DIR}/media/animation1.ozz"
  "${CMAKE_CURRENT_BINARY_DIR}/media/animation2.ozz"
  "${CMAKE_CURRENT_BINARY_DIR}/media/animation3.ozz"
  "${CMAKE_CURRENT_BINARY_DIR}/media/motion1.ozz"
  "${CMAKE_CURRENT_BINARY_DIR}/media/motion2.ozz"
  "${CMAKE_CURRENT_BINARY_DIR}/media/motion3.ozz")
  
target_link_libraries(sample_motion_blend
  sample_framework)
target_copy_shared_libraries(sample_motion_blend)

set_target_properties(sample_motion_blend
  PROPERTIES FOLDER "samples")

if(EMSCRIPTEN)
  # Resource files are embedded to the output file with emscripten
  set_target_properties(sample_motion_blend
    PROPERTIES LINK_FLAGS "--embed-file media --embed-file README.md")

  install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/sample_motion_blend.html
    ${CMAKE_CURRENT_BINARY_DIR}/sample_motion_blend.js
    ${CMAKE_CURRENT_BINARY_DIR}/sample_motion_blend.wasm
    DESTINATION bin/samples/motion_blend)
else()
  install(TARGETS sample_motion_blend DESTINATION bin/samples/motion_blend)
  install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/media DESTINATION bin/samples/motion_blend)
  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/README.md DESTINATION bin/samples/motion_blend)
endif(EMSCRIPTEN)

add_test(NAME sample_motion_blend COMMAND sample_motion_blend "--max_idle_loops=${ozz_sample_testing_loops}" $<$<BOOL:${ozz_run_tests_headless}>:--norender>)
add_test(NAME sample_motion_blend_path COMMAND sample_motion_blend "--skeleton=media/skeleton.ozz" "--animation1=media/animation1.ozz" "--animation2=media/animation2.ozz"  "--animation3=media/animation3.ozz" "--max_idle_loops=${ozz_sample_testing_loops}" $<$<BOOL:${ozz_run_tests_headless}>:--norender>)
add_test(NAME sample_motion_blend_invalid_skeleton_path COMMAND sample_motion_blend "--skeleton=media/bad_skeleton.ozz" $<$<BOOL:${ozz_run_tests_headless}>:--norender>)
set_tests_properties(sample_motion_blend_invalid_skeleton_path PROPERTIES WILL_FAIL true)
add_test(NAME sample_motion_blend_invalid_animation_path COMMAND sample_motion_blend "--animation1=media/bad_animation.ozz" $<$<BOOL:${ozz_run_tests_headless}>:--norender>)
set_tests_properties(sample_motion_blend_invalid_animation_path PROPERTIES WILL_FAIL true)
add_test(NAME sample_motion_blend_invalid_motion_path COMMAND sample_motion_blend "--motion2=media/bad_animation.ozz" $<$<BOOL:${ozz_run_tests_headless}>:--norender>)
set_tests_properties(sample_motion_blend_invalid_motion_path PROPERTIES WILL_FAIL true)
