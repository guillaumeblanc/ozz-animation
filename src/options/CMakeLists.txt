add_library(ozz_options STATIC
  ../../include/ozz/options/options.h
  options.cc)
target_include_directories(ozz_options
  PUBLIC "${PROJECT_SOURCE_DIR}/include")
set_target_properties(ozz_options PROPERTIES FOLDER "ozz")

install(TARGETS ozz_options DESTINATION lib)

fuse_target("ozz_options")
