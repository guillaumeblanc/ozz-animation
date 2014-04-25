# - Tries to find FBX SDK package.
# Once done, it will define
#  FBX_FOUND - System has Fbx SDK installed
#  FBX_INCLUDE_DIRS - The Fbx SDK include directories
#  FBX_LIBRARIES - The libraries needed to use Fbx SDK
#  FBX_LIBRARIES_DEBUG - The libraries needed to use debug Fbx SDK
#
# This module will try to located FBX SDK folder, based on the standard
# directory structure proposed by Autodesk.
# On every platform, the module will look for libraries that matches the
# currently selected cmake generator.
#
# Known issues:
# - On ALL platforms: If there are multiple FBX SDK version installed, the
# current implementation will select the first one it finds.
# - On MAC: If there are multiple FBX SDK compiler supported (clang or gcc), the
# current implementation will select the first one it finds.

#*******************************************************************************
#                                                                              #
#  ozz-animation, 3d skeletal animation libraries and tools.                   #
#  https://code.google.com/p/ozz-animation/                                    #
#                                                                              #
#------------------------------------------------------------------------------#
#                                                                              #
#  Copyright (c) 2012-2014 Guillaume Blanc                                     #
#                                                                              #
#  This software is provided 'as-is', without any express or implied           #
#  warranty. In no event will the authors be held liable for any damages       #
#  arising from the use of this software.                                      #
#                                                                              #
#  Permission is granted to anyone to use this software for any purpose,       #
#  including commercial applications, and to alter it and redistribute it      #
#  freely, subject to the following restrictions:                              #
#                                                                              #
#  1. The origin of this software must not be misrepresented; you must not     #
#  claim that you wrote the original software. If you use this software        #
#  in a product, an acknowledgment in the product documentation would be       #
#  appreciated but is not required.                                            #
#                                                                              #
#  2. Altered source versions must be plainly marked as such, and must not be  #
#  misrepresented as being the original software.                              #
#                                                                              #
#  3. This notice may not be removed or altered from any source                #
#  distribution.                                                               #
#                                                                              #
#******************************************************************************#

###############################################################################
# Generic library search function definition
###############################################################################
function(FindFbxLibrariesGeneric _FBX_ROOT_DIR _OUT_FBX_LIBRARIES _OUT_FBX_LIBRARIES_DEBUG)
  # Directory structure depends on the platform:
  # - Windows: \lib\<compiler_version>\<processor_type>\<build_mode>
  # - Mac OSX: \lib\<compiler_version>\ub\<processor_type>\<build_mode>
  # - Linux: \lib\<compiler_version>\<build_mode>

  # Figures out matching compiler/os directory.
  if(MSVC10)
    set(FBX_CP_PATH "vs2010")
  elseif(MSVC11)
    set(FBX_CP_PATH "vs2012")
  elseif(MSVC12)
    set(FBX_CP_PATH "vs2013")
  elseif(APPLE)
    set(FBX_CP_PATH "*/ub")
  else()
    set(FBX_CP_PATH "*")
  endif()

  # Detects current processor type.
  if(NOT APPLE) # No <processor_type> on APPLE platform
    if(CMAKE_SIZEOF_VOID_P EQUAL 8)
      set(FBX_PROCESSOR_PATH "x64")
    else()
      set(FBX_PROCESSOR_PATH "x86")
    endif()
  endif()

  # Set libraries names to search, sorted by preference.
  set(FBX_SEARCH_LIB_NAMES libfbxsdk-mt.lib fbxsdk-static.a libfbxsdk.a fbxsdk.a)
  set(FBX_SEARCH_LIB_PATH "${_FBX_ROOT_DIR}/lib/${FBX_CP_PATH}/${FBX_PROCESSOR_PATH}")

  find_library(FBX_LIB
    ${FBX_SEARCH_LIB_NAMES}
    HINTS "${FBX_SEARCH_LIB_PATH}/release/")

  if(FBX_LIB)

    # Searches debug version also
    find_library(FBX_LIB_DEBUG
      ${FBX_SEARCH_LIB_NAMES}
      HINTS "${FBX_SEARCH_LIB_PATH}/debug/")

    if(FBX_LIB_DEBUG)
    else()
      set(LIBS_DEBUG ${FBX_LIB} PARENT_SCOPE)
    endif()

    if(UNIX)
      if(APPLE) # APPLE requires to link with Carbon framework
        find_library(CARBON_FRAMEWORK Carbon)
        list(APPEND FBX_LIB ${CARBON_FRAMEWORK})
        list(APPEND FBX_LIB_DEBUG ${CARBON_FRAMEWORK})
      else()
        find_package(Threads)
        list(APPEND FBX_LIB ${CMAKE_THREAD_LIBS_INIT} dl)
        list(APPEND FBX_LIB_DEBUG ${CMAKE_THREAD_LIBS_INIT} dl)
      endif()
    endif()

    set(${_OUT_FBX_LIBRARIES} ${FBX_LIB} PARENT_SCOPE)
    set(${_OUT_FBX_LIBRARIES_DEBUG} ${FBX_LIB_DEBUG} PARENT_SCOPE)
  else()
    message ("A Fbx SDK was found, but doesn't match you compiler settings.")
  endif()

endfunction()

###############################################################################
# Main find package function
###############################################################################

# Tries to find FBX SDK path
set(FBX_SEARCH_PATHS
  "$ENV{FBX_DIR}"
  "$ENV{ProgramW6432}/Autodesk/FBX/FBX SDK/*/"
  "$ENV{PROGRAMFILES}/Autodesk/FBX/FBX SDK/*/"
  "/Applications/Autodesk/FBX SDK/*/")

find_path(FBX_INCLUDE_DIR "fbxsdk.h"
  PATHS ${FBX_SEARCH_PATHS}
  PATH_SUFFIXES "include")

if(FBX_INCLUDE_DIR)
  # Deduce SDK root directory.
  set(FBX_ROOT_DIR "${FBX_INCLUDE_DIR}/..")

  # Fills CMake sytandard variables
  set(FBX_INCLUDE_DIRS "${FBX_INCLUDE_DIR}")

  # Searches libraries according to the current compiler.
  FindFbxLibrariesGeneric(${FBX_ROOT_DIR} FBX_LIBRARIES FBX_LIBRARIES_DEBUG)
endif()

# Handles the QUIETLY and REQUIRED arguments and set FBX_FOUND to TRUE if all listed variables are TRUE
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Fbx
  DEFAULT_MSG
  FBX_LIBRARIES
  FBX_INCLUDE_DIRS)
  