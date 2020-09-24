add_library(ozz_options
  ${PROJECT_SOURCE_DIR}/include/ozz/options/export.h
  ${PROJECT_SOURCE_DIR}/include/ozz/options/options.h
  options.cc)
target_link_libraries(ozz_options ozz_base)

target_compile_definitions(ozz_options PRIVATE $<$<BOOL:${BUILD_SHARED_LIBS}>:OZZ_BUILD_OPTIONS_LIB>)
  
set_target_properties(ozz_options PROPERTIES FOLDER "ozz")

install(TARGETS ozz_options DESTINATION lib)

fuse_target("ozz_options")
