# Adds fbx2mesh utility target.
if(ozz_build_fbx)

  # share meshes with thte sample framework
  add_executable(sample_fbx2mesh
    fbx2mesh.cc
    ${PROJECT_SOURCE_DIR}/samples/framework/mesh.cc
    ${PROJECT_SOURCE_DIR}/samples/framework/mesh.h)
  target_link_libraries(sample_fbx2mesh
    ozz_animation_fbx
    ozz_options)
  set_target_properties(sample_fbx2mesh
    PROPERTIES FOLDER "samples/tools")

  install(TARGETS sample_fbx2mesh DESTINATION bin/samples/tools)

  add_custom_command(
    DEPENDS $<$<BOOL:${ozz_build_fbx}>:BUILD_DATA>
            $<$<AND:$<BOOL:${ozz_build_data}>,$<BOOL:${ozz_build_fbx}>>:sample_fbx2mesh>
            "${ozz_media_directory}/bin/pab_skeleton.ozz"
            "${ozz_media_directory}/fbx/pab/arnaud.fbx"
            "${ozz_media_directory}/fbx/sketchfab/ruby.fbx"
            "${ozz_media_directory}/collada/floor.dae"
    OUTPUT "${ozz_media_directory}/bin/arnaud_mesh.ozz"
           "${ozz_media_directory}/bin/arnaud_mesh_4.ozz"
    OUTPUT "${ozz_media_directory}/bin/ruby_mesh.ozz"
           "${ozz_media_directory}/bin/floor.ozz"
    COMMAND sample_fbx2mesh "--file=${ozz_media_directory}/fbx/pab/arnaud.fbx" "--skeleton=${ozz_media_directory}/bin/pab_skeleton.ozz" "--mesh=${ozz_media_directory}/bin/arnaud_mesh.ozz"
    COMMAND sample_fbx2mesh "--file=${ozz_media_directory}/fbx/pab/arnaud.fbx" "--skeleton=${ozz_media_directory}/bin/pab_skeleton.ozz" "--mesh=${ozz_media_directory}/bin/arnaud_mesh_4.ozz" --nosplit --max_influences=4
    COMMAND sample_fbx2mesh "--file=${ozz_media_directory}/fbx/sketchfab/ruby.fbx" "--skeleton=${ozz_media_directory}/bin/ruby_skeleton.ozz" "--mesh=${ozz_media_directory}/bin/ruby_mesh.ozz" --max_influences=4
    COMMAND sample_fbx2mesh "--file=${ozz_media_directory}/collada/floor.dae" "--mesh=${ozz_media_directory}/bin/floor.ozz" "--skeleton=${ozz_media_directory}/bin/pab_skeleton.ozz" 
    VERBATIM)

  # Creates a target to build sample data
  add_custom_target(BUILD_DATA_SAMPLE ALL DEPENDS
    "${ozz_media_directory}/bin/arnaud_mesh.ozz"
    "${ozz_media_directory}/bin/arnaud_mesh_4.ozz"
    "${ozz_media_directory}/bin/ruby_mesh.ozz"
    "${ozz_media_directory}/bin/floor.ozz"
    VERBATIM)

  add_test(NAME sample_fbx2mesh COMMAND sample_fbx2mesh "--file=${ozz_media_directory}/fbx/pab/skeleton.fbx" "--skeleton=${ozz_media_directory}/bin/pab_skeleton.ozz" "--mesh=${ozz_temp_directory}/mesh.ozz")

  add_test(NAME sample_fbx2mesh_invalid_file COMMAND sample_fbx2mesh "--file=${ozz_temp_directory}/dont_exist.fbx" "--skeleton=${ozz_media_directory}/bin/pab_skeleton.ozz" "--mesh=${ozz_temp_directory}/should_not_exist.ozz")
  set_tests_properties(sample_fbx2mesh_invalid_file PROPERTIES WILL_FAIL true)

  add_test(NAME sample_fbx2mesh_invalid_skeleton COMMAND sample_fbx2mesh "--file=${ozz_media_directory}/fbx/pab/skeleton.fbx" "--skeleton=${ozz_media_directory}/bin/pab_walk.ozz" "--mesh=${ozz_temp_directory}/should_not_exist.ozz")
  set_tests_properties(sample_fbx2mesh_invalid_skeleton PROPERTIES WILL_FAIL true)

  # Ensures nothing was outputted.
  add_test(NAME sample_fbx2mesh_output COMMAND ${CMAKE_COMMAND} -E copy "${ozz_temp_directory}/should_not_exist.ozz" "${ozz_temp_directory}/should_not_exist_too.ozz")
  set_tests_properties(sample_fbx2mesh_output PROPERTIES WILL_FAIL true)
  set_tests_properties(sample_fbx2mesh_output PROPERTIES
    DEPENDS "sample_fbx2mesh_invalid_file
             sample_fbx2mesh_invalid_skeleton")

endif()
