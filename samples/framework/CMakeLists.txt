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
  mesh.h
  mesh.cc
  internal/camera.h
  internal/camera.cc
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
  find_package(OpenGL REQUIRED)

  include_directories(${OPENGL_INCLUDE_DIR})
  
  add_subdirectory(${CMAKE_SOURCE_DIR}/extern/glfw glfw)

  target_link_libraries(sample_framework
    ${OPENGL_LIBRARIES}
    glfw)
endif()

target_link_libraries(sample_framework
  ozz_geometry
  ozz_animation
  ozz_options
  ozz_base)

if(TARGET BUILD_SAMPLE_DATA)
  add_dependencies(sample_framework BUILD_SAMPLE_DATA)
endif()

set_target_properties(sample_framework
  PROPERTIES FOLDER "samples")
  
add_subdirectory(tools)
