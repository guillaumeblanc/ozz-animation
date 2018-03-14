if(NOT ozz_build_fbx)
  return()
endif()

add_library(ozz_animation_fbx STATIC
  ${PROJECT_SOURCE_DIR}/include/ozz/animation/offline/fbx/fbx.h
  fbx.cc
  ${PROJECT_SOURCE_DIR}/include/ozz/animation/offline/fbx/fbx_animation.h
  fbx_animation.cc
  ${PROJECT_SOURCE_DIR}/include/ozz/animation/offline/fbx/fbx_skeleton.h
  fbx_skeleton.cc)
target_include_directories(ozz_animation_fbx PUBLIC
  $<BUILD_INTERFACE:${FBX_INCLUDE_DIRS}>)
target_link_libraries(ozz_animation_fbx
  ozz_animation_offline
  debug ${FBX_LIBRARIES_DEBUG}
  optimized ${FBX_LIBRARIES})
set_target_properties(ozz_animation_fbx
  PROPERTIES FOLDER "ozz/tools")

install(TARGETS ozz_animation_fbx DESTINATION lib)

fuse_target("ozz_animation_fbx")

add_executable(fbx2ozz
  fbx2ozz.h
  fbx2ozz.cc
  fbx2ozz_anim.cc
  fbx2ozz_skel.cc)
target_link_libraries(fbx2ozz
  ozz_animation_offline_tools
  ozz_animation_fbx)
set_target_properties(fbx2ozz
  PROPERTIES FOLDER "ozz/tools")

install(TARGETS fbx2ozz DESTINATION bin/tools)

