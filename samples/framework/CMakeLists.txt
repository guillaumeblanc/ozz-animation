# Framework library
add_library(sample_framework STATIC
  application.h
  application.cc
  imgui.h
  image.h
  image.cc
  profile.h
  profile.cc
  renderer.h
  utils.h
  utils.cc
  mesh.h
  mesh.cc
  internal/camera.h
  internal/camera.cc
  internal/icosphere.h
  internal/immediate.h
  internal/immediate.cc
  internal/imgui_impl.h
  internal/imgui_impl.cc
  internal/renderer_impl.h
  internal/renderer_impl.cc
  internal/shader.h
  internal/shader.cc
  internal/shooter.h
  internal/shooter.cc)

# Samples requires OpenGL package.
if(NOT EMSCRIPTEN)
  add_subdirectory(${PROJECT_SOURCE_DIR}/extern/glfw glfw)

  target_link_libraries(sample_framework
    glfw)
endif()

target_link_libraries(sample_framework
  ozz_geometry
  ozz_animation_offline
  ozz_options)

if(TARGET BUILD_DATA_SAMPLE)
  add_dependencies(sample_framework BUILD_DATA_SAMPLE)
endif()

set_target_properties(sample_framework
  PROPERTIES FOLDER "samples")

add_subdirectory(tools)
