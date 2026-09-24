@echo off
setlocal enabledelayedexpansion
if "%GBDK_HOME%"=="" set "GBDK_HOME=C:\gbdk"
set "LCC=%GBDK_HOME%\bin\lcc.exe"
set "CFLAGS=-Isrc -Iassets"
set "LFLAGS=-Wm-yC -Wm-ynGAME -Wl-yt0x1B -Wl-ya1 -Wl-yoA -autobank"
rem lcc splits paths that contain spaces, so compile with paths relative to the project folder
cd /d "%~dp0"
set "ROOT=%CD%\"
if exist build rmdir /s /q build
mkdir build\obj
set "OBJS="
for /r src %%f in (*.c) do (
  set "REL=%%f"
  set "REL=!REL:%ROOT%=!"
  "%LCC%" %CFLAGS% -c -o "build\obj\%%~nf.o" "!REL!" || goto :fail
  set "OBJS=!OBJS! build\obj\%%~nf.o"
)
for /r assets %%f in (*.c) do (
  set "REL=%%f"
  set "REL=!REL:%ROOT%=!"
  "%LCC%" %CFLAGS% -c -o "build\obj\%%~nf.o" "!REL!" || goto :fail
  set "OBJS=!OBJS! build\obj\%%~nf.o"
)
"%LCC%" %LFLAGS% -o build\game.gb !OBJS! || goto :fail
echo BUILD OK: build\game.gb
exit /b 0
:fail
echo BUILD FAILED
exit /b 1
