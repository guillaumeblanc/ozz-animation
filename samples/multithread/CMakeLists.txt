# Samples requires OpenGL package.
find_package(OpenMP)

if (OPENMP_FOUND)
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS}")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${OpenMP_EXE_LINKER_FLAGS}")

  add_executable(sample_multithread
    sample_multithread.cc
    README)
  target_link_libraries(sample_multithread
    sample_framework
    ozz_animation
    ozz_options
    ozz_base)
  add_custom_command(TARGET sample_multithread POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory media
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_LIST_DIR}/README .
    COMMAND dae2skel "--file=${ozz_media_directory}/collada/alain/skeleton.dae" "--skeleton=media/skeleton.ozz"
    COMMAND dae2anim "--file=${ozz_media_directory}/collada/alain/walk.dae" "--skeleton=media/skeleton.ozz" "--animation=media/animation.ozz")

  set_target_properties(sample_multithread
    PROPERTIES FOLDER "samples"
    DEPENDS dae2anim
    DEPENDS dae2skel)

  install(TARGETS sample_multithread DESTINATION bin/samples/multithread)
  install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/media DESTINATION bin/samples/multithread)
  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/README DESTINATION bin/samples/multithread)

  add_test(NAME sample_multithread COMMAND sample_multithread "--max_idle_loops=${SAMPLE_TESTING_LOOPS}" ${SAMPLE_RENDER_ARGUMENT})
  add_test(NAME sample_multithread_path COMMAND sample_multithread "--skeleton=media/skeleton.ozz" "--animation=media/animation.ozz" "--max_idle_loops=${SAMPLE_TESTING_LOOPS}" ${SAMPLE_RENDER_ARGUMENT})
  add_test(NAME sample_multithread_invalid_skeleton_path COMMAND sample_multithread "--skeleton=media/bad_skeleton.ozz" ${SAMPLE_RENDER_ARGUMENT})
  set_tests_properties(sample_multithread_invalid_skeleton_path PROPERTIES WILL_FAIL true)
  add_test(NAME sample_multithread_invalid_animation_path1 COMMAND sample_multithread "--animation1=media/bad_animation.ozz" ${SAMPLE_RENDER_ARGUMENT})
  set_tests_properties(sample_multithread_invalid_animation_path1 PROPERTIES WILL_FAIL true)
  add_test(NAME sample_multithread_invalid_animation_path2 COMMAND sample_multithread "--animation2=media/bad_animation.ozz" ${SAMPLE_RENDER_ARGUMENT})
  set_tests_properties(sample_multithread_invalid_animation_path2 PROPERTIES WILL_FAIL true)
endif()