# Gerenate binary animation data
set(animations

  # Library data
  "/fbx/alain/skeleton.fbx\;{\"skeleton\":{\"output\":\"${ozz_media_directory}/bin/alain_skeleton.ozz\"}}\;output:${ozz_media_directory}/bin/alain_skeleton.ozz"
  "/fbx/alain/atlas.fbx\;{\"skeleton\":{\"output\":\"${ozz_media_directory}/bin/alain_skeleton.ozz\"},\"animations\":[{\"skeleton\":\"${ozz_media_directory}/bin/alain_skeleton.ozz\",\"output\":\"${ozz_media_directory}/bin/alain_atlas_raw.ozz\",\"raw\":true,\"optimize\":false}]}\;output:${ozz_media_directory}/bin/alain_atlas_raw.ozz\;depend:${ozz_media_directory}/bin/alain_skeleton.ozz"
  "/fbx/alain/locomotions.fbx\;{\"animations\":[{\"skeleton\":\"${ozz_media_directory}/bin/alain_skeleton.ozz\",\"output\":\"${ozz_media_directory}/bin/alain_*.ozz\"}]}\;output:${ozz_media_directory}/bin/alain_walk.ozz\;output:${ozz_media_directory}/bin/alain_jog.ozz\;output:${ozz_media_directory}/bin/alain_run.ozz\;depend:${ozz_media_directory}/bin/alain_skeleton.ozz"
  "/fbx/alain/crossarms.fbx\;{\"animations\":[{\"skeleton\":\"${ozz_media_directory}/bin/alain_skeleton.ozz\",\"output\":\"${ozz_media_directory}/bin/alain_crossarms.ozz\"}]}\;output:${ozz_media_directory}/bin/alain_crossarms.ozz\;depend:${ozz_media_directory}/bin/alain_skeleton.ozz"
  "/fbx/alain/crackhead.fbx\;{\"animations\":[{\"skeleton\":\"${ozz_media_directory}/bin/alain_skeleton.ozz\",\"output\":\"${ozz_media_directory}/bin/alain_crackhead.ozz\"}]}\;output:${ozz_media_directory}/bin/alain_crackhead.ozz\;depend:${ozz_media_directory}/bin/alain_skeleton.ozz"
  "/fbx/alain/crackhead.fbx\;{\"animations\":[{\"skeleton\":\"${ozz_media_directory}/bin/alain_skeleton.ozz\",\"output\":\"${ozz_media_directory}/bin/alain_crackhead_additive.ozz\",\"additive\":true}]}\;output:${ozz_media_directory}/bin/alain_crackhead_additive.ozz\;depend:${ozz_media_directory}/bin/alain_skeleton.ozz"
  "/fbx/robot.fbx\;{\"skeleton\":{\"output\":\"${ozz_media_directory}/bin/robot_skeleton.ozz\"},\"animations\":[{\"skeleton\":\"${ozz_media_directory}/bin/robot_skeleton.ozz\",\"output\":\"${ozz_media_directory}/bin/robot_animation.ozz\",\"optimization_tolerances\":{\"hierarchical\":0.0001},\"tracks\":[{\"imports\":[{\"joint_name\":\"thumb2\",\"property_name\":\"grasp\",\"output\":\"${ozz_media_directory}/bin/robot_track_grasp.ozz\"}]}]}]}\;output:${ozz_media_directory}/bin/robot_skeleton.ozz\;output:${ozz_media_directory}/bin/robot_animation.ozz\;output:${ozz_media_directory}/bin/robot_track_grasp.ozz"

  # Versioning
  "/fbx/alain/skeleton.fbx\;{\"skeleton\":{\"output\":\"${ozz_media_directory}/bin/versioning/raw_skeleton_v1_le.ozz\",\"raw\":true}}\;output:${ozz_media_directory}/bin/versioning/raw_skeleton_v1_le.ozz\;option:--endian=little"
  "/fbx/alain/skeleton.fbx\;{\"skeleton\":{\"output\":\"${ozz_media_directory}/bin/versioning/raw_skeleton_v1_be.ozz\",\"raw\":true}}\;output:${ozz_media_directory}/bin/versioning/raw_skeleton_v1_be.ozz\;option:--endian=big"
  "/fbx/alain/skeleton.fbx\;{\"skeleton\":{\"output\":\"${ozz_media_directory}/bin/versioning/skeleton_v1_le.ozz\"}}\;output:${ozz_media_directory}/bin/versioning/skeleton_v1_le.ozz\;option:--endian=little"
  "/fbx/alain/skeleton.fbx\;{\"skeleton\":{\"output\":\"${ozz_media_directory}/bin/versioning/skeleton_v1_be.ozz\"}}\;output:${ozz_media_directory}/bin/versioning/skeleton_v1_be.ozz\;option:--endian=big"
  "/fbx/alain/run.fbx\;{\"animations\":[{\"skeleton\":\"${ozz_media_directory}/bin/alain_skeleton.ozz\",\"output\":\"${ozz_media_directory}/bin/versioning/raw_animation_v2_le.ozz\",\"raw\":true}]}\;output:${ozz_media_directory}/bin/versioning/raw_animation_v2_le.ozz\;option:--endian=little\;depend:${ozz_media_directory}/bin/alain_skeleton.ozz"
  "/fbx/alain/run.fbx\;{\"animations\":[{\"skeleton\":\"${ozz_media_directory}/bin/alain_skeleton.ozz\",\"output\":\"${ozz_media_directory}/bin/versioning/raw_animation_v2_be.ozz\",\"raw\":true}]}\;output:${ozz_media_directory}/bin/versioning/raw_animation_v2_be.ozz\;option:--endian=big\;depend:${ozz_media_directory}/bin/alain_skeleton.ozz"
  "/fbx/alain/run.fbx\;{\"animations\":[{\"skeleton\":\"${ozz_media_directory}/bin/alain_skeleton.ozz\",\"output\":\"${ozz_media_directory}/bin/versioning/animation_v4_le.ozz\"}]}\;output:${ozz_media_directory}/bin/versioning/animation_v4_le.ozz\;option:--endian=little\;depend:${ozz_media_directory}/bin/alain_skeleton.ozz"
  "/fbx/alain/run.fbx\;{\"animations\":[{\"skeleton\":\"${ozz_media_directory}/bin/alain_skeleton.ozz\",\"output\":\"${ozz_media_directory}/bin/versioning/animation_v4_be.ozz\"}]}\;output:${ozz_media_directory}/bin/versioning/animation_v4_be.ozz\;option:--endian=big\;depend:${ozz_media_directory}/bin/alain_skeleton.ozz"

  # Collada
  "/collada/astro_max.dae\;{\"skeleton\":{\"output\":\"${ozz_media_directory}/bin/astro_max_skeleton.ozz\"},\"animations\":[{\"skeleton\":\"${ozz_media_directory}/bin/astro_max_skeleton.ozz\",\"output\":\"${ozz_media_directory}/bin/astro_max_animation.ozz\"}]}\;output:${ozz_media_directory}/bin/astro_max_animation.ozz\;output:${ozz_media_directory}/bin/astro_max_skeleton.ozz"
  "/collada/astro_maya.dae\;{\"skeleton\":{\"output\":\"${ozz_media_directory}/bin/astro_maya_skeleton.ozz\"},\"animations\":[{\"skeleton\":\"${ozz_media_directory}/bin/astro_maya_skeleton.ozz\",\"output\":\"${ozz_media_directory}/bin/astro_maya_animation.ozz\"}]}\;output:${ozz_media_directory}/bin/astro_maya_animation.ozz\;output:${ozz_media_directory}/bin/astro_maya_skeleton.ozz"
  "/collada/seymour.dae\;{\"skeleton\":{\"output\":\"${ozz_media_directory}/bin/seymour_skeleton.ozz\"},\"animations\":[{\"skeleton\":\"${ozz_media_directory}/bin/seymour_skeleton.ozz\",\"output\":\"${ozz_media_directory}/bin/seymour_animation.ozz\"}]}\;output:${ozz_media_directory}/bin/seymour_animation.ozz\;output:${ozz_media_directory}seymour_skeleton.ozz"
  )

