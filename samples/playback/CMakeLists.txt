set(DAE_COMMAND_OUTPUT
  "${CMAKE_CURRENT_BINARY_DIR}/README"
  "${CMAKE_CURRENT_BINARY_DIR}/media/skeleton.ozz"
  "${CMAKE_CURRENT_BINARY_DIR}/media/skeleton_zup.ozz"
  "${CMAKE_CURRENT_BINARY_DIR}/media/skeleton_seymour.ozz"
  "${CMAKE_CURRENT_BINARY_DIR}/media/skeleton_astro_max.ozz"
  "${CMAKE_CURRENT_BINARY_DIR}/media/skeleton_astro_maya.ozz"
  "${CMAKE_CURRENT_BINARY_DIR}/media/animation.ozz"
  "${CMAKE_CURRENT_BINARY_DIR}/media/animation_seymour.ozz"
  "${CMAKE_CURRENT_BINARY_DIR}/media/animation_astro_max.ozz"
  "${CMAKE_CURRENT_BINARY_DIR}/media/animation_astro_maya.ozz")

add_custom_command(
  DEPENDS "${CMAKE_CURRENT_LIST_DIR}/README"
          "${ozz_media_directory}/collada/alain/skeleton.dae"
          "${ozz_media_directory}/collada/alain/skeleton_zup.dae"
          "${ozz_media_directory}/collada/alain/atlas.dae"
          "${ozz_media_directory}/collada/seymour.dae"
          "${ozz_media_directory}/collada/astro_max.dae"
          "${ozz_media_directory}/collada/astro_maya.dae"
          dae2skel
          dae2anim
  OUTPUT ${DAE_COMMAND_OUTPUT}
  COMMAND ${CMAKE_COMMAND} -E make_directory media
  COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_LIST_DIR}/README .
  COMMAND dae2skel
    "--file=${ozz_media_directory}/collada/alain/skeleton.dae"
    "--skeleton=${CMAKE_CURRENT_BINARY_DIR}/media/skeleton.ozz"
  COMMAND dae2skel
    "--file=${ozz_media_directory}/collada/alain/skeleton_zup.dae"
    "--skeleton=${CMAKE_CURRENT_BINARY_DIR}/media/skeleton_zup.ozz"
  COMMAND dae2skel
    "--file=${ozz_media_directory}/collada/seymour.dae"
    "--skeleton=${CMAKE_CURRENT_BINARY_DIR}/media/skeleton_seymour.ozz"
  COMMAND dae2skel
    "--file=${ozz_media_directory}/collada/astro_max.dae"
    "--skeleton=${CMAKE_CURRENT_BINARY_DIR}/media/skeleton_astro_max.ozz"
  COMMAND dae2skel
    "--file=${ozz_media_directory}/collada/astro_maya.dae"
    "--skeleton=${CMAKE_CURRENT_BINARY_DIR}/media/skeleton_astro_maya.ozz"
  COMMAND dae2anim
    "--file=${ozz_media_directory}/collada/alain/atlas.dae"
    "--skeleton=${CMAKE_CURRENT_BINARY_DIR}/media/skeleton.ozz"
    "--animation=${CMAKE_CURRENT_BINARY_DIR}/media/animation.ozz"
  COMMAND dae2anim
    "--file=${ozz_media_directory}/collada/seymour.dae"
    "--skeleton=${CMAKE_CURRENT_BINARY_DIR}/media/skeleton_seymour.ozz"
    "--animation=${CMAKE_CURRENT_BINARY_DIR}/media/animation_seymour.ozz"
  COMMAND dae2anim
    "--file=${ozz_media_directory}/collada/astro_max.dae"
    "--skeleton=${CMAKE_CURRENT_BINARY_DIR}/media/skeleton_astro_max.ozz"
    "--animation=${CMAKE_CURRENT_BINARY_DIR}/media/animation_astro_max.ozz"
  COMMAND dae2anim
    "--file=${ozz_media_directory}/collada/astro_maya.dae"
    "--skeleton=${CMAKE_CURRENT_BINARY_DIR}/media/skeleton_astro_maya.ozz"
    "--animation=${CMAKE_CURRENT_BINARY_DIR}/media/animation_astro_maya.ozz")

