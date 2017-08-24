add_library(json STATIC
  src/json.hpp)

set_target_properties(json PROPERTIES LINKER_LANGUAGE CXX)

target_include_directories(json PUBLIC
  $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/extern/json/src>)

set_target_properties(json
  PROPERTIES FOLDER "extern")

