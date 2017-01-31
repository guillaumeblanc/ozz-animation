add_library(ozz_base STATIC
  ../../include/ozz/base/endianness.h
  ../../include/ozz/base/gtest_helper.h
  ../../include/ozz/base/memory/allocator.h
  memory/allocator.cc
  ../../include/ozz/base/platform.h
  ../../include/ozz/base/log.h
  log.cc
  ../../include/ozz/base/containers/intrusive_list.h
  ../../include/ozz/base/containers/deque.h
  ../../include/ozz/base/containers/list.h
  ../../include/ozz/base/containers/map.h
  ../../include/ozz/base/containers/queue.h
  ../../include/ozz/base/containers/set.h
  ../../include/ozz/base/containers/stack.h
  ../../include/ozz/base/containers/string.h
  ../../include/ozz/base/containers/string_archive.h
  containers/string_archive.cc
  ../../include/ozz/base/containers/vector.h
  ../../include/ozz/base/containers/vector_archive.h
  ../../include/ozz/base/containers/std_allocator.h
  ../../include/ozz/base/io/archive.h
  io/archive.cc
    ../../include/ozz/base/io/archive_traits.h
  ../../include/ozz/base/io/stream.h
  io/stream.cc
  ../../include/ozz/base/maths/box.h
  maths/box.cc
  ../../include/ozz/base/maths/gtest_math_helper.h
  ../../include/ozz/base/maths/internal/simd_math_config.h
  ../../include/ozz/base/maths/internal/simd_math_ref-inl.h
  ../../include/ozz/base/maths/internal/simd_math_sse-inl.h
  ../../include/ozz/base/maths/math_ex.h
  ../../include/ozz/base/maths/math_constant.h
  ../../include/ozz/base/maths/quaternion.h
  ../../include/ozz/base/maths/rect.h
  ../../include/ozz/base/maths/simd_math.h
  maths/simd_math.cc
  ../../include/ozz/base/maths/soa_float.h
  ../../include/ozz/base/maths/soa_quaternion.h
  ../../include/ozz/base/maths/soa_transform.h
  ../../include/ozz/base/maths/soa_float4x4.h
  ../../include/ozz/base/maths/transform.h
  ../../include/ozz/base/maths/vec_float.h
  ../../include/ozz/base/maths/math_archive.h
  maths/math_archive.cc
  ../../include/ozz/base/maths/soa_math_archive.h
  maths/soa_math_archive.cc
  ../../include/ozz/base/maths/simd_math_archive.h
  maths/simd_math_archive.cc)
set_target_properties(ozz_base PROPERTIES FOLDER "ozz")

install(TARGETS ozz_base DESTINATION lib)

fuse_target("ozz_base")
