add_library(ozz_geometry
  ${ozz_root_dir}/include/ozz/geometry/runtime/skinning_job.h
  skinning_job.cc)
set_target_properties(ozz_geometry
  PROPERTIES FOLDER "ozz")

install(TARGETS ozz_geometry DESTINATION lib)

fuse_target("ozz_geometry")
