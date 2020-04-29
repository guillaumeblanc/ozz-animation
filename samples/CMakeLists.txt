# Adds the sample framework to the include path.
include_directories(./)

# Adds framework.
add_subdirectory(framework)

# Adds samples and helper libraries.
add_subdirectory(additive)
add_subdirectory(attach)
add_subdirectory(baked)
add_subdirectory(blend)
add_subdirectory(foot_ik)
add_subdirectory(look_at)
add_subdirectory(millipede)
add_subdirectory(multithread)
add_subdirectory(optimize)
add_subdirectory(partial_blend)
add_subdirectory(playback)
add_subdirectory(skinning)
add_subdirectory(two_bone_ik)
add_subdirectory(user_channel)
