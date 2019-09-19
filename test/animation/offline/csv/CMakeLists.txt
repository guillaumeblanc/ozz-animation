add_executable(ozz2csv
  ozz2csv.h
  ozz2csv.cc
  ozz2csv_csv.h
  ozz2csv_csv.cc
  ozz2csv_experiences.h
  ozz2csv_experiences.cc
  ozz2csv_generators.h
  ozz2csv_generators.cc
  ozz2csv_main.cc)
target_link_libraries(ozz2csv
  ozz_animation_offline
  ozz_options
  json)
set_target_properties(ozz2csv
  PROPERTIES FOLDER "ozz/tests/tools")

target_compile_features(ozz2csv PUBLIC cxx_std_11)

install(TARGETS ozz2csv DESTINATION lib)

# Atlas animation analysis
#-------------------------
file(MAKE_DIRECTORY ${ozz_temp_directory}/atlas_analysis)

add_custom_command(
    DEPENDS $<$<BOOL:${ozz_build_fbx}>:BUILD_DATA>
            "${ozz_media_directory}/bin/alain_atlas_raw.ozz"
            "${ozz_media_directory}/bin/alain_skeleton.ozz"
            $<$<BOOL:${ozz_build_data}>:ozz2csv>
    OUTPUT  "${ozz_temp_directory}/atlas_analysis/reference_transforms.csv"
            "${ozz_temp_directory}/atlas_analysis/decimate_transforms.csv"
            "${ozz_temp_directory}/atlas_analysis/quantize_transforms.csv"
    COMMAND ozz2csv "--experience=reference" "--generator=passthrough" "--skeleton=${ozz_media_directory}/bin/alain_skeleton.ozz" "--animation=${ozz_media_directory}/bin/alain_atlas_raw.ozz"
    COMMAND ozz2csv "--experience=decimate" "--generator=optimize" "--skeleton=${ozz_media_directory}/bin/alain_skeleton.ozz" "--animation=${ozz_media_directory}/bin/alain_atlas_raw.ozz"
    COMMAND ozz2csv "--experience=quantize" "--generator=runtime" "--skeleton=${ozz_media_directory}/bin/alain_skeleton.ozz" "--animation=${ozz_media_directory}/bin/alain_atlas_raw.ozz"
    WORKING_DIRECTORY "${ozz_temp_directory}/atlas_analysis"
    VERBATIM)

# Creates a target to build analysis data
add_custom_target(BUILD_ANALYSIS ALL
  DEPENDS "${ozz_temp_directory}/atlas_analysis/reference_transforms.csv"
          "${ozz_temp_directory}/atlas_analysis/decimate_transforms.csv"
          "${ozz_temp_directory}/atlas_analysis/quantize_transforms.csv"
  VERBATIM)

# Invalid cases
#--------------

add_test(NAME ozz2csv_bad_skeleton_path COMMAND ozz2csv "--skeleton=unexisting.ozz" "--animation=${ozz_media_directory}/bin/alain_atlas_raw.ozz")
set_tests_properties(ozz2csv_bad_skeleton_path PROPERTIES PASS_REGULAR_EXPRESSION "Failed to open file")

add_test(NAME ozz2csv_bad_animation COMMAND ozz2csv "--skeleton=${ozz_media_directory}/bin/alain_skeleton.ozz" "--animation=${ozz_media_directory}/bin/alain_walk.ozz")
set_tests_properties(ozz2csv_bad_animation PROPERTIES PASS_REGULAR_EXPRESSION "Failed to load instance from file")

add_test(NAME ozz2csv_bad_output_path COMMAND ozz2csv "--skeleton=${ozz_media_directory}/bin/alain_skeleton.ozz" "--animation=${ozz_media_directory}/bin/alain_atlas_raw.ozz" "--path=unexisting")
set_tests_properties(ozz2csv_bad_output_path PROPERTIES PASS_REGULAR_EXPRESSION "Failed opening csv file \"unexisting/")

# Valid cases
#------------

add_test(NAME ozz2csv_valid COMMAND ozz2csv "--skeleton=${ozz_media_directory}/bin/alain_skeleton.ozz" "--animation=${ozz_media_directory}/bin/alain_atlas_raw.ozz")

add_test(NAME ozz2csv_generator COMMAND ozz2csv "--generator=runtime" "--skeleton=${ozz_media_directory}/bin/alain_skeleton.ozz" "--animation=${ozz_media_directory}/bin/alain_atlas_raw.ozz")
set_tests_properties(ozz2csv_generator PROPERTIES PASS_REGULAR_EXPRESSION "Initializing \"runtime\" generator.")

# Experiences
#------------


