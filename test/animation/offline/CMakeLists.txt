add_executable(test_animation_builder
  animation_builder_tests.cc)
target_link_libraries(test_animation_builder
  ozz_animation_offline
  gtest)
set_target_properties(test_animation_builder PROPERTIES FOLDER "ozz/tests/animation_offline")
add_test(NAME test_animation_builder COMMAND test_animation_builder)

add_executable(test_animation_optimizer
  animation_optimizer_tests.cc)
target_link_libraries(test_animation_optimizer
  ozz_animation_offline
  gtest)
set_target_properties(test_animation_optimizer PROPERTIES FOLDER "ozz/tests/animation_offline")
add_test(NAME test_animation_optimizer COMMAND test_animation_optimizer)

add_executable(test_raw_animation_utils
  raw_animation_utils_tests.cc)
target_link_libraries(test_raw_animation_utils
  ozz_animation_offline
  gtest)
set_target_properties(test_raw_animation_utils PROPERTIES FOLDER "ozz/tests/animation_offline")
add_test(NAME test_raw_animation_utils COMMAND test_raw_animation_utils)

add_executable(test_additive_animation_builder
  additive_animation_builder_tests.cc)
target_link_libraries(test_additive_animation_builder
  ozz_animation_offline
  gtest)
set_target_properties(test_additive_animation_builder PROPERTIES FOLDER "ozz/tests/animation_offline")
add_test(NAME test_additive_animation_builder COMMAND test_additive_animation_builder)

add_executable(test_skeleton_builder
  skeleton_builder_tests.cc)
target_link_libraries(test_skeleton_builder
  ozz_animation_offline
  gtest)
set_target_properties(test_skeleton_builder PROPERTIES FOLDER "ozz/tests/animation_offline")
add_test(NAME test_skeleton_builder COMMAND test_skeleton_builder)

add_executable(test_raw_skeleton_archive
  raw_skeleton_archive_tests.cc)
target_link_libraries(test_raw_skeleton_archive
  ozz_animation_offline
  gtest)
set_target_properties(test_raw_skeleton_archive PROPERTIES FOLDER "ozz/tests/animation_offline")
add_test(NAME test_raw_skeleton_archive COMMAND test_raw_skeleton_archive)

add_executable(test_track_builder
  track_builder_tests.cc)
target_link_libraries(test_track_builder
  ozz_animation_offline
  ozz_animation
  ozz_base
  gtest)
set_target_properties(test_track_builder PROPERTIES FOLDER "ozz/tests/animation_offline")
add_test(NAME test_track_builder COMMAND test_track_builder)

add_executable(test_track_optimizer
  track_optimizer_tests.cc)
target_link_libraries(test_track_optimizer
  ozz_animation_offline
  ozz_animation
  ozz_base
  gtest)
set_target_properties(test_track_optimizer PROPERTIES FOLDER "ozz/tests/animation_offline")
add_test(NAME test_track_optimizer COMMAND test_track_optimizer)

add_executable(test_raw_skeleton_archive_versioning
  raw_skeleton_archive_versioning_tests.cc)

target_link_libraries(test_raw_skeleton_archive_versioning
  ozz_animation_offline
  ozz_options
  gtest)
set_target_properties(test_raw_skeleton_archive_versioning PROPERTIES FOLDER "ozz/tests/animation_offline")

add_test(NAME test_raw_skeleton_archive_versioning_le COMMAND test_raw_skeleton_archive_versioning "--file=${ozz_media_directory}/bin/versioning/raw_skeleton_v1_le.ozz" "--joints=67" "--root_name=Hips")
add_test(NAME test_raw_skeleton_archive_versioning_be COMMAND test_raw_skeleton_archive_versioning "--file=${ozz_media_directory}/bin/versioning/raw_skeleton_v1_be.ozz" "--joints=67" "--root_name=Hips")

add_executable(test_raw_animation_archive
  raw_animation_archive_tests.cc)
target_link_libraries(test_raw_animation_archive
  ozz_animation_offline
  gtest)
set_target_properties(test_raw_animation_archive PROPERTIES FOLDER "ozz/tests/animation_offline")
add_test(NAME test_raw_animation_archive COMMAND test_raw_animation_archive)

add_executable(test_raw_animation_archive_versioning
  raw_animation_archive_versioning_tests.cc)
target_link_libraries(test_raw_animation_archive_versioning
  ozz_animation_offline
  ozz_options
  gtest)
set_target_properties(test_raw_animation_archive_versioning PROPERTIES FOLDER "ozz/tests/animation_offline")

add_test(NAME test_raw_animation_archive_versioning_le COMMAND test_raw_animation_archive_versioning "--file=${ozz_media_directory}/bin/versioning/raw_animation_v3_le.ozz" "--tracks=67" "--duration=.66666667" "--name=run")
add_test(NAME test_raw_animation_archive_versioning_be COMMAND test_raw_animation_archive_versioning "--file=${ozz_media_directory}/bin/versioning/raw_animation_v3_be.ozz" "--tracks=67" "--duration=.66666667" "--name=run")

# Previous incompatbile versions.
add_test(NAME test_raw_animation_archive_versioning_le_older2 COMMAND test_raw_animation_archive_versioning "--file=${ozz_media_directory}/bin/versioning/raw_animation_v2_le.ozz" "--tracks=0" "--duration=1" "--name=")
add_test(NAME test_raw_animation_archive_versioning_le_older1 COMMAND test_raw_animation_archive_versioning "--file=${ozz_media_directory}/bin/versioning/raw_animation_v1_le.ozz" "--tracks=0" "--duration=1" "--name=")

# RawTrack
add_executable(test_raw_track_archive
  raw_track_archive_tests.cc)
target_link_libraries(test_raw_track_archive
  ozz_animation_offline
  gtest)
set_target_properties(test_raw_track_archive PROPERTIES FOLDER "ozz/tests/animation_offline")
add_test(NAME test_raw_track_archive COMMAND test_raw_track_archive)

# ozz_animation_offline fuse tests
set_source_files_properties(${PROJECT_BINARY_DIR}/src_fused/ozz_animation_offline.cc PROPERTIES GENERATED 1)
add_executable(test_fuse_animation_offline
  animation_builder_tests.cc
  ${PROJECT_BINARY_DIR}/src_fused/ozz_animation_offline.cc)
add_dependencies(test_fuse_animation_offline BUILD_FUSE_ozz_animation_offline)
target_link_libraries(test_fuse_animation_offline
  ozz_animation
  gtest)
add_test(NAME test_fuse_animation_offline COMMAND test_fuse_animation_offline)
set_target_properties(test_fuse_animation_offline PROPERTIES FOLDER "ozz/tests/animation_offline")

add_subdirectory(fbx)
add_subdirectory(gltf)
add_subdirectory(tools)
