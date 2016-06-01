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

  install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/sample_millipede.html
    ${CMAKE_CURRENT_BINARY_DIR}/sample_millipede.js
    DESTINATION bin/samples/millipede)
else()
  install(TARGETS sample_millipede DESTINATION bin/samples/millipede)
  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/README DESTINATION bin/samples/millipede)
endif(EMSCRIPTEN)

add_test(NAME sample_millipede COMMAND sample_millipede "--max_idle_loops=${ozz_sample_testing_loops}" $<$<BOOL:${ozz_run_tests_headless}>:--norender>)
