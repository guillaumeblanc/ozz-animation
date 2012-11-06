#-----------------
# Includes modules
INCLUDE (CheckIncludeFiles)

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
set(ozz_redebug_all OFF CACHE BOOL "Enable all REDEBUGing features (Valid for DEBUG build only)")
if(ozz_redebug_all)
  message("OZZ_HAS_REDEBUG_ALL is enabled")
  set_property(DIRECTORY APPEND PROPERTY COMPILE_DEFINITIONS_DEBUG OZZ_HAS_REDEBUG_ALL=1)
else()
  message("OZZ_HAS_REDEBUG_ALL is disabled")
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

  # sse2
  set(ozz_build_sse2 ON CACHE BOOL "Enable SSE2 instructions set")
  if(ozz_build_sse2)
    message("OZZ_HAS_SSE2 is enabled")
    set_property(DIRECTORY APPEND PROPERTY COMPILE_DEFINITIONS OZZ_HAS_SSE2=1)
  else()
    message("OZZ_HAS_SSE2 is disabled")
  endif()

  # Removes any exception mode
  string(REGEX REPLACE "/EH.*[ ]?" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")

  # Disables STL exceptions also
  if(NOT ${CMAKE_CXX_FLAGS} MATCHES "/D _HAS_EXCEPTIONS=0")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /D _HAS_EXCEPTIONS=0")
  endif()
  
  # Adds support for SSE instructions
  string(REGEX REPLACE "/arch:SSE[0-9]?[ ]?" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
  if(${ozz_build_sse2} AND CMAKE_CL_64) # x64 implicitly supports SSE2.
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /arch:SSE2")
  endif()

  # Adds support for multiple processes builds
  if(NOT ${CMAKE_CXX_FLAGS} MATCHES "/MP")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP")
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

  # Set the warning level to Wall
  if(NOT CMAKE_CXX_FLAGS MATCHES "-Wall")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")
  endif()

  # Automatically selects native architecture optimizations (sse...)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=native")

  #----------------------
  # For all builds but the default one
  foreach(flag ${cxx_all_but_default_flags})
    # Enables debug glibcxx if NDebug isn't defined
    if(NOT ${flag} MATCHES "NDEBUG" AND NOT ${flag} MATCHES "-D_GLIBCXX_DEBUG")
      set(${flag} "${${flag}} -D_GLIBCXX_DEBUG")
    endif()
  endforeach()

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
