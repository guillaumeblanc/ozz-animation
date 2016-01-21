#-----------------
# Includes modules
include(CheckIncludeFiles)

#----------------------------------
# Set default build type to "Release"
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "Release")
endif()

#------------------------------
# Enables IDE folders y default
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

#------------------------
# Available build options

# Redebug all
if(ozz_build_redebug_all)
  message("OZZ_HAS_REDEBUG_ALL is enabled")
  set_property(DIRECTORY APPEND PROPERTY COMPILE_DEFINITIONS_DEBUG OZZ_HAS_REDEBUG_ALL=1)
endif()

#------------------------
# Lists all the cxx flags
set(cxx_all_flags
  CMAKE_CXX_FLAGS
  CMAKE_C_FLAGS
  CMAKE_CXX_FLAGS_DEBUG
  CMAKE_C_FLAGS_DEBUG
  CMAKE_CXX_FLAGS_MINSIZEREL
  CMAKE_C_FLAGS_MINSIZEREL
  CMAKE_CXX_FLAGS_RELWITHDEBINFO
  CMAKE_C_FLAGS_RELWITHDEBINFO
  CMAKE_CXX_FLAGS_RELEASE
  CMAKE_C_FLAGS_RELEASE)

set(cxx_all_but_default_flags
  CMAKE_CXX_FLAGS_DEBUG
  CMAKE_C_FLAGS_DEBUG
  CMAKE_CXX_FLAGS_MINSIZEREL
  CMAKE_C_FLAGS_MINSIZEREL
  CMAKE_CXX_FLAGS_RELWITHDEBINFO
  CMAKE_C_FLAGS_RELWITHDEBINFO
  CMAKE_CXX_FLAGS_RELEASE
  CMAKE_C_FLAGS_RELEASE)

# Lists debug cxx flags only
set(cxx_debug_flags
  CMAKE_CXX_FLAGS_DEBUG
  CMAKE_C_FLAGS_DEBUG)

# Lists release cxx flags only
set(cxx_release_flags
  CMAKE_CXX_FLAGS_MINSIZEREL
  CMAKE_C_FLAGS_MINSIZEREL
  CMAKE_CXX_FLAGS_RELWITHDEBINFO
  CMAKE_C_FLAGS_RELWITHDEBINFO
  CMAKE_CXX_FLAGS_RELEASE
  CMAKE_C_FLAGS_RELEASE)

