@ECHO OFF

REM Copyright (C) 2011 Guillaume Blanc
REM 

:: Creates MSVC directory
ECHO.Creating Visual Studio build directory...
cd..
IF NOT EXIST build_out-of-source (
  MD build_out-of-source
)
CD build_out-of-source

:: Buils CMake cache
ECHO.Configuring CMake build...
REM -G "Visual Studio 10 Win64"
REM -G "Visual Studio 9 2008"
REM > setup_log.txt 2>&1
CALL cmake ./../
IF NOT ERRORLEVEL 0 (
  ECHO.CMake failed to execute.
  GOTO :failure
) ELSE (
  GOTO :success
)

:success
ECHO.
ECHO.Setup succeded.
GOTO :end

:failure
ECHO.
ECHO.Setup failed.
ECHO.

GOTO :end

:end
