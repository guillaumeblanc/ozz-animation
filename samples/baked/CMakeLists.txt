if(NOT ozz_build_fbx)
  return()
endif()

if(FALSE)
  add_executable(sample_fbx2baked
    fbx2baked.cc)
  target_link_libraries(sample_fbx2baked
    ozz_animation_fbx
    ozz_animation_offline_tools
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
            "${ozz_media_directory}/fbx/domino.fbx"
            sample_fbx2baked
            fbx2anim
    OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/README"
           "${CMAKE_CURRENT_BINARY_DIR}/media/domino_skeleton.ozz"
           "${CMAKE_CURRENT_BINARY_DIR}/media/domino_animation.ozz"
    COMMAND ${CMAKE_COMMAND} -E make_directory media
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_LIST_DIR}/README .
    COMMAND sample_fbx2baked
      "--file=${ozz_media_directory}/fbx/domino.fbx"
      "--skeleton=${CMAKE_CURRENT_BINARY_DIR}/media/domino_skeleton.ozz"
    COMMAND fbx2anim
      "--file=${ozz_media_directory}/fbx/domino.fbx"
      "--skeleton=${CMAKE_CURRENT_BINARY_DIR}/media/domino_skeleton.ozz"
      "--animation=${CMAKE_CURRENT_BINARY_DIR}/media/domino_animation.ozz")
endif()

add_executable(sample_baked
  sample_baked.cc
  "${CMAKE_CURRENT_BINARY_DIR}/README"
  "${CMAKE_CURRENT_BINARY_DIR}/media/domino_skeleton.ozz"
  "${CMAKE_CURRENT_BINARY_DIR}/media/domino_animation.ozz")

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

add_test(NAME sample_baked COMMAND sample_baked "--max_idle_loops=${SAMPLE_TESTING_LOOPS}" ${SAMPLE_RENDER_ARGUMENT})
add_test(NAME sample_baked_load_path COMMAND sample_baked "--skeleton=media/domino_skeleton.ozz" "--animation=media/domino_animation.ozz" "--max_idle_loops=${SAMPLE_TESTING_LOOPS}" ${SAMPLE_RENDER_ARGUMENT})
add_test(NAME sample_baked_load_invalid_skeleton_path COMMAND sample_baked "--skeleton=media/bad_skeleton.ozz" ${SAMPLE_RENDER_ARGUMENT})
set_tests_properties(sample_baked_load_invalid_skeleton_path PROPERTIES WILL_FAIL true)
add_test(NAME sample_baked_load_invalid_animation_path COMMAND sample_baked "--animation=media/bad_animation.ozz" ${SAMPLE_RENDER_ARGUMENT})
set_tests_properties(sample_baked_load_invalid_animation_path PROPERTIES WILL_FAIL true)
