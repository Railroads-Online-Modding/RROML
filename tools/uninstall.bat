@echo off
setlocal

if "%~1"=="" (
    echo Usage: uninstall.bat "D:\SteamLibrary\steamapps\common\Railroads Online"
    exit /b 1
)

set GAME_ROOT=%~1
set LOADER=%GAME_ROOT%\RROML
set MODS=%GAME_ROOT%\Mods
set PROXY_TARGET=%GAME_ROOT%\arr\Binaries\Win64\XINPUT1_3.dll

if exist "%PROXY_TARGET%" del /Q "%PROXY_TARGET%"
if exist "%LOADER%" rmdir /S /Q "%LOADER%"
if exist "%MODS%\ExampleMod\ExampleMod.dll" del /Q "%MODS%\ExampleMod\ExampleMod.dll"
if exist "%MODS%\ExampleMod\mod.json" del /Q "%MODS%\ExampleMod\mod.json"
if exist "%MODS%\ExampleMod" rmdir /Q "%MODS%\ExampleMod"
if exist "%MODS%\UEBridgeMod\UEBridgeMod.dll" del /Q "%MODS%\UEBridgeMod\UEBridgeMod.dll"
if exist "%MODS%\UEBridgeMod\mod.json" del /Q "%MODS%\UEBridgeMod\mod.json"
if exist "%MODS%\UEBridgeMod" rmdir /Q "%MODS%\UEBridgeMod"
if exist "%MODS%\ModsMenuMod\ModsMenuMod.dll" del /Q "%MODS%\ModsMenuMod\ModsMenuMod.dll"
if exist "%MODS%\ModsMenuMod\mod.json" del /Q "%MODS%\ModsMenuMod\mod.json"
if exist "%MODS%\ModsMenuMod" rmdir /Q "%MODS%\ModsMenuMod"

if exist "%MODS%\TimeWarpMod\TimeWarpMod.dll" del /Q "%MODS%\TimeWarpMod\TimeWarpMod.dll"
if exist "%MODS%\TimeWarpMod\mod.json" del /Q "%MODS%\TimeWarpMod\mod.json"
if exist "%MODS%\TimeWarpMod" rmdir /Q "%MODS%\TimeWarpMod"
if exist "%MODS%\MenuPauseMod\MenuPauseMod.dll" del /Q "%MODS%\MenuPauseMod\MenuPauseMod.dll"
if exist "%MODS%\MenuPauseMod\mod.json" del /Q "%MODS%\MenuPauseMod\mod.json"
if exist "%MODS%\MenuPauseMod" rmdir /Q "%MODS%\MenuPauseMod"
if exist "%MODS%\RealRailroading\RealRailroading.dll" del /Q "%MODS%\RealRailroading\RealRailroading.dll"
if exist "%MODS%\RealRailroading\mod.json" del /Q "%MODS%\RealRailroading\mod.json"
if exist "%MODS%\RealRailroading" rmdir /Q "%MODS%\RealRailroading"
if exist "%MODS%\BackgroundAudioMod\BackgroundAudioMod.dll" del /Q "%MODS%\BackgroundAudioMod\BackgroundAudioMod.dll"
if exist "%MODS%\BackgroundAudioMod\mod.json" del /Q "%MODS%\BackgroundAudioMod\mod.json"
if exist "%MODS%\BackgroundAudioMod" rmdir /Q "%MODS%\BackgroundAudioMod"

echo RROML uninstall finished.
echo Any other mod DLLs or mod folders inside "%MODS%" were left alone.
exit /b 0
