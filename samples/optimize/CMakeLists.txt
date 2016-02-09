# Build a runtime Skeleton and a RawAnimation but don't do any optimization so that the sample can do it.
add_custom_command(
  DEPENDS "${CMAKE_CURRENT_LIST_DIR}/README"
          "${ozz_media_directory}/collada/alain/skeleton.dae"
          "${ozz_media_directory}/collada/alain/atlas.dae"
          dae2skel
          dae2anim
  OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/README"
         "${CMAKE_CURRENT_BINARY_DIR}/media/skeleton.ozz"
         "${CMAKE_CURRENT_BINARY_DIR}/media/raw_animation.ozz"
  COMMAND ${CMAKE_COMMAND} -E make_directory media
  COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_LIST_DIR}/README .
  COMMAND dae2skel
    "--file=${ozz_media_directory}/collada/alain/skeleton.dae"
    "--skeleton=${CMAKE_CURRENT_BINARY_DIR}/media/skeleton.ozz"
  COMMAND dae2anim
    "--raw"
    "--nooptimize"
    "--file=${ozz_media_directory}/collada/alain/atlas.dae"
    "--skeleton=${CMAKE_CURRENT_BINARY_DIR}/media/skeleton.ozz"
    "--animation=${CMAKE_CURRENT_BINARY_DIR}/media/raw_animation.ozz")

add_executable(sample_optimize
  sample_optimize.cc
  "${CMAKE_CURRENT_BINARY_DIR}/README"
  "${CMAKE_CURRENT_BINARY_DIR}/media/skeleton.ozz"
  "${CMAKE_CURRENT_BINARY_DIR}/media/raw_animation.ozz")

target_link_libraries(sample_optimize
  ozz_animation_offline
  sample_framework)

set_target_properties(sample_optimize
  PROPERTIES FOLDER "samples")

if(EMSCRIPTEN)
  # Resource files are embedded to the output file with emscripten.
  set_target_properties(sample_optimize
    PROPERTIES LINK_FLAGS "--embed-file media --embed-file README")
    
  # Builds the google gadget file
  add_custom_target(sample_optimize_gadget ALL
    DEPENDS
      "${CMAKE_SOURCE_DIR}/samples/framework/gadget.cmake"
      "${CMAKE_SOURCE_DIR}/samples/framework/gadget.xml.in"
      "${CMAKE_CURRENT_BINARY_DIR}/sample_optimize.js"
    COMMAND ${CMAKE_COMMAND}
      -DOZZ_INPUT_JS="${CMAKE_CURRENT_BINARY_DIR}/sample_optimize.js"
      -DOZZ_OUTPUT_XML="${CMAKE_CURRENT_BINARY_DIR}/sample_optimize.xml"
      -P "${CMAKE_SOURCE_DIR}/samples/framework/gadget.cmake"
    DEPENDS sample_optimize)

  install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/sample_optimize.html
    ${CMAKE_CURRENT_BINARY_DIR}/sample_optimize.js
    ${CMAKE_CURRENT_BINARY_DIR}/sample_optimize.xml
    DESTINATION bin/samples/optimize)
else()
  install(TARGETS sample_optimize DESTINATION bin/samples/optimize)
  install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/media DESTINATION bin/samples/optimize)
  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/README DESTINATION bin/samples/optimize)
endif(EMSCRIPTEN)

add_test(NAME sample_optimize_default COMMAND sample_optimize "--max_idle_loops=${SAMPLE_TESTING_LOOPS}" ${SAMPLE_RENDER_ARGUMENT})
add_test(NAME sample_optimize_path1 COMMAND sample_optimize "--skeleton=media/skeleton.ozz" "--animation=media/raw_animation.ozz" "--max_idle_loops=${SAMPLE_TESTING_LOOPS}" ${SAMPLE_RENDER_ARGUMENT})
add_test(NAME sample_optimize_bin_v1_le COMMAND sample_optimize "--skeleton=${ozz_media_directory}/bin/skeleton_v1_le.ozz" "--animation=${ozz_media_directory}/bin/raw_animation_v1_le.ozz" "--max_idle_loops=${SAMPLE_TESTING_LOOPS}" ${SAMPLE_RENDER_ARGUMENT})
add_test(NAME sample_optimize_bin_v1_be COMMAND sample_optimize "--skeleton=${ozz_media_directory}/bin/skeleton_v1_be.ozz" "--animation=${ozz_media_directory}/bin/raw_animation_v1_be.ozz" "--max_idle_loops=${SAMPLE_TESTING_LOOPS}" ${SAMPLE_RENDER_ARGUMENT})

add_test(NAME sample_optimize_invalid_skeleton_path COMMAND sample_optimize "--skeleton=media/unexisting.ozz" "--animation=media/raw_animation.ozz" "--max_idle_loops=${SAMPLE_TESTING_LOOPS}" ${SAMPLE_RENDER_ARGUMENT})
set_tests_properties(sample_optimize_invalid_skeleton_path PROPERTIES WILL_FAIL true)
add_test(NAME sample_optimize_invalid_animation_path COMMAND sample_optimize "--skeleton=media/raw_skeleton.ozz" "--animation=media/unexisting.ozz" "--max_idle_loops=${SAMPLE_TESTING_LOOPS}" ${SAMPLE_RENDER_ARGUMENT})
set_tests_properties(sample_optimize_invalid_animation_path PROPERTIES WILL_FAIL true)
