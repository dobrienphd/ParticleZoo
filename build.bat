@echo off
setlocal enabledelayedexpansion

REM Parse command-line arguments
set PREFIX=%PROGRAMFILES%\particlezoo
set USE_ROOT=0
set BUILD_TYPE=release

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
if /I "%~1"=="--no-root" (
    set USE_ROOT=0
    shift
    goto :parse_args
)
if /I "%~1"=="--with-root" (
    set USE_ROOT=1
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

REM Try to find ROOT if not disabled
if "%USE_ROOT%"=="1" (
    echo Checking for ROOT installation...
    
    REM Check if ROOT_DIR environment variable is set
    if defined ROOT_DIR (
        if exist "!ROOT_DIR!\bin\root.exe" (
            echo Found ROOT at !ROOT_DIR!
            set ROOT_INCLUDE=/I "!ROOT_DIR!\include"
            set ROOT_LIBS="!ROOT_DIR!\lib\*.lib"
        ) else (
            echo WARNING: ROOT_DIR is set but doesn't appear to contain a valid ROOT installation
            set USE_ROOT=0
        )
    ) else (
        REM Try common ROOT installation locations
        for %%D in ("%PROGRAMFILES%\root" "C:\root" "C:\Program Files\root") do (
            if exist "%%~D\bin\root.exe" (
                echo Found ROOT at %%~D
                set ROOT_DIR=%%~D
                set ROOT_INCLUDE=/I "%%~D\include"
                set ROOT_LIBS="%%~D\lib\*.lib"
                goto :root_found
            )
        )
        
        echo WARNING: ROOT not found. Disabling ROOT support.
        set USE_ROOT=0
    )
)
:root_found

REM Write configuration to config.status
echo Writing configuration to config.status...
(
echo USE_ROOT=%USE_ROOT%
echo PREFIX=%PREFIX%
if "%USE_ROOT%"=="1" (
    echo ROOT_DIR=%ROOT_DIR%
    echo ROOT_INCLUDE=%ROOT_INCLUDE%
    echo ROOT_LIBS=%ROOT_LIBS%
)
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
set ROOT_FLAGS=
if "%USE_ROOT%"=="1" (
    set ROOT_FLAGS=/DUSE_ROOT=1 %ROOT_INCLUDE%
) else (
    set ROOT_LIBS=
)

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
src\topas\TOPASphspFile.cc ^
src\ROOT\ROOTphsp.cc

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
)

if %ERRORLEVEL%==0 (
    echo Deleting intermediate object files...
    del /Q "%OUTDIR%\*.obj"
    echo Build successful.
) else (
    echo Build failed. Intermediate object files are retained.
)

REM Install if requested
if defined DO_INSTALL (
    echo Installing to %PREFIX%...
    if not exist "%PREFIX%\bin" mkdir "%PREFIX%\bin"
    if not exist "%PREFIX%\lib" mkdir "%PREFIX%\lib"
    if not exist "%PREFIX%\include" mkdir "%PREFIX%\include"
    
    copy "%OUTDIR%\PHSPConvert.exe" "%PREFIX%\bin"
    copy "%OUTDIR%\PHSPCombine.exe" "%PREFIX%\bin"
    copy "%OUTDIR%\PHSPImage.exe" "%PREFIX%\bin"
    copy "%OUTDIR%\%LIB_NAME%" "%PREFIX%\lib"
    xcopy /E /I "include\particlezoo" "%PREFIX%\include\particlezoo"
    
    echo Installation complete.
)