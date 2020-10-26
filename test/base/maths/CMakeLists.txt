add_executable(test_math
  math_ex_tests.cc
  box_tests.cc
  rect_tests.cc
  vec_float_tests.cc
  quaternion_tests.cc
  transform_tests.cc)
target_link_libraries(test_math
  ozz_base
  gtest)
target_copy_shared_libraries(test_math)
add_test(NAME test_math COMMAND test_math)
set_target_properties(test_math PROPERTIES FOLDER "ozz/tests/base")

add_executable(test_simd_math
  simd_int_math_tests.cc
  simd_float_math_tests.cc
  simd_float4x4_tests.cc
  simd_quaternion_math_tests.cc
  simd_math_transpose_tests.cc)
target_link_libraries(test_simd_math
  ozz_base
  gtest)
target_copy_shared_libraries(test_simd_math)
add_test(NAME test_simd_math COMMAND test_simd_math)
set_target_properties(test_simd_math PROPERTIES FOLDER "ozz/tests/base")

add_executable(test_soa_math
  soa_float_tests.cc
  soa_quaternion_tests.cc
  soa_transform_tests.cc
  soa_float4x4_tests.cc)
target_link_libraries(test_soa_math
  ozz_base
  gtest)
target_copy_shared_libraries(test_soa_math)
add_test(NAME test_soa_math COMMAND test_soa_math)
set_target_properties(test_soa_math PROPERTIES FOLDER "ozz/tests/base")

add_executable(test_archive_maths
  math_archive_tests.cc
  simd_math_archive_tests.cc
  soa_math_archive_tests.cc)
target_link_libraries(test_archive_maths
  ozz_base
  gtest)
target_copy_shared_libraries(test_archive_maths)
add_test(NAME test_archive_maths COMMAND test_archive_maths)
set_target_properties(test_archive_maths PROPERTIES FOLDER "ozz/tests/base")