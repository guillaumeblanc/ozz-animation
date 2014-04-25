add_library(ozz_animation_offline
  ${CMAKE_SOURCE_DIR}/include/ozz/animation/offline/animation_builder.h
  animation_builder.cc
  ${CMAKE_SOURCE_DIR}/include/ozz/animation/offline/animation_optimizer.h
  animation_optimizer.cc
  ${CMAKE_SOURCE_DIR}/include/ozz/animation/offline/skeleton_builder.h
  skeleton_builder.cc)
set_target_properties(ozz_animation_offline PROPERTIES FOLDER "ozz")

install(TARGETS ozz_animation_offline DESTINATION lib)

add_subdirectory(collada)
add_subdirectory(fbx)
add_subdirectory(tools)

