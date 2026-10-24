@echo off
setlocal

set ROOT=%~dp0..
set MINGW_BIN=D:\msys64\mingw64\bin
set GPP=%MINGW_BIN%\g++.exe
set DLLTOOL=%MINGW_BIN%\dlltool.exe
set INCLUDE_NETFX=C:\Program Files (x86)\Windows Kits\NETFXSDK\4.8.1\Include\um
set OUTDIR=%ROOT%\build\native
set EXPORT_OBJ=%OUTDIR%\XINPUT1_3.exp.o

set PATH=%MINGW_BIN%;%PATH%

if not exist "%GPP%" exit /b 1
if not exist "%DLLTOOL%" exit /b 1
if not exist "%OUTDIR%" mkdir "%OUTDIR%"

"%DLLTOOL%" --input-def "%ROOT%\native\XInputProxy\XINPUT1_3.def" --dllname XINPUT1_3.dll --output-exp "%EXPORT_OBJ%"
if errorlevel 1 exit /b 1

"%GPP%" -shared -static-libgcc -static-libstdc++ -s -o "%OUTDIR%\XINPUT1_3.dll" "%ROOT%\native\XInputProxy\ProxyMain.cpp" "%EXPORT_OBJ%" -I"%INCLUDE_NETFX%" -ld2d1 -ldwrite -ld3d11 -ldxgi -lgdi32 -luser32 -lole32 -lwinmm
if errorlevel 1 exit /b 1

echo Native build finished.
exit /b 0

