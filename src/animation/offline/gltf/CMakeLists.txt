project(gltf2ozz VERSION 1.0 DESCRIPTION gltf2ozz LANGUAGES CXX)

add_executable(gltf2ozz)

target_include_directories(gltf2ozz PUBLIC ../include/ozz/)

target_sources(gltf2ozz PRIVATE gltf2ozz.cc)

target_link_libraries(gltf2ozz PRIVATE ozz_options ozz_animation_offline ozz_animation_tools)

install(TARGETS gltf2ozz DESTINATION bin/tools)