add_executable(demo_collada_animation
  demo_collada_animation.cc)
target_link_libraries(demo_collada_animation
  demo_framework
  ozz_animation_collada
  ozz_animation_offline
  ozz_animation
  ozz_options
  ozz_base)
add_custom_command(TARGET demo_collada_animation POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E make_directory media
  COMMAND ${CMAKE_COMMAND} -E copy "${ozz_media_directory}/collada/skeleton_yup.dae" "media/skeleton_yup.dae"
  COMMAND ${CMAKE_COMMAND} -E copy "${ozz_media_directory}/collada/animation.dae" "media/animation.dae")

set_target_properties(demo_collada_animation
  PROPERTIES FOLDER "demo")

install(TARGETS demo_collada_animation
  DESTINATION bin/demo/collada)
install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/media
  DESTINATION bin/demo/collada)

add_test(NAME demo_collada_animation_default COMMAND demo_collada_animation "--auto_exit_time=.2")
add_test(NAME demo_collada_animation_path1 COMMAND demo_collada_animation "--skeleton=${ozz_media_directory}/collada/skeleton_yup.dae" "--animation=${ozz_media_directory}/collada/animation.dae" "--auto_exit_time=${DEMO_TESTING_TIME}")
add_test(NAME demo_collada_animation_path2 COMMAND demo_collada_animation "--skeleton=${ozz_media_directory}/collada/skeleton_zup.dae" "--animation=${ozz_media_directory}/collada/animation.dae" "--auto_exit_time=${DEMO_TESTING_TIME}")
add_test(NAME demo_collada_animation_path3 COMMAND demo_collada_animation "--skeleton=${ozz_media_directory}/collada/animation.dae" "--animation=${ozz_media_directory}/collada/animation.dae" "--auto_exit_time=${DEMO_TESTING_TIME}")
add_test(NAME demo_collada_animation_path4 COMMAND demo_collada_animation "--skeleton=${ozz_media_directory}/collada/seymour.dae" "--animation=${ozz_media_directory}/collada/seymour.dae" "--auto_exit_time=${DEMO_TESTING_TIME}")
add_test(NAME demo_collada_animation_invalid_skeleton_path COMMAND demo_collada_animation "--skeleton=${ozz_media_directory}/collada/skeleton_yup.dae" "--animation=${ozz_media_directory}/collada/invalide_filename.dae" "--auto_exit_time=${DEMO_TESTING_TIME}")
set_tests_properties(demo_collada_animation_invalid_skeleton_path PROPERTIES WILL_FAIL true)
add_test(NAME demo_collada_animation_invalid_animation_path COMMAND demo_collada_animation "--skeleton=${ozz_media_directory}/collada/invalide_filename.dae" "--animation=${ozz_media_directory}/collada/animation.dae" "--auto_exit_time=${DEMO_TESTING_TIME}")
set_tests_properties(demo_collada_animation_invalid_animation_path PROPERTIES WILL_FAIL true)
