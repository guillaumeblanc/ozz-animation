# Option to build baked data is disabled by default as it is very costly.
option(ozz_demo_build_data "Build demo data" ON)

# Build baked data
if(ozz_demo_build_data AND
   TARGET fbx2skel AND
   TARGET fbx2anim AND
   TARGET sample_fbx2mesh)

  add_custom_command(
    DEPENDS fbx2skel
            fbx2anim
            sample_fbx2mesh
            "${ozz_media_directory}/fbx/alain/magic.fbx"
    OUTPUT "${ozz_media_directory}/bin/magic_skeleton.ozz"
           "${ozz_media_directory}/bin/magic_animation.ozz"
           "${ozz_media_directory}/bin/magic_mesh.ozz"
    COMMAND fbx2skel "--file=${ozz_media_directory}/fbx/magic.fbx" "--skeleton=${ozz_media_directory}/bin/magic_skeleton.ozz"
    COMMAND fbx2anim "--file=${ozz_media_directory}/fbx/magic.fbx" "--skeleton=${ozz_media_directory}/bin/magic_skeleton.ozz" "--animation=${ozz_media_directory}/bin/magic_animation.ozz"
    COMMAND sample_fbx2mesh "--file=${ozz_media_directory}/fbx/magic.fbx" "--skeleton=${ozz_media_directory}/bin/magic_skeleton.ozz" "--mesh=${ozz_media_directory}/bin/magic_mesh.ozz")
endif()

add_custom_command(
  DEPENDS "${CMAKE_CURRENT_LIST_DIR}/README.md"
          "${ozz_media_directory}/bin/magic_mesh.ozz"
          "${ozz_media_directory}/bin/magic_skeleton.ozz"
          "${ozz_media_directory}/bin/magic_animation.ozz"
  OUTPUT  "${CMAKE_CURRENT_BINARY_DIR}/README.md"
          "${CMAKE_CURRENT_BINARY_DIR}/media/mesh.ozz"
          "${CMAKE_CURRENT_BINARY_DIR}/media/skeleton.ozz"
          "${CMAKE_CURRENT_BINARY_DIR}/media/animation.ozz"
  COMMAND ${CMAKE_COMMAND} -E make_directory media
  COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_LIST_DIR}/README.md" .
  COMMAND ${CMAKE_COMMAND} -E copy "${ozz_media_directory}/bin/magic_mesh.ozz" "./media/mesh.ozz"
  COMMAND ${CMAKE_COMMAND} -E copy "${ozz_media_directory}/bin/magic_skeleton.ozz" "./media/skeleton.ozz"
  COMMAND ${CMAKE_COMMAND} -E copy "${ozz_media_directory}/bin/magic_animation.ozz" "./media/animation.ozz")

add_executable(demo
  demo.cc
  "${CMAKE_CURRENT_BINARY_DIR}/README.md"
  "${CMAKE_CURRENT_BINARY_DIR}/media/mesh.ozz"
  "${CMAKE_CURRENT_BINARY_DIR}/media/skeleton.ozz"
  "${CMAKE_CURRENT_BINARY_DIR}/media/animation.ozz")
target_link_libraries(demo
  sample_framework)

set_target_properties(demo
  PROPERTIES FOLDER "samples")

if(EMSCRIPTEN)
  # Resource files are embedded to the output file with emscripten
  set_target_properties(demo
    PROPERTIES LINK_FLAGS "--embed-file media --embed-file README.md.md --memory-init-file 0")

  install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/demo.html
    ${CMAKE_CURRENT_BINARY_DIR}/demo.js
    DESTINATION bin/samples/demo)
else()
  install(TARGETS demo DESTINATION bin/samples/demo)
  install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/media DESTINATION bin/samples/demo)
  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/README.md DESTINATION bin/samples/demo)
endif(EMSCRIPTEN)

add_test(NAME demo COMMAND demo "--max_idle_loops=${ozz_sample_testing_loops}" $<$<BOOL:${ozz_run_tests_headless}>:--norender>)
add_test(NAME demo_path COMMAND demo "--skeleton=media/skeleton.ozz" "--animation=media/animation.ozz" "--mesh=media/mesh.ozz" "--max_idle_loops=${ozz_sample_testing_loops}" $<$<BOOL:${ozz_run_tests_headless}>:--norender>)
add_test(NAME demo_invalid_skeleton_path COMMAND demo "--skeleton=media/bad_skeleton.ozz" $<$<BOOL:${ozz_run_tests_headless}>:--norender>)
set_tests_properties(demo_invalid_skeleton_path PROPERTIES WILL_FAIL true)
add_test(NAME demo_invalid_animation_path COMMAND demo "--animation=media/bad_animation.ozz" $<$<BOOL:${ozz_run_tests_headless}>:--norender>)
set_tests_properties(demo_invalid_animation_path PROPERTIES WILL_FAIL true)
add_test(NAME demo_invalid_mesh_path COMMAND demo "--mesh=media/bad_mesh.ozz" $<$<BOOL:${ozz_run_tests_headless}>:--norender>)
set_tests_properties(demo_invalid_mesh_path PROPERTIES WILL_FAIL true)
add_test(NAME demo_incompatible_skeleton COMMAND demo "--skeleton=${ozz_media_directory}/bin/alain_skeleton.ozz" "--animation=${ozz_media_directory}/bin/magic_animation.ozz" "--mesh=${ozz_media_directory}/bin/magic_mesh.ozz" "--max_idle_loops=${ozz_sample_testing_loops}" $<$<BOOL:${ozz_run_tests_headless}>:--norender>)
set_tests_properties(demo_incompatible_skeleton PROPERTIES WILL_FAIL true)
add_test(NAME demo_incompatible_animation COMMAND demo "--skeleton=${ozz_media_directory}/bin/magic_skeleton.ozz" "--animation=${ozz_media_directory}/bin/alain_walk.ozz" "--mesh=${ozz_media_directory}/bin/magic_mesh.ozz" "--max_idle_loops=${ozz_sample_testing_loops}" $<$<BOOL:${ozz_run_tests_headless}>:--norender>)
set_tests_properties(demo_incompatible_animation PROPERTIES WILL_FAIL true)
add_test(NAME demo_incompatible_mesh COMMAND demo "--skeleton=${ozz_media_directory}/bin/magic_skeleton.ozz" "--animation=${ozz_media_directory}/bin/magic_animation.ozz" "--mesh=${ozz_media_directory}/bin/arnaud_mesh.ozz" "--max_idle_loops=${ozz_sample_testing_loops}" $<$<BOOL:${ozz_run_tests_headless}>:--norender>)
set_tests_properties(demo_incompatible_animation PROPERTIES WILL_FAIL true)
