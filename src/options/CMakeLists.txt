add_library(ozz_options STATIC
  ../../include/ozz/options/options.h
  options.cc)
set_target_properties(ozz_options PROPERTIES FOLDER "ozz")

install(TARGETS ozz_options DESTINATION lib)

fuse_target("ozz_options")
