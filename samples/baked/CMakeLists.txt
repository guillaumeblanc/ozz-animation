# Option to build baked data is disabled by default as it is very costly.
option(ozz_baked_sample_build_data "Build baked sample data" OFF)

# Build baked skeleton import tool.
if(ozz_build_fbx)
  add_executable(sample_fbx2baked
    fbx2baked.cc)
  target_link_libraries(sample_fbx2baked
    ozz_animation_fbx
    ozz_animation_offline_skel_tools)
    
  target_include_directories(sample_fbx2baked PUBLIC
    $<BUILD_INTERFACE:${FBX_INCLUDE_DIRS}>)

  set_target_properties(sample_fbx2baked
    PROPERTIES FOLDER "samples/tools")
endif()

# Build baked data
if(ozz_baked_sample_build_data AND
   TARGET sample_fbx2baked AND
   TARGET fbx2anim)
  add_custom_command(
    DEPENDS "${CMAKE_CURRENT_LIST_DIR}/README.md"
            "${ozz_media_directory}/fbx/ozz_baked.fbx"
            sample_fbx2baked
            fbx2anim
    OUTPUT "${ozz_media_directory}/bin/baked_skeleton.ozz"
           "${ozz_media_directory}/bin/baked_animation.ozz"
    COMMAND sample_fbx2baked
      "--file=${ozz_media_directory}/fbx/ozz_baked.fbx"
      "--skeleton=${ozz_media_directory}/bin/baked_skeleton.ozz"
    COMMAND fbx2anim
      "--file=${ozz_media_directory}/fbx/ozz_baked.fbx"
      "--skeleton=${ozz_media_directory}/bin/baked_skeleton.ozz"
      "--animation=${ozz_media_directory}/bin/baked_animation.ozz"
      "--translation=0.002" # Optimizes a bit more aggressively
      "--rotation=0.002"
      "--hierarchical=1")  # Skeleton hierarchy is flat, so no need for hierarchical tolerance
endif()

add_custom_command(
  DEPENDS "${CMAKE_CURRENT_LIST_DIR}/README.md"
          "${ozz_media_directory}/bin/baked_skeleton.ozz"
          "${ozz_media_directory}/bin/baked_animation.ozz"
  OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/README.md"
         "${CMAKE_CURRENT_BINARY_DIR}/media/skeleton.ozz"
         "${CMAKE_CURRENT_BINARY_DIR}/media/animation.ozz"
  COMMAND ${CMAKE_COMMAND} -E make_directory media
  COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_LIST_DIR}/README.md" .
  COMMAND ${CMAKE_COMMAND} -E copy "${ozz_media_directory}/bin/baked_skeleton.ozz" "./media/skeleton.ozz"
  COMMAND ${CMAKE_COMMAND} -E copy "${ozz_media_directory}/bin/baked_animation.ozz" "./media/animation.ozz")

add_executable(sample_baked
  sample_baked.cc
  "${CMAKE_CURRENT_BINARY_DIR}/README.md"
  "${CMAKE_CURRENT_BINARY_DIR}/media/skeleton.ozz"
  "${CMAKE_CURRENT_BINARY_DIR}/media/animation.ozz")

target_link_libraries(sample_baked
  sample_framework)

set_target_properties(sample_baked
  PROPERTIES FOLDER "samples")

if(EMSCRIPTEN)
  # Resource files are embedded to the output file with emscripten
  set_target_properties(sample_baked
    PROPERTIES LINK_FLAGS "--embed-file media --embed-file README.md --memory-init-file 0")

  install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/sample_baked.html
    ${CMAKE_CURRENT_BINARY_DIR}/sample_baked.js
    DESTINATION bin/samples/baked)
else()
  if(sample_fbx2baked)
    install(TARGETS sample_fbx2baked DESTINATION bin/samples/baked)
  endif()
  install(TARGETS sample_baked DESTINATION bin/samples/baked)
  install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/media DESTINATION bin/samples/baked)
  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/README.md DESTINATION bin/samples/baked)
endif(EMSCRIPTEN)

add_test(NAME sample_baked COMMAND sample_baked "--max_idle_loops=${ozz_sample_testing_loops}" $<$<BOOL:${ozz_run_tests_headless}>:--norender>)
add_test(NAME sample_baked_load_path COMMAND sample_baked "--skeleton=media/skeleton.ozz" "--animation=media/animation.ozz" "--max_idle_loops=${ozz_sample_testing_loops}" $<$<BOOL:${ozz_run_tests_headless}>:--norender>)
add_test(NAME sample_baked_load_invalid_skeleton_path COMMAND sample_baked "--skeleton=media/bad_skeleton.ozz" $<$<BOOL:${ozz_run_tests_headless}>:--norender>)
set_tests_properties(sample_baked_load_invalid_skeleton_path PROPERTIES WILL_FAIL true)
add_test(NAME sample_baked_load_invalid_animation_path COMMAND sample_baked "--animation=media/bad_animation.ozz" $<$<BOOL:${ozz_run_tests_headless}>:--norender>)
set_tests_properties(sample_baked_load_invalid_animation_path PROPERTIES WILL_FAIL true)
