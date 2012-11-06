@ECHO OFF

REM Copyright (C) 2011 Guillaume Blanc
REM 

call setup

:: Buils CMake cache
ECHO.Building
cmake --build ./ --config Release
