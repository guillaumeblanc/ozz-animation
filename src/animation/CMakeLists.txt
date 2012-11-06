add_library(ozz_animation
  ../../include/ozz/animation/key_frame.h
  ../../include/ozz/animation/animation.h
  ../../include/ozz/animation/animation_serialize.h
  animation.cc
  ../../include/ozz/animation/local_to_model_job.h
  local_to_model_job.cc
  ../../include/ozz/animation/sampling_job.h
  sampling_job.cc
  ../../include/ozz/animation/skeleton.h
  ../../include/ozz/animation/skeleton_serialize.h
  skeleton.cc
  ../../include/ozz/animation/utils.h
  utils.cc)
set_target_properties(ozz_animation
  PROPERTIES FOLDER "ozz")

install(TARGETS ozz_animation DESTINATION lib)

add_subdirectory(offline)
