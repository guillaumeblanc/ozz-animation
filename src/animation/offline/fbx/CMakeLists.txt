if(NOT ozz_build_fbx)
  return()
endif()

add_library(ozz_animation_fbx STATIC
  ${PROJECT_SOURCE_DIR}/include/ozz/animation/offline/fbx/fbx.h
  fbx.cc
  fbx_animation.h
  fbx_animation.cc
  ${PROJECT_SOURCE_DIR}/include/ozz/animation/offline/fbx/fbx_base.h
  fbx_base.cc
  fbx_skeleton.h
  fbx_skeleton.cc)
target_include_directories(ozz_animation_fbx PUBLIC
  $<BUILD_INTERFACE:${FBX_INCLUDE_DIRS}>)
target_link_libraries(ozz_animation_fbx
  ozz_animation_offline
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
  ozz_animation_fbx)
set_target_properties(fbx2skel
  PROPERTIES FOLDER "ozz/tools")

install(TARGETS fbx2skel DESTINATION bin/tools)

add_executable(fbx2anim
  fbx2anim.cc)
target_link_libraries(fbx2anim
  ozz_animation_offline_anim_tools
  ozz_animation_fbx)
set_target_properties(fbx2anim
  PROPERTIES FOLDER "ozz/tools")

install(TARGETS fbx2anim DESTINATION bin/tools)

# Gerenate binary skeleton data
set(skeletons
  "/fbx/alain/skeleton.fbx\;/bin/alain_skeleton.ozz"
  "/fbx/alain/skeleton.fbx\;/bin/versioning/raw_skeleton_v1_le.ozz\;option:--raw\;option:--endian=little"
  "/fbx/alain/skeleton.fbx\;/bin/versioning/raw_skeleton_v1_be.ozz\;option:--raw\;option:--endian=big"
  "/fbx/alain/skeleton.fbx\;/bin/versioning/skeleton_v1_le.ozz\;option:--endian=little"
  "/fbx/alain/skeleton.fbx\;/bin/versioning/skeleton_v1_be.ozz\;option:--endian=big"
  "/collada/astro_max.dae\;/bin/astro_max_skeleton.ozz"
  "/collada/astro_maya.dae\;/bin/astro_maya_skeleton.ozz"
  "/collada/seymour.dae\;/bin/seymour_skeleton.ozz")

foreach(line ${skeletons})
  list(GET line 0 src)
  list(GET line 1 dest)

  list(APPEND bin_skeletons ${ozz_media_directory}${dest})

  # Find options
  set(options "")
  foreach(element ${line})
    if (${element} MATCHES "option:(.*)")
      string(SUBSTRING ${element} 7 -1 option)
      list(APPEND options ${option})
    endif()
  endforeach()

  add_custom_command(
    DEPENDS "${ozz_media_directory}${src}"
            fbx2skel
    OUTPUT ${ozz_media_directory}${dest}
    COMMAND fbx2skel
            ${options}
            "--file=${ozz_media_directory}${src}"
            "--skeleton=${ozz_media_directory}${dest}"
    VERBATIM)
endforeach()

