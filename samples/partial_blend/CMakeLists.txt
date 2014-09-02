add_custom_command(
  DEPENDS "${CMAKE_CURRENT_LIST_DIR}/README"
          "${ozz_media_directory}/collada/alain/skeleton.dae"
          "${ozz_media_directory}/collada/alain/walk.dae"
          "${ozz_media_directory}/collada/alain/crossarms.dae"
          dae2skel
          dae2anim
  OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/README"
         "${CMAKE_CURRENT_BINARY_DIR}/media/skeleton.ozz"
         "${CMAKE_CURRENT_BINARY_DIR}/media/walk.ozz"
         "${CMAKE_CURRENT_BINARY_DIR}/media/crossarms.ozz"
  COMMAND ${CMAKE_COMMAND} -E make_directory media
  COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_LIST_DIR}/README .
  COMMAND dae2skel
    "--file=${ozz_media_directory}/collada/alain/skeleton.dae"
    "--skeleton=${CMAKE_CURRENT_BINARY_DIR}/media/skeleton.ozz"
  COMMAND dae2anim
    "--file=${ozz_media_directory}/collada/alain/walk.dae"
    "--skeleton=${CMAKE_CURRENT_BINARY_DIR}/media/skeleton.ozz"
    "--animation=media/walk.ozz"
  COMMAND dae2anim
    "--file=${ozz_media_directory}/collada/alain/crossarms.dae"
    "--skeleton=${CMAKE_CURRENT_BINARY_DIR}/media/skeleton.ozz"
    "--animation=${CMAKE_CURRENT_BINARY_DIR}/media/crossarms.ozz")

add_executable(sample_partial_blend
  sample_partial_blend.cc
  "${CMAKE_CURRENT_BINARY_DIR}/README"
  "${CMAKE_CURRENT_BINARY_DIR}/media/skeleton.ozz"
  "${CMAKE_CURRENT_BINARY_DIR}/media/walk.ozz"
  "${CMAKE_CURRENT_BINARY_DIR}/media/crossarms.ozz")
target_link_libraries(sample_partial_blend
  sample_framework
  ozz_animation
  ozz_options
  ozz_base)

set_target_properties(sample_partial_blend
  PROPERTIES FOLDER "samples")

if(EMSCRIPTEN)
  # Resource files are embedded to the output file with emscripten
  set_target_properties(sample_partial_blend
    PROPERTIES LINK_FLAGS "--embed-file media --embed-file README")
    
  # Builds the google gadget file
  add_custom_target(sample_partial_blend_gadget ALL
    DEPENDS
      "${CMAKE_SOURCE_DIR}/samples/framework/gadget.cmake"
      "${CMAKE_SOURCE_DIR}/samples/framework/gadget.xml.in"
      "${CMAKE_CURRENT_BINARY_DIR}/sample_partial_blend.js"
    COMMAND ${CMAKE_COMMAND}
      -DOZZ_INPUT_JS="${CMAKE_CURRENT_BINARY_DIR}/sample_partial_blend.js"
      -DOZZ_OUTPUT_XML="${CMAKE_CURRENT_BINARY_DIR}/sample_partial_blend.xml"
      -P "${CMAKE_SOURCE_DIR}/samples/framework/gadget.cmake"
    DEPENDS sample_partial_blend)

  install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/sample_partial_blend.html
    ${CMAKE_CURRENT_BINARY_DIR}/sample_partial_blend.js
    ${CMAKE_CURRENT_BINARY_DIR}/sample_partial_blend.xml
    DESTINATION bin/samples/partial_blend)
else()
  install(TARGETS sample_partial_blend DESTINATION bin/samples/partial_blend)
  install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/media DESTINATION bin/samples/partial_blend)
  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/README DESTINATION bin/samples/partial_blend)
endif(EMSCRIPTEN)

add_test(NAME sample_partial_blend COMMAND sample_partial_blend "--max_idle_loops=${SAMPLE_TESTING_LOOPS}" ${SAMPLE_RENDER_ARGUMENT})
add_test(NAME sample_partial_blend_path COMMAND sample_partial_blend "--skeleton=media/skeleton.ozz" "--lower_body_animation=media/walk.ozz" "--upper_body_animation=media/crossarms.ozz" "--max_idle_loops=${SAMPLE_TESTING_LOOPS}" ${SAMPLE_RENDER_ARGUMENT})
add_test(NAME sample_partial_blend_invalid_skeleton_path COMMAND sample_partial_blend "--skeleton=media/bad_skeleton.ozz" ${SAMPLE_RENDER_ARGUMENT})
set_tests_properties(sample_partial_blend_invalid_skeleton_path PROPERTIES WILL_FAIL true)
add_test(NAME sample_partial_blend_invalid_animation_path1 COMMAND sample_partial_blend "--lower_body_animation=media/bad_animation.ozz" ${SAMPLE_RENDER_ARGUMENT})
set_tests_properties(sample_partial_blend_invalid_animation_path1 PROPERTIES WILL_FAIL true)
add_test(NAME sample_partial_blend_invalid_animation_path2 COMMAND sample_partial_blend "--upper_body_animation=media/bad_animation.ozz" ${SAMPLE_RENDER_ARGUMENT})
set_tests_properties(sample_partial_blend_invalid_animation_path2 PROPERTIES WILL_FAIL true)
