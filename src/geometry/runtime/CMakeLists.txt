set(LIB_SOURCES
  ${PROJECT_SOURCE_DIR}/include/ozz/geometry/runtime/skinning_job.h
  skinning_job.cc)

if(ozz_build_dll)
  add_library(ozz_geometry SHARED ${LIB_SOURCES})
  target_compile_definitions(ozz_geometry PUBLIC OZZ_USE_DYNAMIC_LINKING)
  target_compile_definitions(ozz_geometry PRIVATE OZZ_BUILD_GEOMETRY_LIB)
  target_compile_options(ozz_geometry PUBLIC /wd4251)
else()
  add_library(ozz_geometry STATIC ${LIB_SOURCES})
endif()

target_link_libraries(ozz_geometry
  ozz_base)
set_target_properties(ozz_geometry
  PROPERTIES FOLDER "ozz")

install(TARGETS ozz_geometry DESTINATION lib)

fuse_target("ozz_geometry")
