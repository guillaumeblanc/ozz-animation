add_executable(demo_load
  demo_load.cc)
target_link_libraries(demo_load
  demo_framework
  ozz_animation
  ozz_options
  ozz_base)
add_custom_command(TARGET demo_load POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E make_directory media
  COMMAND dae2skel "--collada=${ozz_media_directory}/collada/skeleton_yup.dae" "--skeleton=media/skeleton.ozz"
  COMMAND dae2anim "--collada=${ozz_media_directory}/collada/animation.dae" "--skeleton=media/skeleton.ozz" "--animation=media/animation.ozz")

set_target_properties(demo_load
  PROPERTIES FOLDER "demo"
  DEPENDS dae2anim
  DEPENDS dae2skel)

install(TARGETS demo_load
  DESTINATION bin/demo/load)
install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/media
  DESTINATION bin/demo/load)

add_test(NAME demo_load COMMAND demo_load "--auto_exit_time=${DEMO_TESTING_TIME}")
add_test(NAME demo_load_path COMMAND demo_load "--skeleton=media/skeleton.ozz" "--animation=media/animation.ozz" "--auto_exit_time=${DEMO_TESTING_TIME}")
add_test(NAME demo_load_invalid_skeleton_path COMMAND demo_load "--skeleton=media/bad_skeleton.ozz")
set_tests_properties(demo_load_invalid_skeleton_path PROPERTIES WILL_FAIL true)
add_test(NAME demo_load_invalid_animation_path COMMAND demo_load "--animation=media/bad_animation.ozz")
set_tests_properties(demo_load_invalid_animation_path PROPERTIES WILL_FAIL true)
