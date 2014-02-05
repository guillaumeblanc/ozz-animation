include_directories(lib)

# Buils Win32 specific files and options
if(WIN32)
include_directories(lib/win32)
set(specific_file_list 
  lib/win32/platform.h
  lib/win32/win32_dllmain.c
  lib/win32/win32_enable.c
  lib/win32/win32_fullscreen.c
  lib/win32/win32_glext.c
  lib/win32/win32_init.c
  lib/win32/win32_joystick.c
  lib/win32/win32_thread.c
  lib/win32/win32_time.c
  lib/win32/win32_window.c)
endif()

# Samples requires X11 package.
find_package(X11)
if(X11_FOUND)
set_property(DIRECTORY APPEND PROPERTY COMPILE_DEFINITIONS _GLFW_HAS_GLXGETPROCADDRESSARB=)
include_directories(lib/x11)
include_directories(${X11_INCLUDE_DIR})
set(specific_file_list 
  lib/x11/platform.h
  lib/x11/x11_enable.c
  lib/x11/x11_fullscreen.c
  lib/x11/x11_glext.c
  lib/x11/x11_init.c
  lib/x11/x11_joystick.c
  lib/x11/x11_keysym2unicode.c
  lib/x11/x11_thread.c
  lib/x11/x11_time.c
  lib/x11/x11_window.c)
endif()

add_library(glfw
  include/GL/glfw.h
  include/GL/glext.h
  lib/enable.c
  lib/fullscreen.c
  lib/glext.c
  lib/image.c
  lib/init.c
  lib/input.c
  lib/internal.h
  lib/joystick.c
  lib/stream.c
  lib/tga.c
  lib/thread.c
  lib/time.c
  lib/window.c
  ${specific_file_list})

target_link_libraries(glfw
  ${OPENGL_glu_LIBRARY}
  ${OPENGL_gl_LIBRARY}
  ${X11_LIBRARIES})

set_target_properties(glfw
  PROPERTIES FOLDER "contrib")
