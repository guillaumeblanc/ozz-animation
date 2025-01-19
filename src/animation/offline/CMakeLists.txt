
add_library(ozz_animation_offline
  ${PROJECT_SOURCE_DIR}/include/ozz/animation/offline/export.h
  decimate.h
  ${PROJECT_SOURCE_DIR}/include/ozz/animation/offline/raw_animation.h
  raw_animation.cc
  raw_animation_archive.cc
  ${PROJECT_SOURCE_DIR}/include/ozz/animation/offline/raw_animation_utils.h
  raw_animation_utils.cc
  ${PROJECT_SOURCE_DIR}/include/ozz/animation/offline/animation_builder.h
  animation_builder.cc
  ${PROJECT_SOURCE_DIR}/include/ozz/animation/offline/animation_optimizer.h
  animation_optimizer.cc
  ${PROJECT_SOURCE_DIR}/include/ozz/animation/offline/additive_animation_builder.h
  additive_animation_builder.cc
  ${PROJECT_SOURCE_DIR}/include/ozz/animation/offline/motion_extractor.h
  motion_extractor.cc
  ${PROJECT_SOURCE_DIR}/include/ozz/animation/offline/raw_skeleton.h
  raw_skeleton.cc
  raw_skeleton_archive.cc
  ${PROJECT_SOURCE_DIR}/include/ozz/animation/offline/skeleton_builder.h
  skeleton_builder.cc
  ${PROJECT_SOURCE_DIR}/include/ozz/animation/offline/raw_track.h
  raw_track.cc
  ${PROJECT_SOURCE_DIR}/include/ozz/animation/offline/raw_track_utils.h
  raw_track_utils.cc
  ${PROJECT_SOURCE_DIR}/include/ozz/animation/offline/track_builder.h
  track_builder.cc
  ${PROJECT_SOURCE_DIR}/include/ozz/animation/offline/track_optimizer.h
  track_optimizer.cc)

target_compile_definitions(ozz_animation_offline PRIVATE $<$<BOOL:${BUILD_SHARED_LIBS}>:OZZ_BUILD_ANIMOFFLINE_LIB>)

target_link_libraries(ozz_animation_offline ozz_animation)

set_target_properties(ozz_animation_offline PROPERTIES FOLDER "ozz")

install(TARGETS ozz_animation_offline DESTINATION lib)

fuse_target("ozz_animation_offline")

add_subdirectory(tools) # tools target need to be defined before other targets that uses them
add_subdirectory(fbx)
add_subdirectory(gltf)

