add_executable(demo_blend
  demo_blend.cc)
target_link_libraries(demo_blend
  demo_framework
  ozz_animation
  ozz_options
  ozz_base)
add_custom_command(TARGET demo_blend POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E make_directory media
  COMMAND dae2skel "--collada=${ozz_media_directory}/collada/skeleton_yup.dae" "--skeleton=media/skeleton.ozz"
  COMMAND dae2anim "--collada=${ozz_media_directory}/collada/animation.dae" "--skeleton=media/skeleton.ozz" "--animation=media/animation1.ozz"
  COMMAND dae2anim "--collada=${ozz_media_directory}/collada/animation.dae" "--skeleton=media/skeleton.ozz" "--animation=media/animation2.ozz")

set_target_properties(demo_blend
  PROPERTIES FOLDER "demo"
  DEPENDS dae2anim
  DEPENDS dae2skel)

install(TARGETS demo_blend
  DESTINATION bin/demo/blend)
install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/media
  DESTINATION bin/demo/blend)

add_test(NAME demo_blend COMMAND demo_blend "--auto_exit_time=${DEMO_TESTING_TIME}")
add_test(NAME demo_blend_path COMMAND demo_blend "--skeleton=media/skeleton.ozz" "--animation1=media/animation1.ozz" "--animation2=media/animation2.ozz" "--auto_exit_time=${DEMO_TESTING_TIME}")
add_test(NAME demo_blend_invalid_skeleton_path COMMAND demo_blend "--skeleton=media/bad_skeleton.ozz")
set_tests_properties(demo_blend_invalid_skeleton_path PROPERTIES WILL_FAIL true)
add_test(NAME demo_blend_invalid_animation_path1 COMMAND demo_blend "--animation1=media/bad_animation.ozz")
set_tests_properties(demo_blend_invalid_animation_path1 PROPERTIES WILL_FAIL true)
add_test(NAME demo_blend_invalid_animation_path2 COMMAND demo_blend "--animation2=media/bad_animation.ozz")
set_tests_properties(demo_blend_invalid_animation_path2 PROPERTIES WILL_FAIL true)