# Generate a command for each line of the configuration
foreach(line ${animations})

  # First 3 elements are mandatory
  list(GET line 0 src)
  list(GET line 1 config)

  # Optional elements
  list(LENGTH line line_length)

  # Loops over all remaining arguments to find outputs.
  set(outputs "")
  foreach(element ${line})
    if (${element} MATCHES "output:(.*)")
      string(SUBSTRING ${element} 7 -1 output)
      list(APPEND outputs ${output})
    endif()
  endforeach()
  list(APPEND all_outputs ${outputs})

  # output is required
  if (NOT outputs)
    message("Missing output file(s) list.")
  endif()

  # Loops over all remaining arguments to find options.
  set(options "")
  foreach(element ${line})
    if (${element} MATCHES "option:(.*)")
      string(SUBSTRING ${element} 7 -1 option)
      list(APPEND options ${option})
    endif()
  endforeach()

  # Loops over all remaining arguments to find dependencies.
  set(options "")
  foreach(element ${line})
    if (${element} MATCHES "depend:(.*)")
      string(SUBSTRING ${element} 7 -1 depend)
      list(APPEND depends ${depend})
    endif()
  endforeach()

  # Dump config file
  string(MD5 hash "${line}")
  set(config_filename "${ozz_temp_directory}/config_${hash}.json")
  file(WRITE "${config_filename}" "${config}")

  # Create build command
  add_custom_command(
    DEPENDS "${ozz_media_directory}${src}"
            ${depends}
            $<$<AND:$<BOOL:${ozz_build_data}>,$<BOOL:${ozz_build_fbx}>>:fbx2ozz>
    OUTPUT  ${outputs}
    COMMAND fbx2ozz
            "--file=${ozz_media_directory}${src}"
            "--config_file=${config_filename}"
            ${options}
    VERBATIM)
endforeach()

# outputs default configuration
# !!! It doesn't work this way because fbx2ozz expects to read file and skeleton
#add_custom_command(
#  OUTPUT  "${ozz_media_directory}/default_config.json"
#  COMMAND fbx2ozz
#          "--file=unused"
#          "--skeleton=unused"
#          "--dump_full_config=${ozz_media_directory}/default_config.json"
#  VERBATIM)

# Creates a target to build data
add_custom_target(BUILD_DATA ALL
  DEPENDS ${all_outputs}
#         "${ozz_media_directory}/default_config.json"
 VERBATIM)
