set(LIB_SOURCES
  ../../include/ozz/options/options.h
  options.cc)

if(ozz_build_dll)
  add_library(ozz_options SHARED ${LIB_SOURCES})
  target_compile_definitions(ozz_options PUBLIC OZZ_USE_DYNAMIC_LINKING)
  target_compile_definitions(ozz_options PRIVATE OZZ_BUILD_OPTIONS_LIB)
  target_compile_options(ozz_options PUBLIC /wd4251)
else()
  add_library(ozz_options STATIC ${LIB_SOURCES})
endif()

target_include_directories(ozz_options PUBLIC
  $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:$<INSTALL_PREFIX>/include>)
set_target_properties(ozz_options PROPERTIES FOLDER "ozz")

install(TARGETS ozz_options DESTINATION lib)

fuse_target("ozz_options")
