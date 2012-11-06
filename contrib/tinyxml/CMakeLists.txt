add_library(tinyxml
  tinystr.cpp
  tinystr.h
  tinyxml.cpp
  tinyxml.h
  tinyxmlerror.cpp
  tinyxmlparser.cpp)

set_target_properties(tinyxml PROPERTIES FOLDER "contrib")