#--------------------------------------
# Modify default MSVC compilation flags
if(MSVC)

  #---------------------------
  # For the common build flags

  # Disables crt secure warnings
  set_property(DIRECTORY APPEND PROPERTY COMPILE_DEFINITIONS _CRT_SECURE_NO_WARNINGS)

  # SSE detection
  if(ozz_build_sse2 OR CMAKE_CL_64) # x64 implicitly supports SSE2.
    message("OZZ_HAS_SSE2 is enabled")
    set_property(DIRECTORY APPEND PROPERTY COMPILE_DEFINITIONS OZZ_HAS_SSE2=1)
  endif()

  # Adds support for SSE instructions
  string(REGEX REPLACE " /arch:SSE[0-9]?" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
  string(REGEX REPLACE " /arch:SSE[0-9]?" "" CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")
  if(ozz_build_sse2 AND NOT CMAKE_CL_64) # x64 implicitly supports SSE2.
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /arch:SSE2")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /arch:SSE2")
  endif()

  # Removes any exception mode
  string(REGEX REPLACE " /EH.*" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
  string(REGEX REPLACE " /EH.*" "" CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")

  # Disables STL exceptions also
  if(NOT ${CMAKE_CXX_FLAGS} MATCHES "/D _HAS_EXCEPTIONS=0")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /D _HAS_EXCEPTIONS=0")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /D _HAS_EXCEPTIONS=0")
  endif()
  

  # Adds support for multiple processes builds
  if(NOT ${CMAKE_CXX_FLAGS} MATCHES "/MP")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /MP")
  endif()

  #---------------
  # For all builds
  foreach(flag ${cxx_all_flags})
    # Set the warning level to W4
    string(REGEX REPLACE "/W3" "/W4" ${flag} "${${flag}}")
    
    # Prefers static lining with runtime libraries, to ease installation
    string(REGEX REPLACE "/MD" "/MT" ${flag} "${${flag}}")

	# Prefers /Ox (full optimization) to /O2 (maximize speed)
    string(REGEX REPLACE "/O2" "/Ox" ${flag} "${${flag}}")
  endforeach()
  
  #----------------------
  # For debug builds only
  foreach(flag ${cxx_debug_flags})
    # Enables smaller type checks
    if(NOT ${flag} MATCHES "/RTCc")
      set(${flag} "${${flag}} /RTCc")
    endif()
  endforeach()

#--------------------------------------
# else consider the compiler as GCC compatible
# Modify default GCC compilation flags
else()

  #---------------------------
  # For the common build flags

  # Enable c++11
  if(ozz_build_cpp11)
    if(NOT CMAKE_CXX_FLAGS MATCHES "-std=c\\+\\+11")
      set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
    endif()
  endif()

  # Set the warning level to Wall
  if(NOT CMAKE_CXX_FLAGS MATCHES "-Wall")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall")
  endif()

  # Automatically selects native architecture optimizations (sse...)
  #if(NOT CMAKE_CXX_FLAGS MATCHES "-march")
  #  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=native")
  #  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -march=native")
  #endif()

  #----------------------
  # Enables debug glibcxx if NDebug isn't defined, not supported by APPLE
  if(NOT APPLE)
    foreach(flag ${cxx_all_but_default_flags})
      if(NOT ${flag} MATCHES "NDEBUG" AND NOT ${flag} MATCHES "-D_GLIBCXX_DEBUG")
        set(${flag} "${${flag}} -D_GLIBCXX_DEBUG")
      endif()
    endforeach()
  endif()

  #----------------------
  # Sets emscripten output
  if(EMSCRIPTEN)
    SET(CMAKE_EXECUTABLE_SUFFIX ".html")

    #----------------------
    # Disable emscripten absolute-paths warning
    if(NOT CMAKE_CXX_FLAGS MATCHES "-Wno-warn-absolute-paths")
      set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-warn-absolute-paths")
      set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-warn-absolute-paths")
    endif()

    #----------------------
    # Sets emscripten output
    if(NOT CMAKE_CXX_FLAGS MATCHES "-s DISABLE_GL_EMULATION=1")
      set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -s DISABLE_GL_EMULATION=1")
      set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -s DISABLE_GL_EMULATION=1")
    endif()
  endif()

  #----------------------
  # Handles coverage options
  if(ozz_build_coverage)
    # Extern libraries and samples are not included in the coverage, as not covered by automatic dashboard tests.
    set(CTEST_CUSTOM_COVERAGE_EXCLUDE ${CTEST_CUSTOM_COVERAGE_EXCLUDE} "extern/" "samples/")

    # Linker options
    if(NOT ${CMAKE_SHARED_LINKER_FLAGS_DEBUG} MATCHES "-fprofile-arcs -ftest-coverage")
      set(CMAKE_SHARED_LINKER_FLAGS_DEBUG "${CMAKE_SHARED_LINKER_FLAGS_DEBUG} -fprofile-arcs -ftest-coverage")
    endif()
    if(NOT ${CMAKE_EXE_LINKER_FLAGS_DEBUG} MATCHES "-fprofile-arcs -ftest-coverage")
      set(CMAKE_EXE_LINKER_FLAGS_DEBUG "${CMAKE_EXE_LINKER_FLAGS_DEBUG} -fprofile-arcs -ftest-coverage")
    endif()

    # Compiler options
    foreach(flag ${cxx_debug_flags})
      if(NOT ${flag} MATCHES "-fprofile-arcs -ftest-coverage")
        set(${flag} "${${flag}} -fprofile-arcs -ftest-coverage")
      endif()
    endforeach()
  endif()

endif()

#---------------------
# Prints all the flags
message("---------------------------------------------------------")
message("Default build type is: ${CMAKE_BUILD_TYPE}")
message("The following compilation flags will be used:")
foreach(flag ${cxx_all_flags})
  message(${flag} " ${${flag}}")
endforeach()
message("---------------------------------------------------------")

#-----------------------------------------------
# Updates cmake cache, will be visible in the UI
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}" CACHE STRING "Flags used by the compiler for all buids" FORCE)
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG}" CACHE STRING "Flags used by the compiler for debug buids" FORCE)
set(CMAKE_CXX_FLAGS_MINSIZEREL "${CMAKE_CXX_FLAGS_MINSIZEREL}" CACHE STRING "Flags used by the compiler for MinSizeRel buids" FORCE)
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO}" CACHE STRING "Flags used by the compiler for RelWithDebugInfo buids" FORCE)
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE}" CACHE STRING "Flags used by the compiler for release buids" FORCE)

#----------------------------------------------
# Modifies output directory for all executables
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ".")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ".")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ".")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL ".")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO ".")

#-------------------------------
# Set a postfix for output files
set(CMAKE_DEBUG_POSTFIX "_d")
set(CMAKE_RELEASE_POSTFIX "_r")
set(CMAKE_MINSIZEREL_POSTFIX "_rs")
set(CMAKE_RELWITHDEBINFO_POSTFIX "_rd")
