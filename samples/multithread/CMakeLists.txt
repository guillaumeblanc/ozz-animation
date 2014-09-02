# Samples requires OpenGL package.
find_package(OpenMP)

if (NOT OPENMP_FOUND)
  return()
endif()

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${OpenMP_EXE_LINKER_FLAGS}")

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

add_executable(sample_multithread
  sample_multithread.cc
  "${CMAKE_CURRENT_BINARY_DIR}/README"
  "${CMAKE_CURRENT_BINARY_DIR}/media/skeleton.ozz"
  "${CMAKE_CURRENT_BINARY_DIR}/media/animation.ozz")

target_link_libraries(sample_multithread
  sample_framework
  ozz_animation
  ozz_options
  ozz_base)

set_target_properties(sample_multithread
  PROPERTIES FOLDER "samples")

if(EMSCRIPTEN)
  # Resource files are embedded to the output file with emscripten
  set_target_properties(sample_multithread
    PROPERTIES LINK_FLAGS "--embed-file media --embed-file README")

  # Builds the google gadget file
  add_custom_target(sample_multithread_gadget ALL
    DEPENDS
      "${CMAKE_SOURCE_DIR}/samples/framework/gadget.cmake"
      "${CMAKE_SOURCE_DIR}/samples/framework/gadget.xml.in"
      "${CMAKE_CURRENT_BINARY_DIR}/sample_multithread.js"
    COMMAND ${CMAKE_COMMAND}
      -DOZZ_INPUT_JS="${CMAKE_CURRENT_BINARY_DIR}/sample_multithread.js"
      -DOZZ_OUTPUT_XML="${CMAKE_CURRENT_BINARY_DIR}/sample_multithread.xml"
      -P "${CMAKE_SOURCE_DIR}/samples/framework/gadget.cmake"
    DEPENDS sample_multithread)

  install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/sample_multithread.html
    ${CMAKE_CURRENT_BINARY_DIR}/sample_multithread.js
    ${CMAKE_CURRENT_BINARY_DIR}/sample_multithread.xml
    DESTINATION bin/samples/multithread)
else()
  install(TARGETS sample_multithread DESTINATION bin/samples/multithread)
  install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/media DESTINATION bin/samples/multithread)
  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/README DESTINATION bin/samples/multithread)
endif(EMSCRIPTEN)

add_test(NAME sample_multithread COMMAND sample_multithread "--max_idle_loops=${SAMPLE_TESTING_LOOPS}" ${SAMPLE_RENDER_ARGUMENT})
add_test(NAME sample_multithread_path COMMAND sample_multithread "--skeleton=media/skeleton.ozz" "--animation=media/animation.ozz" "--max_idle_loops=${SAMPLE_TESTING_LOOPS}" ${SAMPLE_RENDER_ARGUMENT})
add_test(NAME sample_multithread_invalid_skeleton_path COMMAND sample_multithread "--skeleton=media/bad_skeleton.ozz" ${SAMPLE_RENDER_ARGUMENT})
set_tests_properties(sample_multithread_invalid_skeleton_path PROPERTIES WILL_FAIL true)
add_test(NAME sample_multithread_invalid_animation_path1 COMMAND sample_multithread "--animation1=media/bad_animation.ozz" ${SAMPLE_RENDER_ARGUMENT})
set_tests_properties(sample_multithread_invalid_animation_path1 PROPERTIES WILL_FAIL true)
add_test(NAME sample_multithread_invalid_animation_path2 COMMAND sample_multithread "--animation2=media/bad_animation.ozz" ${SAMPLE_RENDER_ARGUMENT})
set_tests_properties(sample_multithread_invalid_animation_path2 PROPERTIES WILL_FAIL true)
