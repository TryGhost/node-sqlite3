@ECHO OFF
SETLOCAL
SET EL=0

ECHO ~~~~~~~~~~~~~~~~~~~~~~~~~~~~ %~f0 ~~~~~~~~~~~~~~~~~~~~~~~~~~~~

SET PATH=C:\Python27;%PATH%

SET APPVEYOR_REPO_COMMIT_MESSAGE=local build

IF EXIST lib\binding ECHO deleting lib/binding && RD /Q /S lib\binding
IF %ERRORLEVEL% NEQ 0 GOTO ERROR
IF EXIST node_modules ECHO deleting node_modules && RD /Q /S node_modules
IF %ERRORLEVEL% NEQ 0 GOTO ERROR

GOTO VS2015

ECHO                 ============================
ECHO                           VS2013
ECHO                 ============================
SET nodejs_version=4.4.2
SET platform=x86
SET msvs_toolset=12
SET TOOLSET_ARGS=

CALL scripts\build-appveyor.bat
IF %ERRORLEVEL% NEQ 0 GOTO ERROR

GOTO DONE

:VS2015

IF EXIST lib\binding ECHO deleting lib/binding && RD /Q /S lib\binding
IF %ERRORLEVEL% NEQ 0 GOTO ERROR
IF EXIST node_modules ECHO deleting node_modules && RD /Q /S node_modules
IF %ERRORLEVEL% NEQ 0 GOTO ERROR

ECHO                 ============================
ECHO                           VS2015
ECHO                 ============================
SET nodejs_version=0.10.40
ECHO SET nodejs_version^: %nodejs_version%
SET platform=x64
ECHO SET platform^: %platform%
SET msvs_toolset=14
ECHO SET msvs_toolset^: %msvs_toolset%
::SET TOOLSET_ARGS=--dist-url=https://s3.amazonaws.com/mapbox/node-cpp11 --toolset=v140

CALL scripts\build-appveyor.bat
IF %ERRORLEVEL% NEQ 0 GOTO ERROR




GOTO DONE

:ERROR
ECHO ~~~~~~~~~~~~~~~~~~~~~~ ERROR %~f0 ~~~~~~~~~~~~~~~~~~~~~~~~~~~~
ECHO ERRORLEVEL^: %ERRORLEVEL%
SET EL=%ERRORLEVEL%

:DONE
ECHO ~~~~~~~~~~~~~~~~~~~~~~ DONE %~f0 ~~~~~~~~~~~~~~~~~~~~~~~~~~~~

EXIT /b %EL%
