# List all expected animations
set(animations
  "pab_atlas.ozz"
  "pab_crossarms.ozz"
  "pab_crackhead.ozz"
  "pab_walk.ozz"
  "pab_jog.ozz"
  "pab_run.ozz")
# List them in a file for the sample to read
file(GENERATE OUTPUT "./media/animations.txt" CONTENT "$<JOIN:${animations},\n>")

# Copy all animations to the media directory
set(outputs "")
foreach(animation ${animations})
  add_custom_command(
    DEPENDS $<$<BOOL:${ozz_build_fbx}>:BUILD_DATA>
            "${ozz_media_directory}/bin/${animation}"
    OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/media/${animation}"
    COMMAND ${CMAKE_COMMAND} -E copy "${ozz_media_directory}/bin/${animation}" "./media"
    VERBATIM)
    list(APPEND outputs "./media/${animation}")
endforeach()

add_custom_command(
  DEPENDS $<$<BOOL:${ozz_build_fbx}>:BUILD_DATA>
          "${CMAKE_CURRENT_LIST_DIR}/README.md"
          "${ozz_media_directory}/bin/pab_skeleton.ozz"
  OUTPUT  "${CMAKE_CURRENT_BINARY_DIR}/README.md"
          "${CMAKE_CURRENT_BINARY_DIR}/media/skeleton.ozz"
  COMMAND ${CMAKE_COMMAND} -E make_directory media
  COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_LIST_DIR}/README.md" .
  COMMAND ${CMAKE_COMMAND} -E copy "${ozz_media_directory}/bin/pab_skeleton.ozz" "./media/skeleton.ozz"
  VERBATIM)

add_executable(sample_browse
  sample_browse.cc
  "${CMAKE_CURRENT_BINARY_DIR}/README.md"
  "${CMAKE_CURRENT_BINARY_DIR}/media/skeleton.ozz"
  ${outputs})
target_link_libraries(sample_browse
  sample_framework)
target_copy_shared_libraries(sample_browse)

set_target_properties(sample_browse
  PROPERTIES FOLDER "samples")

if(EMSCRIPTEN)
  # Resource files are embedded to the output file with emscripten
  set_target_properties(sample_browse
    PROPERTIES LINK_FLAGS "--embed-file media --embed-file README.md")

  install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/sample_browse.html
    ${CMAKE_CURRENT_BINARY_DIR}/sample_browse.js
    ${CMAKE_CURRENT_BINARY_DIR}/sample_browse.wasm
    DESTINATION bin/samples/browse)
else()
  install(TARGETS sample_browse DESTINATION bin/samples/browse)
  install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/media DESTINATION bin/samples/browse)
  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/README.md DESTINATION bin/samples/browse)
endif(EMSCRIPTEN)

add_test(NAME sample_browse COMMAND sample_browse "--max_idle_loops=${ozz_sample_testing_loops}" $<$<BOOL:${ozz_run_tests_headless}>:--norender>)
add_test(NAME sample_browse_path COMMAND sample_browse "--skeleton=media/skeleton.ozz" "--animations=media/animations.txt" "--max_idle_loops=${ozz_sample_testing_loops}" $<$<BOOL:${ozz_run_tests_headless}>:--norender>)
add_test(NAME sample_browse_invalid_skeleton_path COMMAND sample_browse "--skeleton=media/bad_skeleton.ozz" $<$<BOOL:${ozz_run_tests_headless}>:--norender>)
set_tests_properties(sample_browse_invalid_skeleton_path PROPERTIES WILL_FAIL true)
add_test(NAME sample_browse_invalid_animations_path COMMAND sample_browse "--animations=media/bad_list.txt" $<$<BOOL:${ozz_run_tests_headless}>:--norender>)
set_tests_properties(sample_browse_invalid_animations_path PROPERTIES WILL_FAIL true)