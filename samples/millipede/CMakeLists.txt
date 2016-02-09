add_custom_command(
  OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/README"
  DEPENDS "${CMAKE_CURRENT_LIST_DIR}/README"
  COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_LIST_DIR}/README .)

add_executable(sample_millipede
  sample_millipede.cc
  ${CMAKE_CURRENT_BINARY_DIR}/README)

target_link_libraries(sample_millipede
  ozz_animation_offline
  sample_framework)

set_target_properties(sample_millipede
  PROPERTIES FOLDER "samples")

if(EMSCRIPTEN)
  set_target_properties(sample_millipede
    PROPERTIES LINK_FLAGS "--embed-file README")
    
  # Builds the google gadget file
  add_custom_target(sample_millipede_gadget ALL
    DEPENDS
      "${CMAKE_SOURCE_DIR}/samples/framework/gadget.cmake"
      "${CMAKE_SOURCE_DIR}/samples/framework/gadget.xml.in"
      "${CMAKE_CURRENT_BINARY_DIR}/sample_millipede.js"
    COMMAND ${CMAKE_COMMAND}
      -DOZZ_INPUT_JS="${CMAKE_CURRENT_BINARY_DIR}/sample_millipede.js"
      -DOZZ_OUTPUT_XML="${CMAKE_CURRENT_BINARY_DIR}/sample_millipede.xml"
      -P "${CMAKE_SOURCE_DIR}/samples/framework/gadget.cmake"
    DEPENDS sample_millipede)

  install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/sample_millipede.html
    ${CMAKE_CURRENT_BINARY_DIR}/sample_millipede.js
    ${CMAKE_CURRENT_BINARY_DIR}/sample_millipede.xml
    DESTINATION bin/samples/millipede)
else()
  install(TARGETS sample_millipede DESTINATION bin/samples/millipede)
  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/README DESTINATION bin/samples/millipede)
endif(EMSCRIPTEN)

add_test(NAME sample_millipede COMMAND sample_millipede "--max_idle_loops=${SAMPLE_TESTING_LOOPS}" ${SAMPLE_RENDER_ARGUMENT})