# Also pushes fbx files if supported
if(ozz_build_fbx)
  set(FBX_COMMAND_OUTPUT
    "${CMAKE_CURRENT_BINARY_DIR}/media/skeleton_fbx.ozz"
    "${CMAKE_CURRENT_BINARY_DIR}/media/animation_fbx.ozz"
    "${CMAKE_CURRENT_BINARY_DIR}/media/skeleton_mixed.ozz"
    "${CMAKE_CURRENT_BINARY_DIR}/media/animation_mixed.ozz")

  add_custom_command(
    DEPENDS "${ozz_media_directory}/fbx/alain/skeleton.fbx"
            "${ozz_media_directory}/fbx/alain/run.fbx"
            "${ozz_media_directory}/fbx/alain/jog.fbx"
            "${ozz_media_directory}/collada/alain/skeleton.dae"
          fbx2skel
          fbx2anim
    OUTPUT ${FBX_COMMAND_OUTPUT}
    COMMAND ${CMAKE_COMMAND} -E make_directory media
    COMMAND fbx2skel
      "--file=${ozz_media_directory}/fbx/alain/skeleton.fbx"
      "--skeleton=${CMAKE_CURRENT_BINARY_DIR}/media/skeleton_fbx.ozz"
    COMMAND fbx2anim
      "--file=${ozz_media_directory}/fbx/alain/run.fbx"
      "--skeleton=${CMAKE_CURRENT_BINARY_DIR}/media/skeleton_fbx.ozz"
      "--animation=${CMAKE_CURRENT_BINARY_DIR}/media/animation_fbx.ozz"
    COMMAND dae2skel
      "--file=${ozz_media_directory}/collada/alain/skeleton.dae"
      "--skeleton=${CMAKE_CURRENT_BINARY_DIR}/media/skeleton_mixed.ozz"
    COMMAND fbx2anim
      "--file=${ozz_media_directory}/fbx/alain/jog.fbx"
      "--skeleton=${CMAKE_CURRENT_BINARY_DIR}/media/skeleton_mixed.ozz"
      "--animation=${CMAKE_CURRENT_BINARY_DIR}/media/animation_mixed.ozz")
endif()

add_executable(sample_playback
  sample_playback.cc
  "${CMAKE_CURRENT_BINARY_DIR}/README"
  "${CMAKE_CURRENT_BINARY_DIR}/media/skeleton.ozz"
  "${CMAKE_CURRENT_BINARY_DIR}/media/animation.ozz"
  ${FBX_COMMAND_OUTPUT})
target_link_libraries(sample_playback
  sample_framework
  ozz_animation
  ozz_options
  ozz_base)

set_target_properties(sample_playback
  PROPERTIES FOLDER "samples")

if(EMSCRIPTEN)
  # Resource files are embedded to the output file with emscripten
  set_target_properties(sample_playback
    PROPERTIES LINK_FLAGS "--embed-file media --embed-file README")
    
  # Builds the google gadget file
  add_custom_target(sample_playback_gadget ALL
    DEPENDS
      "${CMAKE_SOURCE_DIR}/samples/framework/gadget.cmake"
      "${CMAKE_SOURCE_DIR}/samples/framework/gadget.xml.in"
      "${CMAKE_CURRENT_BINARY_DIR}/sample_playback.js"
    COMMAND ${CMAKE_COMMAND}
      -DOZZ_INPUT_JS="${CMAKE_CURRENT_BINARY_DIR}/sample_playback.js"
      -DOZZ_OUTPUT_XML="${CMAKE_CURRENT_BINARY_DIR}/sample_playback.xml"
      -P "${CMAKE_SOURCE_DIR}/samples/framework/gadget.cmake"
    DEPENDS sample_playback)

  install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/sample_playback.html
    ${CMAKE_CURRENT_BINARY_DIR}/sample_playback.js
    ${CMAKE_CURRENT_BINARY_DIR}/sample_playback.xml
    DESTINATION bin/samples/playback)
