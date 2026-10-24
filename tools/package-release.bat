@echo off
setlocal

set ROOT=%~dp0..
set RELEASE=%ROOT%\build\release
set STAGE=%RELEASE%\stage
set BUILD_MANAGED=%ROOT%\build\managed
set BUILD_NATIVE=%ROOT%\build\native
set RROML_VERSION=1.2.0
set SLIGHTLY_BETTER_VISUALS_VERSION=1.0.0

call "%ROOT%\tools\build-managed.bat"
if errorlevel 1 exit /b 1

call "%ROOT%\tools\build-native.bat"
if errorlevel 1 exit /b 1

if exist "%STAGE%" rmdir /s /q "%STAGE%"
if not exist "%RELEASE%" mkdir "%RELEASE%"
mkdir "%STAGE%\RROML\arr\Binaries\Win64"
mkdir "%STAGE%\RROML\RROML"
mkdir "%STAGE%\SlightlyBetterVisuals\Mods\SlightlyBetterVisuals"

copy /Y "%BUILD_NATIVE%\XINPUT1_3.dll" "%STAGE%\RROML\arr\Binaries\Win64\XINPUT1_3.dll" >nul
copy /Y "%BUILD_MANAGED%\RROML.Core.dll" "%STAGE%\RROML\RROML\RROML.Core.dll" >nul
copy /Y "%BUILD_MANAGED%\RROML.Abstractions.dll" "%STAGE%\RROML\RROML\RROML.Abstractions.dll" >nul
if exist "%BUILD_MANAGED%\MoonSharp.Interpreter.dll" copy /Y "%BUILD_MANAGED%\MoonSharp.Interpreter.dll" "%STAGE%\RROML\RROML\MoonSharp.Interpreter.dll" >nul

if exist "%BUILD_MANAGED%\SlightlyBetterVisuals.dll" copy /Y "%BUILD_MANAGED%\SlightlyBetterVisuals.dll" "%STAGE%\SlightlyBetterVisuals\Mods\SlightlyBetterVisuals\SlightlyBetterVisuals.dll" >nul
if exist "%ROOT%\src\SlightlyBetterVisuals\mod.json" copy /Y "%ROOT%\src\SlightlyBetterVisuals\mod.json" "%STAGE%\SlightlyBetterVisuals\Mods\SlightlyBetterVisuals\mod.json" >nul

if exist "%RELEASE%\RROML-v%RROML_VERSION%.zip" del /q "%RELEASE%\RROML-v%RROML_VERSION%.zip"
if exist "%RELEASE%\Slightly-Better-Visuals-v%SLIGHTLY_BETTER_VISUALS_VERSION%.zip" del /q "%RELEASE%\Slightly-Better-Visuals-v%SLIGHTLY_BETTER_VISUALS_VERSION%.zip"

powershell -NoProfile -Command "Compress-Archive -Path '%STAGE%\RROML\*' -DestinationPath '%RELEASE%\RROML-v%RROML_VERSION%.zip'"
if errorlevel 1 exit /b 1
powershell -NoProfile -Command "Compress-Archive -Path '%STAGE%\SlightlyBetterVisuals\*' -DestinationPath '%RELEASE%\Slightly-Better-Visuals-v%SLIGHTLY_BETTER_VISUALS_VERSION%.zip'"
if errorlevel 1 exit /b 1

echo Release packages created in "%RELEASE%".
exit /b 0