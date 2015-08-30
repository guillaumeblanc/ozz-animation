# Generates sample data
add_custom_command(
  DEPENDS "${CMAKE_CURRENT_LIST_DIR}/README"
          "${ozz_media_directory}/collada/alain/skeleton.dae"
          "${ozz_media_directory}/collada/alain/walk.dae"
          "${ozz_media_directory}/collada/alain/jog.dae"
          "${ozz_media_directory}/collada/alain/run.dae"
          dae2skel
          dae2anim
  OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/README"
         "${CMAKE_CURRENT_BINARY_DIR}/media/skeleton.ozz"
         "${CMAKE_CURRENT_BINARY_DIR}/media/walk.ozz"
         "${CMAKE_CURRENT_BINARY_DIR}/media/jog.ozz"
         "${CMAKE_CURRENT_BINARY_DIR}/media/run.ozz"
  COMMAND ${CMAKE_COMMAND} -E make_directory media
  COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_LIST_DIR}/README .
  COMMAND dae2skel
    "--file=${ozz_media_directory}/collada/alain/skeleton.dae"
    "--skeleton=${CMAKE_CURRENT_BINARY_DIR}/media/skeleton.ozz"
  COMMAND dae2anim
    "--file=${ozz_media_directory}/collada/alain/walk.dae"
    "--skeleton=${CMAKE_CURRENT_BINARY_DIR}/media/skeleton.ozz"
    "--animation=${CMAKE_CURRENT_BINARY_DIR}/media/walk.ozz"
  COMMAND dae2anim
    "--file=${ozz_media_directory}/collada/alain/jog.dae"
    "--skeleton=${CMAKE_CURRENT_BINARY_DIR}/media/skeleton.ozz"
    "--animation=${CMAKE_CURRENT_BINARY_DIR}/media/jog.ozz"
  COMMAND dae2anim
    "--file=${ozz_media_directory}/collada/alain/run.dae"
    "--skeleton=${CMAKE_CURRENT_BINARY_DIR}/media/skeleton.ozz"
    "--animation=${CMAKE_CURRENT_BINARY_DIR}/media/run.ozz")

# Adds sample executable
add_executable(sample_blend
  sample_blend.cc
  "${CMAKE_CURRENT_BINARY_DIR}/README"
  "${CMAKE_CURRENT_BINARY_DIR}/media/skeleton.ozz"
  "${CMAKE_CURRENT_BINARY_DIR}/media/walk.ozz"
  "${CMAKE_CURRENT_BINARY_DIR}/media/jog.ozz"
  "${CMAKE_CURRENT_BINARY_DIR}/media/run.ozz")
  
target_link_libraries(sample_blend
  sample_framework)

set_target_properties(sample_blend
  PROPERTIES FOLDER "samples")

if(EMSCRIPTEN)
  # Resource files are embedded to the output file with emscripten
  set_target_properties(sample_blend
    PROPERTIES LINK_FLAGS "--embed-file media --embed-file README")

  # Builds the google gadget file
  add_custom_target(sample_blend_gadget ALL
    DEPENDS
      "${CMAKE_SOURCE_DIR}/samples/framework/gadget.cmake"
      "${CMAKE_SOURCE_DIR}/samples/framework/gadget.xml.in"
      "${CMAKE_CURRENT_BINARY_DIR}/sample_blend.js"
    COMMAND ${CMAKE_COMMAND}
      -DOZZ_INPUT_JS="${CMAKE_CURRENT_BINARY_DIR}/sample_blend.js"
      -DOZZ_OUTPUT_XML="${CMAKE_CURRENT_BINARY_DIR}/sample_blend.xml"
      -P "${CMAKE_SOURCE_DIR}/samples/framework/gadget.cmake"
    DEPENDS sample_blend)

  install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/sample_blend.html
    ${CMAKE_CURRENT_BINARY_DIR}/sample_blend.js
    ${CMAKE_CURRENT_BINARY_DIR}/sample_blend.xml
    DESTINATION bin/samples/blend)
else()
  install(TARGETS sample_blend DESTINATION bin/samples/blend)
  install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/media DESTINATION bin/samples/blend)
  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/README DESTINATION bin/samples/blend)
endif(EMSCRIPTEN)

add_test(NAME sample_blend COMMAND sample_blend "--max_idle_loops=${SAMPLE_TESTING_LOOPS}" ${SAMPLE_RENDER_ARGUMENT})
add_test(NAME sample_blend_path COMMAND sample_blend "--skeleton=media/skeleton.ozz" "--animation1=media/walk.ozz" "--animation2=media/jog.ozz"  "--animation3=media/run.ozz" "--max_idle_loops=${SAMPLE_TESTING_LOOPS}" ${SAMPLE_RENDER_ARGUMENT})
add_test(NAME sample_blend_invalid_skeleton_path COMMAND sample_blend "--skeleton=media/bad_skeleton.ozz" ${SAMPLE_RENDER_ARGUMENT})
set_tests_properties(sample_blend_invalid_skeleton_path PROPERTIES WILL_FAIL true)
add_test(NAME sample_blend_invalid_animation_path1 COMMAND sample_blend "--animation1=media/bad_animation.ozz" ${SAMPLE_RENDER_ARGUMENT})
set_tests_properties(sample_blend_invalid_animation_path1 PROPERTIES WILL_FAIL true)
add_test(NAME sample_blend_invalid_animation_path2 COMMAND sample_blend "--animation2=media/bad_animation.ozz" ${SAMPLE_RENDER_ARGUMENT})
set_tests_properties(sample_blend_invalid_animation_path2 PROPERTIES WILL_FAIL true)
add_test(NAME sample_blend_invalid_animation_path3 COMMAND sample_blend "--animation3=media/bad_animation.ozz" ${SAMPLE_RENDER_ARGUMENT})
set_tests_properties(sample_blend_invalid_animation_path3 PROPERTIES WILL_FAIL true)
