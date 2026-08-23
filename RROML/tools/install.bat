@echo off
setlocal

if "%~1"=="" (
    echo Usage: install.bat "D:\SteamLibrary\steamapps\common\Railroads Online"
    exit /b 1
)

set GAME_ROOT=%~1
set ROOT=%~dp0..
set BUILD=%ROOT%\build\managed
set LOADER=%GAME_ROOT%\RROML
set MODS=%GAME_ROOT%\Mods
set PROXY=%ROOT%\build\native\XINPUT1_3.dll
set PROXY_TARGET=%GAME_ROOT%\arr\Binaries\Win64\XINPUT1_3.dll

call "%ROOT%\tools\build-managed.bat"
if errorlevel 1 exit /b 1

call "%ROOT%\tools\build-native.bat"
if errorlevel 1 exit /b 1

if not exist "%GAME_ROOT%\arr.exe" (
    echo Could not find arr.exe in "%GAME_ROOT%"
    exit /b 1
)

if not exist "%LOADER%" mkdir "%LOADER%"
if not exist "%MODS%" mkdir "%MODS%"
if not exist "%LOADER%\Logs" mkdir "%LOADER%\Logs"

copy /Y "%BUILD%\RROML.Abstractions.dll" "%LOADER%\RROML.Abstractions.dll" >nul
copy /Y "%BUILD%\RROML.Core.dll" "%LOADER%\RROML.Core.dll" >nul
if exist "%BUILD%\MoonSharp.Interpreter.dll" copy /Y "%BUILD%\MoonSharp.Interpreter.dll" "%LOADER%\MoonSharp.Interpreter.dll" >nul
copy /Y "%PROXY%" "%PROXY_TARGET%" >nul

if not exist "%LOADER%\RROML.config.json" (
    >"%LOADER%\RROML.config.json" echo {"Enabled":true,"VerboseLogging":true,"ShowOverlay":true,"OverlayPosition":"TopLeft","DisabledMods":[]}
) else (
    powershell -NoProfile -ExecutionPolicy Bypass -Command "$path = '%LOADER%\RROML.config.json'; $json = Get-Content -Raw -Path $path | ConvertFrom-Json; if ($null -eq $json.PSObject.Properties['ShowOverlay']) { $json | Add-Member -NotePropertyName ShowOverlay -NotePropertyValue $true } else { $json.ShowOverlay = $true }; if ($null -eq $json.PSObject.Properties['OverlayPosition']) { $json | Add-Member -NotePropertyName OverlayPosition -NotePropertyValue 'TopLeft' } elseif ([string]::IsNullOrWhiteSpace([string]$json.OverlayPosition)) { $json.OverlayPosition = 'TopLeft' }; $json | ConvertTo-Json -Compress | Set-Content -Path $path"
)

if exist "%BUILD%\ExampleMod.dll" (
    if not exist "%MODS%\ExampleMod" mkdir "%MODS%\ExampleMod"
    copy /Y "%BUILD%\ExampleMod.dll" "%MODS%\ExampleMod\ExampleMod.dll" >nul
    copy /Y "%ROOT%\src\ExampleMod\mod.json" "%MODS%\ExampleMod\mod.json" >nul
)
if exist "%BUILD%\UEBridgeMod.dll" (
    if not exist "%MODS%\UEBridgeMod" mkdir "%MODS%\UEBridgeMod"
    copy /Y "%BUILD%\UEBridgeMod.dll" "%MODS%\UEBridgeMod\UEBridgeMod.dll" >nul
    copy /Y "%ROOT%\src\UEBridgeMod\mod.json" "%MODS%\UEBridgeMod\mod.json" >nul
)
if exist "%BUILD%\ModsMenuMod.dll" (
    if not exist "%MODS%\ModsMenuMod" mkdir "%MODS%\ModsMenuMod"
    copy /Y "%BUILD%\ModsMenuMod.dll" "%MODS%\ModsMenuMod\ModsMenuMod.dll" >nul
    copy /Y "%ROOT%\src\ModsMenuMod\mod.json" "%MODS%\ModsMenuMod\mod.json" >nul
)
if exist "%BUILD%\TimeWarpMod.dll" (
    if not exist "%MODS%\TimeWarpMod" mkdir "%MODS%\TimeWarpMod"
    copy /Y "%BUILD%\TimeWarpMod.dll" "%MODS%\TimeWarpMod\TimeWarpMod.dll" >nul
    copy /Y "%ROOT%\src\TimeWarpMod\mod.json" "%MODS%\TimeWarpMod\mod.json" >nul
)
if exist "%BUILD%\MenuPauseMod.dll" (
    if not exist "%MODS%\MenuPauseMod" mkdir "%MODS%\MenuPauseMod"
    copy /Y "%BUILD%\MenuPauseMod.dll" "%MODS%\MenuPauseMod\MenuPauseMod.dll" >nul
    copy /Y "%ROOT%\src\MenuPauseMod\mod.json" "%MODS%\MenuPauseMod\mod.json" >nul
)
if exist "%BUILD%\RealRailroading.dll" (
    if not exist "%MODS%\RealRailroading" mkdir "%MODS%\RealRailroading"
    copy /Y "%BUILD%\RealRailroading.dll" "%MODS%\RealRailroading\RealRailroading.dll" >nul
    copy /Y "%ROOT%\src\RealRailroading\mod.json" "%MODS%\RealRailroading\mod.json" >nul
)
if exist "%BUILD%\BackgroundAudioMod.dll" (
    if not exist "%MODS%\BackgroundAudioMod" mkdir "%MODS%\BackgroundAudioMod"
    copy /Y "%BUILD%\BackgroundAudioMod.dll" "%MODS%\BackgroundAudioMod\BackgroundAudioMod.dll" >nul
    copy /Y "%ROOT%\src\BackgroundAudioMod\mod.json" "%MODS%\BackgroundAudioMod\mod.json" >nul
)
if exist "%BUILD%\SlightlyBetterVisuals.dll" (
    if not exist "%MODS%\SlightlyBetterVisuals" mkdir "%MODS%\SlightlyBetterVisuals"
    copy /Y "%BUILD%\SlightlyBetterVisuals.dll" "%MODS%\SlightlyBetterVisuals\SlightlyBetterVisuals.dll" >nul
    copy /Y "%ROOT%\src\SlightlyBetterVisuals\mod.json" "%MODS%\SlightlyBetterVisuals\mod.json" >nul
)
if exist "%ROOT%\src\ExampleLuaMod" (
    if not exist "%MODS%\ExampleLuaMod" mkdir "%MODS%\ExampleLuaMod"
    if exist "%ROOT%\src\ExampleLuaMod\main.lua" copy /Y "%ROOT%\src\ExampleLuaMod\main.lua" "%MODS%\ExampleLuaMod\main.lua" >nul
    if exist "%ROOT%\src\ExampleLuaMod\mod.json" copy /Y "%ROOT%\src\ExampleLuaMod\mod.json" "%MODS%\ExampleLuaMod\mod.json" >nul
)

echo Install finished.
echo Files placed in:
echo   "%LOADER%"
echo   "%MODS%"
echo   "%PROXY_TARGET%"
exit /b 0