add_library(ozz_options
  ../../include/ozz/options/options.h
  options.cc)
set_target_properties(ozz_options PROPERTIES FOLDER "ozz")

install(TARGETS ozz_options DESTINATION lib)
