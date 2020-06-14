if(EMSCRIPTEN)
  return()
endif()

# Sample requires thread libraries
find_package(Threads)
if (NOT Threads_FOUND)
  message("Multithread sample discarded because threading libraries aren't available.")
  return()
endif()

add_custom_command(
  DEPENDS $<$<BOOL:${ozz_build_fbx}>:BUILD_DATA>
          "${CMAKE_CURRENT_LIST_DIR}/README.md"
          "${ozz_media_directory}/bin/pab_skeleton.ozz"
          "${ozz_media_directory}/bin/pab_walk.ozz"
  OUTPUT  "${CMAKE_CURRENT_BINARY_DIR}/README.md"
          "${CMAKE_CURRENT_BINARY_DIR}/media/skeleton.ozz"
          "${CMAKE_CURRENT_BINARY_DIR}/media/animation.ozz"
  COMMAND ${CMAKE_COMMAND} -E make_directory media
  COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_LIST_DIR}/README.md" .
  COMMAND ${CMAKE_COMMAND} -E copy "${ozz_media_directory}/bin/pab_skeleton.ozz" "./media/skeleton.ozz"
  COMMAND ${CMAKE_COMMAND} -E copy "${ozz_media_directory}/bin/pab_walk.ozz" "./media/animation.ozz"
  VERBATIM)

add_executable(sample_multithread
  sample_multithread.cc
  "${CMAKE_CURRENT_BINARY_DIR}/README.md"
  "${CMAKE_CURRENT_BINARY_DIR}/media/skeleton.ozz"
  "${CMAKE_CURRENT_BINARY_DIR}/media/animation.ozz")

target_link_libraries(sample_multithread
  sample_framework
  ${CMAKE_THREAD_LIBS_INIT})

set_target_properties(sample_multithread
  PROPERTIES FOLDER "samples")

if(EMSCRIPTEN)
  set_target_properties(sample_multithread PROPERTIES COMPILE_FLAGS "-s USE_PTHREADS=1")

  # Resource files are embedded to the output file with emscripten
  set_target_properties(sample_multithread
    PROPERTIES LINK_FLAGS "-s TOTAL_MEMORY=100663296 -s USE_PTHREADS=1 --embed-file media --embed-file README.md --memory-init-file 0")

  install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/sample_multithread.html
    ${CMAKE_CURRENT_BINARY_DIR}/sample_multithread.js
    ${CMAKE_CURRENT_BINARY_DIR}/sample_multithread.wasm
    DESTINATION bin/samples/multithread)
else()
  install(TARGETS sample_multithread DESTINATION bin/samples/multithread)
  install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/media DESTINATION bin/samples/multithread)
  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/README.md DESTINATION bin/samples/multithread)
endif(EMSCRIPTEN)

add_test(NAME sample_multithread COMMAND sample_multithread "--max_idle_loops=${ozz_sample_testing_loops}" $<$<BOOL:${ozz_run_tests_headless}>:--norender>)
add_test(NAME sample_multithread_path COMMAND sample_multithread "--skeleton=media/skeleton.ozz" "--animation=media/animation.ozz" "--max_idle_loops=${ozz_sample_testing_loops}" $<$<BOOL:${ozz_run_tests_headless}>:--norender>)
add_test(NAME sample_multithread_invalid_skeleton_path COMMAND sample_multithread "--skeleton=media/bad_skeleton.ozz" $<$<BOOL:${ozz_run_tests_headless}>:--norender>)
set_tests_properties(sample_multithread_invalid_skeleton_path PROPERTIES WILL_FAIL true)
add_test(NAME sample_multithread_invalid_animation_path1 COMMAND sample_multithread "--animation1=media/bad_animation.ozz" $<$<BOOL:${ozz_run_tests_headless}>:--norender>)
set_tests_properties(sample_multithread_invalid_animation_path1 PROPERTIES WILL_FAIL true)
add_test(NAME sample_multithread_invalid_animation_path2 COMMAND sample_multithread "--animation2=media/bad_animation.ozz" $<$<BOOL:${ozz_run_tests_headless}>:--norender>)
set_tests_properties(sample_multithread_invalid_animation_path2 PROPERTIES WILL_FAIL true)
