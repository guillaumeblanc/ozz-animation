add_subdirectory(${CMAKE_SOURCE_DIR}/extern/tinyxml tinyxml)
include_directories(${CMAKE_SOURCE_DIR}/extern/tinyxml)

add_library(ozz_animation_collada
  ${CMAKE_SOURCE_DIR}/include/ozz/animation/offline/collada/collada.h
  collada.cc
  collada_animation.h
  collada_animation.cc
  collada_base.h
  collada_base.cc
  collada_skeleton.h
  collada_skeleton.cc
  collada_transform.h
  collada_transform.cc)
target_link_libraries(ozz_animation_collada
  tinyxml)
set_target_properties(ozz_animation_collada
  PROPERTIES FOLDER "ozz")

install(TARGETS ozz_animation_collada DESTINATION lib)

add_executable(dae2skel
  dae2skel.cc)
target_link_libraries(dae2skel
  ozz_animation_offline_tools
  ozz_animation_collada
  ozz_animation_offline
  ozz_animation
  ozz_options
  ozz_base)
set_target_properties(dae2skel
  PROPERTIES FOLDER "ozz/tools")

install(TARGETS dae2skel DESTINATION bin/tools)
  
add_executable(dae2anim
  dae2anim.cc)
target_link_libraries(dae2anim
  ozz_animation_offline_tools
  ozz_animation_collada
  ozz_animation_offline
  ozz_animation
  ozz_options
  ozz_base)
set_target_properties(dae2anim
  PROPERTIES FOLDER "ozz/tools")

install(TARGETS dae2anim DESTINATION bin/tools)
