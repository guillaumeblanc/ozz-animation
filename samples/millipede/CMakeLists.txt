add_executable(sample_millipede
  sample_millipede.cc
  README)

target_link_libraries(sample_millipede
  sample_framework
  ozz_animation_offline
  ozz_animation
  ozz_options
  ozz_base)

add_custom_command(TARGET sample_millipede POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_LIST_DIR}/README .)

set_target_properties(sample_millipede
  PROPERTIES FOLDER "samples")

install(TARGETS sample_millipede DESTINATION bin/samples/millipede)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/README DESTINATION bin/samples/millipede)

add_test(NAME sample_millipede COMMAND sample_millipede "--max_idle_loops=${SAMPLE_TESTING_LOOPS}" ${SAMPLE_RENDER_ARGUMENT})
