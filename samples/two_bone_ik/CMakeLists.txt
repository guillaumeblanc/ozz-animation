
add_custom_command(
  DEPENDS $<$<BOOL:${ozz_build_fbx}>:BUILD_DATA>
          "${CMAKE_CURRENT_LIST_DIR}/README.md"
          "${ozz_media_directory}/bin/robot_skeleton.ozz"
          "${ozz_media_directory}/bin/robot_track_target.ozz"
          "${ozz_media_directory}/bin/robot_track_aim.ozz"
  OUTPUT  "${CMAKE_CURRENT_BINARY_DIR}/README.md"
          "${CMAKE_CURRENT_BINARY_DIR}/media/skeleton.ozz"
          "${CMAKE_CURRENT_BINARY_DIR}/media/skeleton.ozz"
          "${CMAKE_CURRENT_BINARY_DIR}/media/track_target.ozz"
          "${CMAKE_CURRENT_BINARY_DIR}/media/track_aim.ozz"
  COMMAND ${CMAKE_COMMAND} -E copy "${ozz_media_directory}/bin/robot_skeleton.ozz" "./media/skeleton.ozz"
  COMMAND ${CMAKE_COMMAND} -E copy "${ozz_media_directory}/bin/robot_track_target.ozz" "./media/track_target.ozz"
  COMMAND ${CMAKE_COMMAND} -E copy "${ozz_media_directory}/bin/robot_track_aim.ozz" "./media/track_aim.ozz"
  COMMAND ${CMAKE_COMMAND} -E make_directory media
  COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_LIST_DIR}/README.md" .
  COMMAND ${CMAKE_COMMAND} -E copy "${ozz_media_directory}/bin/robot_skeleton.ozz" "./media/skeleton.ozz"
  COMMAND ${CMAKE_COMMAND} -E copy "${ozz_media_directory}/bin/robot_track_target.ozz" "./media/track_target.ozz"
  COMMAND ${CMAKE_COMMAND} -E copy "${ozz_media_directory}/bin/robot_track_aim.ozz" "./media/track_aim.ozz")

add_executable(sample_two_bone_ik
  sample_two_bone_ik.cc
  config.json
  "${CMAKE_CURRENT_BINARY_DIR}/README.md"
  "${CMAKE_CURRENT_BINARY_DIR}/media/skeleton.ozz"
  "${CMAKE_CURRENT_BINARY_DIR}/media/track_target.ozz"
  "${CMAKE_CURRENT_BINARY_DIR}/media/track_aim.ozz")
target_link_libraries(sample_two_bone_ik
  sample_framework
  ozz_animation_offline
  ozz_animation)

set_target_properties(sample_two_bone_ik
  PROPERTIES FOLDER "samples")

if(EMSCRIPTEN)
  # Resource files are embedded to the output file with emscripten
  set_target_properties(sample_two_bone_ik
    PROPERTIES LINK_FLAGS "--embed-file media --embed-file README.md --memory-init-file 0")

  install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/sample_two_bone_ik.html
    ${CMAKE_CURRENT_BINARY_DIR}/sample_two_bone_ik.js
    DESTINATION bin/samples/two_bone_ik)
else()
  install(TARGETS sample_two_bone_ik DESTINATION bin/samples/two_bone_ik)
  install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/media DESTINATION bin/samples/two_bone_ik)
  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/README.md DESTINATION bin/samples/two_bone_ik)
endif(EMSCRIPTEN)

add_test(NAME sample_two_bone_ik COMMAND sample_two_bone_ik "--max_idle_loops=${ozz_sample_testing_loops}" $<$<BOOL:${ozz_run_tests_headless}>:--norender>)

#add_test(NAME sample_two_bone_ik_path1 COMMAND sample_two_bone_ik "--skeleton=media/skeleton.ozz" "--max_idle_loops=${ozz_sample_testing_loops}" $<$<BOOL:${ozz_run_tests_headless}>:--norender>)
#add_test(NAME sample_two_bone_ik_invalid_skeleton_path COMMAND sample_two_bone_ik "--skeleton=media/bad_skeleton.ozz" $<$<BOOL:${ozz_run_tests_headless}>:--norender>)
#set_tests_properties(sample_two_bone_ik_invalid_skeleton_path PROPERTIES WILL_FAIL true)