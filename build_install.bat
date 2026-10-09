@echo off
rem Builds SMT (Release, x64) with the newest Visual Studio on this PC and installs SMT.asi into the game, keeping
rem the one it replaces as x64\rollback\SMT_previous.asi and the very first one as SMT_original.asi. Close the game
rem first. It only replaces an SMT.asi that is already installed, so the mod is never loaded twice under two names.
rem   build_install.bat [the game's Sources\Bin folder]
rem Without a folder the Steam default is used. Run it from a cmd window or by a double click.
setlocal
cd /d "%~dp0"
set "BIN=%~1"
if "%BIN%"=="" set "BIN=C:\Program Files (x86)\Steam\steamapps\common\Snowrunner\Sources\Bin"

echo == Visual Studio
set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if not exist "%VSWHERE%" goto novs
set "MSBUILD="
for /f "usebackq delims=" %%i in (`"%VSWHERE%" -latest -products * -requires Microsoft.Component.MSBuild -find MSBuild\**\Bin\MSBuild.exe`) do set "MSBUILD=%%i"
for /f "usebackq tokens=1 delims=." %%v in (`"%VSWHERE%" -latest -products * -property installationVersion`) do set "VSMAJOR=%%v"
if not defined MSBUILD goto novs
rem the project asks for Visual Studio 2026's compiler (v145); an older Visual Studio builds it with its own
set "TOOLSET="
if "%VSMAJOR%"=="17" set "TOOLSET=/p:PlatformToolset=v143"
if "%VSMAJOR%"=="16" set "TOOLSET=/p:PlatformToolset=v142"
echo Visual Studio %VSMAJOR%: %MSBUILD% %TOOLSET%
for /f "delims=" %%b in ('git rev-parse --abbrev-ref HEAD 2^>nul') do set "GITBRANCH=%%b"
echo Branch: %GITBRANCH%

echo == the build
"%MSBUILD%" "SnowRunner Manual Transmission.sln" /p:Configuration=Release /p:Platform=x64 %TOOLSET% /m /v:minimal /nologo || goto fail
set "ASI=%~dp0x64\Release\SMT.asi"
if not exist "%ASI%" set "ASI=%~dp0DLL\x64\Release\SMT.asi"
if not exist "%ASI%" goto noasi

echo == the install
tasklist /fi "imagename eq SnowRunner.exe" 2>nul | find /i "SnowRunner.exe" >nul && goto running
if not exist "%BIN%\SnowRunner.exe" goto nobin
if not exist "%BIN%\SMT.asi" goto noold
if not exist x64\rollback mkdir x64\rollback
rem the one installed before this script ever ran is kept for good, the last one replaced as well
if not exist x64\rollback\SMT_original.asi copy /y "%BIN%\SMT.asi" x64\rollback\SMT_original.asi >nul
copy /y "%BIN%\SMT.asi" x64\rollback\SMT_previous.asi >nul || goto nocopy
copy /y "%ASI%" "%BIN%\SMT.asi" >nul || goto nocopy
echo.
echo INSTALL OK: SMT.asi from %GITBRANCH% is in the game. The one it replaced is x64\rollback\SMT_previous.asi,
echo the one from before this script first ran is x64\rollback\SMT_original.asi.
pause
exit /b 0

:novs
echo No Visual Studio with MSBuild was found (vswhere.exe or MSBuild.exe is missing).
goto fail
:noasi
echo The build ran but no SMT.asi was found under x64\Release.
goto fail
:running
echo The game is running. Close it and run build_install.bat again.
goto fail
:nobin
echo There is no SnowRunner.exe in "%BIN%".
echo Give the game's Sources\Bin folder: build_install.bat "D:\Games\Snowrunner\Sources\Bin"
goto fail
:noold
echo There is no SMT.asi in "%BIN%" to replace. The .asi files there:
dir /b "%BIN%\*.asi"
echo If SMT is installed under another name, rename it SMT.asi first, or copy "%ASI%" in by hand.
goto fail
:nocopy
echo SMT.asi could not be copied into "%BIN%". Is the game still closing? Try again, or run the window as administrator.
goto fail
:fail
echo.
echo FAILED: nothing was installed.
pause
exit /b 1
