add_library(ozz_animation
  ${ozz_root_dir}/include/ozz/animation/runtime/animation.h
  animation.cc
  animation_keyframe.h
  ${ozz_root_dir}/include/ozz/animation/runtime/blending_job.h
  blending_job.cc
  ${ozz_root_dir}/include/ozz/animation/runtime/local_to_model_job.h
  local_to_model_job.cc
  ${ozz_root_dir}/include/ozz/animation/runtime/sampling_job.h
  sampling_job.cc
  ${ozz_root_dir}/include/ozz/animation/runtime/skeleton.h
  skeleton.cc
  ${ozz_root_dir}/include/ozz/animation/runtime/skeleton_utils.h
  skeleton_utils.cc)
set_target_properties(ozz_animation
  PROPERTIES FOLDER "ozz")

install(TARGETS ozz_animation DESTINATION lib)

fuse_target("ozz_animation")