else()
  install(TARGETS sample_playback DESTINATION bin/samples/playback)
  install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/media DESTINATION bin/samples/playback)
  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/README DESTINATION bin/samples/playback)
endif(EMSCRIPTEN)

add_test(NAME sample_playback COMMAND sample_playback "--max_idle_loops=${SAMPLE_TESTING_LOOPS}" ${SAMPLE_RENDER_ARGUMENT})
add_test(NAME sample_playback_path1 COMMAND sample_playback "--skeleton=media/skeleton.ozz" "--animation=media/animation.ozz" "--max_idle_loops=${SAMPLE_TESTING_LOOPS}" ${SAMPLE_RENDER_ARGUMENT})
add_test(NAME sample_playback_zup COMMAND sample_playback  "--skeleton=media/skeleton_zup.ozz" "--animation=media/animation.ozz" "--max_idle_loops=${SAMPLE_TESTING_LOOPS}" ${SAMPLE_RENDER_ARGUMENT})
add_test(NAME sample_playback_seymour COMMAND sample_playback  "--skeleton=media/skeleton_seymour.ozz" "--animation=media/animation_seymour.ozz" "--max_idle_loops=${SAMPLE_TESTING_LOOPS}" ${SAMPLE_RENDER_ARGUMENT})
add_test(NAME sample_playback_astro_max COMMAND sample_playback  "--skeleton=media/skeleton_astro_max.ozz" "--animation=media/animation_astro_max.ozz" "--max_idle_loops=${SAMPLE_TESTING_LOOPS}" ${SAMPLE_RENDER_ARGUMENT})
add_test(NAME sample_playback_astro_maya COMMAND sample_playback  "--skeleton=media/skeleton_astro_maya.ozz" "--animation=media/animation_astro_maya.ozz" "--max_idle_loops=${SAMPLE_TESTING_LOOPS}" ${SAMPLE_RENDER_ARGUMENT})
add_test(NAME sample_playback_v2_le COMMAND sample_playback  "--skeleton=${ozz_media_directory}/bin/skeleton_v1_le.ozz" "--animation=${ozz_media_directory}/bin/animation_v2_le.ozz" "--max_idle_loops=${SAMPLE_TESTING_LOOPS}" ${SAMPLE_RENDER_ARGUMENT})
add_test(NAME sample_playback_v2_be COMMAND sample_playback  "--skeleton=${ozz_media_directory}/bin/skeleton_v1_be.ozz" "--animation=${ozz_media_directory}/bin/animation_v2_be.ozz" "--max_idle_loops=${SAMPLE_TESTING_LOOPS}" ${SAMPLE_RENDER_ARGUMENT})

add_test(NAME sample_playback_invalid_skeleton_path COMMAND sample_playback "--skeleton=media/bad_skeleton.ozz" ${SAMPLE_RENDER_ARGUMENT})
set_tests_properties(sample_playback_invalid_skeleton_path PROPERTIES WILL_FAIL true)
add_test(NAME sample_playback_invalid_animation_path COMMAND sample_playback "--animation=media/bad_animation.ozz" ${SAMPLE_RENDER_ARGUMENT})
set_tests_properties(sample_playback_invalid_animation_path PROPERTIES WILL_FAIL true)

# Also pushes fbx tests if supported
if(ozz_build_fbx)
  add_test(NAME sample_playback_fbx COMMAND sample_playback "--skeleton=media/skeleton_fbx.ozz" "--animation=media/animation_fbx.ozz" "--max_idle_loops=${SAMPLE_TESTING_LOOPS}" ${SAMPLE_RENDER_ARGUMENT})
  add_test(NAME sample_playback_mixed COMMAND sample_playback "--skeleton=media/skeleton_mixed.ozz" "--animation=media/animation_mixed.ozz" "--max_idle_loops=${SAMPLE_TESTING_LOOPS}" ${SAMPLE_RENDER_ARGUMENT})
endif()
