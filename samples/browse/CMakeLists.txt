# Gather data from animations_bin_path folder
set(animations_bin_path "${ozz_media_directory}/bin/quaternius")

# List animations in a file for the sample to read, exclude raw animations that are not supported by the sample.
file(GLOB animations_list LIST_DIRECTORIES false RELATIVE ${animations_bin_path} "${animations_bin_path}/Rig_*.ozz")
list(FILTER animations_list EXCLUDE REGEX "_raw")
file(GLOB animations_path_list LIST_DIRECTORIES false "${animations_bin_path}/Rig_*.ozz")
list(FILTER animations_path_list EXCLUDE REGEX "_raw")
file(GENERATE OUTPUT "./media/animations.txt" CONTENT "$<JOIN:${animations_list},\n>")

set(animations_sample_path_list ${animations_path_list})
list(TRANSFORM animations_sample_path_list REPLACE "${animations_bin_path}/" "${CMAKE_CURRENT_BINARY_DIR}/media/")

add_custom_command(
  DEPENDS $<$<BOOL:${ozz_build_fbx}>:BUILD_DATA>
          $<$<BOOL:${ozz_build_fbx}>:BUILD_DATA_SAMPLE>
          "${CMAKE_CURRENT_LIST_DIR}/README.md"
          "${ozz_media_directory}/bin/quaternius/skeleton.ozz"
          "${ozz_media_directory}/bin/quaternius/mesh.ozz"
          ${animations_path_list}
  OUTPUT  "${CMAKE_CURRENT_BINARY_DIR}/README.md"
          "${CMAKE_CURRENT_BINARY_DIR}/media/skeleton.ozz"
          "${CMAKE_CURRENT_BINARY_DIR}/media/mesh.ozz"
          ${animations_sample_path_list}
  COMMAND ${CMAKE_COMMAND} -E make_directory media
  COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_LIST_DIR}/README.md" .
  COMMAND ${CMAKE_COMMAND} -E copy_directory "${ozz_media_directory}/bin/quaternius" "./media"
  VERBATIM)

add_executable(sample_browse
  sample_browse.cc
  "${CMAKE_CURRENT_BINARY_DIR}/README.md"
  "${CMAKE_CURRENT_BINARY_DIR}/media/skeleton.ozz"
  ${animations_sample_path_list})
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
add_test(NAME sample_browse_invalid_mesh_path COMMAND sample_browse "--mesh=media/bad_mesh.ozz" $<$<BOOL:${ozz_run_tests_headless}>:--norender>)
set_tests_properties(sample_browse_invalid_mesh_path PROPERTIES WILL_FAIL true)