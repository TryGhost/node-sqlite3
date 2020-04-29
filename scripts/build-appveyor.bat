@ECHO OFF
SETLOCAL
SET EL=0

ECHO APPVEYOR^: %APPVEYOR%
ECHO nodejs_version^: %nodejs_version%
ECHO platform^: %platform%

ECHO downloading/installing node
powershell Update-NodeJsInstallation (Get-NodeJsLatestBuild $env:nodejs_version) $env:PLATFORM
IF %ERRORLEVEL% NEQ 0 GOTO ERROR

set iterations=500
:loop
set /a iterations -= 1
CALL node -v
IF %ERRORLEVEL% NEQ 0 GOTO ERROR
if %iterations% GTR 0 GOTO loop


GOTO DONE

:ERROR
ECHO ~~~~~~~~~~~~~~~~~~~~~~ ERROR %~f0 ~~~~~~~~~~~~~~~~~~~~~~~~~~~~
ECHO ERRORLEVEL^: %ERRORLEVEL%
SET EL=%ERRORLEVEL%

:DONE
ECHO ~~~~~~~~~~~~~~~~~~~~~~ DONE %~f0 ~~~~~~~~~~~~~~~~~~~~~~~~~~~~

EXIT /b %EL%
