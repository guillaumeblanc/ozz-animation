if(NOT ozz_build_gltf)
  return()
endif()

add_executable(gltf2ozz
  gltf2ozz.cc)

target_compile_features(gltf2ozz PUBLIC cxx_std_11)
target_link_libraries(gltf2ozz PRIVATE
  ozz_options
  ozz_animation_offline
  ozz_animation_tools)
set_target_properties(gltf2ozz
  PROPERTIES FOLDER "ozz/tools")

install(TARGETS gltf2ozz DESTINATION bin/tools)
