add_executable(demo_millipede demo_millipede.cc)

target_link_libraries(demo_millipede
  demo_framework
  ozz_animation_offline
  ozz_animation
  ozz_base
  ozz_options)

set_target_properties(demo_millipede
  PROPERTIES FOLDER "demo")

install(TARGETS demo_millipede
  DESTINATION bin/demo/millipede)

add_test(NAME demo_millipede COMMAND demo_millipede "--auto_exit_time=${DEMO_TESTING_TIME}")
