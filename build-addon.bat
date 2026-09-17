@echo off
rem Builds the ReShade add-on, x64 Release. Output: x64\Release\addon\SMT.addon64
rem   build-addon.bat          with the toolset named in the project (v145)
rem   build-addon.bat v143     with Visual Studio 2022's toolset
rem Run it from a Developer Command Prompt. Outside one, it looks for Visual Studio 2022 Community.
setlocal
where msbuild >nul 2>&1 || call "%ProgramFiles%\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat" >nul 2>&1
where msbuild >nul 2>&1 || (echo msbuild not found. Run this from a Developer Command Prompt. & exit /b 1)
set TOOLSET=
if not "%~1"=="" set TOOLSET=/p:PlatformToolset=%~1
msbuild "%~dp0DLL\SMT.addon.vcxproj" /nologo /v:m /m /p:Configuration=Release /p:Platform=x64 %TOOLSET%
if errorlevel 1 (echo BUILD FAILED & exit /b 1)
echo BUILD OK: %~dp0x64\Release\addon\SMT.addon64
