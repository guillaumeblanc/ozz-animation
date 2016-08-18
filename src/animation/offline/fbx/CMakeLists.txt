if(NOT ozz_build_fbx)
  return()
endif()

include_directories(${FBX_INCLUDE_DIRS})

add_library(ozz_animation_fbx
  ${CMAKE_SOURCE_DIR}/include/ozz/animation/offline/fbx/fbx.h
  fbx.cc
  fbx_animation.h
  fbx_animation.cc
  ${CMAKE_SOURCE_DIR}/include/ozz/animation/offline/fbx/fbx_base.h
  fbx_base.cc
  fbx_skeleton.h
  fbx_skeleton.cc)
target_link_libraries(ozz_animation_fbx
  debug ${FBX_LIBRARIES_DEBUG}
  optimized ${FBX_LIBRARIES})
set_target_properties(ozz_animation_fbx
  PROPERTIES FOLDER "ozz")

install(TARGETS ozz_animation_fbx DESTINATION lib)

fuse_target("ozz_animation_fbx")

add_executable(fbx2skel
  fbx2skel.cc)
target_link_libraries(fbx2skel
  ozz_animation_offline_skel_tools
  ozz_animation_fbx
  ozz_animation_offline
  ozz_animation
  ozz_options
  ozz_base)
set_target_properties(fbx2skel
  PROPERTIES FOLDER "ozz/tools")

install(TARGETS fbx2skel DESTINATION bin/tools)
  
add_executable(fbx2anim
  fbx2anim.cc)
target_link_libraries(fbx2anim
  ozz_animation_offline_anim_tools
  ozz_animation_fbx
  ozz_animation_offline
  ozz_animation
  ozz_options
  ozz_base)
set_target_properties(fbx2anim
  PROPERTIES FOLDER "ozz/tools")
  
install(TARGETS fbx2anim DESTINATION bin/tools)

# Gerenate binary skeleton data
set(skeletons
  "/fbx/alain/skeleton.fbx\;/bin/alain_skeleton.ozz"
  "/collada/astro_max.dae\;/bin/astro_max_skeleton.ozz"
  "/collada/astro_maya.dae\;/bin/astro_maya_skeleton.ozz"
  "/collada/seymour.dae\;/bin/seymour_skeleton.ozz")

foreach(line ${skeletons})
  list(GET line 0 src) 
  list(GET line 1 dest)

  list(APPEND bin_skeletons ${ozz_media_directory}${dest})

  add_custom_command(
    DEPENDS "${ozz_media_directory}${src}"
            fbx2skel
    OUTPUT ${ozz_media_directory}${dest}
    COMMAND fbx2skel
            "--file=${ozz_media_directory}${src}"
            "--skeleton=${ozz_media_directory}${dest}")
endforeach()

# Gerenate binary animation data
set(animations
  "/fbx/alain/atlas.fbx\;/bin/alain_skeleton.ozz\;/bin/alain_atlas.ozz"
  "/fbx/alain/atlas.fbx\;/bin/alain_skeleton.ozz\;/bin/alain_atlas_raw.ozz\;--raw\;--nooptimize"
  "/fbx/alain/walk.fbx\;/bin/alain_skeleton.ozz\;/bin/alain_walk.ozz"
  "/fbx/alain/jog.fbx\;/bin/alain_skeleton.ozz\;/bin/alain_jog.ozz"
  "/fbx/alain/run.fbx\;/bin/alain_skeleton.ozz\;/bin/alain_run.ozz"
  "/fbx/alain/crossarms.fbx\;/bin/alain_skeleton.ozz\;/bin/alain_crossarms.ozz"
  "/fbx/alain/crackhead.fbx\;/bin/alain_skeleton.ozz\;/bin/alain_crackhead.ozz"
  "/fbx/alain/crackhead.fbx\;/bin/alain_skeleton.ozz\;/bin/alain_crackhead_additive.ozz\;--additive"
  "/collada/astro_max.dae\;/bin/astro_max_skeleton.ozz\;/bin/astro_max_animation.ozz"
  "/collada/astro_maya.dae\;/bin/astro_maya_skeleton.ozz\;/bin/astro_maya_animation.ozz"
  "/collada/seymour.dae\;/bin/seymour_skeleton.ozz\;/bin/seymour_animation.ozz")

foreach(line ${animations})

  list(GET line 0 src) 
  list(GET line 1 skel)
  list(GET line 2 dest)

  # Optional elements
  list(LENGTH line line_length)

  set(option0 "")
  set(option1 "")
  set(option2 "")
  if (${line_length} GREATER 3)
    list(GET line 3 option0)
  endif()
  if (${line_length} GREATER 4)
    list(GET line 4 option1)
  endif()
  if (${line_length} GREATER 5)
    list(GET line 5 option2)
  endif()
  
  list(APPEND bin_animations ${ozz_media_directory}${dest})
  
  add_custom_command(
    DEPENDS "${ozz_media_directory}${src}"
            "${ozz_media_directory}${skel}"
            fbx2anim
    OUTPUT ${ozz_media_directory}${dest}
    COMMAND fbx2anim
            ${option0}
            ${option1}
            ${option2}
            "--file=${ozz_media_directory}${src}"
            "--skeleton=${ozz_media_directory}${skel}"
            "--animation=${ozz_media_directory}${dest}")
endforeach()

# Creates a target to build data
add_custom_target(BUILD_DATA ALL
  DEPENDS ${bin_animations}
          ${bin_skeletons})
