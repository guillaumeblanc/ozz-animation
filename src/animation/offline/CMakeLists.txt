add_library(ozz_animation_offline STATIC
  ${ozz_root_dir}/include/ozz/animation/offline/raw_animation.h
  raw_animation.cc
  raw_animation_archive.cc
  ${ozz_root_dir}/include/ozz/animation/offline/raw_animation_utils.h
  raw_animation_utils.cc
  ${ozz_root_dir}/include/ozz/animation/offline/animation_builder.h
  animation_builder.cc
  ${ozz_root_dir}/include/ozz/animation/offline/animation_optimizer.h
  animation_optimizer.cc
  ${ozz_root_dir}/include/ozz/animation/offline/additive_animation_builder.h
  additive_animation_builder.cc
  ${ozz_root_dir}/include/ozz/animation/offline/raw_skeleton.h
  raw_skeleton.cc
  raw_skeleton_archive.cc
  ${ozz_root_dir}/include/ozz/animation/offline/skeleton_builder.h
  skeleton_builder.cc)
set_target_properties(ozz_animation_offline PROPERTIES FOLDER "ozz")

install(TARGETS ozz_animation_offline DESTINATION lib)

fuse_target("ozz_animation_offline")

add_subdirectory(fbx)
add_subdirectory(tools)

