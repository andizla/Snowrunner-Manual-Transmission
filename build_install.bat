@echo off
rem Builds the ReShade add-on (SMT.addon64, Release, x64) with the newest Visual Studio on this PC and installs it into
rem the game, keeping the one it replaces as x64\rollback\SMT_previous.addon64 and the very first one as
rem SMT_original.addon64. Close the game first. Without an SMT.addon64 in the game it is a first install; it stops when
rem SMT.asi is in the game too (the two builds hook the same game functions, and the add-on refuses to start beside it)
rem or when another add-on there looks like SMT under another name, so the mod is never loaded twice.
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
"%MSBUILD%" "DLL\SMT.addon.vcxproj" /p:Configuration=Release /p:Platform=x64 %TOOLSET% /m /v:minimal /nologo || goto fail
set "ADDON=%~dp0x64\Release\addon\SMT.addon64"
if not exist "%ADDON%" goto noaddon

echo == the install
tasklist /fi "imagename eq SnowRunner.exe" 2>nul | find /i "SnowRunner.exe" >nul && goto running
if not exist "%BIN%\SnowRunner.exe" goto nobin
set "ASI="
for %%d in ("%BIN%" "%BIN%\scripts" "%BIN%\plugins") do if exist "%%~d\SMT.asi" set "ASI=%%~d\SMT.asi"
if defined ASI goto asi
if not exist "%BIN%\SMT.addon64" goto first
if not exist x64\rollback mkdir x64\rollback
rem the one installed before this script ever ran is kept for good, the last one replaced as well
if not exist x64\rollback\SMT_original.addon64 copy /y "%BIN%\SMT.addon64" x64\rollback\SMT_original.addon64 >nul
copy /y "%BIN%\SMT.addon64" x64\rollback\SMT_previous.addon64 >nul || goto nocopy
copy /y "%ADDON%" "%BIN%\SMT.addon64" >nul || goto nocopy
echo.
echo INSTALL OK: SMT.addon64 from %GITBRANCH% is in the game. The one it replaced is x64\rollback\SMT_previous.addon64,
echo the one from before this script first ran is x64\rollback\SMT_original.addon64.
pause
exit /b 0

:first
set "OTHER="
for /f "delims=" %%f in ('dir /b "%BIN%\*smt*.addon64" "%BIN%\*transmission*.addon64" "%BIN%\*smt*.addon" "%BIN%\*transmission*.addon" 2^>nul') do set "OTHER=%BIN%\%%f"
if defined OTHER goto other
copy /y "%ADDON%" "%BIN%\SMT.addon64" >nul || goto nocopy
echo.
echo INSTALL OK: SMT.addon64 from %GITBRANCH% is in the game for the first time. The Add-ons tab of the ReShade
echo overlay lists it as "SnowRunner Manual Transmission".
pause
exit /b 0

:novs
echo No Visual Studio with MSBuild was found (vswhere.exe or MSBuild.exe is missing).
goto fail
:noaddon
echo The build ran but no SMT.addon64 was found under x64\Release\addon.
goto fail
:running
echo The game is running. Close it and run build_install.bat again.
goto fail
:nobin
echo There is no SnowRunner.exe in "%BIN%".
echo Give the game's Sources\Bin folder: build_install.bat "D:\Games\Snowrunner\Sources\Bin"
goto fail
:asi
echo "%ASI%" is in the game: the add-on does not start beside the ASI build. Take SMT.asi out and run this again.
goto fail
:other
echo There is no SMT.addon64 in the game, but "%OTHER%" looks like SMT under another name.
echo Rename it SMT.addon64 and run build_install.bat again, so the mod is not loaded twice.
goto fail
:nocopy
echo SMT.addon64 could not be copied into "%BIN%". Is the game still closing? Try again, or run the window as administrator.
goto fail
:fail
echo.
echo FAILED: nothing was installed.
pause
exit /b 1
