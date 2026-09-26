@echo off
setlocal enabledelayedexpansion
rem Runs every unit test: engine (tests/engine, GBDK + headless emulator) and
rem tools (tests/tools, node --test + headless Chrome/Edge). Needs Node 22+.
chcp 65001 >nul
cd /d "%~dp0"
set "LOGDIR=build\tests"
if not exist "%LOGDIR%" mkdir "%LOGDIR%"
set "ENGINE_LOG=%LOGDIR%\engine.log"
set "TOOLS_TAP=%LOGDIR%\tools.tap"
set /a EP=0, EF=0, TP=0, TF=0
set "ERRORS="

where node >nul 2>&1 || (
  echo ERROR: node not found in PATH
  set "ERRORS=node not found"
  goto :summary
)

echo ============================================================
echo  Engine tests  (tests\engine, builds one ROM per suite...)
echo ============================================================
node tests\engine\run.mjs > "%ENGINE_LOG%" 2>&1
type "%ENGINE_LOG%"
set "ENGINE_OK="
for /f "tokens=1,3" %%a in ('findstr /r /c:"^[0-9][0-9]* passed, [0-9][0-9]* failed" "%ENGINE_LOG%"') do (
  set /a EP=%%a, EF=%%b
  set "ENGINE_OK=1"
)
if not defined ENGINE_OK set "ERRORS=!ERRORS! [engine tests did not run: see output above]"

echo.
echo ============================================================
echo  Tools tests  (tests\tools, headless Chrome/Edge)
echo ============================================================
if exist "%TOOLS_TAP%" del "%TOOLS_TAP%"
node --test --test-reporter=spec --test-reporter-destination=stdout --test-reporter=tap --test-reporter-destination="%TOOLS_TAP%" "tests/tools/*.test.mjs"
set "TOOLS_OK="
if exist "%TOOLS_TAP%" (
  for /f "tokens=3" %%a in ('findstr /b /c:"# pass " "%TOOLS_TAP%"') do set /a TP=%%a & set "TOOLS_OK=1"
  for /f "tokens=3" %%a in ('findstr /b /c:"# fail " "%TOOLS_TAP%"') do set /a TF+=%%a
  for /f "tokens=3" %%a in ('findstr /b /c:"# cancelled " "%TOOLS_TAP%"') do set /a TF+=%%a
)
if not defined TOOLS_OK set "ERRORS=!ERRORS! [tools tests did not run: see output above]"

:summary
set /a PASS=EP+TP, FAIL=EF+TF, TOTAL=PASS+FAIL
echo.
echo ============================================================
echo  RESULTS
echo ============================================================
echo  Engine : !EP! passed, !EF! failed
echo  Tools  : !TP! passed, !TF! failed
echo  ------------------------------------------------------------
echo  Total  : !TOTAL!   Pass: !PASS!   Fail: !FAIL!
if defined ERRORS echo  ERROR  :!ERRORS!
echo.
if "!FAIL!"=="0" if not defined ERRORS (
  echo  ALL TESTS PASSED
  set "RC=0"
  goto :end
)
echo  SOME TESTS FAILED  (failure messages are listed above, under each suite)
set "RC=1"
:end
echo.
set /p "DUMMY=Press Enter to close..."
exit /b %RC%
