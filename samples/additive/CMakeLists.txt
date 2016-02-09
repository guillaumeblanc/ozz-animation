add_custom_command(
  DEPENDS "${CMAKE_CURRENT_LIST_DIR}/README"
          "${ozz_media_directory}/bin/mesh.ozz"
          "${ozz_media_directory}/collada/alain/skeleton.dae"
          "${ozz_media_directory}/collada/alain/walk.dae"
          "${ozz_media_directory}/collada/alain/crackhead.dae"
          dae2skel
          dae2anim
  OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/README"
         "${CMAKE_CURRENT_BINARY_DIR}/media/mesh.ozz"
         "${CMAKE_CURRENT_BINARY_DIR}/media/skeleton.ozz"
         "${CMAKE_CURRENT_BINARY_DIR}/media/walk.ozz"
         "${CMAKE_CURRENT_BINARY_DIR}/media/additive.ozz"
  COMMAND ${CMAKE_COMMAND} -E make_directory media
  COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_LIST_DIR}/README .
  COMMAND ${CMAKE_COMMAND} -E copy "${ozz_media_directory}/bin/mesh.ozz"
    "${CMAKE_CURRENT_BINARY_DIR}/media/mesh.ozz"
  COMMAND dae2skel
    "--file=${ozz_media_directory}/collada/alain/skeleton.dae"
    "--skeleton=${CMAKE_CURRENT_BINARY_DIR}/media/skeleton.ozz"
  COMMAND dae2anim
    "--file=${ozz_media_directory}/collada/alain/walk.dae"
    "--skeleton=${CMAKE_CURRENT_BINARY_DIR}/media/skeleton.ozz"
    "--animation=media/walk.ozz"
  COMMAND dae2anim
    "--additive"
    "--file=${ozz_media_directory}/collada/alain/crackhead.dae"
    "--skeleton=${CMAKE_CURRENT_BINARY_DIR}/media/skeleton.ozz"
    "--animation=${CMAKE_CURRENT_BINARY_DIR}/media/additive.ozz")

add_executable(sample_additive
  sample_additive.cc
  "${CMAKE_CURRENT_BINARY_DIR}/README"
  "${CMAKE_CURRENT_BINARY_DIR}/media/mesh.ozz"
  "${CMAKE_CURRENT_BINARY_DIR}/media/skeleton.ozz"
  "${CMAKE_CURRENT_BINARY_DIR}/media/walk.ozz"
  "${CMAKE_CURRENT_BINARY_DIR}/media/additive.ozz")
target_link_libraries(sample_additive
  sample_framework)

set_target_properties(sample_additive
  PROPERTIES FOLDER "samples")

if(EMSCRIPTEN)
  # Resource files are embedded to the output file with emscripten
  set_target_properties(sample_additive
    PROPERTIES LINK_FLAGS "--embed-file media --embed-file README")
    
  # Builds the google gadget file
  add_custom_target(sample_additive_gadget ALL
    DEPENDS
      "${CMAKE_SOURCE_DIR}/samples/framework/gadget.cmake"
      "${CMAKE_SOURCE_DIR}/samples/framework/gadget.xml.in"
      "${CMAKE_CURRENT_BINARY_DIR}/sample_additive.js"
    COMMAND ${CMAKE_COMMAND}
      -DOZZ_INPUT_JS="${CMAKE_CURRENT_BINARY_DIR}/sample_additive.js"
      -DOZZ_OUTPUT_XML="${CMAKE_CURRENT_BINARY_DIR}/sample_additive.xml"
      -P "${CMAKE_SOURCE_DIR}/samples/framework/gadget.cmake"
    DEPENDS sample_additive)

  install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/sample_additive.html
    ${CMAKE_CURRENT_BINARY_DIR}/sample_additive.js
    ${CMAKE_CURRENT_BINARY_DIR}/sample_additive.xml
    DESTINATION bin/samples/additive)
else()
  install(TARGETS sample_additive DESTINATION bin/samples/additive)
  install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/media DESTINATION bin/samples/additive)
  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/README DESTINATION bin/samples/additive)
endif(EMSCRIPTEN)

add_test(NAME sample_additive COMMAND sample_additive "--max_idle_loops=${SAMPLE_TESTING_LOOPS}" ${SAMPLE_RENDER_ARGUMENT})
add_test(NAME sample_additive_path COMMAND sample_additive "--skeleton=media/skeleton.ozz" "--animation=media/walk.ozz" "--additive_animation=media/additive.ozz" "--mesh=media/mesh.ozz" "--max_idle_loops=${SAMPLE_TESTING_LOOPS}" ${SAMPLE_RENDER_ARGUMENT})
add_test(NAME sample_additive_invalid_skeleton_path COMMAND sample_additive "--skeleton=media/bad_skeleton.ozz" ${SAMPLE_RENDER_ARGUMENT})
set_tests_properties(sample_additive_invalid_skeleton_path PROPERTIES WILL_FAIL true)
add_test(NAME sample_additive_invalid_animation_path1 COMMAND sample_additive "--animation=media/bad_animation.ozz" ${SAMPLE_RENDER_ARGUMENT})
set_tests_properties(sample_additive_invalid_animation_path1 PROPERTIES WILL_FAIL true)
add_test(NAME sample_additive_invalid_animation_path2 COMMAND sample_additive "--additive_animation=media/bad_animation.ozz" ${SAMPLE_RENDER_ARGUMENT})
set_tests_properties(sample_additive_invalid_animation_path2 PROPERTIES WILL_FAIL true)
add_test(NAME sample_additive_invalid_mesh_path COMMAND sample_additive "--mesh=media/bad_mesh.ozz" ${SAMPLE_RENDER_ARGUMENT})
set_tests_properties(sample_additive_invalid_mesh_path PROPERTIES WILL_FAIL true)
