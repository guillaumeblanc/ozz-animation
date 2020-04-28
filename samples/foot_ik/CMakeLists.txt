
add_custom_command(
  DEPENDS $<$<BOOL:${ozz_build_fbx}>:BUILD_DATA>
          $<$<BOOL:${ozz_build_fbx}>:BUILD_DATA_SAMPLE>
          "${CMAKE_CURRENT_LIST_DIR}/README.md"
          "${ozz_media_directory}/bin/pab_skeleton.ozz"
          "${ozz_media_directory}/bin/pab_crossarms.ozz"
          "${ozz_media_directory}/bin/arnaud_mesh.ozz"
          "${ozz_media_directory}/bin/floor.ozz"
  OUTPUT  "${CMAKE_CURRENT_BINARY_DIR}/README.md"
          "${CMAKE_CURRENT_BINARY_DIR}/media/skeleton.ozz"
          "${CMAKE_CURRENT_BINARY_DIR}/media/animation.ozz"
          "${CMAKE_CURRENT_BINARY_DIR}/media/mesh.ozz"
          "${CMAKE_CURRENT_BINARY_DIR}/media/floor.ozz"
  COMMAND ${CMAKE_COMMAND} -E make_directory media
  COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_LIST_DIR}/README.md" .
  COMMAND ${CMAKE_COMMAND} -E copy "${ozz_media_directory}/bin/pab_skeleton.ozz" "./media/skeleton.ozz"
  COMMAND ${CMAKE_COMMAND} -E copy "${ozz_media_directory}/bin/pab_crossarms.ozz" "./media/animation.ozz"
  COMMAND ${CMAKE_COMMAND} -E copy "${ozz_media_directory}/bin/arnaud_mesh.ozz" "./media/mesh.ozz"
  COMMAND ${CMAKE_COMMAND} -E copy "${ozz_media_directory}/bin/floor.ozz" "./media/floor.ozz"
  VERBATIM)

add_executable(sample_foot_ik
  sample_foot_ik.cc
  "${CMAKE_CURRENT_BINARY_DIR}/README.md"
  "${CMAKE_CURRENT_BINARY_DIR}/media/skeleton.ozz"
  "${CMAKE_CURRENT_BINARY_DIR}/media/animation.ozz"
  "${CMAKE_CURRENT_BINARY_DIR}/media/mesh.ozz"
  "${CMAKE_CURRENT_BINARY_DIR}/media/floor.ozz")
target_link_libraries(sample_foot_ik
  sample_framework)

set_target_properties(sample_foot_ik
  PROPERTIES FOLDER "samples")

if(EMSCRIPTEN)
  # Resource files are embedded to the output file with emscripten
  set_target_properties(sample_foot_ik
    PROPERTIES LINK_FLAGS "--embed-file media --embed-file README.md --memory-init-file 0")

  install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/sample_foot_ik.html
    ${CMAKE_CURRENT_BINARY_DIR}/sample_foot_ik.js
    ${CMAKE_CURRENT_BINARY_DIR}/sample_foot_ik.wasm
    DESTINATION bin/samples/foot_ik)
else()
  install(TARGETS sample_foot_ik DESTINATION bin/samples/foot_ik)
  install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/media DESTINATION bin/samples/foot_ik)
  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/README.md DESTINATION bin/samples/foot_ik)
endif(EMSCRIPTEN)

add_test(NAME sample_foot_ik COMMAND sample_foot_ik "--max_idle_loops=${ozz_sample_testing_loops}" $<$<BOOL:${ozz_run_tests_headless}>:--norender>)
add_test(NAME sample_foot_ik_path1 COMMAND sample_foot_ik "--skeleton=media/skeleton.ozz" "--animation=media/animation.ozz" "--mesh=media/mesh.ozz" "--max_idle_loops=${ozz_sample_testing_loops}" $<$<BOOL:${ozz_run_tests_headless}>:--norender>)
add_test(NAME sample_foot_ik_invalid_skeleton_path COMMAND sample_foot_ik "--skeleton=media/bad_skeleton.ozz" $<$<BOOL:${ozz_run_tests_headless}>:--norender>)
set_tests_properties(sample_foot_ik_invalid_skeleton_path PROPERTIES WILL_FAIL true)
add_test(NAME sample_foot_ik_invalid_animation_path COMMAND sample_foot_ik "--animation=media/bad_animation.ozz" $<$<BOOL:${ozz_run_tests_headless}>:--norender>)
set_tests_properties(sample_foot_ik_invalid_animation_path PROPERTIES WILL_FAIL true)
add_test(NAME sample_foot_ik_invalid_mesh_path COMMAND sample_foot_ik "--mesh=media/bad_mesh.ozz" $<$<BOOL:${ozz_run_tests_headless}>:--norender>)
set_tests_properties(sample_foot_ik_invalid_mesh_path PROPERTIES WILL_FAIL true)
