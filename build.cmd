@echo off
setlocal
if defined VSCMD_ARG_TGT_ARCH if /i "%VSCMD_ARG_TGT_ARCH%"=="x64" goto compile
set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if not exist "%VSWHERE%" (
    echo Install Visual Studio Build Tools with Desktop development with C++.
    exit /b 1
)
for /f "usebackq tokens=*" %%i in (`"%VSWHERE%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do set "VSINSTALL=%%i"
if not defined VSINSTALL (
    echo MSVC x64 build tools were not found.
    exit /b 1
)
call "%VSINSTALL%\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b 1
:compile
pushd "%~dp0" || exit /b 1
if not exist build mkdir build
if not exist build (
    popd
    exit /b 1
)
pushd src
rc /nologo /fo ..\build\app.res app.rc
set "RESOURCE_RESULT=%ERRORLEVEL%"
popd
if not "%RESOURCE_RESULT%"=="0" (
    popd
    exit /b %RESOURCE_RESULT%
)
cl /nologo /std:c++17 /W4 /O1 /GL /Gy /Gw /GR- /MT /EHsc /guard:cf /utf-8 src\main.cpp build\app.res /Fo:build\main.obj /Fe:build\NoSiesta.exe /link /LTCG /INCREMENTAL:NO /SUBSYSTEM:WINDOWS /OPT:REF /OPT:ICF /GUARD:CF /DYNAMICBASE /HIGHENTROPYVA /NXCOMPAT /CETCOMPAT user32.lib gdi32.lib gdiplus.lib dwmapi.lib shlwapi.lib comctl32.lib
set "BUILD_RESULT=%ERRORLEVEL%"
popd
exit /b %BUILD_RESULT%
