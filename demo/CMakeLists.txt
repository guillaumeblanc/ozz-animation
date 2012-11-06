# Demo requires OpenGL package.
find_package(OpenGL REQUIRED)

# Default time for testing demos.
set(DEMO_TESTING_TIME ".5")

# Adds glfw.
add_subdirectory(${CMAKE_SOURCE_DIR}/contrib/glfw glfw)

# Adds the demo framework to the include path.
include_directories(./)

# Adds demos and helper libraries.
add_subdirectory(framework)
add_subdirectory(millipede)
add_subdirectory(collada_animation)
add_subdirectory(load_animation)
