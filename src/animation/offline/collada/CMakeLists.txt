add_subdirectory(${CMAKE_SOURCE_DIR}/extern/tinyxml tinyxml)
include_directories(${CMAKE_SOURCE_DIR}/extern/tinyxml)

add_library(ozz_animation_collada
  ${CMAKE_SOURCE_DIR}/include/ozz/animation/offline/collada/collada.h
  collada.cc
  collada_animation.h
  collada_animation.cc
  collada_base.h
  collada_base.cc
  collada_skeleton.h
  collada_skeleton.cc
  collada_transform.h
  collada_transform.cc)
target_link_libraries(ozz_animation_collada
  tinyxml)
set_target_properties(ozz_animation_collada
  PROPERTIES FOLDER "ozz")

install(TARGETS ozz_animation_collada DESTINATION lib)

if(CMAKE_CROSSCOMPILING)
  find_package(dae2skel)
else()
  add_executable(dae2skel
    dae2skel.cc)
  target_link_libraries(dae2skel
    ozz_animation_offline_tools
    ozz_animation_collada
    ozz_animation_offline
    ozz_animation
    ozz_options
    ozz_base)
  set_target_properties(dae2skel
    PROPERTIES FOLDER "ozz/tools")

  install(TARGETS dae2skel DESTINATION bin/tools)

  export(TARGETS dae2skel FILE "${CMAKE_BINARY_DIR}/dae2skelConfig.cmake")
endif()

if(CMAKE_CROSSCOMPILING)
  find_package(dae2anim)
else()
  add_executable(dae2anim
    dae2anim.cc)
  target_link_libraries(dae2anim
    ozz_animation_offline_tools
    ozz_animation_collada
    ozz_animation_offline
    ozz_animation
    ozz_options
    ozz_base)
  set_target_properties(dae2anim
    PROPERTIES FOLDER "ozz/tools")

  install(TARGETS dae2anim DESTINATION bin/tools)
  
  export(TARGETS dae2anim FILE "${CMAKE_BINARY_DIR}/dae2animConfig.cmake")

  # Uses the tools to archive a skeleton and an animation with the current object format.
  add_custom_command(TARGET dae2anim POST_BUILD
    DEPENDS
      "${ozz_media_directory}/collada/alain/skeleton.dae"
      "${ozz_media_directory}/collada/alain/walk.dae"
    COMMAND ${CMAKE_COMMAND} -E make_directory "${ozz_media_directory}/bin"
    COMMAND dae2skel "--file=${ozz_media_directory}/collada/alain/skeleton.dae" "--skeleton=${ozz_media_directory}/bin/skeleton_v1_le.ozz" "--endian=little"
    COMMAND dae2skel "--file=${ozz_media_directory}/collada/alain/skeleton.dae" "--skeleton=${ozz_media_directory}/bin/skeleton_v1_be.ozz" "--endian=big"
    COMMAND dae2skel "--raw" "--file=${ozz_media_directory}/collada/alain/skeleton.dae" "--skeleton=${ozz_media_directory}/bin/raw_skeleton_v1_le.ozz" "--endian=little"
    COMMAND dae2skel "--raw" "--file=${ozz_media_directory}/collada/alain/skeleton.dae" "--skeleton=${ozz_media_directory}/bin/raw_skeleton_v1_be.ozz" "--endian=big"
    COMMAND dae2anim "--file=${ozz_media_directory}/collada/alain/run.dae" "--skeleton=${ozz_media_directory}/bin/skeleton_v1_le.ozz" "--animation=${ozz_media_directory}/bin/animation_v2_le.ozz" "--endian=little"
    COMMAND dae2anim "--file=${ozz_media_directory}/collada/alain/run.dae" "--skeleton=${ozz_media_directory}/bin/skeleton_v1_le.ozz" "--animation=${ozz_media_directory}/bin/animation_v2_be.ozz" "--endian=big"
    COMMAND dae2anim "--raw" "--file=${ozz_media_directory}/collada/alain/run.dae" "--skeleton=${ozz_media_directory}/bin/skeleton_v1_le.ozz" "--animation=${ozz_media_directory}/bin/raw_animation_v1_le.ozz" "--endian=little"
    COMMAND dae2anim "--raw" "--file=${ozz_media_directory}/collada/alain/run.dae" "--skeleton=${ozz_media_directory}/bin/skeleton_v1_le.ozz" "--animation=${ozz_media_directory}/bin/raw_animation_v1_be.ozz" "--endian=big")
endif()
