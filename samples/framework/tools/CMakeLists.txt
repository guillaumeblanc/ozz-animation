# Adds fbx2mesh utility target.
if(ozz_build_fbx)
  include_directories(${FBX_INCLUDE_DIRS})

  add_executable(sample_fbx2mesh
    fbx2mesh.cc)
  target_link_libraries(sample_fbx2mesh
    sample_framework
    ozz_animation_fbx
    ozz_animation_offline
    ozz_animation
    ozz_options
    ozz_base)
  set_target_properties(sample_fbx2mesh
    PROPERTIES FOLDER "samples/tools")
  if(NOT MSVC)
    set_target_properties(sample_fbx2mesh
      PROPERTIES LINK_FLAGS "-Wno-deprecated-declarations")
  endif()
  
  install(TARGETS sample_fbx2mesh DESTINATION bin/samples/tools)

  add_custom_command(TARGET sample_fbx2mesh POST_BUILD
    DEPENDS
      "${ozz_media_directory}/fbx/alain/arnaud.fbx"
    COMMAND sample_fbx2mesh "--file=${ozz_media_directory}/fbx/alain/arnaud.fbx" "--skeleton=${ozz_media_directory}/bin/skeleton_v1_le.ozz" "--mesh=${ozz_media_directory}/bin/mesh.ozz")

  set(ozz_mesh_generation_dependency sample_fbx2mesh)

  add_test(NAME sample_fbx2mesh COMMAND sample_fbx2mesh "--file=${ozz_media_directory}/fbx/alain/skeleton.fbx" "--skeleton=${ozz_media_directory}/bin/skeleton_v1_le.ozz" "--mesh=${ozz_temp_directory}/mesh.ozz")

  add_test(NAME sample_fbx2mesh_invalid_file COMMAND sample_fbx2mesh "--file=${ozz_temp_directory}/dont_exist.fbx" "--skeleton=${ozz_media_directory}/bin/skeleton_v1_le.ozz" "--mesh=${ozz_temp_directory}/should_not_exist.ozz")
  set_tests_properties(sample_fbx2mesh_invalid_file PROPERTIES WILL_FAIL true)

  add_test(NAME sample_fbx2mesh_invalid_skeleton COMMAND sample_fbx2mesh "--file=${ozz_media_directory}/fbx/alain/skeleton.fbx" "--skeleton=${ozz_media_directory}/bin/animation.ozz" "--mesh=${ozz_temp_directory}/should_not_exist.ozz")
  set_tests_properties(sample_fbx2mesh_invalid_skeleton PROPERTIES WILL_FAIL true)

  # Ensures nothing was outputted. Unfortunately there's no way to ensure this test is ran last
  add_test(NAME sample_fbx2mesh_output COMMAND ${CMAKE_COMMAND} -E copy "${ozz_temp_directory}/should_not_exist.ozz" "${ozz_temp_directory}/should_not_exist_too.ozz")
  set_tests_properties(sample_fbx2mesh_output PROPERTIES WILL_FAIL true)
endif()
