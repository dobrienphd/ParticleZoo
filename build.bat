@echo off
setlocal enabledelayedexpansion

REM Parse command-line arguments
set PREFIX=%LOCALAPPDATA%\particlezoo
set BUILD_TYPE=release
set ROOT_FLAGS=
set ROOT_LIBS=

:parse_args
if "%~1"=="" goto :end_parse_args
if /I "%~1"=="--prefix" (
    set "PREFIX=%~2"
    shift
    shift
    goto :parse_args
)
if /I "%~1:~0,9%"=="--prefix=" (
    set "PREFIX=!%~1:~9!"
    shift
    goto :parse_args
)
if /I "%~1"=="debug" (
    set BUILD_TYPE=debug
    shift
    goto :parse_args
)
if /I "%~1"=="release" (
    set BUILD_TYPE=release
    shift
    goto :parse_args
)
if /I "%~1"=="install" (
    set DO_INSTALL=1
    shift
    goto :parse_args
)
echo Unknown option: %~1
shift
goto :parse_args
:end_parse_args

echo Configuration:
echo   Build type: %BUILD_TYPE%
echo   Install prefix: %PREFIX%


REM Write configuration to config.status
echo Writing configuration to config.status...
(
echo PREFIX=%PREFIX%
) > config.status

echo Configuration complete. USE_ROOT=%USE_ROOT%

REM Set output directories
set GCC_BIN_DIR_REL=build\msvc\release
set GCC_BIN_DIR_DBG=build\msvc\debug
set OUTDIR=build\msvc\%BUILD_TYPE%

REM Create output directory if it doesn't exist
if not exist "%OUTDIR%" mkdir "%OUTDIR%"

