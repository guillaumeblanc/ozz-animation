#Set compilers settings for all platforms/compilers.
#---------------------------------------------------

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
# Cross compiler compilation flags

# Simd math force ref
if(ozz_build_simd_ref)
  set_property(DIRECTORY APPEND PROPERTY COMPILE_DEFINITIONS OZZ_BUILD_SIMD_REF)
endif()

#--------------------------------------
# Modify default MSVC compilation flags
if(MSVC)

  #---------------------------
  # For the common build flags

  # Disables crt secure warnings
  set_property(DIRECTORY APPEND PROPERTY COMPILE_DEFINITIONS _CRT_SECURE_NO_WARNINGS)

  # Removes any exception mode
  string(REGEX REPLACE " /EH.*" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
  string(REGEX REPLACE " /EH.*" "" CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")
  set_property(DIRECTORY APPEND PROPERTY COMPILE_DEFINITIONS _HAS_EXCEPTIONS=0)
  
  # Adds support for multiple processes builds
  set_property(DIRECTORY APPEND PROPERTY COMPILE_OPTIONS "/MP")

  # Set warning as error
  set_property(DIRECTORY APPEND PROPERTY COMPILE_OPTIONS "/WX")

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

#--------------------------------------
# else consider the compiler as GCC compatible
# Modify default GCC compilation flags
else()

  #---------------------------
  # For the common build flags

  # Enable c++11
  if(ozz_build_cpp11)
    set_property(DIRECTORY APPEND PROPERTY COMPILE_OPTIONS "$<$<STREQUAL:$<TARGET_PROPERTY:LINKER_LANGUAGE>,CXX>:-std=c++11>")
  endif()

  # Enable extra level of warning
  #if(NOT CMAKE_CXX_FLAGS MATCHES "-Wextra")
  #  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wextra")
  #endif()

  # Set the warning level to Wall
  set_property(DIRECTORY APPEND PROPERTY COMPILE_OPTIONS "-Wall")

  # Automatically selects native architecture optimizations (sse...)
  #set_property(DIRECTORY APPEND PROPERTY COMPILE_OPTIONS "-march=native")

  #----------------------
  # Enables debug glibcxx if NDebug isn't defined, not supported by APPLE
  if(NOT APPLE)
    set_property(DIRECTORY APPEND PROPERTY COMPILE_DEFINITIONS $<$<CONFIG:Debug>:_GLIBCXX_DEBUG>)
  endif()

  #----------------------
  # Sets emscripten output
  if(EMSCRIPTEN)
    SET(CMAKE_EXECUTABLE_SUFFIX ".html")

    #if(NOT ozz_build_simd_ref)
    #  set_property(DIRECTORY APPEND PROPERTY COMPILE_OPTIONS "-msse2")
    #endif()
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
    #set_property(DIRECTORY APPEND PROPERTY COMPILE_OPTIONS "-fprofile-arcs -ftest-coverage")
  endif()

endif()

#---------------------
# Prints all the flags
message(STATUS "---------------------------------------------------------")
message(STATUS "Default build type is: ${CMAKE_BUILD_TYPE}")
message(STATUS "The following compilation flags will be used:")
foreach(flag ${cxx_all_flags})
  message(${flag} " ${${flag}}")
endforeach()

message(STATUS "---------------------------------------------------------")

get_directory_property(DirectoryCompileOptions DIRECTORY ${CMAKE_SOURCE_DIR} COMPILE_OPTIONS)
message(STATUS "Directory Compile Options:")
foreach(opt ${DirectoryCompileOptions})
  message(STATUS ${opt})
endforeach()

message(STATUS "---------------------------------------------------------")

get_directory_property(DirectoryCompileDefinitions DIRECTORY ${CMAKE_SOURCE_DIR} COMPILE_DEFINITIONS)
message(STATUS "Directory Compile Definitions:")
foreach(def ${DirectoryCompileDefinitions})
  message(STATUS ${def})
endforeach()

message(STATUS "---------------------------------------------------------")

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
