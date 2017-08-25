add_library(json
  dist/jsoncpp.cpp
  dist/json/json.h
  dist/json/json-forwards.h)

set_target_properties(json
  PROPERTIES FOLDER "extern")

set_target_properties(json
  PROPERTIES COMPILE_OPTIONS "/wd4702")

target_include_directories(json PUBLIC
  $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/extern/jsoncpp/dist>)