REM locate VS installation via vswhere
set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if not exist "%VSWHERE%" (
  echo ERROR: vswhere.exe not found at "%VSWHERE%"
  exit /b 1
)
for /f "usebackq tokens=*" %%I in (`"%VSWHERE%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do (
  set "VS_PATH=%%I"
)

if not defined VS_PATH (
  echo ERROR: Could not detect Visual Studio install.
  echo Please ensure Visual Studio is installed with C++ development tools.
  exit /b 1
)
echo Visual Studio installation found at: %VS_PATH%
call "%VS_PATH%\Common7\Tools\VsDevCmd.bat"

REM Common include paths
set INCLUDES=/I include

REM Add ROOT flags if enabled
REM ROOT support removed

REM Common source files
set COMMON_SRCS=src\PhaseSpaceFileReader.cc ^
src\PhaseSpaceFileWriter.cc ^
src\utilities\formats.cc ^
src\utilities\argParse.cc ^
src\egs\egsphspFile.cc ^
src\peneasy\penEasyphspFile.cc ^
src\IAEA\IAEAHeader.cc ^
src\IAEA\IAEAphspFile.cc ^
src\topas\TOPASHeader.cc ^
src\topas\TOPASphspFile.cc

REM Static Library sources (same as common sources)
set LIB_SRCS=%COMMON_SRCS%
set LIB_NAME=libparticlezoo.lib

if /I "%BUILD_TYPE%"=="debug" (
    echo Debug build.
    
    REM --- Static library ---
    echo Building Debug static library...
    cl.exe /EHsc /std:c++20 /Od /Ob0 /Zi /W4 /WX /Fo"%OUTDIR%\\" %INCLUDES% %ROOT_FLAGS% /c %LIB_SRCS%
    lib.exe /OUT:%OUTDIR%\%LIB_NAME% %OUTDIR%\*.obj
    
    REM --- PHSPConvert.exe ---
    echo Building Debug PHSPConvert...
    cl.exe /EHsc /std:c++20 /Od /Ob0 /Zi /W4 /WX /Fo"%OUTDIR%\\" ^
      %INCLUDES% %ROOT_FLAGS% ^
      %COMMON_SRCS% PHSPConvert.cc ^
      /link /DEBUG /OUT:%OUTDIR%\PHSPConvert.exe %ROOT_LIBS%

    REM --- PHSPCombine.exe ---
    echo Building Debug PHSPCombine...
    cl.exe /EHsc /std:c++20 /Od /Ob0 /Zi /W4 /WX /Fo"%OUTDIR%\\" ^
      %INCLUDES% %ROOT_FLAGS% ^
      %COMMON_SRCS% PHSPCombine.cc ^
      /link /DEBUG /OUT:%OUTDIR%\PHSPCombine.exe %ROOT_LIBS%
      
    REM --- PHSPImage.exe ---
    echo Building Debug PHSPImage...
    cl.exe /EHsc /std:c++20 /Od /Ob0 /Zi /W4 /WX /Fo"%OUTDIR%\\" ^
      %INCLUDES% %ROOT_FLAGS% ^
      %COMMON_SRCS% PHSPImage.cc ^
      /link /DEBUG /OUT:%OUTDIR%\PHSPImage.exe %ROOT_LIBS%

    REM --- Dynamic library (.dll) ---
    if not exist "%OUTDIR%\bin" mkdir "%OUTDIR%\bin"
    echo Building Debug dynamic library...
    cl.exe /EHsc /std:c++20 /Od /Ob0 /Zi /W4 /WX %INCLUDES% %ROOT_FLAGS% /LD %LIB_SRCS% /link /OUT:%OUTDIR%\bin\particlezoo.dll %ROOT_LIBS%

) else (
    echo Release build.
    
    REM --- Static library ---
    echo Building Release static library...
    cl.exe /EHsc /std:c++20 /O2 /Ob2 /W4 /WX /Fo"%OUTDIR%\\" %INCLUDES% %ROOT_FLAGS% /c %LIB_SRCS%
    lib.exe /OUT:%OUTDIR%\%LIB_NAME% %OUTDIR%\*.obj
    
    REM --- PHSPConvert.exe ---
    echo Building Release PHSPConvert...
    cl.exe /EHsc /std:c++20 /O2 /Ob2 /W4 /WX /Fo"%OUTDIR%\\" ^
      %INCLUDES% %ROOT_FLAGS% ^
      %COMMON_SRCS% PHSPConvert.cc ^
      /link /OUT:%OUTDIR%\PHSPConvert.exe %ROOT_LIBS%

    REM --- PHSPCombine.exe ---
    echo Building Release PHSPCombine...
    cl.exe /EHsc /std:c++20 /O2 /Ob2 /W4 /WX /Fo"%OUTDIR%\\" ^
      %INCLUDES% %ROOT_FLAGS% ^
      %COMMON_SRCS% PHSPCombine.cc ^
      /link /OUT:%OUTDIR%\PHSPCombine.exe %ROOT_LIBS%
      
    REM --- PHSPImage.exe ---
    echo Building Release PHSPImage...
    cl.exe /EHsc /std:c++20 /O2 /Ob2 /W4 /WX /Fo"%OUTDIR%\\" ^
      %INCLUDES% %ROOT_FLAGS% ^
      %COMMON_SRCS% PHSPImage.cc ^
      /link /OUT:%OUTDIR%\PHSPImage.exe %ROOT_LIBS%

    REM --- Dynamic library (.dll) ---
    echo Building Release dynamic library...
    cl.exe /EHsc /std:c++20 /O2 /Ob2 /W4 /WX %INCLUDES% %ROOT_FLAGS% /LD %LIB_SRCS% /link /OUT:%OUTDIR%\particlezoo.dll %ROOT_LIBS%
)

if %ERRORLEVEL%==0 (
    echo Deleting intermediate object files...
    del /Q "%OUTDIR%\*.obj"
    echo Build successful.
    echo.
    echo Build artifacts can be found in: %CD%\%OUTDIR%
    echo   - Executables: PHSPConvert.exe, PHSPCombine.exe, PHSPImage.exe
    echo   - Libraries: libparticlezoo.lib, particlezoo.dll
    echo.
)

if %ERRORLEVEL% NEQ 0 (
    echo Build failed. Intermediate object files are retained.
)

REM Install if requested
if defined DO_INSTALL (
    echo Installing to %PREFIX%...
    
    REM Try to create directories
    echo Creating directories...
    
    mkdir "%PREFIX%\bin" 2>nul
    if not exist "%PREFIX%\bin" (
        set INSTALL_FAILED=1
    ) else (
        mkdir "%PREFIX%\include" 2>nul
        mkdir "%PREFIX%\lib" 2>nul
        copy "%OUTDIR%\PHSPConvert.exe" "%PREFIX%\bin" >nul
        copy "%OUTDIR%\PHSPCombine.exe" "%PREFIX%\bin" >nul
        copy "%OUTDIR%\PHSPImage.exe" "%PREFIX%\bin" >nul
        copy "%OUTDIR%\particlezoo.dll" "%PREFIX%\bin" >nul
        copy "%OUTDIR%\%LIB_NAME%" "%PREFIX%\lib" >nul
        xcopy /E /I "include\particlezoo" "%PREFIX%\include\particlezoo" >nul
    )
    
    if defined INSTALL_FAILED (
        echo.
        echo Installation failed. This is likely due to insufficient permissions.
        echo.
        echo Try running this script with administrator privileges:
        echo   1. Right-click on build.bat 
        echo   2. Select "Run as administrator"
        echo.
    ) else (
        echo Installation complete.
        echo.
        echo Files have been installed to:
        echo   - Executables and dynamic library: %PREFIX%\bin
        echo   - Static library: %PREFIX%\lib
        echo   - Headers: %PREFIX%\include\particlezoo
        echo.
        echo REMINDER: To use the executables from any directory, add %PREFIX%\bin to your PATH
        echo   You can do this by running this command:
        echo    setx PATH "%%PATH%%;%PREFIX%\bin"
        echo.
    )
)