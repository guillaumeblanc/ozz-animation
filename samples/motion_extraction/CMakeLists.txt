
add_custom_command(
  DEPENDS $<$<BOOL:${ozz_build_fbx}>:BUILD_DATA>
  "${CMAKE_CURRENT_LIST_DIR}/README.md"
  "${ozz_media_directory}/bin/pab_skeleton.ozz"
  "${ozz_media_directory}/bin/pab_atlas_raw.ozz"
  OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/README.md"
  "${CMAKE_CURRENT_BINARY_DIR}/media/skeleton.ozz"
  "${CMAKE_CURRENT_BINARY_DIR}/media/raw_animation.ozz"
  COMMAND ${CMAKE_COMMAND} -E make_directory media
  COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_LIST_DIR}/README.md" .
  COMMAND ${CMAKE_COMMAND} -E copy "${ozz_media_directory}/bin/pab_skeleton.ozz" "./media/skeleton.ozz"
  COMMAND ${CMAKE_COMMAND} -E copy "${ozz_media_directory}/bin/pab_atlas_raw.ozz" "./media/raw_animation.ozz"
  VERBATIM)

add_executable(sample_motion_extraction
  sample_motion_extraction.cc
  "${CMAKE_CURRENT_BINARY_DIR}/README.md"
  "${CMAKE_CURRENT_BINARY_DIR}/media/skeleton.ozz"
  "${CMAKE_CURRENT_BINARY_DIR}/media/raw_animation.ozz")
target_link_libraries(sample_motion_extraction
  sample_framework)
target_copy_shared_libraries(sample_motion_extraction)

set_target_properties(sample_motion_extraction
  PROPERTIES FOLDER "samples")

if(EMSCRIPTEN)
  # Resource files are embedded to the output file with emscripten
  set_target_properties(sample_motion_extraction
    PROPERTIES LINK_FLAGS "--embed-file media --embed-file README.md")

  install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/sample_motion_extraction.html
    ${CMAKE_CURRENT_BINARY_DIR}/sample_motion_extraction.js
    ${CMAKE_CURRENT_BINARY_DIR}/sample_motion_extraction.wasm
    DESTINATION bin/samples/motion)
else()
  install(TARGETS sample_motion_extraction DESTINATION bin/samples/motion_extraction)
  install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/media DESTINATION bin/samples/motion_extraction)
  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/README.md DESTINATION bin/samples/motion_extraction)
endif(EMSCRIPTEN)

add_test(NAME sample_motion_extraction COMMAND sample_motion_extraction "--max_idle_loops=${ozz_sample_testing_loops}" $<$<BOOL:${ozz_run_tests_headless}>:--norender>)
add_test(NAME sample_motion_extraction_path COMMAND sample_motion_extraction "--skeleton=media/skeleton.ozz" "--animation=media/raw_animation.ozz" "--max_idle_loops=${ozz_sample_testing_loops}" $<$<BOOL:${ozz_run_tests_headless}>:--norender>)
add_test(NAME sample_motion_extraction_invalid_skeleton_path COMMAND sample_motion_extraction "--skeleton=media/bad_skeleton.ozz" $<$<BOOL:${ozz_run_tests_headless}>:--norender>)
set_tests_properties(sample_motion_extraction_invalid_skeleton_path PROPERTIES WILL_FAIL true)
add_test(NAME sample_motion_extraction_invalid_animation_path COMMAND sample_motion_extraction "--animation=media/bad_animation.ozz" $<$<BOOL:${ozz_run_tests_headless}>:--norender>)
set_tests_properties(sample_motion_extraction_invalid_animation_path PROPERTIES WILL_FAIL true)

