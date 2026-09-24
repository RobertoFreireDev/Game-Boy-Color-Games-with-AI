@echo off
if "%EMULICIOUS%"=="" set "EMULICIOUS=C:\Emulicious\Emulicious.exe"
start "" "%EMULICIOUS%" "%~dp0build\game.gb"
