set(baked_source_file "${ozz_media_directory}/fbx/ozz_explode.fbx")

if(ozz_build_fbx AND EXISTS "${baked_source_file}")
  add_executable(sample_fbx2baked
    fbx2baked.cc)
  target_link_libraries(sample_fbx2baked
    ozz_animation_fbx
    ozz_animation_offline_skel_tools
    ozz_animation_offline
    ozz_animation
    ozz_options
    ozz_base)
    
  target_include_directories(sample_fbx2baked PUBLIC
    ${FBX_INCLUDE_DIRS})

  set_target_properties(sample_fbx2baked
    PROPERTIES FOLDER "samples/tools")

  add_custom_command(
    DEPENDS "${CMAKE_CURRENT_LIST_DIR}/README"
            "${baked_source_file}"
            sample_fbx2baked
            fbx2anim
    OUTPUT "${ozz_media_directory}/bin/baked_skeleton.ozz"
           "${ozz_media_directory}/bin/baked_animation.ozz"
    COMMAND sample_fbx2baked
      "--file=${baked_source_file}"
      "--skeleton=${ozz_media_directory}/bin/baked_skeleton.ozz"
    COMMAND fbx2anim
      "--file=${baked_source_file}"
      "--skeleton=${ozz_media_directory}/bin/baked_skeleton.ozz"
      "--animation=${ozz_media_directory}/bin/baked_animation.ozz")
endif()

add_custom_command(
  DEPENDS "${CMAKE_CURRENT_LIST_DIR}/README"
          "${ozz_media_directory}/bin/baked_skeleton.ozz"
          "${ozz_media_directory}/bin/baked_animation.ozz"
  OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/README"
         "${CMAKE_CURRENT_BINARY_DIR}/media/baked_skeleton.ozz"
         "${CMAKE_CURRENT_BINARY_DIR}/media/baked_animation.ozz"
  COMMAND ${CMAKE_COMMAND} -E make_directory media
  COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_LIST_DIR}/README" .
  COMMAND ${CMAKE_COMMAND} -E copy "${ozz_media_directory}/bin/baked_skeleton.ozz" "./media"
  COMMAND ${CMAKE_COMMAND} -E copy "${ozz_media_directory}/bin/baked_animation.ozz" "./media")

add_executable(sample_baked
  sample_baked.cc
  "${CMAKE_CURRENT_BINARY_DIR}/README"
  "${CMAKE_CURRENT_BINARY_DIR}/media/baked_skeleton.ozz"
  "${CMAKE_CURRENT_BINARY_DIR}/media/baked_animation.ozz")

target_link_libraries(sample_baked
  sample_framework)

set_target_properties(sample_baked
  PROPERTIES FOLDER "samples")

if(EMSCRIPTEN)
  # Resource files are embedded to the output file with emscripten
  set_target_properties(sample_baked
    PROPERTIES LINK_FLAGS "--embed-file media --embed-file README --memory-init-file 0")

  install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/sample_baked.html
    ${CMAKE_CURRENT_BINARY_DIR}/sample_baked.js
    DESTINATION bin/samples/baked)
else()
  install(TARGETS sample_baked DESTINATION bin/samples/baked)
  install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/media DESTINATION bin/samples/baked)
  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/README DESTINATION bin/samples/baked)
endif(EMSCRIPTEN)

add_test(NAME sample_baked COMMAND sample_baked "--max_idle_loops=${ozz_sample_testing_loops}" $<$<BOOL:${ozz_run_tests_headless}>:--norender>)
add_test(NAME sample_baked_load_path COMMAND sample_baked "--skeleton=media/baked_skeleton.ozz" "--animation=media/baked_animation.ozz" "--max_idle_loops=${ozz_sample_testing_loops}" $<$<BOOL:${ozz_run_tests_headless}>:--norender>)
add_test(NAME sample_baked_load_invalid_skeleton_path COMMAND sample_baked "--skeleton=media/bad_skeleton.ozz" $<$<BOOL:${ozz_run_tests_headless}>:--norender>)
set_tests_properties(sample_baked_load_invalid_skeleton_path PROPERTIES WILL_FAIL true)
add_test(NAME sample_baked_load_invalid_animation_path COMMAND sample_baked "--animation=media/bad_animation.ozz" $<$<BOOL:${ozz_run_tests_headless}>:--norender>)
set_tests_properties(sample_baked_load_invalid_animation_path PROPERTIES WILL_FAIL true)
