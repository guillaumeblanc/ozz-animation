@ECHO OFF

REM Copyright (C) 2011 Guillaume Blanc
REM

:: Clears out-of-source directory if exists
cd..
IF EXIST build_out-of-source (
  ECHO.Clearing out-of-source build dicrectory
  RD /S /Q build_out-of-source
)

ECHO.Distclean completed


