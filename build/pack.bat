@ECHO OFF

REM Copyright (C) 2011 Guillaume Blanc
REM 

ECHO.Preparing setup and building binaries
call build

ECHO.Packing sources
cpack -G "ZIP" --config CPackSourceConfig.cmake
cpack -G "TBZ2" --config CPackSourceConfig.cmake

ECHO.Packing binaries
cpack -G "ZIP" -C Release
cpack -G "TBZ2" -C Release