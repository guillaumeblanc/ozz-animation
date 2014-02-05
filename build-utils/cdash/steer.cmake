# Checks CTEST_CMAKE_GENERATOR argument.
if (NOT CTEST_CMAKE_GENERATOR)
  message("Missing mandatory CTEST_CMAKE_GENERATOR argument.")
endif ()

# Checks CTEST_CONFIGURATION_TYPE argument.
if (NOT CTEST_CONFIGURATION_TYPE)
  set(CTEST_CONFIGURATION_TYPE "Release")
  message("Missing CTEST_CONFIGURATION_TYPE argument, selecting \"${CTEST_CONFIGURATION_TYPE}\" by default.")
endif ()

# Checks CTEST_GIT_BRANCH argument.
if (NOT CTEST_GIT_BRANCH)
  set(CTEST_GIT_BRANCH "master")
  message("Missing CTEST_GIT_BRANCH argument, selecting \"${CTEST_GIT_BRANCH}\" by default.")
endif ()

# Names the site if it was not specified as argument.
if (NOT CTEST_SITE)
  find_program(HOSTNAME_CMD NAMES hostname)
  if (NOT HOSTNAME_CMD-NOTFOUND)
    exec_program(${HOSTNAME_CMD} ARGS OUTPUT_VARIABLE HOSTNAME)
    set(CTEST_SITE "${HOSTNAME}")
  else ()
    set(CTEST_SITE "un-named-site")
  endif ()
  message("Missing CTEST_SITE argument, selecting \"${CTEST_SITE}\" by default.")
endif ()

# Names the build's name if it was not specified as argument.
if (NOT CTEST_BUILD_NAME)
  set(CTEST_BUILD_NAME "${CTEST_CMAKE_GENERATOR}-${CTEST_CONFIGURATION_TYPE}")
  message("Missing CTEST_BUILD_NAME argument, selecting \"${CTEST_BUILD_NAME}\" by default.")
endif ()

# Find temp folder
if (EXISTS "$ENV{TMPDIR}")
  set(TEMP_FOLDER "$ENV{TMPDIR}")
elseif (EXISTS "$ENV{TMP}")
  set(TEMP_FOLDER "$ENV{TMP}")
elseif (EXISTS "$ENV{TEMP}")
  set(TEMP_FOLDER "$ENV{TEMP}")
else ()
  set(TEMP_FOLDER "/tmp")
endif ()

# Source and binary directories.
set(CTEST_SOURCE_NAME "ozz-animation")
set(CTEST_BINARY_NAME ${CTEST_BUILD_NAME})
set(CTEST_DASHBOARD_ROOT "${TEMP_FOLDER}/ozz-dashboard-${CTEST_GIT_BRANCH}")
set(CTEST_SOURCE_DIRECTORY "${CTEST_DASHBOARD_ROOT}/${CTEST_SOURCE_NAME}")
set(CTEST_BINARY_DIRECTORY "${CTEST_DASHBOARD_ROOT}/${CTEST_BINARY_NAME}")

# Setup initial cache.
# There should be no space before variables names. Be also careful to escape
# any quotes inside of this string.
set(CTEST_INITIAL_CACHE "
${CTEST_INITIAL_CACHE}
${CTEST_INITIAL_CACHE_0}
${CTEST_INITIAL_CACHE_1}
${CTEST_INITIAL_CACHE_2}
${CTEST_INITIAL_CACHE_3}
// Enable CDash
ozz_enable_cdash:BOOL=ON")

# Wipes binary directory before starting.
if(EXISTS "${CTEST_BINARY_DIRECTORY}")
  ctest_empty_binary_directory(${CTEST_BINARY_DIRECTORY})
endif()

# Requires git.
find_program(GIT_COMMAND NAMES git)
if(GIT_COMMAND-NOTFOUND)
  message("Git program was not found")
endif()

set(CTEST_CMAKE_COMMAND "\"${CMAKE_EXECUTABLE_NAME}\"")
set(CTEST_COMMAND "\"${CTEST_EXECUTABLE_NAME}\" -D Nightly")

# Clones or update repository.
if(NOT EXISTS "${CTEST_SOURCE_DIRECTORY}")
  file(MAKE_DIRECTORY "${CTEST_SOURCE_DIRECTORY}")
  set(CTEST_CHECKOUT_COMMAND "${GIT_COMMAND} clone -b ${CTEST_GIT_BRANCH} https://code.google.com/p/ozz-animation/ ${CTEST_SOURCE_NAME}")
endif()
set(CTEST_UPDATE_COMMAND "${GIT_COMMAND}")

# Setup coverage command if enabled
if(OZZ_ENABLE_COVERAGE)
  find_program(GCOV_COMMAND NAMES gcov)
  if (NOT GCOV_COMMAND-NOTFOUND)
    set(CTEST_COVERAGE_COMMAND ${GCOV_COMMAND})

    # Enables ozz coverage in the inital cache
    set(CTEST_INITIAL_CACHE "
${CTEST_INITIAL_CACHE}
// Enable coverage tests
ozz_enable_coverage:BOOL=ON")

  # Contrib libraries and samples are not included in the coverage, as not covered by automatic dashboard tests.
  set(CTEST_CUSTOM_COVERAGE_EXCLUDE ${CTEST_CUSTOM_COVERAGE_EXCLUDE} "contrib/" "samples/")

  else()
    # Disables coverage as coverage tool was not found
    set(OZZ_ENABLE_COVERAGE OFF)
  endif()
endif()

# Create initial cache file
file(WRITE "${CTEST_BINARY_DIRECTORY}/CMakeCache.txt" "${CTEST_INITIAL_CACHE}")

# Starts.
ctest_start("Nightly")

# Updates source directory.
ctest_update()

# Configure.
ctest_configure()

# Build.
ctest_build()

# runs tests.
ctest_test()

# Runs coverage tests.
if (OZZ_ENABLE_COVERAGE)
  ctest_coverage()
endif()

# Runs memory checks.
#ctest_memcheck()

# Submit to dashboard.
ctest_submit()
