@ECHO OFF
SETLOCAL
SET EL=0

ECHO APPVEYOR^: %APPVEYOR%
ECHO nodejs_version^: %nodejs_version%
ECHO platform^: %platform%

ECHO downloading/installing node
powershell Update-NodeJsInstallation (Get-NodeJsLatestBuild $env:nodejs_version) $env:PLATFORM
IF %ERRORLEVEL% NEQ 0 GOTO ERROR

for /l %x in (1, 1, 1000) do (
  CALL npm -v
  IF %ERRORLEVEL% NEQ 0 GOTO ERROR
)

:ERROR
ECHO ~~~~~~~~~~~~~~~~~~~~~~ ERROR %~f0 ~~~~~~~~~~~~~~~~~~~~~~~~~~~~
ECHO ERRORLEVEL^: %ERRORLEVEL%
SET EL=%ERRORLEVEL%

:DONE
ECHO ~~~~~~~~~~~~~~~~~~~~~~ DONE %~f0 ~~~~~~~~~~~~~~~~~~~~~~~~~~~~

EXIT /b %EL%
