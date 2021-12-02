add_library(json
  dist/jsoncpp.cpp
  dist/json/json.h
  dist/json/json-forwards.h)

set_target_properties(json
  PROPERTIES FOLDER "extern")

target_compile_definitions(json
  PUBLIC $<$<BOOL:${BUILD_SHARED_LIBS}>:JSON_DLL>
  PRIVATE $<$<BOOL:${BUILD_SHARED_LIBS}>:JSON_DLL_BUILD>)

set_target_properties(json
  PROPERTIES COMPILE_OPTIONS $<$<CXX_COMPILER_ID:MSVC>:/wd4702>)
set_target_properties(json
  PROPERTIES COMPILE_OPTIONS $<$<CXX_COMPILER_ID:MSVC>:/wd4275>)

target_compile_options(json
  PUBLIC $<$<CXX_COMPILER_ID:MSVC>:/wd4702>
  PUBLIC $<$<CXX_COMPILER_ID:MSVC>:/wd4275>)

target_include_directories(json PUBLIC
  $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/extern/jsoncpp/dist>)
