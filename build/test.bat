@ECHO OFF

REM Copyright (C) 2011 Guillaume Blanc
REM 

call build

:: Runs test target
ECHO.Building
cmake --build ./ --target run_tests --config Release
