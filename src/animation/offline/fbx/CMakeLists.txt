if(NOT ozz_build_fbx)
  return()
endif()

include_directories(${FBX_INCLUDE_DIRS})

add_library(ozz_animation_fbx
  ${CMAKE_SOURCE_DIR}/include/ozz/animation/offline/fbx/fbx.h
  fbx.cc
  fbx_animation.h
  fbx_animation.cc
  fbx_base.h
  fbx_base.cc
  fbx_skeleton.h
  fbx_skeleton.cc)
target_link_libraries(ozz_animation_fbx
  debug ${FBX_LIBRARIES_DEBUG}
  optimized ${FBX_LIBRARIES})
set_target_properties(ozz_animation_fbx
  PROPERTIES FOLDER "ozz")

install(TARGETS ozz_animation_fbx DESTINATION lib)

add_executable(fbx2skel
  fbx2skel.cc)
target_link_libraries(fbx2skel
  ozz_animation_offline_tools
  ozz_animation_fbx
  ozz_animation_offline
  ozz_animation
  ozz_options
  ozz_base)
set_target_properties(fbx2skel
  PROPERTIES FOLDER "ozz/tools")

install(TARGETS fbx2skel DESTINATION bin/tools)
  
add_executable(fbx2anim
  fbx2anim.cc)
target_link_libraries(fbx2anim
  ozz_animation_offline_tools
  ozz_animation_fbx
  ozz_animation_offline
  ozz_animation
  ozz_options
  ozz_base)
set_target_properties(fbx2anim
  PROPERTIES FOLDER "ozz/tools")

install(TARGETS fbx2anim DESTINATION bin/tools)
