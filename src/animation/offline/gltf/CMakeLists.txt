if(NOT ozz_build_gltf)
  return()
endif()

add_executable(gltf2ozz
  gltf2ozz.cc)

target_link_libraries(gltf2ozz PRIVATE
  ozz_animation_tools)
set_target_properties(gltf2ozz
  PROPERTIES FOLDER "ozz/tools")

install(TARGETS gltf2ozz DESTINATION bin/tools)
