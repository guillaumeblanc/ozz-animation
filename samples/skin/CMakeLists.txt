if(ozz_build_fbx)
  include_directories(${FBX_INCLUDE_DIRS})

  add_executable(sample_skin_fbx2skin
    fbx2skin.cc
    skin_mesh.h
    skin_mesh.cc)
  target_link_libraries(sample_skin_fbx2skin
    ozz_animation_fbx
    ozz_animation_offline
    ozz_animation
    ozz_options
    ozz_base)
  set_target_properties(sample_skin_fbx2skin
    PROPERTIES FOLDER "samples")

  add_custom_command(TARGET sample_skin_fbx2skin POST_BUILD
    DEPENDS
      "${ozz_media_directory}/fbx/alain/skeleton.fbx"
    COMMAND sample_skin_fbx2skin "--file=${ozz_media_directory}/fbx/alain/skeleton.fbx" "--skeleton=${ozz_media_directory}/bin/skeleton_v1_le.ozz" "--skin=${ozz_media_directory}/bin/mesh.ozz")

  set(ozz_mesh_generation_dependency sample_skin_fbx2skin)

  install(TARGETS sample_skin_fbx2skin DESTINATION bin/samples/skin)

  add_test(NAME sample_skin_fbx2skin COMMAND sample_skin_fbx2skin "--file=${ozz_media_directory}/fbx/alain/skeleton.fbx" "--skeleton=${ozz_media_directory}/bin/skeleton_v1_le.ozz" "--skin=${ozz_temp_directory}/mesh.ozz")

  add_test(NAME sample_skin_fbx2skin_invalid_file COMMAND sample_skin_fbx2skin "--file=${ozz_temp_directory}/dont_exist.fbx" "--skeleton=${ozz_media_directory}/bin/skeleton_v1_le.ozz" "--skin=${ozz_temp_directory}/should_not_exist.ozz")
  set_tests_properties(sample_skin_fbx2skin_invalid_file PROPERTIES WILL_FAIL true)

  add_test(NAME sample_skin_fbx2skin_invalid_skeleton COMMAND sample_skin_fbx2skin "--file=${ozz_media_directory}/fbx/alain/skeleton.fbx" "--skeleton=${ozz_media_directory}/bin/animation.ozz" "--skin=${ozz_temp_directory}/should_not_exist.ozz")
  set_tests_properties(sample_skin_fbx2skin_invalid_skeleton PROPERTIES WILL_FAIL true)

  # Ensures nothing was outputted. Unfortunately there's no way to ensure this test is ran last
  add_test(NAME sample_skin_fbx2skin_output COMMAND ${CMAKE_COMMAND} -E copy "${ozz_temp_directory}/should_not_exist.ozz" "${ozz_temp_directory}/should_not_exist_too.ozz")
  set_tests_properties(sample_skin_fbx2skin_output PROPERTIES WILL_FAIL true)
endif()

add_custom_command(
  DEPENDS "${CMAKE_CURRENT_LIST_DIR}/README"
          "${ozz_media_directory}/collada/alain/skeleton.dae"
          "${ozz_media_directory}/collada/alain/walk.dae"
          "${ozz_media_directory}/bin/mesh.ozz"
          ${ozz_mesh_generation_dependency}
  OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/README"
         "${CMAKE_CURRENT_BINARY_DIR}/media/skeleton.ozz"
         "${CMAKE_CURRENT_BINARY_DIR}/media/animation.ozz"
         "${CMAKE_CURRENT_BINARY_DIR}/media/mesh.ozz"
  COMMAND ${CMAKE_COMMAND} -E make_directory media
  COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_LIST_DIR}/README .
  COMMAND ${CMAKE_COMMAND} -E copy "${ozz_media_directory}/bin/mesh.ozz"
    "${CMAKE_CURRENT_BINARY_DIR}/media/mesh.ozz"
  COMMAND ${CMAKE_COMMAND} -E copy "${ozz_media_directory}/bin/skeleton_v1_le.ozz"
    "${CMAKE_CURRENT_BINARY_DIR}/media/skeleton.ozz"
  COMMAND ${CMAKE_COMMAND} -E copy "${ozz_media_directory}/bin/animation_v2_le.ozz"
    "${CMAKE_CURRENT_BINARY_DIR}/media/animation.ozz")

