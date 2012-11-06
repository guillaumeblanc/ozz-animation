include_directories(${CMAKE_SOURCE_DIR}/contrib/glfw/include)

add_library(demo_framework
  application.h
  application.cc
  renderer.h
  imgui.h
  profile.h
  profile.cc
  utils.h
  utils.cc
  internal/camera.h
  internal/camera.cc
  internal/imgui_impl.h
  internal/imgui_impl.cc
  internal/renderer_impl.h
  internal/renderer_impl.cc)

target_link_libraries(demo_framework
  glfw)

set_target_properties(demo_framework
  PROPERTIES FOLDER "demo")