# Gerenate binary animation data
set(animations
  # Library data
  "/fbx/alain/atlas.fbx\;/bin/alain_skeleton.ozz\;{\"animations\":[{\"output\":\"${ozz_media_directory}/bin/alain_atlas_raw.ozz\",\"raw\":true,\"optimize\":false}]}\;output:/bin/alain_atlas_raw.ozz"
  "/fbx/alain/locomotions.fbx\;/bin/alain_skeleton.ozz\;{\"animations\":[{\"output\":\"${ozz_media_directory}/bin/alain_*.ozz\"}]}\;output:/bin/alain_walk.ozz\;output:/bin/alain_jog.ozz\;output:/bin/alain_run.ozz"
  "/fbx/alain/crossarms.fbx\;/bin/alain_skeleton.ozz\;{\"animations\":[{\"output\":\"${ozz_media_directory}/bin/alain_crossarms.ozz\"}]}\;output:/bin/alain_crossarms.ozz"
  "/fbx/alain/crackhead.fbx\;/bin/alain_skeleton.ozz\;{\"animations\":[{\"output\":\"${ozz_media_directory}/bin/alain_crackhead.ozz\"}]}\;output:/bin/alain_crackhead.ozz"
  "/fbx/alain/crackhead.fbx\;/bin/alain_skeleton.ozz\;{\"animations\":[{\"output\":\"${ozz_media_directory}/bin/alain_crackhead_additive.ozz\",\"additive\":true}]}\;output:/bin/alain_crackhead_additive.ozz"

  # Versioning
  "/fbx/alain/run.fbx\;/bin/alain_skeleton.ozz\;{\"animations\":[{\"output\":\"${ozz_media_directory}/bin/versioning/raw_animation_v2_le.ozz\",\"raw\":true}]}\;output:/bin/versioning/raw_animation_v2_le.ozz\;option:--endian=little"
  "/fbx/alain/run.fbx\;/bin/alain_skeleton.ozz\;{\"animations\":[{\"output\":\"${ozz_media_directory}/bin/versioning/raw_animation_v2_be.ozz\",\"raw\":true}]}\;output:/bin/versioning/raw_animation_v2_be.ozz\;option:--endian=big"
  "/fbx/alain/run.fbx\;/bin/alain_skeleton.ozz\;{\"animations\":[{\"output\":\"${ozz_media_directory}/bin/versioning/animation_v4_le.ozz\"}]}\;output:/bin/versioning/animation_v4_le.ozz\;option:--endian=little"
  "/fbx/alain/run.fbx\;/bin/alain_skeleton.ozz\;{\"animations\":[{\"output\":\"${ozz_media_directory}/bin/versioning/animation_v4_be.ozz\"}]}\;output:/bin/versioning/animation_v4_be.ozz\;option:--endian=big"

  # Collada
  "/collada/astro_max.dae\;/bin/astro_max_skeleton.ozz\;{\"animations\":[{\"output\":\"${ozz_media_directory}/bin/astro_max_animation.ozz\"}]}\;output:/bin/astro_max_animation.ozz"
  "/collada/astro_maya.dae\;/bin/astro_maya_skeleton.ozz\;{\"animations\":[{\"output\":\"${ozz_media_directory}/bin/astro_maya_animation.ozz\"}]}\;output:/bin/astro_maya_animation.ozz"
  "/collada/seymour.dae\;/bin/seymour_skeleton.ozz\;{\"animations\":[{\"output\":\"${ozz_media_directory}/bin/seymour_animation.ozz\"}]}\;output:/bin/seymour_animation.ozz"
  )

# Generate a command for each line of the configuration
set(id 0)
foreach(line ${animations})
  math(EXPR id "${id}+1")

  # First 3 elements are mandatory
  list(GET line 0 src)
  list(GET line 1 skel)
  list(GET line 2 config)

  # Optional elements
  list(LENGTH line line_length)

  # Loops over all remaining arguments.
  set(outputs "")
  foreach(element ${line})
    if (${element} MATCHES "output:(.*)")
      string(SUBSTRING ${element} 7 -1 output)
      list(APPEND outputs ${ozz_media_directory}${output})
    endif()
  endforeach()

  # output is required
  if (NOT outputs)
    message("Missing output file(s) list.")
  endif()

  list(APPEND bin_animations ${outputs})

  # Dump config file
  set(config_filename "${ozz_temp_directory}/fbx_config${id}.json")
  file(WRITE "${config_filename}" "${config}")

  # Create build command
  add_custom_command(
    DEPENDS "${ozz_media_directory}${src}"
            "${ozz_media_directory}${skel}"
            fbx2anim
    OUTPUT  ${outputs}
    COMMAND fbx2anim
            "--file=${ozz_media_directory}${src}"
            "--skeleton=${ozz_media_directory}${skel}"
            "--config_file=${config_filename}"
    VERBATIM)
endforeach()

# Creates a target to build data
add_custom_target(BUILD_DATA ALL
  DEPENDS ${bin_animations}
          ${bin_skeletons}
  VERBATIM)
