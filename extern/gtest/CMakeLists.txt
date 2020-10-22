add_library(gtest STATIC
  fused-src/gtest/gtest_main.cc
  fused-src/gtest/gtest-all.cc
  fused-src/gtest/gtest.h)
set_target_properties(gtest
  PROPERTIES FOLDER "extern")

# gtest library requires linking with thread's library
find_package(Threads)
target_link_libraries(gtest
  ${CMAKE_THREAD_LIBS_INIT})

target_include_directories(gtest PUBLIC
  $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/extern/gtest/fused-src>)
