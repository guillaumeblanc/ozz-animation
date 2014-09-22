add_library(ozz_animation
  ../../../include/ozz/animation/runtime/animation.h
  ../../../include/ozz/animation/runtime/animation_archive.h
  animation.cc
  animation_keyframe.h
  ../../../include/ozz/animation/runtime/blending_job.h
  blending_job.cc
  ../../../include/ozz/animation/runtime/local_to_model_job.h
  local_to_model_job.cc
  ../../../include/ozz/animation/runtime/sampling_job.h
  sampling_job.cc
  ../../../include/ozz/animation/runtime/skeleton.h
  ../../../include/ozz/animation/runtime/skeleton_archive.h
  ../../../include/ozz/animation/runtime/skeleton_utils.h
  skeleton.cc
  skeleton_utils.cc)
set_target_properties(ozz_animation
  PROPERTIES FOLDER "ozz")

install(TARGETS ozz_animation DESTINATION lib)
