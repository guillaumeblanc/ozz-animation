include_directories(${CMAKE_SOURCE_DIR}/extern/glfw/include)

add_library(sample_framework
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
  internal/camera.h
  internal/camera.cc
  internal/imgui_impl.h
  internal/imgui_impl.cc
  internal/renderer_impl.h
  internal/renderer_impl.cc
  internal/shooter.h
  internal/shooter.cc)

target_link_libraries(sample_framework
  glfw)

set_target_properties(sample_framework
  PROPERTIES FOLDER "samples")
