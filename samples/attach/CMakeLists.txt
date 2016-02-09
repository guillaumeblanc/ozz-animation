add_custom_command(
  DEPENDS "${CMAKE_CURRENT_LIST_DIR}/README"
          "${ozz_media_directory}/collada/alain/skeleton.dae"
          "${ozz_media_directory}/collada/alain/walk.dae"
          dae2skel
          dae2anim
  OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/README"
         "${CMAKE_CURRENT_BINARY_DIR}/media/skeleton.ozz"
         "${CMAKE_CURRENT_BINARY_DIR}/media/animation.ozz"
  COMMAND ${CMAKE_COMMAND} -E make_directory media
  COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_LIST_DIR}/README .
  COMMAND dae2skel
    "--file=${ozz_media_directory}/collada/alain/skeleton.dae"
    "--skeleton=${CMAKE_CURRENT_BINARY_DIR}/media/skeleton.ozz"
  COMMAND dae2anim
    "--file=${ozz_media_directory}/collada/alain/walk.dae"
    "--skeleton=${CMAKE_CURRENT_BINARY_DIR}/media/skeleton.ozz"
    "--animation=${CMAKE_CURRENT_BINARY_DIR}/media/animation.ozz")

add_executable(sample_attach
  sample_attach.cc
  "${CMAKE_CURRENT_BINARY_DIR}/README"
  "${CMAKE_CURRENT_BINARY_DIR}/media/skeleton.ozz"
  "${CMAKE_CURRENT_BINARY_DIR}/media/animation.ozz")

target_link_libraries(sample_attach
  sample_framework)

set_target_properties(sample_attach
  PROPERTIES FOLDER "samples")

if(EMSCRIPTEN)
  # Resource files are embedded to the output file with emscripten
  set_target_properties(sample_attach
    PROPERTIES LINK_FLAGS "--embed-file media --embed-file README")

  # Builds the google gadget file
  add_custom_target(sample_attach_gadget ALL
    DEPENDS
      "${CMAKE_SOURCE_DIR}/samples/framework/gadget.cmake"
      "${CMAKE_SOURCE_DIR}/samples/framework/gadget.xml.in"
      "${CMAKE_CURRENT_BINARY_DIR}/sample_attach.js"
    COMMAND ${CMAKE_COMMAND}
      -DOZZ_INPUT_JS="${CMAKE_CURRENT_BINARY_DIR}/sample_attach.js"
      -DOZZ_OUTPUT_XML="${CMAKE_CURRENT_BINARY_DIR}/sample_attach.xml"
      -P "${CMAKE_SOURCE_DIR}/samples/framework/gadget.cmake"
    DEPENDS sample_attach)

  install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/sample_attach.html
    ${CMAKE_CURRENT_BINARY_DIR}/sample_attach.js
    ${CMAKE_CURRENT_BINARY_DIR}/sample_attach.xml
    DESTINATION bin/samples/attach)
else()
  install(TARGETS sample_attach DESTINATION bin/samples/attach)
  install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/media DESTINATION bin/samples/attach)
  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/README DESTINATION bin/samples/attach)
endif(EMSCRIPTEN)

add_test(NAME sample_attach COMMAND sample_attach "--max_idle_loops=${SAMPLE_TESTING_LOOPS}" ${SAMPLE_RENDER_ARGUMENT})
add_test(NAME sample_attach_load_path COMMAND sample_attach "--skeleton=media/skeleton.ozz" "--animation=media/animation.ozz" "--max_idle_loops=${SAMPLE_TESTING_LOOPS}" ${SAMPLE_RENDER_ARGUMENT})
add_test(NAME sample_attach_load_invalid_skeleton_path COMMAND sample_attach "--skeleton=media/bad_skeleton.ozz" ${SAMPLE_RENDER_ARGUMENT})
set_tests_properties(sample_attach_load_invalid_skeleton_path PROPERTIES WILL_FAIL true)
add_test(NAME sample_attach_load_invalid_animation_path COMMAND sample_attach "--animation=media/bad_animation.ozz" ${SAMPLE_RENDER_ARGUMENT})
set_tests_properties(sample_attach_load_invalid_animation_path PROPERTIES WILL_FAIL true)
