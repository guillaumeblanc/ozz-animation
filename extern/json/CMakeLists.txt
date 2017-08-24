add_library(json INTERFACE)

target_include_directories(json INTERFACE
  $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/extern/json/src>)
