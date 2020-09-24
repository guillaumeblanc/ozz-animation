add_library(ozz_geometry
  ${PROJECT_SOURCE_DIR}/include/ozz/geometry/runtime/export.h
  ${PROJECT_SOURCE_DIR}/include/ozz/geometry/runtime/skinning_job.h
skinning_job.cc)
target_compile_definitions(ozz_geometry PRIVATE $<$<BOOL:${BUILD_SHARED_LIBS}>:OZZ_BUILD_GEOMETRY_LIB>)

target_link_libraries(ozz_geometry ozz_base)
set_target_properties(ozz_geometry PROPERTIES FOLDER "ozz")

install(TARGETS ozz_geometry DESTINATION lib)

fuse_target("ozz_geometry")
