add_library(ozz_animation_offline_tools
  ${CMAKE_SOURCE_DIR}/include/ozz/animation/offline/tools/convert2anim.h
  ${CMAKE_SOURCE_DIR}/include/ozz/animation/offline/tools/convert2skel.h
  convert2anim.cc
  convert2skel.cc)
set_target_properties(ozz_animation_offline_tools
  PROPERTIES FOLDER "ozz")

install(TARGETS ozz_animation_offline_tools DESTINATION lib)
