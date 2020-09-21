add_library(ozz_options
  ../../include/ozz/options/options.h
  options.cc)

target_compile_definitions(ozz_options
  PUBLIC $<$<BOOL:${BUILD_SHARED_LIBS}>:OZZ_USE_DYNAMIC_LINKING>
  PRIVATE $<$<BOOL:${BUILD_SHARED_LIBS}>:OZZ_BUILD_OPTIONS_LIB>)

target_compile_options(ozz_options PUBLIC $<$<CXX_COMPILER_ID:MSVC>:/wd4251>)
target_include_directories(ozz_options PUBLIC
  $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:$<INSTALL_PREFIX>/include>)
set_target_properties(ozz_options PROPERTIES FOLDER "ozz")

install(TARGETS ozz_options DESTINATION lib)

fuse_target("ozz_options")
