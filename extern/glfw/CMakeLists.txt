set(OpenGL_GL_PREFERENCE LEGACY)
find_package(OpenGL REQUIRED)

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
  if(MSYS OR MINGW)
    # Disables warnings in glfw.
    set_source_files_properties(${specific_file_list} PROPERTIES COMPILE_FLAGS "-Wno-unknown-pragmas")
  endif()
endif()

# Samples requires X11 package on linux.
if(UNIX AND NOT APPLE)
  find_package(X11)
  if(X11_FOUND)
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
    # Needs _GLFW_HAS_GLXGETPROCADDRESSARB preprocessor definition
    set_source_files_properties(${specific_file_list} PROPERTIES COMPILE_DEFINITIONS _GLFW_HAS_GLXGETPROCADDRESSARB=)
  endif()
endif()

# Mac OSX
if(UNIX AND APPLE)
  find_library(COCOA_FRAMEWORK Cocoa)
  find_library(IOKIT_FRAMEWORK IOKit)

  include_directories(lib/cocoa)
  set(specific_objc_file_list
    lib/cocoa/cocoa_enable.m
    lib/cocoa/cocoa_fullscreen.m
    lib/cocoa/cocoa_glext.m
    lib/cocoa/cocoa_init.m
    lib/cocoa/cocoa_joystick.m
    lib/cocoa/cocoa_time.m
    lib/cocoa/cocoa_window.m)

  # Disables warnings in glfw.
  set_source_files_properties(${specific_objc_file_list} PROPERTIES COMPILE_FLAGS
    "-Wno-deprecated-declarations -Wno-incompatible-pointer-types -Wno-implicit-function-declaration -Wno-int-conversion")

  # Appends other c files
  set(specific_file_list
    ${specific_objc_file_list}
    lib/cocoa/cocoa_thread.c
    lib/cocoa/platform.h)
endif()

add_library(glfw STATIC
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

target_include_directories(glfw PUBLIC
  $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/extern/glfw/include>)
target_include_directories(glfw PUBLIC
  $<BUILD_INTERFACE:${OPENGL_INCLUDE_DIR}>)
target_include_directories(glfw PRIVATE
  $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/extern/glfw/lib>)
# Disables GL deprecation warnings on macos
target_compile_definitions(glfw PUBLIC $<$<BOOL:APPLE>:GL_SILENCE_DEPRECATION>)

target_link_libraries(glfw
  ${OPENGL_LIBRARIES}
  ${X11_LIBRARIES}
  ${COCOA_FRAMEWORK}
  ${IOKIT_FRAMEWORK})

set_target_properties(glfw
  PROPERTIES FOLDER "extern")
