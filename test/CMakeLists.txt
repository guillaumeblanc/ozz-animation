# Sets GTest specific flags
set_property(DIRECTORY APPEND PROPERTY COMPILE_DEFINITIONS GTEST_HAS_EXCEPTIONS=0)

# Set the gtest include directory
include_directories(../contrib/gtest/fused-src/)

# Adds GTest library
add_subdirectory(../contrib/gtest ./gtest)

# Adds test file directories
add_subdirectory(base)
add_subdirectory(animation)
add_subdirectory(options)
