# load_from_file

add_executable(load_from_file
  load_from_file.cc)
target_link_libraries(load_from_file
  ozz_animation)
set_target_properties(load_from_file
  PROPERTIES FOLDER "howtos")
add_test(NAME load_from_file COMMAND load_from_file "${ozz_media_directory}/bin/pab_skeleton.ozz")
add_test(NAME load_from_file_no_arg COMMAND load_from_file)
set_tests_properties(load_from_file_no_arg PROPERTIES WILL_FAIL true)
add_test(NAME load_from_file_bad_arg COMMAND load_from_file "${ozz_media_directory}/bin/doesn_t_exist.ozz")
set_tests_properties(load_from_file_bad_arg PROPERTIES WILL_FAIL true)

# custom_skeleton_importer

add_executable(custom_skeleton_importer
  custom_skeleton_importer.cc)
target_link_libraries(custom_skeleton_importer
  ozz_animation_offline)
set_target_properties(custom_skeleton_importer
  PROPERTIES FOLDER "howtos")
add_test(NAME custom_skeleton_importer COMMAND custom_skeleton_importer)

# custom_animation_importer

add_executable(custom_animation_importer
  custom_animation_importer.cc)
target_link_libraries(custom_animation_importer
  ozz_animation_offline)
set_target_properties(custom_animation_importer
  PROPERTIES FOLDER "howtos")
add_test(NAME custom_animation_importer COMMAND custom_animation_importer)
