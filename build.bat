@echo off
SetLocal EnableDelayedExpansion

REM --- Configuration ---
set outputName=ymk

set compiler=g++

set srcDir=src
set binDir=bin
set defines=-DYDEBUG

REM Compiler Flags
set compilerFlags=-g -Wall

REM Include Flags
set includeFlags=-I./include

REM Libs
set libs=-Lbin

echo ------------------------------
echo C++ build script for %outputName%
echo ------------------------------

REM --- Preparation ---
if not exist "%binDir%" mkdir "%binDir%"

REM --- Gather Source Files ---
set sourceFiles=
echo Finding source files in %srcDir%...

for /r "%srcDir%" %%f in (*.cpp) do (
    set "sourceFiles=!sourceFiles! "%%f""
    echo   - %%~nxf
)

if "!sourceFiles!"=="" (
    echo Error: No .cpp files found in %srcDir%
    goto Fail
)

echo.
echo Compiling and Linking...

REM --- Debug: Print the exact command ---
REM This helps you see if paths look wrong
echo [CMD] %compiler% !sourceFiles! %compilerFlags% %includeFlags% %libs% %defines% -o "%binDir%\%outputName%.exe"

REM --- Compile Command ---
REM Note: Changed output path to use backslash \
%compiler% !sourceFiles! %compilerFlags% %includeFlags% %libs% %defines% -o "%binDir%\%outputName%.exe"

REM --- Check Result ---
if %ERRORLEVEL% EQU 0 (
    echo.
    echo ------------------------------
    echo [SUCCESS] Build completed.
    echo Output: %binDir%\%outputName%.exe
    echo ------------------------------
) else (
    goto Fail
)

exit /b 0

:Fail
echo.
echo ------------------------------
echo [FAILED] Build aborted.
echo ------------------------------
exit /b 1