@echo off
setlocal enabledelayedexpansion

REM Parse command-line arguments
set PREFIX=%LOCALAPPDATA%\particlezoo
set BUILD_TYPE=release
set JOBS=

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
if /I "%~1"=="-j" (
    set "JOBS=%~2"
    shift
    shift
    goto :parse_args
)
if /I "%~1:~0,3%"=="-j=" (
    set "JOBS=%~1:~3%"
    shift
    goto :parse_args
)
if /I "%~1"=="--jobs" (
    set "JOBS=%~2"
    shift
    shift
    goto :parse_args
)
if /I "%~1:~0,7%"=="--jobs=" (
    set "JOBS=%~1:~7%"
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
if defined JOBS echo   Parallel jobs: %JOBS%


REM Write configuration to config.status
echo Writing configuration to config.status...
(
echo PREFIX=%PREFIX%
) > config.status

echo Configuration complete.

REM Set output directories
set GCC_BIN_DIR_REL=build\msvc\release
set GCC_BIN_DIR_DBG=build\msvc\debug
set OUTDIR=build\msvc\%BUILD_TYPE%
set OBJDIR=%OUTDIR%\obj

REM Create output/object directories if they don't exist
if not exist "%OUTDIR%" mkdir "%OUTDIR%"
if not exist "%OBJDIR%" mkdir "%OBJDIR%"

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
    set CFLAGS=/EHsc /std:c++20 /Od /Ob0 /Zi /W4 /WX
) else (
    echo Release build.
    set CFLAGS=/EHsc /std:c++20 /O2 /Ob2 /W4 /WX
)

REM Add multi-processor compilation if jobs specified (default: let MSVC pick if just /MP)
if defined JOBS (
    for /f "tokens=*" %%J in ("%JOBS%") do set CFLAGS=%CFLAGS% /MP%%J
) else (
    set CFLAGS=%CFLAGS% /MP
)

echo Compiling common sources (parallel)...
set PDB=%OUTDIR%\particlezoo.pdb
cl.exe %CFLAGS% /FS /Fd"%PDB%" /Fo"%OBJDIR%\\" %INCLUDES% /c %COMMON_SRCS% || goto :build_fail

REM Build OBJ_LIST (now that objects are produced by single invocation)
set "OBJ_LIST="
for %%F in (%COMMON_SRCS%) do set "OBJ_LIST=!OBJ_LIST! %OBJDIR%\%%~nF.obj"
REM echo Objects: !OBJ_LIST!

echo Building static library %LIB_NAME% ...
lib.exe /OUT:%OUTDIR%\%LIB_NAME% !OBJ_LIST! || goto :build_fail
REM Build executables (compile unique source then link with common objects)
echo Building PHSPConvert.exe ...
cl.exe %CFLAGS% /Fo"%OBJDIR%\\" %INCLUDES% /c PHSPConvert.cc || goto :build_fail
cl.exe %CFLAGS% /Fe"%OUTDIR%\PHSPConvert.exe" !OBJ_LIST! %OBJDIR%\PHSPConvert.obj || goto :build_fail

echo Building PHSPCombine.exe ...
cl.exe %CFLAGS% /Fo"%OBJDIR%\\" %INCLUDES% /c PHSPCombine.cc || goto :build_fail
cl.exe %CFLAGS% /Fe"%OUTDIR%\PHSPCombine.exe" !OBJ_LIST! %OBJDIR%\PHSPCombine.obj || goto :build_fail

echo Building PHSPImage.exe ...
cl.exe %CFLAGS% /Fo"%OBJDIR%\\" %INCLUDES% /c PHSPImage.cc || goto :build_fail
cl.exe %CFLAGS% /Fe"%OUTDIR%\PHSPImage.exe" !OBJ_LIST! %OBJDIR%\PHSPImage.obj || goto :build_fail

echo Building PHSPSplit.exe ...
cl.exe %CFLAGS% /Fo"%OBJDIR%\\" %INCLUDES% /c PHSPSplit.cc || goto :build_fail
cl.exe %CFLAGS% /Fe"%OUTDIR%\PHSPSplit.exe" !OBJ_LIST! %OBJDIR%\PHSPSplit.obj || goto :build_fail

REM Build dynamic library
if not exist "%OUTDIR%\bin" mkdir "%OUTDIR%\bin"
echo Building dynamic library particlezoo.dll ...
link /DLL /OUT:%OUTDIR%\bin\particlezoo.dll !OBJ_LIST! || goto :build_fail
goto :build_success

:build_success
echo Build successful.
echo Artifacts in %OUTDIR%
goto :post_build

:build_fail
echo Build failed.
exit /b 1

:post_build

REM Decide whether to keep objects; keeping helps incremental builds. Uncomment to delete.
REM del /Q "%OBJDIR%\*.obj"

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
    REM Added /Y to suppress overwrite prompts (prevents hidden hang when artifacts already exist)
    copy /Y "%OUTDIR%\PHSPConvert.exe" "%PREFIX%\bin\" >nul
    copy /Y "%OUTDIR%\PHSPCombine.exe" "%PREFIX%\bin\" >nul
    copy /Y "%OUTDIR%\PHSPImage.exe" "%PREFIX%\bin\" >nul
    copy /Y "%OUTDIR%\PHSPSplit.exe" "%PREFIX%\bin\" >nul
	copy /Y "%OUTDIR%\bin\particlezoo.dll" "%PREFIX%\bin\" >nul
    copy /Y "%OUTDIR%\%LIB_NAME%" "%PREFIX%\lib\" >nul
    xcopy /E /I /Y "include\particlezoo" "%PREFIX%\include\particlezoo" >nul
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