add_executable(demo_load_animation
  demo_load_animation.cc)
target_link_libraries(demo_load_animation
  demo_framework
  ozz_animation
  ozz_options
  ozz_base)
add_custom_command(TARGET demo_load_animation POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E make_directory media
  COMMAND dae2skel "--collada=${ozz_media_directory}/collada/skeleton_yup.dae" "--skeleton=media/skeleton.ozz"
  COMMAND dae2anim "--collada=${ozz_media_directory}/collada/animation.dae" "--skeleton=media/skeleton.ozz" "--animation=media/animation.ozz")

set_target_properties(demo_load_animation
  PROPERTIES FOLDER "demo"
  DEPENDS dae2anim
  DEPENDS dae2skel)

install(TARGETS demo_load_animation
  DESTINATION bin/demo/load)
install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/media
  DESTINATION bin/demo/load)

add_test(NAME demo_load_animation COMMAND demo_load_animation "--auto_exit_time=${DEMO_TESTING_TIME}")
add_test(NAME demo_load_animation_path COMMAND demo_load_animation "--skeleton=media/skeleton.ozz" "--animation=media/animation.ozz" "--auto_exit_time=${DEMO_TESTING_TIME}")
add_test(NAME demo_load_animation_invalid_skeleton_path COMMAND demo_load_animation "--skeleton=media/bad_skeleton.ozz" "--animation=media/animation.ozz" "--auto_exit_time=${DEMO_TESTING_TIME}")
set_tests_properties(demo_load_animation_invalid_skeleton_path PROPERTIES WILL_FAIL true)
add_test(NAME demo_load_animation_invalid_animation_path COMMAND demo_load_animation "--skeleton=media/skeleton.ozz" "--animation=media/bad_animation.ozz" "--auto_exit_time=${DEMO_TESTING_TIME}")
set_tests_properties(demo_load_animation_invalid_animation_path PROPERTIES WILL_FAIL true)
