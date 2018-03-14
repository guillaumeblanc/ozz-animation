add_subdirectory(${PROJECT_SOURCE_DIR}/extern/jsoncpp json)

add_library(ozz_animation_tools STATIC
  ${PROJECT_SOURCE_DIR}/include/ozz/animation/offline/tools/convert2ozz.h
  convert2ozz.cc
  convert2ozz_anim.h
  convert2ozz_anim.cc
  convert2ozz_config.h
  convert2ozz_config.cc
  convert2ozz_skel.h
  convert2ozz_skel.cc)
target_link_libraries(ozz_animation_tools
  ozz_animation_offline
  ozz_options
  json)
set_target_properties(ozz_animation_tools
  PROPERTIES FOLDER "ozz/tools")

install(TARGETS ozz_animation_tools DESTINATION lib)

fuse_target("ozz_animation_tools")
