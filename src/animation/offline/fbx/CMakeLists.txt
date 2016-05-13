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

add_executable(fbx2skel
  fbx2skel.cc)
target_link_libraries(fbx2skel
  ozz_animation_offline_tools
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
  ozz_animation_offline_tools
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

foreach(pair ${skeletons})
  list(GET pair 0 src) 
  list(GET pair 1 dest)

  list(APPEND bin_skeletons ${ozz_media_directory}${dest})

  add_custom_command(
    DEPENDS "${ozz_media_directory}${src}"
            fbx2skel
    OUTPUT ${ozz_media_directory}${dest}
    COMMAND fbx2skel
            "--file=${ozz_media_directory}${src}"
            "--skeleton=${ozz_media_directory}${dest}")
endforeach()

add_custom_target(convert_skeletons ALL DEPENDS ${bin_skeletons})
set_target_properties(convert_skeletons PROPERTIES FOLDER "ozz/tools")

# Gerenate binary animation data
set(animations
  "/fbx/alain/atlas.fbx\;/bin/alain_skeleton.ozz\;/bin/alain_atlas.ozz"
  "/fbx/alain/crackhead.fbx\;/bin/alain_skeleton.ozz\;/bin/alain_crackhead.ozz"
  "/fbx/alain/crackhead.fbx\;/bin/alain_skeleton.ozz\;/bin/alain_crackhead.ozz"
  "/fbx/alain/crossarms.fbx\;/bin/alain_skeleton.ozz\;/bin/alain_crossarms.ozz"
  "/fbx/alain/jog.fbx\;/bin/alain_skeleton.ozz\;/bin/alain_jog.ozz"
  "/fbx/alain/run.fbx\;/bin/alain_skeleton.ozz\;/bin/alain_run.ozz"
  "/fbx/alain/crackhead.fbx\;/bin/alain_skeleton.ozz\;/bin/alain_crackhead.ozz"
  "/fbx/alain/walk.fbx\;/bin/alain_skeleton.ozz\;/bin/alain_walk.ozz"
  "/collada/astro_max.dae\;/bin/astro_max_skeleton.ozz\;/bin/astro_max_animation.ozz"
  "/collada/astro_maya.dae\;/bin/astro_maya_skeleton.ozz\;/bin/astro_maya_animation.ozz"
  "/collada/seymour.dae\;/bin/seymour_skeleton.ozz\;/bin/seymour_animation.ozz")

foreach(triple ${animations})
  list(GET triple 0 src) 
  list(GET triple 1 skel)
  list(GET triple 2 dest)
  
  list(APPEND bin_animations ${ozz_media_directory}${dest})
  
  add_custom_command(
    DEPENDS "${ozz_media_directory}${src}" "${ozz_media_directory}${skel}"
            fbx2anim
    OUTPUT ${ozz_media_directory}${dest}
    COMMAND fbx2anim
            "--file=${ozz_media_directory}${src}"
            "--skeleton=${ozz_media_directory}${skel}"
            "--animation=${ozz_media_directory}${dest}")
endforeach()

add_custom_target(convert_animations ALL DEPENDS ${bin_animations})
set_target_properties(convert_animations PROPERTIES FOLDER "ozz/tools")