add_executable(sample_skin
  sample_skin.cc
  skin_mesh.h
  skin_mesh.cc
  "${CMAKE_CURRENT_BINARY_DIR}/README"
  "${CMAKE_CURRENT_BINARY_DIR}/media/skeleton.ozz"
  "${CMAKE_CURRENT_BINARY_DIR}/media/animation.ozz"
  "${CMAKE_CURRENT_BINARY_DIR}/media/mesh.ozz")

target_link_libraries(sample_skin
  sample_framework
  ozz_geometry
  ozz_animation
  ozz_options
  ozz_base)

set_target_properties(sample_skin
  PROPERTIES FOLDER "samples")

if(EMSCRIPTEN)
  # Resource files are embedded to the output file with emscripten
  set_target_properties(sample_skin
    PROPERTIES LINK_FLAGS "--embed-file media --embed-file README")

  # Builds the google gadget file
  add_custom_target(sample_skin_gadget ALL
    DEPENDS
      "${CMAKE_SOURCE_DIR}/samples/framework/gadget.cmake"
      "${CMAKE_SOURCE_DIR}/samples/framework/gadget.xml.in"
      "${CMAKE_CURRENT_BINARY_DIR}/sample_skin.js"
    COMMAND ${CMAKE_COMMAND}
      -DOZZ_INPUT_JS="${CMAKE_CURRENT_BINARY_DIR}/sample_skin.js"
      -DOZZ_OUTPUT_XML="${CMAKE_CURRENT_BINARY_DIR}/sample_skin.xml"
      -P "${CMAKE_SOURCE_DIR}/samples/framework/gadget.cmake"
    DEPENDS sample_skin)

  install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/sample_skin.html
    ${CMAKE_CURRENT_BINARY_DIR}/sample_skin.js
    ${CMAKE_CURRENT_BINARY_DIR}/sample_skin.xml
    DESTINATION bin/samples/skin)
else()
  install(TARGETS sample_skin DESTINATION bin/samples/skin)
  install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/media DESTINATION bin/samples/skin)
  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/README DESTINATION bin/samples/skin)
endif(EMSCRIPTEN)

add_test(NAME sample_skin COMMAND sample_skin "--max_idle_loops=${SAMPLE_TESTING_LOOPS}" ${SAMPLE_RENDER_ARGUMENT})
add_test(NAME sample_skin_load_path COMMAND sample_skin "--skeleton=media/skeleton.ozz" "--animation=media/animation.ozz" "--mesh=media/mesh.ozz" "--max_idle_loops=${SAMPLE_TESTING_LOOPS}" ${SAMPLE_RENDER_ARGUMENT})
add_test(NAME sample_skin_load_invalid_skeleton_path COMMAND sample_skin "--skeleton=media/bad_skeleton.ozz" ${SAMPLE_RENDER_ARGUMENT})
set_tests_properties(sample_skin_load_invalid_skeleton_path PROPERTIES WILL_FAIL true)
add_test(NAME sample_skin_load_invalid_animation_path COMMAND sample_skin "--animation=media/bad_animation.ozz" ${SAMPLE_RENDER_ARGUMENT})
set_tests_properties(sample_skin_load_invalid_animation_path PROPERTIES WILL_FAIL true)
add_test(NAME sample_skin_load_invalid_mesh_path COMMAND sample_skin "--mesh=media/bad_mesh.ozz" ${SAMPLE_RENDER_ARGUMENT})
set_tests_properties(sample_skin_load_invalid_mesh_path PROPERTIES WILL_FAIL true)
